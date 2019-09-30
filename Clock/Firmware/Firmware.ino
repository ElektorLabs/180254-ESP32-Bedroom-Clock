/* 
 * This is the arduino  clock sketch for 180254  
 * to compile the code you need to include some libarys
 * Time by Michael Margolis ( Arduino )
 * Arduino JSON 6.x
 * CRC32 by Christopher Baker 
 * PubSubClient
 * Adafruit BME280 Library 1.0.7 by Adafruit
 * Adafruit Unified Sensor 1.0.2 by Adafruit
 * Adafruit TSL2561 by Adafruit
 * Adafruit VEML6070 by Adafruit
 * WebSockets by Markus Sattler
 * NTP client libary from https://github.com/gmag11/NtpClient/
 * DHTesp
 * 
 * Hardware used: ESP32-PICO-KIT
 * 
 */

#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <TimeLib.h>
#include <Ticker.h>
#include <PubSubClient.h>                   // MQTT-Library

/* I2C Sensors */
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_VEML6070.h>
#include <Adafruit_TSL2561_U.h>

#include "DHTesp.h"

#include "ArduinoJson.h"
#include "NTP_Client.h"
#include "timecore.h"
#include "datastore.h"
#include "websocket_if.h"
#include "sevensegmentdisplay.h"

typedef struct{
  int16_t temp;
  bool valid;
} temperature_t;

typedef struct{
  float temperature; //°C
  float humidity; //%
  float pressure; //hPa
  bool valid;
} bme280_result_t;

#define bmeAddress 0x76

void renew_ota_password( void );

Timecore timec;
NTP_Client NTPC;

Ticker TimeKeeper;
/* 63 Char max and 17 missign for the mac */
TaskHandle_t Task1;
TaskHandle_t MQTTTaskHandle = NULL;

SemaphoreHandle_t xSensorSemaphore = NULL;

WiFiClient espClient;                       // WiFi ESP Client  
PubSubClient mqttclient(espClient);             // MQTT Client 

bool hasBME280 = false;
bool has_TSL2561 = false;
bool has_dht = false;
Adafruit_BME280 bme;
Adafruit_VEML6070 uv = Adafruit_VEML6070();
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
DHTesp dhtSensor;

volatile temperature_t Current_Temp = {.temp=-99, .valid=false};
volatile bool display_pw = false;
volatile bool display_update = false;
volatile uint8_t update_status = 0;
volatile uint8_t PwBuffer[5]={0,};

