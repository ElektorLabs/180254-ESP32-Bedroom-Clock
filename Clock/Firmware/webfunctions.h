#ifndef WEBFUNCTION_H_
#define WEBFUNCTION_H_

/**************************************************************************************************
*    Function      : response_settings
*    Description   : none
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/     
void response_settings( void );

/**************************************************************************************************
*    Function      : ntp_settings_update
*    Description   : Parses POST for new ntp settings
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void ntp_settings_update( void );

/**************************************************************************************************
*    Function      : timezone_update
*    Description   : none
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void timezone_update( void );

/**************************************************************************************************
*    Function      : timezone_overrides_update
*    Description   : none
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void timezone_overrides_update( void );

/**************************************************************************************************
*    Function      : settime_update
*    Description   : none
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void settime_update( void );

/**************************************************************************************************
*    Function      : update_ledactivespan
*    Description   : none
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void update_ledactivespan( void );

/**************************************************************************************************
*    Function      : update_notes
*    Description   : none
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void update_notes();

/**************************************************************************************************
*    Function      : read_notes
*    Description   : none
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void read_notes();

/**************************************************************************************************
*    Function      : led_update
*    Description   : Parses POST for new led settings
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/  
void led_update( void );

/**************************************************************************************************
*    Function      : led_status
*    Description   : Process GET for led settings
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/  
void led_status( void );

/**************************************************************************************************
*    Function      : update_display_sleepmode
*    Description   : Parses POST for new sleeptime 
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/
void ledactivespan_send( void );

/**************************************************************************************************
*    Function      : updates the mqtt settings
*    Description   : Parses POST for new MQTT settings 
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/
void mqttsettings_update( void );


/**************************************************************************************************
*    Function      : Reads back the MQTT settings
*    Description   : Sends a json string 
*    Input         : none
*    Output        : none
*    Remarks       : only contains if a password is set no data itself
**************************************************************************************************/
void read_mqttsetting( void );

#endif
