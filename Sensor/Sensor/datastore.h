#ifndef DATASTORE_H_
 #define DATASTORE_H_
 



typedef struct {
  bool enable;
  char mqttservename[129];
  uint16_t mqttserverport;
  char mqttusername[129];
  char mqttpassword[129];
  char mqtttopic[501];
  char mqtthostname[65];
}mqttsettings_t; /*956 byte */

/**************************************************************************************************
 *    Function      : datastoresetup
 *    Description   : Gets the EEPROM Emulation set up
 *    Input         : none 
 *    Output        : none
 *    Remarks       : We use 4096 byte for EEPROM 
 **************************************************************************************************/
void datastoresetup();


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
 *    Function      : erase_eeprom
 *    Description   : writes the whole EEPROM with 0xFF  
 *    Input         : none
 *    Output        : none
 *    Remarks       : This will invalidate all user data 
 **************************************************************************************************/
void erase_eeprom( void );


#endif
