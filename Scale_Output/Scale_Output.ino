#include "HX711.h"
#include <Adafruit_NeoPixel.h>

//This value is obtained using the SparkFun_HX711_Calibration sketch. Currently calibrated to lbs
#define calibration_factor -9330.0 

#define DOUT  3
#define CLK  2
#define LED_PIN 4

HX711 scale(DOUT, CLK);
int totalPixels = 24; // LEDs in the neopixel ring
int kegWeightFull = 130; //in lbs, which is what the scale is calibrated to
int dimFactor = 2; // keeps the ratios but dims the brightness of the LED ring
float colorTiers = 10; // Helps which rgbVal to display based on the weight, needs to be float for division purposes

int rgbVals[11][3] = {
  { 255, 0, 0 }, // Empty, red
  { 255, 16, 0},
  { 255, 32, 0 },
  { 255, 64, 0 },
  { 255, 128, 0 },
  { 255, 255, 0 },
  { 128, 255, 0 },
  { 64, 255, 0 },
  { 32, 255, 0 },
  { 16, 255, 0 },
  { 0, 255, 0 }, // Full, green
};

Adafruit_NeoPixel strip = Adafruit_NeoPixel(totalPixels, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);

  scale.set_scale(calibration_factor); 

  //Assuming there is no weight on the scale at start up, reset the scale to 0
  scale.tare();  

  strip.begin();
}

void loop() {
  Serial.print("Reading: ");
  
  //scale.get_units() returns a float
  Serial.print(scale.get_units(), 1); 
  
   //You can change this to kg but you'll need to refactor the calibration_factor
  Serial.print(" lbs");
  Serial.println();
  
  setColor();
}

void setColor(){
  int red = 0;
  int green = 0;
  int blue = 0;
  
  float weight = scale.get_units();
  float percentage = weight / kegWeightFull;

  // Normalize anything over 100% to 100%
  if (percentage > 1){
    percentage = 1;
  }

  // Determine how many pixels to light up and what color "tier" to set
  int activePixels = percentage * totalPixels;
  float tierSize = 1 / colorTiers;
  int colorTier = percentage / tierSize;

  // Pick the color based on how full the keg is
  red = rgbVals[colorTier][0];
  green = rgbVals[colorTier][1];
  blue = rgbVals[colorTier][2];
  
  // Apply the color settings
  for(int i=0; i<totalPixels; i++) {
    if (i > 0 && i >= activePixels){ 
      strip.setPixelColor(i, 0, 0, 0);
    } else {
      strip.setPixelColor(i, red / dimFactor, green / dimFactor, blue / dimFactor);
    }
  }
  
  strip.show();
}

