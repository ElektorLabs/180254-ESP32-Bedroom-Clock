#ifdef USEDISPLAY
  //redraw the main view
  void redrawDisplay() {
    disp.clearBuffer();
    drawGraph();
    if (alarmEnabled)
      drawAlarmLine();
    drawAlert();
    disp.sendBuffer();
  }
  
  //draw text on the display
  void drawText(String text) {
    String txt = text;
    String line;
    int y = 7;
    Serial.println("Drawing text:");
    disp.clearBuffer();
    while (txt != "") {
      if (txt.indexOf("\n") != -1)
        line = txt.substring(0, txt.indexOf("\n"));
      else
        line = txt;
      txt.remove(0, line.length() + 1);
      disp.drawStr(0, y, line.c_str());
      Serial.println(line);
      y += 8;
    }
    disp.sendBuffer();
  }
  
  //draw the info page
  void drawInfo() {
    disp.clearBuffer();
    if (WiFi.getMode() != WIFI_STA) {
      disp.drawStr(0, 7, "Access point active");
      disp.drawStr(0, 15, String("Name: " + APSSID).c_str());
      disp.drawStr(0, 23, String("IP: " + WiFi.softAPIP().toString()).c_str());
    }
    else if (WiFi.status() == WL_CONNECTED) {
      disp.drawStr(0, 7, "Connected to:");
      disp.drawStr(0, 15, ssid.c_str());
      disp.drawStr(0, 23, String("IP: " + WiFi.localIP().toString()).c_str());
    }
    else {
      disp.drawStr(0, 7, "Not connected");
    }
    if (alarmEnabled) {
      disp.drawStr(0, 39, "Alarm level (Rs/Ro):");
      disp.drawStr(0, 47, String(alarmLevel).c_str());
    }
    else
      disp.drawStr(0, 39, "Alarm disabled");
    
    disp.drawStr(0, 63, String("Thingspeak " + String(thingspeakEnabled ? "enabled" : "disabled")).c_str());
    disp.sendBuffer();
  }
  
  //draw a graph of the stored measurements
  void drawGraph() {
    float segWidth = (float)graphW/(float)(nrMeasurements-1);
    
    disp.drawLine(graphXO, graphYO, graphXO+graphW, graphYO); //x axis
    disp.drawLine(graphXO, graphYO, graphXO, graphYO-graphH); //y axis
  
    disp.setFontDirection(3);
    disp.drawStr(graphXO-2, graphYO+2, "0");
    disp.drawStr(graphXO-2, graphYO-graphH+2, "1");
    disp.drawStr(graphXO-2, graphYO-(graphH/2)+12, "Rs/Ro");
    disp.setFontDirection(0);
    disp.drawStr(graphXO+5, 63, String("Last " + String(nrMeasurements) + " measurements").c_str());
    
    for (int i = 0; i < nrMeasurements - 1; i++) {
      int val1 = constrain(mapf(measurements[i], 0, 1, graphYO, graphYO-graphH), graphYO-graphH, graphYO);
      int val2 = constrain(mapf(measurements[i + 1], 0, 1, graphYO, graphYO-graphH), graphYO-graphH, graphYO);
      int start = segWidth * (float)i;
      int stop = segWidth * (float)(i + 1);
      disp.drawLine(graphXO + start, val1, graphXO + stop, val2);
      disp.drawPixel(graphXO + stop, graphYO - 1);
    }
  }
  
  //draw horizontal dotted alarm line
  void drawAlarmLine() {
    int dotLength = 5;
    for (int xpos = graphXO; xpos < graphXO + graphW; xpos += dotLength*2) {
      int l = fmin(dotLength, graphXO + graphW - xpos);
      int y = mapf(alarmLevel, 0, 1, graphYO, graphYO-graphH);
      disp.drawLine(xpos, y, xpos + l, y);
    }
  }
  
  //draw an alert message on the display
  void drawAlert() {
    if (alertText == "")
      return;
    int textLenPxHalf = alertText.length() * 5 / 2;
    disp.setDrawColor(0);
    disp.drawBox(60 - textLenPxHalf, 0, textLenPxHalf*2 + 4, 10);
    disp.setDrawColor(1);
    disp.drawStr(62 - textLenPxHalf, 8, alertText.c_str());
    disp.drawFrame(60 - textLenPxHalf, 0, textLenPxHalf*2 + 4, 10);
  }
#endif
