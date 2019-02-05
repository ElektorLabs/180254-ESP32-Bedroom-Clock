//this definition defines if the board has a display (prototype board) or not (PCB in housing)
//#define USEDISPLAY

/* 
 * This is the arduino  sensor sketch for 180254  
 * to compile the code you need to include some libarys
 * Arduino JSON 5.x
 * CRC32 by Christopher Baker 
 * PubSubClient
 * DHT ESP Library
 * 
 * Hardware used: ESP32-PICO-KIT
 * 
 */

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>
#include <math.h>
#include <ArduinoJson.h>
#include "datastore.h"
#include <DHTesp.h>                         // DHT ESP Library
#include <PubSubClient.h>                   // MQTT-Library


#ifdef USEDISPLAY
  #include <U8g2lib.h>
#endif
  
//time the info messages appear on the display on startup
#define infoDelay 2500

//pin definitions
#define gasSensorPin 35
#define btn 9
#define alarmPin 19
#define dispClk 10
#define dispDat 5
#define dispD_C 23
#define dispRst 18
#define STALed 5
#define APLed 18
#define alarmLed 23
#define DHT_PIN   21                        // define DHT Pin

//measurement definitions
#define measurementResistor 10 //kOhm
#define vcc 3.3
#define adcRes 4096
#define measureInterval 1000

//alarm values
#define frequency 2000 //Hz
#define alarmInterval 125 //ms

//graph values
#define graphXO 10 //x origin
#define graphYO 55 //y origin
#define graphW 110 //width
#define graphH 50  //height

#define lastConnectedTimeout 600000 //ms = 10 minutes: 10 * 60 * 1000
#define hourMs 3600000 //60 * 60 * 1000

//sensor values
#define nrMeasurements 20
float measurements[nrMeasurements]; //sensor resistance values
float sensRo = 1;
float measAvg = 0;
unsigned int measAvgNr = 0;
float alarmLevel = 0;
bool alarmActive = false;
bool alarmEnabled = false;
bool alarmBeep = false;
bool alarmTest = false;

//log settings (https://thingspeak.com/channels/546772/private_show)
String thingspeakApi;
bool thingspeakEnabled;
unsigned long uploadInterval = hourMs; //default value: every hour

//serial variables
bool serialRdy = false;
String serialIn;

//network settings
String APSSID = "ESP32";
String ssid;
String pass;

#ifdef USEDISPLAY
  //display variables
  bool updateDisplay = false;
  String alertText;
#endif

//button variables
bool prevBtn = false;

bool hasDHT = false;
unsigned long lastUploadTime = 0;
unsigned long lastMeasureTime = 0;
unsigned long lastAPConnection = 0;
unsigned long lastAlarmSound = 0;

volatile float Temperatur = 0;
volatile float Humidity = 0;

TaskHandle_t MQTTTaskHandle;
TaskHandle_t DHTTaskHandle;

#ifdef USEDISPLAY
  //Init ssd1306 display                      ?        clock,   data,    cs,            d/c,     rst
  U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI disp(U8G2_R0, dispClk, dispDat, U8X8_PIN_NONE, dispD_C, dispRst);
#endif

//webserver and client pointers
WebServer* server = NULL;
WiFiClient* client = NULL;

Preferences pref;
DHTesp dht;                                 // DHT22 Sensor

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  SPIFFS.begin();
  datastoresetup();
  pinMode(gasSensorPin, ANALOG);
  pinMode(btn, INPUT_PULLUP);
  
  #ifdef USEDISPLAY
    disp.begin();
    disp.setFont(u8g2_font_5x7_tr);
  #else
    pinMode(STALed, OUTPUT);
    pinMode(APLed, OUTPUT);
    pinMode(alarmLed, OUTPUT);
    digitalWrite(STALed, LOW);
    digitalWrite(APLed, LOW);
    digitalWrite(alarmLed, LOW);
  #endif

  dht.setup(DHT_PIN, DHTesp::AUTO_DETECT);                       // setup sensor
  if(dht.getModel() == DHTesp::DHT11 ){
    Serial.println("DHT11 found, need to wait 1 second");
    delay(1000);
  }
  /* We do two reads from the sensor, if they failt we disable it */
  Temperatur = dht.getTemperature();
  Humidity = dht.getHumidity();
  if( dht.getStatus() != DHTesp::ERROR_NONE ){
    /* Sensor seems faulty */
    Serial.println("No Sensor found!");
    hasDHT = false;
  } else {
    hasDHT = true;
    /* We start a temp aquisiation thread */
  }


  ledcSetup(0, frequency, 8); //channel 0, frequency, resolution 8 bit
  ledcAttachPin(alarmPin, 0);

  for (int i = 0; i < nrMeasurements; i++)
    measurements[i] = 0;

  loadNetworkCredentials();
  loadSensorSettings();
  
  initWiFi();

  #ifdef USEDISPLAY
    updateDisplay = true;
  #endif

   xTaskCreatePinnedToCore(
     MQTT_Task,
     "MQTT_Task",
     20000,
     NULL,
     1,
     &MQTTTaskHandle,
   1);

  if(hasDHT==true){
     xTaskCreatePinnedToCore(
       DHT_Task,
       "DHT_Task",
       10000,
       NULL,
       1,
       &DHTTaskHandle,
     1);
  }
}