/**************************************************************************************************
 *    Function      : setup
 *    Description   : Get all components in ready state
 *    Input         : none 
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void setup()
{
  /* First we setup the serial console with 115k2 8N1 */
  Serial.begin (115200);
  /* The next is to initilaize the datastore, here the eeprom emulation */
  datastoresetup();
  /* This is for the flash file system to access the webcontent */
  SPIFFS.begin();
  /* Set the LED output */
  SevenSegmentSetup(BEDROOMCLOCK_1_2);
  
  SevenSegmentWrite(0,'B');
  SevenSegmentWrite(1,'o');
  SevenSegmentWrite(2,'o');
  SevenSegmentWrite(3,'t');
  
  xSensorSemaphore = xSemaphoreCreateMutex();

  if(false == bme.begin(bmeAddress)){
    hasBME280 = false;
  } else {
    hasBME280 = true;
    //recommended settings for weather monitoring
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF);
    SevenSegmentWrite(0,'B');
    SevenSegmentWrite(1,'m');
    SevenSegmentWrite(2,'e');
    SevenSegmentWrite(3,' ');
    vTaskDelay(1000);
  }

  if(!tsl.begin())
  {
    /* There was a problem detecting the TSL2561 ... check your connections */
    Serial.println("No TSL2561 detected, searched on ADDR:0x39, ( addr pin floating )");
    has_TSL2561 = false;
  } else {
     tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
     /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
     //tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
     // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
     tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */
     has_TSL2561 = true;
     SevenSegmentWrite(0,'t');
     SevenSegmentWrite(1,'l');
     SevenSegmentWrite(2,'s');
     SevenSegmentWrite(3,' ');
    vTaskDelay(1000);
  }

  uv.begin(VEML6070_1_T);  // pass in the integration time constant

  dhtSensor.setup( 15 , DHTesp::AUTO_DETECT);
  
  /* And here also the "Terminal" to log messages, ugly but working */
  Serial.println(F("Booting..."));
  for(uint32_t i=0;i<25;i++){
    if(digitalRead( 0 ) == false){
        Serial.println(F("Erase EEPROM"));
        erase_eeprom();  
        break;
    } else {
      vTaskDelay( 100 / portTICK_PERIOD_MS ); 
    }
  }

  TempAndHumidity newValues = dhtSensor.getTempAndHumidity();
  if (dhtSensor.getStatus() != 0) {
  // We assume a problem and don't use it 
    has_dht = false;
     Serial.printf("DHT11 /DHT22 failed with status %i",dhtSensor.getStatus());
  } else {
    has_dht = true;
    Serial.println(F("DHT11 /DHT22 detected"));
     SevenSegmentWrite(0,'d');
     SevenSegmentWrite(1,'h');
     SevenSegmentWrite(2,'t');
     SevenSegmentWrite(3,' ');
     vTaskDelay(1000);
  }
  
  SevenSegmentWrite(0,'W');
  SevenSegmentWrite(1,'i');
  SevenSegmentWrite(2,'F');
  SevenSegmentWrite(3,'i');
  
  Serial.println(F("Init WiFi"));   
  
  initWiFi();
  
  // Port defaults to 3232
  ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  //ArduinoOTA.setHostname("myesp32");



  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      //Display specific mod to show four digit Password
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
       display_update = false;
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      display_update = true;
      update_status = (progress / (total / 100));
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      display_pw=false;
      display_pw=true;
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      }
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();


   /* We read the Config from flash */
  Serial.println(F("Read Timecore Config"));
  timecoreconf_t cfg = read_timecoreconf();
  timec.SetConfig(cfg);
  /* Now we start with the config for the Timekeeping and sync */
  TimeKeeper.attach_ms(1000, _1SecondTick);
  
 
  /* The NTP is running in its own task */
   xTaskCreatePinnedToCore(
      NTP_Task,       /* Function to implement the task */
      "NTP_Task",  /* Name of the task */
      10000,          /* Stack size in words */
      NULL,           /* Task input parameter */
      1,              /* Priority of the task */
      NULL,           /* Task handle. */
      1);   

   xTaskCreatePinnedToCore(
   MQTT_Task,               /* Function to implement the task */
   "MQTT_Task",             /* Name of the task */
   10000,                   /* Stack size in words */
   NULL,                    /* Task input parameter */
   1,                       /* Priority of the task */
   &MQTTTaskHandle,         /* Task handle */
   0);                      /* CORE to pin */

 
   xTaskCreatePinnedToCore(
   Display_Task,
   "Display_Task",
   10000,
   NULL,
   1,
   NULL,
   0);
 
  /* We now start the Websocket part */
  ws_service_begin();
  
  
}

/**************************************************************************************************
 *    Function      : _1SecondTick
 *    Description   : Runs all fnctions inside once a second
 *    Input         : none 
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void _1SecondTick( void ){
     timec.RTC_Tick();  
}



/**************************************************************************************************
 *    Function      : loop
 *    Description   : Superloop
 *    Input         : none 
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void loop()
{  
  NetworkTask();
  ws_task();
  /* timeupdate done here is sort of really bad */
  ArduinoOTA.handle();
}


