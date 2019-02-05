/* ISR driven multiplex for the seven segment displays */
#include "datastore.h"
#include "sevensegmentdisplay.h"


typedef struct {
bool Segment[8];
bool UpsideDown;  
} SevenSegemntDisp_t;

#define SegmentA ( 0 )
#define SegmentB ( 1 )
#define SegmentC ( 2 )
#define SegmentD ( 3 )
#define SegmentE ( 4 )
#define SegmentF ( 5 )
#define SegmentG ( 6 )
#define SegmentDP ( 7 )

// setting PWM properties
// We use 5kHz for the PWM
#define PWM_FREQ ( 5000 )
// We use 16 Bit resulution 
#define RWM_RES ( 13 )


#define SEGMENT_OFF LOW 
#define SEGMENT_ON HIGH
#define DISPLAY_ON HIGH
#define DISPLAY_OFF LOW

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
/*
volatile int DisplaySegmentPins[]= {12,13,26,32,33,14,27,25 };
volatile int DisplayCommonPins[]= {23 ,18 , 05 ,10 };
*/

/* Pins for Version 1.2 */
volatile int DisplaySegmentPins[]= {25,26,32,33,27,14,12,13 };
volatile int DisplayCommonPins[]= {23 ,19 , 18 ,05 };

volatile SevenSegemntDisp_t DisplayElement[4] = { { .Segment={false,},.UpsideDown=false }, };
volatile uint8_t current_segment=0;

void SevenSegmentPinSetup( void );
void SevenSegmentSetElement(volatile SevenSegemntDisp_t* elementptr, char Element );
void SevenSegmentSetDP(volatile SevenSegemntDisp_t* elementptr, bool DP_Ena);

void SevenSegmentSetup(){
  displaysettings_t led_settings =  eepread_ledsettings();
    /* Setup pins */
   SevenSegmentPinSetup();
   /* Last step is to setup the timer */
   timer = timerBegin(0, 80, true);
   timerAttachInterrupt(timer, &onTimer, true);
   /* we fire the interrupt at 400Hz */
   timerAlarmWrite(timer, 2500, true); 
   timerAlarmEnable(timer);
   /* read the last settings */
   SevenSegmentBrightness( led_settings.ledlevel );
   
}



void SevenSegmentPinSetup( ){

  for(uint8_t i=0;i<( sizeof(DisplayCommonPins ) / sizeof( DisplayCommonPins[0] ));i++){
    digitalWrite(DisplayCommonPins[i],DISPLAY_OFF);
    pinMode(DisplayCommonPins[i], OUTPUT);
  }

  /* Disable all segments */
  for(uint8_t i=0;i<( sizeof(DisplaySegmentPins ) / sizeof( DisplaySegmentPins[0] ));i++){
    digitalWrite(DisplaySegmentPins[i],SEGMENT_OFF);
    pinMode(DisplaySegmentPins[i], OUTPUT);
  }

  /* we need to rotate DIS1 */
  DisplayElement[2].UpsideDown=true;
  /* We use only one channel */
  ledcSetup(0, PWM_FREQ ,RWM_RES );
  ledcWrite(0, 0); 

}

void SevenSegmentBrightness(uint16_t Level){
  Level = Level >> 3;
  if(Level>8192){
    Serial.println("Brightness level clipped!"); 
  }
  ledcWrite(0, Level ); 
}

uint16_t GetSevenSegmentBrightness( ){
 uint32_t value = ledcRead(0);
 /* We need to shift it to get 16 bit from out 13 */
 if(value > 8192 ){
  Serial.println("Brightness level out of range");
 }  
 value = value <<3;
 if(value>UINT16_MAX){
  value = UINT16_MAX;
 } 

 return value; 

}

void SevenSegmentDP( uint8_t idx, bool dpon ){
  if(idx<4){
    SevenSegmentSetDP(&DisplayElement[idx],dpon);
  }
}
void SevenSegmentWrite( uint8_t idx, char element){
   if(idx<4){
    SevenSegmentSetElement(&DisplayElement[idx],element);
  }
}

void SevenSegmentSetDP(volatile SevenSegemntDisp_t* elementptr, bool DP_Ena){
  elementptr->Segment[SegmentDP]=DP_Ena;
}