void loop() {
  //handle serial
  checkSerial();

  //handle WiFi
  if (server != NULL)
    server->handleClient();
  
  //if the current wifi mode isn't STA and the timout time has passed -> restart
  if ((WiFi.getMode() != WIFI_STA) && ((lastAPConnection + lastConnectedTimeout) < millis())) {
    if (digitalRead(btn)) {
      Serial.println("Last connection was more than 10 minutes ago. Restart.");
      ESP.restart();
    }
    else {//button still pressed
      lastAPConnection = millis(); //wait some time before checking again
    }
  }

  //reinit if the wifi connection is lost
  if ((WiFi.getMode() == WIFI_STA) && (WiFi.status() == WL_DISCONNECTED)) {
    
    #ifndef USEDISPLAY
      digitalWrite(STALed, LOW);
    #endif
    
    Serial.println("Lost connection. Trying to reconnect.");
    initWiFi();
  }

  //measure sensor
  if ((lastMeasureTime + measureInterval) < millis()) {
    lastMeasureTime = millis();
    addMeasurement(measureSensor()/sensRo);

    //set alarm if the last measured value gets below the set alarm level
    if ((measurements[nrMeasurements - 1] < alarmLevel) && (measurements[nrMeasurements - 2] > alarmLevel))
      alarmActive = true;
    else if (measurements[nrMeasurements - 1] > alarmLevel) //disable the alarm if the last measurement is above the alarm level
      alarmActive = false;

    #ifdef USEDISPLAY
      updateDisplay = true;
    #endif
  }

  //sound (and show) alarm
  if ((lastAlarmSound + alarmInterval) < millis()) {
    lastAlarmSound = millis();
    alarmBeep = !alarmBeep;
  }
  if (alarmTest || (alarmActive && alarmEnabled)) {
    ledcWrite(0, ((alarmBeep) ? 128 : 0)); //128 = 50% duty cycle
    
    #ifndef USEDISPLAY
      digitalWrite(alarmLed, alarmBeep);
    #endif
  }
  else {
    ledcWrite(0, 0);
    
    #ifndef USEDISPLAY
      digitalWrite(alarmLed, LOW);
    #endif
  }

  //handle button
  if (digitalRead(btn) != prevBtn) {
    Serial.println("Button " + String(digitalRead(btn) ? "released" : "pressed"));
    if ((alarmActive || alarmTest) && !digitalRead(btn)) { //disable the alarm if this is on
      alarmActive = false;
      alarmTest = false;
    }

    #ifdef USEDISPLAY
      updateDisplay = true;
    #endif
    
    //update the last AP connection time so the module keeps in AP mode (if this is active)
    lastAPConnection = millis(); 
  }
  prevBtn = digitalRead(btn);

  #ifdef USEDISPLAY
    //update display
    if (updateDisplay) {
      updateDisplay = false;
      if (!digitalRead(btn))
        drawInfo();
      else
        redrawDisplay();
    }
  #endif

  //upload data if the uploadperiod has passed and if WiFi is connected
  if (((lastUploadTime + uploadInterval) < millis()) && (WiFi.status() == WL_CONNECTED)) {
    lastUploadTime = millis();
    Serial.println("Upload interval time passed");
  
    if (thingspeakEnabled) {
      
      #ifdef USEDISPLAY
        alertText = "Uploading data";
        redrawDisplay();
      #endif
      
      if (uploadToThingspeak()) {
        Serial.println("Uploaded successfully");
        
        #ifdef USEDISPLAY
          alertText = "";
        #endif
      }
      else {
        Serial.println("Uploading failed");
        
        #ifdef USEDISPLAY
          alertText = "Uploading failed";
          redrawDisplay();
          delay(infoDelay);
          alertText = "";
        #endif
      }

      measAvg = 0;
      measAvgNr = 0;
    }
    else
      Serial.println("Thingspeak disabled");
  }
}

//add a measurement to the list
void addMeasurement(float m) {
  for (int i = 0; i < nrMeasurements - 1; i++) {
    measurements[i] = measurements[i + 1];
  }
  measurements[nrMeasurements - 1] = m;
  addMeasAvg(m);
}

void addMeasAvg(float m) {
  measAvgNr++;
  measAvg = (measAvg * (measAvgNr - 1) / measAvgNr) + (m / measAvgNr);
}

