/* 
 * This is the arduino  clock sketch for 180254  
 * to compile the code you need to include some libarys
 * Time by Michael Margolis ( Arduino )
 * Arduino JSON 5.x
 * CRC32 by Christopher Baker 
 * PubSubClient
 * WebSockets by Markus Sattler
 * NTP client libary from https://github.com/gmag11/NtpClient/tree/develop (use develop branch )
 * 
 * Hardware used: ESP32-PICO-KIT
 * 
 */

#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>
#include <FS.h>
#include <SPIFFS.h>

#include <TimeLib.h>
#include <Ticker.h>
#include <PubSubClient.h>                   // MQTT-Library

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


Timecore timec;
NTP_Client NTPC;

Ticker TimeKeeper;
/* 63 Char max and 17 missign for the mac */
TaskHandle_t Task1;
TaskHandle_t MQTTTaskHandle;


volatile temperature_t Current_Temp = {.temp=-99, .valid=false};


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
  SevenSegmentSetup();
  
  SevenSegmentWrite(0,'B');
  SevenSegmentWrite(1,'o');
  SevenSegmentWrite(2,'o');
  SevenSegmentWrite(3,'t');
  
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
  
  SevenSegmentWrite(0,'W');
  SevenSegmentWrite(1,'i');
  SevenSegmentWrite(2,'F');
  SevenSegmentWrite(3,'i');
  
  Serial.println(F("Init WiFi"));   
  
  initWiFi();
  /* We need to figure out if we are in AP mode and show a few settings */

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
   MQTT_Task,
   "MQTT_Task",
   10000,
   NULL,
   1,
   &MQTTTaskHandle,
   1);

 
   xTaskCreatePinnedToCore(
   Display_Task,
   "Display_Task",
   10000,
   NULL,
   1,
   NULL,
   1);
 
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
  
}


void Display_Task( void* param ){
  /* We display if we are AP or STA and also our IP */
  wifi_mode_t mode;
  IPAddress ip = WiFi.localIP();
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
  } else {
    SevenSegmentWrite(0,'S');
    SevenSegmentWrite(1,'T');
    SevenSegmentWrite(2,'A');
    SevenSegmentWrite(3,' ');
  }
  vTaskDelay( 1000 / portTICK_PERIOD_MS ); 
  /* Next is to display the ip */
  SevenSegmentWrite(0,'I');
  SevenSegmentWrite(1,'P');
  SevenSegmentWrite(2,' ');
  SevenSegmentWrite(3,' ');

  SevenSegmentDP(0, false);
  SevenSegmentDP(1, true);
  SevenSegmentDP(2, true);
  SevenSegmentDP(3, false);
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


      
  while( 1 == 1 ){
      vTaskDelay( 200 / portTICK_PERIOD_MS ); 
      
      datum_t d = timec.GetLocalTimeDate();
      if( (  ( ( d.minute + 1 ) % 10 ) == 0 ) && (  Current_Temp.valid == true)  ) {
          /* We display +/- 99° 6 times a hour at n9 minutes */
          Current_Temp.valid = false;
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
    
            SevenSegmentDP(0, false);
            SevenSegmentDP(1, true);
            SevenSegmentDP(2, true);
            SevenSegmentDP(3, false);
    
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

 if(length < 1500 ){ /* we only process messages up to 1500 elements */
  Serial.print("Message arrived in topic: ");  // print "MQTT-Message arrived"
  Serial.print(topic);

  const size_t capacity = JSON_OBJECT_SIZE(3) + 3000;
  DynamicJsonBuffer jsonBuffer(capacity);
  
   
  JsonObject& root = jsonBuffer.parseObject(payload);
  if( root.containsKey("temperature") ){
       float temp = root["temperature"];
       /* We copy that to the current displayed temp */ 
       Current_Temp.valid=true;
       Current_Temp.temp= temp; 
       Serial.printf("Temperature found :%i °C",Current_Temp.temp);
  }
  
 

 }
  
}








 


