#include <FastSPI_LED2.h>

// How many leds are in the strip?
#define NUM_LEDS 240

/*
 * pin 11 -> DATA
 * pin 13 -> CLK
 */

#define MODE_HDRA	0
#define MODE_HDRd	1
#define MODE_HDRa	2
#define MODE_HDRhi	3
#define MODE_HDRlo	4
#define MODE_HDRchk	5
#define MODE_DATAr	6
#define MODE_DATAg	7
#define MODE_DATAb	8

// This is an array of leds.  One item for each led in your strip.
CRGB leds[NUM_LEDS];
//brg
unsigned long t;
unsigned long startTime;
unsigned long lastByteTime;
unsigned long lastAckTime;
boolean sleeping = false;
uint8_t hi;
uint8_t lo;
uint8_t chk;
uint8_t mode = MODE_HDRA;
int channels;
int channel;
int testled=0;

// This function sets up the ledsand tells the controller about them
void setup() {
      // sanity check delay - allows reprogramming if accidently blowing power w/leds
      delay(2000);
      Serial.begin(115200);
      //remember to set this to correct values, my strip is BRG instead of RGB
      FastLED.addLeds<LPD8806, BRG>(leds, NUM_LEDS);
}

void clearLeds(){
  memset(leds, 0,  NUM_LEDS * sizeof(struct CRGB));
  FastLED.show();
  delay(20);
}

// This function runs over and over, and is where you do the magic to light
// your leds.
void loop(){
  int16_t c;

  t = millis();

  if((t - lastAckTime) > 1000) {
    Serial.print("Ada\n"); // Send ACK string to host
    lastAckTime = t; // Reset counter
  }
  if ((t - lastByteTime) > 5000) {
    mode = MODE_HDRA;     // After 5 seconds reset to HDRA
    if(!sleeping)
      clearLeds(); //also clear led
    sleeping = true;
  }
  if ((t - lastByteTime) > 10000) { //after 10 seconds activate test mode
    testmode();
  }
  
  if(Serial.available()){
    sleeping = false;
    c = Serial.read();
    lastByteTime = t;
    lastAckTime = t;
    
    switch(mode){
    case MODE_HDRA:
      if (c == 0x41) {
        mode = MODE_HDRd;
      }
      break;
    case MODE_HDRd:
      if (c == 0x64) {
        mode = MODE_HDRa;
      }
      break;
    case MODE_HDRa:
      if (c == 0x61) {
        mode = MODE_HDRhi;
      }
      break;
    case MODE_HDRhi:
      hi = c;
      mode = MODE_HDRlo;
      break;
    case MODE_HDRlo:
      lo = c;
      mode = MODE_HDRchk;
      break;
    case MODE_HDRchk:
      chk = c;
      if (chk == (hi ^ lo ^ 0x55)){
        mode = MODE_DATAr;
        channels = (long)hi*256+(long)lo;
        channel = 0;
       } else {
        // wrong checksum, reset header
        mode = MODE_HDRA;
      }
      break;
    case MODE_DATAr:
      // we are in the data business ;-)
      // ignore all data for channels higher than NUM_LEDS
      if (channel<NUM_LEDS) { leds[channel].r = c; }
      mode = MODE_DATAb;
      break;
    case MODE_DATAb:
      // we are in the data business ;-)
      // ignore all data for channels higher than NUM_LEDS
      if (channel<NUM_LEDS) { leds[channel].b = c; }
      mode = MODE_DATAg;
      break;
    case MODE_DATAg:
      // we are in the data business ;-)
      // ignore all data for channels higher than NUM_LEDS
      if (channel<NUM_LEDS) { leds[channel].g = c; }
      channel++;
      if (channel>channels) {
        FastSPI_LED.show();
        mode = MODE_HDRA;
      } else {
        mode = MODE_DATAr;
      }
      break;
    default:
      // I should not be here, back to
      mode = MODE_HDRA;
    }    
  }
}

void testmode(){
  //clear leds off, then light one up
  memset(leds, 0,  NUM_LEDS * sizeof(struct CRGB));
  channel = testled%NUM_LEDS;
  if(testled<NUM_LEDS)
    leds[channel]= CRGB::White; 
  else if(testled<NUM_LEDS*2)
    leds[channel]= CRGB::Red; 
  else if(testled<NUM_LEDS*3)
    leds[channel]= CRGB::Green; 
  else if(testled<NUM_LEDS*4)
    leds[channel]= CRGB::Blue; 
  else
    testled = -1;
  testled = testled + 1;
  delay(100);
  FastLED.show();
}