//arduino map function, but using float values
float mapf(float x, float inl, float inh, float outl, float outh) {
  return (x - inl) * (outh - outl) / (inh - inl) + outl;
}

//measure the sensor resistance
float measureSensor() {
  float vSensor = (float)analogRead(gasSensorPin) * vcc/adcRes;
  float rs = (5.0*measurementResistor)/vSensor - measurementResistor; //sensor has 5v supply
  //Serial.println("vSensor: " + String(vSensor) + "V, rs: " + String(rs) + "kOhm");
  return rs;
}

//store the current sensor resistance as Ro (clean air resistance)
void calibrateSensor() {
  sensRo = measureSensor();
  storeSensorSettings();
}


void MQTT_Task( void* prarm ){
   const size_t capacity = JSON_OBJECT_SIZE(4);
   DynamicJsonBuffer jsonBuffer(capacity);
   String JsonString = "";
   uint32_t ulNotificationValue;
   int32_t last_message = millis();
   mqttsettings_t Settings = eepread_mqttsettings();
                         
   Serial.println("MQTT Thread Start");
   WiFiClient espClient;                       // WiFi ESP Client  
   PubSubClient client(espClient);             // MQTT Client 
   client.setCallback(callback);             // define Callback function
   while(1==1){

   /* if settings have changed we need to inform this task that a reload and reconnect is requiered */ 
   ulNotificationValue = ulTaskNotifyTake( pdTRUE, 0 );

   if( ulNotificationValue&0x01 != 0 ){
      Serial.println("Reload MQTT Settings");
      /* we need to reload the settings and do a reconnect */
      if(true == client.connected() ){
        client.disconnect();
      }
      Settings = eepread_mqttsettings();
      Serial.printf("USER: %s \n\r",Settings.mqttusername);
      Serial.printf("PASS: %s \n\r",Settings.mqttpassword);
      Serial.printf("HOSTNAME: %s \n\r",Settings.mqtthostname);
      Serial.printf("SERVER: %s \n\r",Settings.mqttservename);
      Serial.printf("PORT: %i \n\r",Settings.mqttserverport);
      Serial.printf("TOPIC: %s \n\r", Settings.mqtttopic);
      Serial.printf("ENABLE: %i \n\r", Settings.enable);
   }

   if(  (last_message + 30000 )  <  millis()  ){
     last_message=millis();
     if(true == client.connected() ){
      JsonString="";
      /* Every minute we send a new set of data to the mqtt channel */
      JsonObject& root = jsonBuffer.createObject();
      root["temperature"] = Temperatur;
      root["humidity"] = Humidity;
      root["TG2600_AirQ"] = measurements[nrMeasurements - 1];;
      root.printTo(JsonString);
      Serial.print("MQTT Send:");
      Serial.println(JsonString);
     }
     client.publish(Settings.mqtttopic, JsonString.c_str(), true); 
  
   }
  
    

   
   
   if(!client.connected()) {             
        /* sainity check */
        if( (Settings.mqttserverport!=0) && (Settings.mqttservename[0]!=0) && ( Settings.enable != false ) ){
      
              Serial.print("Connecting to MQTT...");  // connect to MQTT
              client.setServer(Settings.mqttservename, Settings.mqttserverport); // Init MQTT     
              if (client.connect(Settings.mqtthostname, Settings.mqttusername, Settings.mqttpassword)) {
                Serial.println("connected");          // successfull connected  
                client.subscribe(Settings.mqtttopic);             // subscibe MQTT Topic
              } else {
                Serial.print("failed with state ");   // MQTT not connected       
              }
        }
   } else{
        client.loop();                            // loop on client
   }
   delay(10);
 }
}

/***************************
 * callback - MQTT message
 ***************************/
void callback(char* topic, byte* payload, unsigned int length) {

  double tempDouble=0; 
  char tempC[10];                           // Temperature (Character)  
  Serial.print("Message arrived in topic: ");  // print "MQTT-Message arrived"
  Serial.print(topic);
  Serial.print("Message: "); 
  /* We also get out own messages if we subscibe to the channel! */
  for(uint32_t i=0;i<length;i++){
    Serial.print((char)(*payload));
    payload++;
  }
  Serial.printf("\n\r");
  
}


void DHT_Task( void* param){

  float temp=0;
  float hum =0;
  Serial.println("DHT Thread Start");
  while (1==1){
  
    temp = dht.getTemperature();
    hum = dht.getHumidity();
    
    if(dht.getStatus() == DHTesp::ERROR_NONE ){
      Temperatur = temp;
      Humidity = hum;
      //Serial.printf("Temp: %f , Humidity %f ",temp,hum);
    }
    vTaskDelay(   2500 / portTICK_PERIOD_MS );
 }
   
}



