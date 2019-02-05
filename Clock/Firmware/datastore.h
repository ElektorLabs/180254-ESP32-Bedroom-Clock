#ifndef DATASTORE_H_
 #define DATASTORE_H_
 
#include "timecore.h"

typedef struct {
  char ssid[128];
  char pass[128];
} credentials_t;
/* 256 byte */

typedef struct {
  bool enable;
  char mqttservename[129];
  uint16_t mqttserverport;
  char mqttusername[129];
  char mqttpassword[129];
  char mqtttopic[501];
  char mqtthostname[65];
}mqttsettings_t; /*956 byte */

typedef struct{
  char ntpServerName[129];
  bool NTPEnable;
  int32_t SyncIntervall;
} ntp_config_t; 
/* 137 byte */

typedef struct{
  uint16_t ledlevel;
} displaysettings_t;



/**************************************************************************************************
 *    Function      : datastoresetup
 *    Description   : Gets the EEPROM Emulation set up
 *    Input         : none 
 *    Output        : none
 *    Remarks       : We use 4096 byte for EEPROM 
 **************************************************************************************************/
void datastoresetup();



/**************************************************************************************************
 *    Function      : write_ntp_config
 *    Description   : writes the ntp config
 *    Input         : ntp_config_t c 
 *    Output        : none
 *    Remarks       : none 
 **************************************************************************************************/
void write_ntp_config(ntp_config_t c);


/**************************************************************************************************
 *    Function      : read_ntp_config
 *    Description   : writes the ntp config
 *    Input         : none
 *    Output        : ntp_config_t
 *    Remarks       : none
 **************************************************************************************************/
ntp_config_t read_ntp_config( void );



/**************************************************************************************************
 *    Function      : write_timecoreconf
 *    Description   : writes the time core config
 *    Input         : timecoreconf_t
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void write_timecoreconf(timecoreconf_t c);


/**************************************************************************************************
 *    Function      : read_timecoreconf
 *    Description   : reads the time core config
 *    Input         : none
 *    Output        : timecoreconf_t
 *    Remarks       : none
 **************************************************************************************************/
timecoreconf_t read_timecoreconf( void );


/**************************************************************************************************
 *    Function      : write_credentials
 *    Description   : writes the wifi credentials
 *    Input         : credentials_t
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void write_credentials(credentials_t c);


/**************************************************************************************************
 *    Function      : read_credentials
 *    Description   : reads the wifi credentials
 *    Input         : none
 *    Output        : credentials_t
 *    Remarks       : none
 **************************************************************************************************/
credentials_t read_credentials( void );

/**************************************************************************************************
 *    Function      : eepwrite_notes
 *    Description   : writes the user notes 
 *    Input         : uint8_t* data, uint32_t size
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void eepwrite_notes(uint8_t* data, uint32_t size);

/**************************************************************************************************
 *    Function      : eepread_notes
 *    Description   : reads the user notes 
 *    Input         : uint8_t* data, uint32_t size
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void eepread_notes(uint8_t* data, uint32_t size);

/**************************************************************************************************
 *    Function      : eepwrite_mqttsettings
 *    Description   : write the mqtt settings
 *    Input         : mqttsettings_t data
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void eepwrite_mqttsettings(mqttsettings_t data);

/**************************************************************************************************
 *    Function      : eepread_mqttsettings
 *    Description   : reads the mqtt settings
 *    Input         : none
 *    Output        : mqttsettings_t
 *    Remarks       : none
 **************************************************************************************************/
mqttsettings_t eepread_mqttsettings( void );

/**************************************************************************************************
 *    Function      : eepwrite_ledsettings
 *    Description   : write the led settings
 *    Input         : displaysettings_t data
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void eepwrite_ledsettings(displaysettings_t data);

/**************************************************************************************************
 *    Function      : eepread_ledsettings
 *    Description   : reads the mqtt settings
 *    Input         : none
 *    Output        : displaysettings_t
 *    Remarks       : none
 **************************************************************************************************/
displaysettings_t eepread_ledsettings( void );

/**************************************************************************************************
 *    Function      : erase_eeprom
 *    Description   : writes the whole EEPROM with 0xFF  
 *    Input         : none
 *    Output        : none
 *    Remarks       : This will invalidate all user data 
 **************************************************************************************************/
void erase_eeprom( void );


#endif
