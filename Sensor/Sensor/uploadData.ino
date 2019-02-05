//upload the sensor data to thingspeak
bool uploadToThingspeak() {
  Serial.println("Uploading to thingspeak");
  
  if (thingspeakApi == "") {
    Serial.println("Thingspeak API key not set");
    return false;
  }

  float uploadVal = measurements[nrMeasurements - 1]; //upload last value
  Serial.println("Value: " + String(uploadVal));
  
  String thingspeakHost = "api.thingspeak.com";
  String thingspeakUrl = "/update";
  thingspeakUrl += "?api_key=" + thingspeakApi;
  thingspeakUrl += "&field1=" + String(uploadVal);

  String resp = performRequest(thingspeakHost, thingspeakUrl);
  return (resp != "" && !resp.startsWith("0"));
}