void SevenSegmentSetElement(volatile SevenSegemntDisp_t* elementptr, char Element ){
  SevenSegemntDisp_t NewSegment = { .Segment={false,},.UpsideDown=false };
  NewSegment.UpsideDown=elementptr->UpsideDown;
  SevenSegemntDisp_t* ptr = &NewSegment;
  
  NewSegment.Segment[SegmentDP]=elementptr->Segment[SegmentDP];
  /* Turn everything off */
  ptr->Segment[SegmentA] = LOW;
  ptr->Segment[SegmentB] = LOW;
  ptr->Segment[SegmentC] = LOW;
  ptr->Segment[SegmentD] = LOW;
  ptr->Segment[SegmentE] = LOW;
  ptr->Segment[SegmentF] = LOW;
  ptr->Segment[SegmentG] = LOW;
  

  if('0' == Element){
    ptr->Segment[SegmentA] = HIGH;
    ptr->Segment[SegmentB] = HIGH;
    ptr->Segment[SegmentC] = HIGH;
    ptr->Segment[SegmentD] = HIGH;
    ptr->Segment[SegmentE] = HIGH;
    ptr->Segment[SegmentF] = HIGH;
  }
  
  if('1' == Element){
    ptr->Segment[SegmentB] = HIGH;
    ptr->Segment[SegmentC] = HIGH;
   
  }

  if('2' == Element){
    ptr->Segment[SegmentA] = HIGH;
    ptr->Segment[SegmentB] = HIGH;
    ptr->Segment[SegmentD] = HIGH;
    ptr->Segment[SegmentE] = HIGH;
    ptr->Segment[SegmentG] = HIGH; 
  }

  if('3' == Element){
    ptr->Segment[SegmentA] = HIGH;
    ptr->Segment[SegmentB] = HIGH;
    ptr->Segment[SegmentC] = HIGH;
    ptr->Segment[SegmentD] = HIGH;
    ptr->Segment[SegmentG] = HIGH; 
  }

  if('4' == Element){
    ptr->Segment[SegmentB] = HIGH;
    ptr->Segment[SegmentC] = HIGH;
    ptr->Segment[SegmentF] = HIGH;
    ptr->Segment[SegmentG] = HIGH;
  }

  if('5' == Element){
    ptr->Segment[SegmentA] = HIGH;
    ptr->Segment[SegmentF] = HIGH;
    ptr->Segment[SegmentG] = HIGH;
    ptr->Segment[SegmentC] = HIGH;
    ptr->Segment[SegmentD] = HIGH;

  }

  if('6' == Element){
    ptr->Segment[SegmentA] = HIGH;
    ptr->Segment[SegmentF] = HIGH;
    ptr->Segment[SegmentE] = HIGH;
    ptr->Segment[SegmentD] = HIGH;
    ptr->Segment[SegmentC] = HIGH;
    ptr->Segment[SegmentG] = HIGH;
  }

  if('7' == Element){
    ptr->Segment[SegmentA] = HIGH;
    ptr->Segment[SegmentB] = HIGH;
    ptr->Segment[SegmentC] = HIGH;
  }

  if('8' == Element){
    
    ptr->Segment[SegmentA] = HIGH;
    ptr->Segment[SegmentB] = HIGH;
    ptr->Segment[SegmentC] = HIGH;
    ptr->Segment[SegmentD] = HIGH;
    ptr->Segment[SegmentE] = HIGH;
    ptr->Segment[SegmentF] = HIGH;
    ptr->Segment[SegmentG] = HIGH;
  
  }

  if('9' == Element){
    ptr->Segment[SegmentG] = HIGH;
    ptr->Segment[SegmentF] = HIGH;
    ptr->Segment[SegmentA] = HIGH;
    ptr->Segment[SegmentB] = HIGH;
    ptr->Segment[SegmentC] = HIGH;
    ptr->Segment[SegmentD] = HIGH;

  }


  if(Element=='-'){
    ptr->Segment[SegmentG]= HIGH;
  }

  if(Element=='\370'){ // ASCII code 248 or degree symbol: '°'
    ptr->Segment[SegmentA]= HIGH;
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
    ptr->Segment[SegmentG]= HIGH;

  }

  /* we need to get everything down to small letters */
  if( ( Element>=97 ) && (Element<=122) ){
    Element-=32;
  }

  if(Element=='A'){
    ptr->Segment[SegmentA]= HIGH;
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
    ptr->Segment[SegmentG]= HIGH;
   }

  if(Element=='B'){
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
    ptr->Segment[SegmentG]= HIGH;

  }

  if(Element=='C'){
    ptr->Segment[SegmentA]= HIGH;
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
  }

  if(Element=='D'){
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentG]= HIGH;
  }

  if(Element=='E'){
    ptr->Segment[SegmentA]= HIGH;
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
    ptr->Segment[SegmentG]= HIGH;
  }

  if(Element=='F'){
    ptr->Segment[SegmentA]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
    ptr->Segment[SegmentG]= HIGH;
  }

  if(Element=='G'){

    ptr->Segment[SegmentA]= HIGH;
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
  }

  if(Element=='H'){
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
    ptr->Segment[SegmentG]= HIGH;
  }

  if(Element=='I'){
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;    
  }

  if(Element=='J'){
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
  }

  if(Element=='K'){
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
    ptr->Segment[SegmentG]= HIGH;
  }

  if(Element=='L'){
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
  }

  if(Element=='M'){
    ptr->Segment[SegmentA]= HIGH;
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
  }

  if(Element=='N'){
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentG]= HIGH;
  }

  if(Element=='O'){
    ptr->Segment[SegmentA]= HIGH;
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
  }

  if(Element=='P'){
    ptr->Segment[SegmentA]= HIGH;
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
    ptr->Segment[SegmentG]= HIGH;
  }

  if(Element=='Q'){
    ptr->Segment[SegmentA]= HIGH;
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
    ptr->Segment[SegmentG]= HIGH;

  }

  if(Element=='R'){
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentG]= HIGH;
  }

  if(Element=='S'){
    ptr->Segment[SegmentA]= HIGH;
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
    ptr->Segment[SegmentG]= HIGH;
  }

  if(Element=='T'){
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
    ptr->Segment[SegmentG]= HIGH;
  }

 if(Element=='U'){
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
  }

  if(Element=='V'){
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
  }

  if(Element=='W'){
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
  }

  if(Element=='X'){
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
    ptr->Segment[SegmentG]= HIGH;
  }

  if(Element=='Y'){
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentC]= HIGH;
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentF]= HIGH;
    ptr->Segment[SegmentG]= HIGH;
  }

  if(Element=='Z'){
    ptr->Segment[SegmentA]= HIGH;
    ptr->Segment[SegmentB]= HIGH;
    ptr->Segment[SegmentD]= HIGH;
    ptr->Segment[SegmentE]= HIGH;
  }

  /* Last we need to check if we have to rotate it */
  if(elementptr->UpsideDown==true){
    bool tempval=false;
    /* Rotate elements */
    /* We need to rotate the shown segments 
     * A <-> D 
     * F <-> C
     * E <-> B
     */
   
    tempval = ptr->Segment[SegmentA];
    ptr->Segment[SegmentA] = ptr->Segment[SegmentD];
    ptr->Segment[SegmentD] = tempval;
    
    tempval = ptr->Segment[SegmentF];
    ptr->Segment[SegmentF] = ptr->Segment[SegmentC];
    ptr->Segment[SegmentC] = tempval; 

    tempval = ptr->Segment[SegmentE];
    ptr->Segment[SegmentE] = ptr->Segment[SegmentB];
    ptr->Segment[SegmentB] = tempval; 

  } 

  /* Copy data back */
  memcpy( (void*)(elementptr), (const void*)(ptr), sizeof(SevenSegemntDisp_t) );
  
}


