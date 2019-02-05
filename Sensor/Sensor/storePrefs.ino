/*
 * Note: preference names can't be longer than 15 characters
 */
void loadNetworkCredentials() {
  pref.begin("tgs2600", false);
  ssid = pref.getString("ssid");
  pass = pref.getString("pass");
  pref.end();
}

void storeNetworkCredentials() {
  pref.begin("tgs2600", false);
  pref.putString("ssid", ssid);
  pref.putString("pass", pass);
  pref.end();
}

void loadSensorSettings() {
  pref.begin("tgs2600", false);
  thingspeakEnabled = pref.getBool("thingspeakEn");
  thingspeakApi = pref.getString("thingspeakApi");
  uploadInterval = ((pref.getUInt("uploadInterval") != 0) ? pref.getUInt("uploadInterval") : hourMs);
  sensRo = ((pref.getFloat("sensRo") != 0) ? pref.getFloat("sensRo") : 1);
  alarmLevel = pref.getFloat("alarmLevel");
  alarmEnabled = pref.getBool("alarmEnabled");
  pref.end();
}

void storeSensorSettings() {
  pref.begin("tgs2600", false);
  pref.putBool("thingspeakEn", thingspeakEnabled);
  pref.putString("thingspeakApi", thingspeakApi);
  pref.putUInt("uploadInterval", uploadInterval);
  pref.putFloat("sensRo", sensRo);
  pref.putFloat("alarmLevel", alarmLevel);
  pref.putBool("alarmEnabled", alarmEnabled);
  pref.end();
}

