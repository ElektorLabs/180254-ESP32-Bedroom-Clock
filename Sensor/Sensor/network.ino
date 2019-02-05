#include <ArduinoJson.h>
#include "datastore.h"

extern TaskHandle_t MQTTTaskHandle;

String SSIDList(String separator = ",") {
  Serial.println("Scanning networks");
  String ssidList;
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    Serial.println(String(i) + ": " + ssid);
    if (ssidList.indexOf(ssid) != -1) {
      Serial.println("SSID already in list");
    }
    else {
      if (ssidList != "")
        ssidList += separator;
      ssidList += ssid;
    }
  }
  return ssidList;
}

//send a list of available networks to the client connected to the webserver
void getSSIDList() {
  Serial.println("SSID list requested");
  sendData(SSIDList());
}

//store the wifi settings configured on the webpage and restart the esp to connect to this network
void setWiFiSettings() {
  Serial.println("WiFi settings received");
  ssid = server->arg("ssid");
  pass = server->arg("pass");
  String response = "Attempting to connect to '" + ssid + "'. The WiFi module restarts and tries to connect to the network.";
  sendData(response);
  Serial.println("Saving network credentials and restart.");
  storeNetworkCredentials();
  delay(1000);
  ESP.restart();
}

//send the wifi settings to the connected client of the webserver
void getWiFiSettings() {
  Serial.println("WiFi settings requested");
  String response;
  response += ssid + ",";
  response += SSIDList(";");
  sendData(response);
}

//store the upload settings configured on the webpage
void setSensorSettings() {
  Serial.println("Upload settings received");
  thingspeakApi = server->arg("tsApi");
  thingspeakEnabled = (server->arg("tsEnabled") == "1");
  long recvInterval = server->arg("interval").toInt();
  uploadInterval = ((recvInterval <= 0) ? hourMs : (recvInterval * 60 * 1000)); //convert to ms (default is 1 hr)
  alarmLevel = server->arg("alarm").toFloat();
  alarmEnabled = (server->arg("alarmEnabled") == "1");
  sendData("Sensor settings stored");
  storeSensorSettings();
}

void getSensorSettings() {
  Serial.println("Upload settings requested");
  String response;
  response += thingspeakApi + ",";
  response += String(thingspeakEnabled ? "1" : "0") + ",";
  response += String(uploadInterval / 1000 / 60) + ","; //convert to minutes
  response += String(alarmLevel, 2) + ",";
  response += String(alarmEnabled) + ",";
  response += String(sensRo);
  sendData(response);
}

void setSensorCalibration() {
  Serial.println("Sensor calibration requested");
  calibrateSensor();
  sendData("Sensor calibration stored," + String(sensRo));
}

void testAlarm() {
  Serial.println("Alarm test requested");
  alarmTest = true;
  sendData("Alarm active");
}

void getMeasurements() {
  Serial.println("Measurements requested");
  String response = String(alarmLevel) + ",";
  for (int i = 0; i < nrMeasurements; i++) {
    response += String(measurements[i]);
    if (i != (nrMeasurements - 1))
      response  += ",";
  }
  sendData(response);
}

//restart the esp as requested on the webpage
void restart() {
  sendData("The ESP32 will restart and you will be disconnected from the '" + APSSID + "' network.");
  delay(1000);
  ESP.restart();
}

