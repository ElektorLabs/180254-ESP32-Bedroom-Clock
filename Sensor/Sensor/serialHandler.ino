void checkSerial() {
  while (Serial.available()) {
    char in = Serial.read();
    serialIn += in;
    if (in == '\n')
      serialRdy = true;
  }
  handleSerial();
}

void handleSerial() {
  if (serialRdy) {
    Serial.println(serialIn);
    if (serialIn.indexOf("thingspeakUpload") != -1) {
      if (uploadToThingspeak())
        Serial.println("Success");
      else
        Serial.println("Failed");
    }
    else if (serialIn.indexOf("sensorSettings") != -1) {
      Serial.println("Calibration: " + String(sensRo));
      Serial.println("Alarm level: " + String(alarmLevel));
    }
    else if (serialIn.indexOf("wifiStatus") != -1) {
      Serial.println("WiFi status: " + WiFiStatusToString());
    }else if (serialIn.indexOf("WifiSettings") != -1) {
      if (serialIn.indexOf("set") != -1) {
        ssid = serialIn.substring(serialIn.indexOf(":") + 1, serialIn.indexOf(","));
        pass = serialIn.substring(serialIn.indexOf(",") + 1, serialIn.indexOf("\n"));
        storeNetworkCredentials();
      }
      Serial.println("WiFi settings: SSID: " + ssid + ", pass: " + pass);
    }
    else if (serialIn.indexOf("thingspeakSettings") != -1) {
      Serial.println("Thingspeak settings:");
      Serial.println("  Api key:  " + thingspeakApi);
      Serial.println("  Enabled:  " + String(thingspeakEnabled ? "Yes" : "No"));
      Serial.println("  Interval: " + String(uploadInterval) + "ms (" + String(uploadInterval/1000/60) + " minutes)");
    }
    else if (serialIn.indexOf("restart") != -1) {
      ESP.restart();
    }
    else if (serialIn.indexOf("timeActive") != -1) {
      unsigned long time = millis();
      Serial.println("Time active: " + String(time) + "ms, " + String(time/1000/60) + " minutes");
    }
    else if (serialIn.indexOf("testAlarm") != -1) {
      alarmTest = true;
    }
    
    serialRdy = false;
    serialIn = "";
  }
}