/* We need to setup some variables in ram as volatile ( shared ones ) 
 *  also we do here some pin definitions and borrow some code 
 */
/* INTERRUP Section */


void IRAM_ATTR onTimer() {
  static const uint8_t chan = 0 ;
  portENTER_CRITICAL_ISR(&timerMux);
  /* This runs with 400Hz with 4 Displays resulting in 100 refreshs per display per second*/
    /* Next Segment next time */
  /* Switch all Segments off, here by setting the output to VCC / HIGH */
  for(uint8_t i=0;i<( sizeof(DisplayCommonPins ) / sizeof( DisplayCommonPins[0] ));i++){
    digitalWrite(DisplayCommonPins[i],DISPLAY_OFF);
    pinMatrixOutDetach(DisplayCommonPins[i], false, false);
  }

  /* Disable all segments */
  for(uint8_t i=0;i<( sizeof(DisplaySegmentPins ) / sizeof( DisplaySegmentPins[0] ));i++){
    digitalWrite(DisplaySegmentPins[i],SEGMENT_OFF);
  }
  /* display is now completly off */

  
  
  /* Write the corresponding value to the pins */
  for(uint8_t i=0;i<( sizeof(DisplayElement[current_segment].Segment ) / sizeof( DisplayElement[current_segment].Segment[0] ));i++){
    if(DisplayElement[current_segment].Segment[i]!=false){
          digitalWrite(DisplaySegmentPins[i],SEGMENT_ON);      
    }
  }
  
  //digitalWrite(DisplayCommonPins[current_segment],DISPLAY_ON);
  //We use CH0 static here */ 
  pinMatrixOutAttach(DisplayCommonPins[current_segment], ((chan/8)?LEDC_LS_SIG_OUT0_IDX:LEDC_HS_SIG_OUT0_IDX) + (chan%8), false, false);

  current_segment++;
  if(current_segment >= ( sizeof(DisplayCommonPins ) / sizeof( DisplayCommonPins[0] )) ){
    current_segment=0;
  }
  portEXIT_CRITICAL_ISR(&timerMux);
 
}
