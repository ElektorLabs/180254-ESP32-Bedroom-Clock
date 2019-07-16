#ifndef _sevensegmentdisplay_h_
#define _sevensegmentdisplay_h_
#include "Arduino.h"

typedef enum {
  BEDROOMCLOCK_1_0,
  BEDROOMCLOCK_1_2,
  BEDROOMCLOCK_MINI_1_0,
  BEDROOMCLOCK_HW_CNT,  
} BedroomclockHW_t;

void onTimer( void );
void SevenSegmentSetup(BedroomclockHW_t HW );
void SevenSegmentDP( uint8_t idx, bool dpon ); 
void SevenSegmentWrite( uint8_t idx, char element);
void SevenSegmentBrightness(uint16_t Level);
uint16_t GetSevenSegmentBrightness( void );
BedroomclockHW_t GetSelectedHardware( void );
#endif
