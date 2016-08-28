//thanks to Adafruit for the example RGB LED code and libraries

#include "SPI.h"
#include "Adafruit_NeoPixel.h"
/**********************************************/

// Number of RGB LEDs in strand:
int nLEDs = 19;

// Chose a pin for output; can be any valid output pin:
int dataPin  = 2;

//make an instance of the strip object
Adafruit_NeoPixel strip = Adafruit_NeoPixel(nLEDs, dataPin, NEO_GRB + NEO_KHZ800);

//runs on power on
void setup() {
  // Start up the LED strip
  strip.begin();
// Start serial at 9600 baud
  Serial.begin(9600); 
  // Update the strip, to start they are all 'off'
  strip.show();
}

//repeats while on
void loop() {
  //read the front(A0) and back(A1) force sensor values
  int forceReading1=analogRead(A0);
  int forceReading2=analogRead(A1);
  
  //iterate throught the LEDs(only half way because the shoe is a mirror image
  for (int i=0; i < strip.numPixels()/2; i++) {
      //set the current LED and its mirrored pair to a combination of the front and back force sensor readings, proportional to their distance from each sensor
      strip.setPixelColor(i, getColorFromForce((forceReading1*i+forceReading2*((strip.numPixels()/2)-i-1))/10));
      strip.setPixelColor(strip.numPixels()-i-1, getColorFromForce((forceReading1*i+forceReading2*((strip.numPixels()/2)-i-1))/10));
    }
    //send the new colors to the strip
  strip.show();
}

//helper function that shifts and scales the force reading to a range of 0-384(for the color wheel function)
uint32_t getColorFromForce(int force){
  //the shift and scaling factors were found experimentally
  int color=(force-384)/1.3;
  if (color>384){
    color=384;
  }
  else if (color<0){
    color=0;
  }
  //return the color that matches the force
  return Wheel(color);
}

//Input a value 0 to 384 to get a color value.
//The colours are a transition b - g -r - back to b
uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      b = 127 - WheelPos % 128;   //blue down
      g = WheelPos % 128;      // Green up
      r = 0;                  //red off
      break; 
    case 1:
      g = 127 - WheelPos % 128;  //green down
      r = WheelPos % 128;      //red up
      b = 0;                  //blue off
      break; 
    case 2:
      r = 127 - WheelPos % 128;  //red down 
      b = WheelPos % 128;      //blue up
      g = 0;                  //green off
      break; 
  }
  return(strip.Color(r,g,b));
}