void Display_Task( void* param ){
  /* We display if we are AP or STA and also our IP */
  wifi_mode_t mode;
  IPAddress ip;
  esp_wifi_get_mode(&mode);

  SevenSegmentDP(0, false);
  SevenSegmentDP(1, false);
  SevenSegmentDP(2, false);
  SevenSegmentDP(3, false);

  if(mode == WIFI_MODE_AP ) {
    /* We display AP */
    

    SevenSegmentWrite(0,'A');
    SevenSegmentWrite(1,'P');
    SevenSegmentWrite(2,' ');
    SevenSegmentWrite(3,' ');
    ip = WiFi.softAPIP();
  } else {
    SevenSegmentWrite(0,'S');
    SevenSegmentWrite(1,'T');
    SevenSegmentWrite(2,'A');
    SevenSegmentWrite(3,' ');
    ip = WiFi.localIP();
  }
  vTaskDelay( 1000 / portTICK_PERIOD_MS ); 
  /* Next is to display the ip */
  SevenSegmentWrite(0,'I');
  SevenSegmentWrite(1,'P');
  SevenSegmentWrite(2,' ');
  SevenSegmentWrite(3,' ');

  if(GetSelectedHardware() == BEDROOMCLOCK_MINI_1_0 ) {
      SevenSegmentDP(0, false);
      SevenSegmentDP(1, true);
      SevenSegmentDP(2, false);
      SevenSegmentDP(3, false);
  } else {
    SevenSegmentDP(0, false);
    SevenSegmentDP(1, true);
    SevenSegmentDP(2, true);
    SevenSegmentDP(3, false);
  }
  vTaskDelay( 1000 / portTICK_PERIOD_MS ); 
  SevenSegmentWrite(0,' ');
  SevenSegmentWrite(1,' ');
  SevenSegmentWrite(2,' ');
  SevenSegmentWrite(3,' ');
  
  SevenSegmentDP(0, false);
  SevenSegmentDP(1, false);
  SevenSegmentDP(2, false);
  SevenSegmentDP(3, false);
  for(uint32_t i=0;i<4;i++){
    uint8_t one=0;
    uint8_t two=0;
    uint8_t three=0;

    one = ip[i]/100;
    two = ( ip[i]-(one*100) ) /10;
    three = ( ip[i]-(one*100)-(two*10) );
    SevenSegmentWrite(1,'0'+one);
    SevenSegmentWrite(2,'0'+two);
    SevenSegmentWrite(3,'0'+three);
    SevenSegmentWrite(4,' ');
    vTaskDelay( 1000 / portTICK_PERIOD_MS );
  }


   uint32_t no_pw = 0;  
  while( 1 == 1 ){
     vTaskDelay( 200 / portTICK_PERIOD_MS );
     if ( (display_update == true ) ) { 
                uint8_t hundret = update_status / 100;
                update_status = update_status - (hundret*100);
                uint8_t tens = update_status / 10;
                uint8_t rest = update_status -( tens*10);
                
                SevenSegmentDP(0, false);
                SevenSegmentDP(1, false);
                SevenSegmentDP(2, false);
                SevenSegmentDP(3, false);
                SevenSegmentWrite(0,'0'+hundret);
                SevenSegmentWrite(1,'0'+tens);
                SevenSegmentWrite(2,'0'+rest);
                SevenSegmentWrite(3,'P');

                
      }else {
         
          datum_t d = timec.GetLocalTimeDate();
          if(  ( ( ( d.minute + 1 ) % 10 ) == 0 )  && ( ( true == has_dht ) || (true == hasBME280 ) ) )  {
              Current_Temp.valid = false;
              if( xSemaphoreTake( xSensorSemaphore, portMAX_DELAY ) == pdTRUE )
              {
                    
                  if(true == hasBME280 ){
                      bme280_result_t bme280 = readBME();
                      Current_Temp.valid = true;
                      Current_Temp.temp = bme280.temperature;
                  } else if ( true == has_dht ){
                      TempAndHumidity dhtvalues = dhtSensor.getTempAndHumidity();
                      Current_Temp.valid = true;
                      Current_Temp.temp = dhtvalues.temperature;
                  } 
                   xSemaphoreGive( xSensorSemaphore );
              }
              if(true == Current_Temp.valid ) {
              
              /* We display +/- 99° 6 times a hour at n9 minutes */
              
              int16_t temp = Current_Temp.temp;
              char t_ten = '0';
              char t = '0';
              if(temp>=0){
                SevenSegmentWrite(0,' ');
                t_ten = t_ten + ( temp / 10 );
                t = t + ( temp % 10 );          
              } else {
                SevenSegmentWrite(0,'-');
                t_ten = t_ten + ( temp / (-10) );
                t = t + ( temp % (-10) );   
              }
              SevenSegmentDP(0, false);
              SevenSegmentDP(1, false);
              SevenSegmentDP(2, false);
              SevenSegmentDP(3, false);

              SevenSegmentWrite(1,t_ten);
              SevenSegmentWrite(2,t);
              SevenSegmentWrite(3,'\370');
              } else {
                   if(d.second%2 == 0){
                  if(GetSelectedHardware() == BEDROOMCLOCK_MINI_1_0 ) {
                      SevenSegmentDP(0, false);
                      SevenSegmentDP(1, true);
                      SevenSegmentDP(2, false);
                      SevenSegmentDP(3, false);
                  } else {
                    SevenSegmentDP(0, false);
                    SevenSegmentDP(1, true);
                    SevenSegmentDP(2, true);
                    SevenSegmentDP(3, false);
                  }
              } else {
                
                SevenSegmentDP(0, false);
                SevenSegmentDP(1, false);
                SevenSegmentDP(2, false);
                SevenSegmentDP(3, false);
        
              }
              uint8_t ten_h = '0' +( d.hour / 10 );
              uint8_t h = '0' +(d.hour % 10);
        
              uint8_t ten_m = '0' +( d.minute / 10 );
              uint8_t m = '0' +(d.minute % 10);
            
              SevenSegmentWrite(0,ten_h);
              SevenSegmentWrite(1,h);
              SevenSegmentWrite(2,ten_m);
              SevenSegmentWrite(3,m);
              
              
              }
            
          } else {

          
              if(d.second%2 == 0){
                  if(GetSelectedHardware() == BEDROOMCLOCK_MINI_1_0 ) {
                      SevenSegmentDP(0, false);
                      SevenSegmentDP(1, true);
                      SevenSegmentDP(2, false);
                      SevenSegmentDP(3, false);
                  } else {
                    SevenSegmentDP(0, false);
                    SevenSegmentDP(1, true);
                    SevenSegmentDP(2, true);
                    SevenSegmentDP(3, false);
                  }
              } else {
                
                SevenSegmentDP(0, false);
                SevenSegmentDP(1, false);
                SevenSegmentDP(2, false);
                SevenSegmentDP(3, false);
        
              }
              uint8_t ten_h = '0' +( d.hour / 10 );
              uint8_t h = '0' +(d.hour % 10);
        
              uint8_t ten_m = '0' +( d.minute / 10 );
              uint8_t m = '0' +(d.minute % 10);
            
              SevenSegmentWrite(0,ten_h);
              SevenSegmentWrite(1,h);
              SevenSegmentWrite(2,ten_m);
              SevenSegmentWrite(3,m);
          }
        }
          
          
      }
  
  
}

