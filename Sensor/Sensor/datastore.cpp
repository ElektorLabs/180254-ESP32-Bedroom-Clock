#include <EEPROM.h>
#include <CRC32.h>
#include "datastore.h"


/* function prototypes */
void eepwrite_struct(void* data_in, uint32_t e_size, uint32_t address );
bool eepread_struct( void* element, uint32_t e_size, uint32_t startaddr  );

/* This will be the layout used by the data within the flash */
#define MQTT_START 0
/* this takes 1024byte */
/**************************************************************************************************
 *    Function      : datastoresetup
 *    Description   : Gets the EEPROM Emulation set up
 *    Input         : none 
 *    Output        : none
 *    Remarks       : We use 4096 byte for EEPROM 
 **************************************************************************************************/
void datastoresetup() {
  /* We emulate 4096 byte here */
  EEPROM.begin(4096);

}







/**************************************************************************************************
 *    Function      : eepread_struct
 *    Description   : reads a given block from flash / eeprom 
 *    Input         : void* element, uint32_t e_size, uint32_t startaddr  
 *    Output        : bool ( true if read was okay )
 *    Remarks       : Reads a given datablock into flash and checks the the crc32 
 **************************************************************************************************/
bool eepread_struct( void* element, uint32_t e_size, uint32_t startaddr  ){
  
  bool done = true;
  CRC32 crc;
  Serial.println("Read EEPROM");
  uint8_t* ret_ptr=(uint8_t*)(element);
  uint8_t data;
  
  for(uint32_t i=startaddr;i<(e_size+startaddr);i++){
      data = EEPROM.read(i);
      crc.update(data);
      *ret_ptr=data;
      ret_ptr++; 
  }
  /* Next is to read the crc32*/
  uint32_t data_crc = crc.finalize(); 
  uint32_t saved_crc=0;
  uint8_t* bytedata = (uint8_t*)&saved_crc;
  for(uint32_t z=e_size+startaddr;z<e_size+startaddr+sizeof(data_crc);z++){
    *bytedata=EEPROM.read(z);
    bytedata++;
  } 
  
  if(saved_crc!=data_crc){
    Serial.println("SAVED CONF");
    done = false;
  }

  return done;
}

/**************************************************************************************************
 *    Function      : eepwrite_struct
 *    Description   : writes the display settings
 *    Input         : void* data, uint32_t e_size, uint32_t address 
 *    Output        : bool
 *    Remarks       : Writes a given datablock into flash and adds a crc32 
 **************************************************************************************************/
void eepwrite_struct(void* data_in, uint32_t e_size, uint32_t address ){
  Serial.println("Write EEPROM");
  uint8_t* data=(uint8_t*)(data_in);
  CRC32 crc;
  
 
  for(uint32_t i=address;i<e_size+address;i++){
      EEPROM.write(i,*data);
      crc.update(*data);
      data++;
  }
 /* the last thing to do is to calculate the crc32 for the data and write it to the end */
  uint32_t data_crc = crc.finalize(); 
  uint8_t* bytedata = (uint8_t*)&data_crc;
  for(uint32_t z=e_size+address;z<e_size+address+sizeof(data_crc);z++){
    EEPROM.write(z,*bytedata);
    bytedata++;
  } 
  EEPROM.commit();
  
}


/**************************************************************************************************
 *    Function      : eepwrite_mqttsettings
 *    Description   : write the mqtt settings
 *    Input         : mqttsettings_t data
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void eepwrite_mqttsettings(mqttsettings_t data){
  eepwrite_struct( ( (void*)(&data) ), sizeof(mqttsettings_t) , MQTT_START );
}

/**************************************************************************************************
 *    Function      : eepread_mqttsettings
 *    Description   : reads the mqtt settings
 *    Input         : none
 *    Output        : mqttsettings_t
 *    Remarks       : none
 **************************************************************************************************/
mqttsettings_t eepread_mqttsettings( void ){
  
  mqttsettings_t retval;
  if(false == eepread_struct( (void*)(&retval), sizeof(mqttsettings_t) , MQTT_START ) ){ 
    Serial.println("MQTT CONF");
    bzero((void*)&retval,sizeof( mqttsettings_t ));
    eepwrite_mqttsettings(retval);
  }
  return retval;

}


/**************************************************************************************************
 *    Function      : erase_eeprom
 *    Description   : writes the whole EEPROM with 0xFF  
 *    Input         : none
 *    Output        : none
 *    Remarks       : This will invalidate all user data 
 **************************************************************************************************/
void erase_eeprom( void ){
  
 
  for(uint32_t i=0;i<4096;i++){
     EEPROM.write(i,0xFF);
  }
  EEPROM.commit();
 
}






