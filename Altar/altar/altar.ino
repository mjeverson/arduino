#include "HX711.h"
#include <Adafruit_NeoPixel.h>
#include <Tlc5940.h>

//This value is obtained using the SparkFun_HX711_Calibration sketch. Currently calibrated to lbs
#define calibration_factor -9890.0
// previously -9330.0 

#define CLK  A0
#define DOUT  A1
#define LED_ALTAR_READY A2
#define LED_SAFETY_ALTAR A3
#define LED_SAFETY_TABLE A4

#define GO_BUTTON 0
#define SPELL_BUTTON 1

HX711 scale(DOUT, CLK);

//http://alex.kathack.com/codes/tlc5940arduino/html_r011/group__CoreFunctions.html
//4095 is the max value for a channel on a tlc5940
#define TLC_ON 4095
#define TLC_OFF 0

//int totalPixels = 24; // LEDs in the neopixel ring
//int kegWeightFull = 130; //in lbs, which is what the scale is calibrated to
//int dimFactor = 2; // keeps the ratios but dims the brightness of the LED ring
//float colorTiers = 10; // Helps which rgbVal to display based on the weight, needs to be float for division purposes


//Adafruit_NeoPixel strip = Adafruit_NeoPixel(totalPixels, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);

  Tlc.init();
  Tlc.clear();
  Tlc.update();

  scale.set_scale(calibration_factor); 

  //Assuming there is no weight on the scale at start up, reset the scale to 0
  scale.tare();  

  strip.begin();
}

void loop() {
  //scale.get_units() returns a float
  Serial.print(scale.get_units(), 1); 
  
   //You can change this to kg but you'll need to refactor the calibration_factor
  Serial.print(" lbs");
  Serial.println();
  
  for (int c = 0; c < 7; c++) {
    if (holes[c].uniquePress()) {
      Tlc.set(SOL[c], TLC_ON); // Flame on!
      delay(1000);
    }
  }
}