//get the content type of a filename
String getContentType(String filename) {
  if (server->hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

//send a file from the SPIFFS to the connected client of the webserver
void sendFile() {
  String path = server->uri();
  Serial.println("Got request for: " + path);
  if (path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  Serial.println("Content type: " + contentType);
  if (SPIFFS.exists(path)) {
    Serial.println("File " + path + " found");
    File file = SPIFFS.open(path, "r");
    server->streamFile(file, contentType);
    file.close();
  }
  else {
    Serial.println("File '" + path + "' doesn't exist");
    server->send(404, "text/plain", "The requested file doesn't exist");
  }
  
  lastAPConnection = millis();
}

//send data to the connected client of the webserver
void sendData(String data) {
  Serial.println("Sending: " + data);
  server->send(200, "text/plain", data);
  
  lastAPConnection = millis();
}

//initialize wifi by connecting to a wifi network or creating an accesspoint
void initWiFi() {
  
  #ifndef USEDISPLAY
    digitalWrite(APLed, LOW);
    digitalWrite(STALed, LOW);
  #endif
  
  Serial.print("WiFi: ");
  if (!digitalRead(btn)) {
    Serial.println("AP");
    configureSoftAP();
  }
  else {
    Serial.println("STA");
    if (!connectWiFi()) {
      Serial.println("Connecting failed. Starting AP");
      configureSoftAP();
    }
    else {
      configureClient();
      
      #ifndef USEDISPLAY
        digitalWrite(STALed, HIGH);
      #endif
      configureServer();
    }
  }
}

//connect the esp32 to a wifi network
bool connectWiFi() {
  if (ssid == "") {
    Serial.println("SSID unknown");
    return false;
  }
  WiFi.mode(WIFI_STA);
  Serial.println("Attempting to connect to " + ssid + ", pass: " + pass);

  #ifdef USEDISPLAY
    drawText("Attempting to connect to:\n" + ssid);
  #endif
  
  WiFi.begin(ssid.c_str(), pass.c_str());
  for (int timeout = 0; timeout < 15; timeout++) { //max 15 seconds
    int status = WiFi.status();
    if ((status == WL_CONNECTED) || (status == WL_CONNECT_FAILED) || (status == WL_NO_SSID_AVAIL))
      break;
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to " + ssid);
    Serial.println("WiFi status: " + WiFiStatusToString());
    WiFi.disconnect();

    #ifdef USEDISPLAY
      drawText("Failed to connect to:\n" + ssid);
      delay(infoDelay);
    #endif
    
    return false;
  }
  Serial.println("Connected to " + ssid);
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  #ifdef USEDISPLAY
    drawText("Connected to:\n" + ssid + "\nIP: " + WiFi.localIP().toString());
    delay(infoDelay);
  #endif
  
  return true;
}

//configure the access point of the esp32
void configureSoftAP() {
  Serial.println("Configuring AP: " + String(APSSID));
  WiFi.mode(WIFI_AP);
  WiFi.softAP(APSSID.c_str(), NULL, 1, 0, 1);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(ip);
  lastAPConnection = millis();

  #ifdef USEDISPLAY
    //set the alert message
    alertText = "Access point";
    
    drawText("Access point started\nName: " + APSSID + "\nIP: " + WiFi.softAPIP().toString());
    delay(infoDelay);
  #else
    digitalWrite(APLed, HIGH);
  #endif
  
  configureServer();
}

//initialize the webserver on port 80
void configureServer() {
  server = new WebServer(80);
  server->on("/mqtt/settings",HTTP_POST,mqttsettings_update);
  server->on("/mqtt/settings",HTTP_GET,read_mqttsetting);
  server->on("/setWiFiSettings", HTTP_GET, setWiFiSettings);
  server->on("/getWiFiSettings", HTTP_GET, getWiFiSettings);
  server->on("/setSensorSettings", HTTP_GET, setSensorSettings);
  server->on("/getSensorSettings", HTTP_GET, getSensorSettings);
  server->on("/setSensorCalibration", HTTP_GET, setSensorCalibration);
  server->on("/getMeasurements", HTTP_GET, getMeasurements);
  server->on("/testAlarm", testAlarm);
  server->on("/getSSIDList", HTTP_GET, getSSIDList);
  server->on("/restart", HTTP_GET, restart);
  server->on("/data/temperatur_humidity", HTTP_GET, getTemperaturHumidity);
  server->onNotFound(sendFile); //handle everything except the above things
  server->begin();
  Serial.println("Webserver started");
}

void configureClient() {
  client = new WiFiClient();
}

//request the specified url of the specified host
String performRequest(String host, String url, int port = 80, String method = "GET", String headers = "Connection: close\r\n", String data = "") {
  Serial.println("Connecting to host '" + host + "' on port " + String(port));
  client->connect(host.c_str(), port); //default ports: http: port 80, https: 443
  String request = method + " " + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   headers + "\r\n";
  Serial.println("Requesting url: " + request);
  client->print(request);
  if (data != "") {
    Serial.println("Data: " + data);
    client->print(data + "\r\n");
  }
  
  unsigned long timeout = millis();
  while (client->available() == 0) {
    if (timeout + 5000 < millis()) {
      Serial.println("Client timeout");
      client->stop();
      return "";
    }
  }
  //read client reply
  String response;
  while(client->available()) {
    response = client->readStringUntil('\r');
  }
  Serial.println("Response: " + response);
  client->stop();
  return response;
}

String WiFiStatusToString() {
  switch (WiFi.status()) {
    case WL_IDLE_STATUS:     return "IDLE"; break;
    case WL_NO_SSID_AVAIL:   return "NO SSID AVAIL"; break;
    case WL_SCAN_COMPLETED:  return "SCAN COMPLETED"; break;
    case WL_CONNECTED:       return "CONNECTED"; break;
    case WL_CONNECT_FAILED:  return "CONNECT_FAILED"; break;
    case WL_CONNECTION_LOST: return "CONNECTION LOST"; break;
    case WL_DISCONNECTED:    return "DISCONNECTED"; break;
    case WL_NO_SHIELD:       return "NO SHIELD"; break;
    default:                 return "Undefined: " + String(WiFi.status()); break;
  }
}


void mqttsettings_update( ){
 
  mqttsettings_t Data = eepread_mqttsettings();
  
  if( ! server->hasArg("MQTT_USER") || server->arg("MQTT_USER") == NULL ) { 
    /* we are missing something here */
  } else { 
    String value = server->arg("MQTT_USER");
    strncpy(Data.mqttusername, value.c_str(),128);
  }

  if( ! server->hasArg("MQTT_PASS") || server->arg("MQTT_PASS") == NULL ) { 
    /* we are missing something here */
  } else { 
    String value = server->arg("MQTT_PASS");
    strncpy(Data.mqttpassword, value.c_str(),128);
  }

  if( ! server->hasArg("MQTT_SERVER") || server->arg("MQTT_SERVER") == NULL ) { 
    /* we are missing something here */
  } else { 
    String value = server->arg("MQTT_SERVER");
    strncpy(Data.mqttservename, value.c_str(),128);
  }

  if( ! server->hasArg("MQTT_HOST") || server->arg("MQTT_HOST") == NULL ) { 
    /* we are missing something here */
  } else { 
    String value = server->arg("MQTT_HOST");
    strncpy(Data.mqtthostname, value.c_str(),64);
  }

  if( ! server->hasArg("MQTT_PORT") || server->arg("MQTT_PORT") == NULL ) { 
    /* we are missing something here */
  } else { 
    int32_t value = server->arg("MQTT_PORT").toInt();
    if( (value>=0) && ( value<=UINT16_MAX ) ){
      Data.mqttserverport = value;
    }
  }
  
  if( ! server->hasArg("MQTT_TOPIC") || server->arg("MQTT_TOPIC") == NULL ) { 
    /* we are missing something here */
  } else { 
    String value = server->arg("MQTT_TOPIC");
    strncpy(Data.mqtttopic, value.c_str(),500);
  }

  if( ! server->hasArg("MQTT_ENA") || server->arg("MQTT_ENA") == NULL ) { 
    /* we are missing something here */
  } else { 
    bool value = false;
    if(server->arg("MQTT_ENA")=="true"){
      value = true;
    }
    Data.enable = value;
  }

   


  
  /* write data to the eeprom */
  eepwrite_mqttsettings(Data);
  
  xTaskNotify( MQTTTaskHandle, 0x01, eSetBits );
  server->send(200); 

}


void read_mqttsetting(){
  String response ="";
  mqttsettings_t Data = eepread_mqttsettings();
  const size_t capacity = JSON_OBJECT_SIZE(7);
  DynamicJsonBuffer jsonBuffer(2000);
  
  JsonObject& root = jsonBuffer.createObject();
  root["mqttena"]= (bool)(Data.enable);
  root["mqttserver"] = String(Data.mqttservename);
  root["mqtthost"] = String(Data.mqtthostname);
  root["mqttport"] = Data.mqttserverport;
  root["mqttuser"] = String(Data.mqttusername);
  root["mqtttopic"] = String(Data.mqtttopic);
  if(Data.mqttpassword[0]!=0){
    root["mqttpass"] = "********";
  } else {
    root["mqttpass"] ="";
  }

  root.printTo(response);
  sendData(response);

}

void getTemperaturHumidity( ){
  String response ="";
  mqttsettings_t Data = eepread_mqttsettings();
  const size_t capacity = JSON_OBJECT_SIZE(7);
  DynamicJsonBuffer jsonBuffer(2000);  
  JsonObject& root = jsonBuffer.createObject();
  root["temperatur"]= Temperatur;
  root["humidity"] = Humidity;
  root.printTo(response);
  sendData(response);

}


