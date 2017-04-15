#include "HX711.h"
#include <Adafruit_NeoPixel.h>
#include <Tlc5940.h>
#include <Button.h>

//This value is obtained using the SparkFun_HX711_Calibration sketch. Currently calibrated to lbs
#define calibration_factor -9890.0
// previously -9330.0 

#define CLK  A0
#define DOUT  A1
#define LED_ALTAR_READY A2
#define LED_SAFETY_ALTAR A3
#define LED_SAFETY_TABLE A4

// Getting weird behaviour of some of the other pins like 0 and 8
#define GO_BUTTON 7
#define SPELL_BUTTON 8

//Button GO_BUTTON = Button(2, BUTTON_PULLUP_INTERNAL, true, 1000);
//Button SPELL_BUTTON = Button(4, BUTTON_PULLUP_INTERNAL, true, 1000);

//http://alex.kathack.com/codes/tlc5940arduino/html_r011/group__CoreFunctions.html
//4095 is the max value for a channel on a tlc5940
#define TLC_ON 4095
#define TLC_OFF 0

HX711 scale(DOUT, CLK);

//int totalPixels = 24; // LEDs in the neopixel ring
//int kegWeightFull = 130; //in lbs, which is what the scale is calibrated to
//int dimFactor = 2; // keeps the ratios but dims the brightness of the LED ring
//float colorTiers = 10; // Helps which rgbVal to display based on the weight, needs to be float for division purposes


//Adafruit_NeoPixel strip = Adafruit_NeoPixel(totalPixels, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);

  pinMode(LED_ALTAR_READY, OUTPUT);     
  pinMode(LED_SAFETY_ALTAR, OUTPUT);      
  pinMode(LED_SAFETY_TABLE, OUTPUT);
pinMode(GO_BUTTON, INPUT_PULLUP);      
  pinMode(SPELL_BUTTON, INPUT_PULLUP); 
pinMode(2, OUTPUT);  
digitalWrite(2, HIGH);
pinMode(4, OUTPUT);  
digitalWrite(4, HIGH);

  Tlc.init();
  Tlc.clear();
  Tlc.update();

  scale.set_scale(calibration_factor); 

  //Assuming there is no weight on the scale at start up, reset the scale to 0
  scale.tare();  

//  strip.begin();
}

void loop() {
  //scale.get_units() returns a float
  Serial.print(scale.get_units(), 1); 
  
   //You can change this to kg but you'll need to refactor the calibration_factor
  Serial.print(" lbs");
  Serial.println();
  Serial.print("GO_BUTTON: ");
//  Serial.print(GO_BUTTON.isPressed());
Serial.print(!digitalRead(GO_BUTTON));
  Serial.print(" | SPELL_BUTTON: ");
//  Serial.print(SPELL_BUTTON.isPressed());
Serial.print(!digitalRead(SPELL_BUTTON));
  Serial.println();
  
//  if(scale.get_units() > 100){
//    Tlc.set(0, TLC_ON); // Flame on!
//  } else {
//    Tlc.set(0, TLC_OFF); // Flame on!
//  }
  
  // Inverted because PULLUP resistor
  if((!digitalRead(GO_BUTTON) || !digitalRead(SPELL_BUTTON)) && scale.get_units() > 100){
    Tlc.set(0, TLC_ON); // Flame on!
  } else {
    Tlc.set(0, TLC_OFF); // Flame on!
  }
  
  Tlc.update();
}