void NTP_Task( void * param){
  Serial.println(F("Setup NTP now"));
  NTPC.ReadSettings();
  NTPC.begin( &timec );
  NTPC.Sync();

  /* As we are in a sperate thread we can run in an endless loop */
  while( 1==1 ){
    /* This will send the Task to sleep for one second */
    vTaskDelay( 1000 / portTICK_PERIOD_MS );  
    NTPC.Tick();
    NTPC.Task();
  }
}


void MQTT_Task( void* prarm ){
   DynamicJsonDocument  root(300);
   String JsonString = "";
   uint32_t ulNotificationValue;
   int32_t last_message = millis();
   mqttsettings_t Settings = eepread_mqttsettings();
                         
   Serial.println("MQTT Thread Start");
   mqttclient.setCallback(callback);             // define Callback function
   while(1==1){

   /* if settings have changed we need to inform this task that a reload and reconnect is requiered */ 
   if(Settings.enable != false){
    ulNotificationValue = ulTaskNotifyTake( pdTRUE, 0 );
   } else {
    Serial.println("MQTT disabled, going to sleep");
    if(true == mqttclient.connected() ){
        mqttclient.disconnect();
    }
    ulNotificationValue = ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    Serial.println("MQTT awake from sleep");
   }

   if( ulNotificationValue&0x01 != 0 ){
      Serial.println("Reload MQTT Settings");
      /* we need to reload the settings and do a reconnect */
      if(true == mqttclient.connected() ){
        mqttclient.disconnect();
      }
      Settings = eepread_mqttsettings();
   }

   if(Settings.enable != false ) {
  
       if(!mqttclient.connected()) {             
            /* sainity check */
            if( (Settings.mqttserverport!=0) && (Settings.mqttservername[0]!=0) && ( Settings.enable != false ) ){
                  /* We try only every second to connect */
                  Serial.print("Connecting to MQTT...");  // connect to MQTT
                  mqttclient.setServer(Settings.mqttservername, Settings.mqttserverport); // Init MQTT     
                  if (mqttclient.connect(Settings.mqtthostname, Settings.mqttusername, Settings.mqttpassword)) {
                    Serial.println("connected");          // successfull connected  
                    mqttclient.subscribe(Settings.mqtttopic);             // subscibe MQTT Topic
                  } else {
                    Serial.println("failed");   // MQTT not connected       
                  }
            }
       } else{
            mqttclient.loop();                            // loop on client
            /* Check if we need to send data to the MQTT Topic, currently hardcode intervall */
            uint32_t intervall_end = last_message +( Settings.mqtttxintervall * 1000 );
            if( ( Settings.mqtttxintervall > 0) && ( intervall_end  <  millis() ) ){
              last_message=millis();
              /* messurment readings */
              bme280_result_t bme280 = readBME();
              int16_t UV_value = readUV();
              float Lux = readLux();
              JsonString="";
              root.clear();
             
              JsonObject data = root.createNestedObject("data");            
              /* We don't support wind and rain at the moment */
              /*
              JsonObject data_wind = data.createNestedObject("wind");
              data_wind["direction"] =-1;
              data_wind["speed"] =-1;
              data["rain"] = -1 ;
              */
              /* we preffer bme280 over dht11 / dht22 */
              if( xSemaphoreTake( xSensorSemaphore, portMAX_DELAY ) == pdTRUE )
              {
              
                  if(true == hasBME280 ){
                    data["temperature"] = bme280.temperature;
                    data["humidity"] = bme280.humidity;
                    data["airpressure"] = bme280.pressure;
                  } else if ( true == has_dht ){
                    TempAndHumidity dhtvalues = dhtSensor.getTempAndHumidity();
                    data["temperature"] = dhtvalues.temperature;
                    data["humidity"] = dhtvalues.humidity;
                    data["airpressure"] = 0;
                  } else {
                    data["temperature"] = 0;
                    data["humidity"] = 0;
                    data["airpressure"] = 0;
                
                  }
                  xSemaphoreGive( xSensorSemaphore );
              }
              /* Currently not supported */
              data["PM2_5"] = -1;
              data["PM10"] = -1;
              data["Lux"] = UV_value ;
              data["UV"] = Lux;
            
              serializeJson(root,JsonString);
             
              if ( 0 == mqttclient.publish(Settings.mqtttopic, JsonString.c_str())){
                Serial.println("MQTT pub failed");  
              }
            }
       }
       vTaskDelay( 100/portTICK_PERIOD_MS );
   } 
 }
}

/***************************
 * callback - MQTT message
 ***************************/
void callback(char* topic, byte* payload, unsigned int length) {

}

bme280_result_t readBME() {
  bme280_result_t result;
  if( hasBME280 == true ){
    bme.takeForcedMeasurement();
    result.temperature = bme.readTemperature();
    result.humidity = bme.readHumidity();
    result.pressure = bme.readPressure() / 100.0;
    result.valid=true;
  } else {
    result.temperature = 0;
    result.humidity = 0;
    result.pressure = 0;
    result.valid=false;
  }
}

int16_t readUV( void ){
  int16_t level = uv.readUV();
  return level;
}

float readLux( void ){
  float lux_level=-1;
  if(has_TSL2561==false){
    lux_level=-1;
  } else {
  sensors_event_t event;
    tsl.getEvent(&event);
    if (event.light)
    {
      lux_level = event.light; 
      
    }
  }
 return lux_level;
}



 
