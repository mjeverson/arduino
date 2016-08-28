/*
 Example using the SparkFun HX711 breakout board with a scale
 By: Nathan Seidle
 SparkFun Electronics
 Date: November 19th, 2014
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 This example demonstrates basic scale output. See the calibration sketch to get the calibration_factor for your
 specific load cell setup.
 
 This example code uses bogde's excellent library: https://github.com/bogde/HX711
 bogde's library is released under a GNU GENERAL PUBLIC LICENSE
 
 The HX711 does one thing well: read load cells. The breakout board is compatible with any wheat-stone bridge
 based load cell which should allow a user to measure everything from a few grams to tens of tons.
 Arduino pin 2 -> HX711 CLK
 3 -> DAT
 5V -> VCC
 GND -> GND
 
 The HX711 board can be powered from 2.7V to 5V so the Arduino 5V power should be fine.
 
*/

#include "HX711.h"
#include <Adafruit_NeoPixel.h>

#define calibration_factor -9330.0 //This value is obtained using the SparkFun_HX711_Calibration sketch

#define DOUT  3
#define CLK  2
#define LED_PIN 4

HX711 scale(DOUT, CLK);
int N_PIXELS = 24;
int kegWeightFull = 130;
int kegWeightEmpty = 10;
int updateFrequency = 1;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  Serial.println("HX711 scale demo");

  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.tare();  //Assuming there is no weight on the scale at start up, reset the scale to 0

  strip.begin();
  Serial.println("Readings:");
}

void loop() {
  Serial.print("Reading: ");
  Serial.print(scale.get_units(), 1); //scale.get_units() returns a float
  Serial.print(" lbs"); //You can change this to kg but you'll need to refactor the calibration_factor
  Serial.println();
  
  setColor();
}

void setColor(){
  float weight = scale.get_units();
  float percentage = weight / kegWeightFull;
  
  int red = 0;
  int green = 0;
  int blue = 0;
  
  int pixels = percentage * 24;
  int percentile = percentage / 0.125;
  
  Serial.print("Percentile: ");
  Serial.print(percentile);
  Serial.println();
  Serial.print("Pixels: ");
  Serial.print(pixels);
  Serial.println();
  
  if (percentile == 0){
    red = 255;
    green = 0;
  } else if (percentile == 1){
    red = 255;
    green = 16;
  } else if (percentile == 2){
    red = 255;
    green = 32;
  } else if (percentile == 3){
    red = 255;
    green = 64;
  } else if (percentile == 4){
    red = 255;
    green = 128;
  } else if (percentile == 5){
    red = 255;
    green = 255;
  } else if (percentile == 6){
    red = 128;
    green = 255;
  } else if (percentile == 7){
    red = 64;
    green = 255;
  } else if (percentile == 8){
    red = 32;
    green = 255;
  } else if (percentile > 8){
    red = 0;
    green = 255;
  } else{
    red = 255;
    green = 0;
  }
  
  for(int i=0; i<N_PIXELS; i++) {
    if ((i + 1) > pixels && i > 0){ 
      strip.setPixelColor(i, 0, 0, 0);
    } else {
      strip.setPixelColor(i, red / 2, green / 2, blue / 2);
    }
  }
  
  strip.show();
}

