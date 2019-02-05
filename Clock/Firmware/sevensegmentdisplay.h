#ifndef _sevensegmentdisplay_h_
#define _sevensegmentdisplay_h_
#include "Arduino.h"

void onTimer( void );
void SevenSegmentSetup(void );
void SevenSegmentDP( uint8_t idx, bool dpon ); 
void SevenSegmentWrite( uint8_t idx, char element);
void SevenSegmentBrightness(uint16_t Level);
uint16_t GetSevenSegmentBrightness( void );
#endif
