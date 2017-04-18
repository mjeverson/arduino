#include <HX711.h>
#include <Tlc5940.h>

//TODO: only need one of these, try FastLED
#include <FastLED.h>
#include <Adafruit_NeoPixel.h>

//This value is obtained using the SparkFun_HX711_Calibration sketch. Currently calibrated to lbs, previously -9330.0 
#define calibration_factor -9890.0

#define CLK  A0
#define DOUT  A1
#define LED_ALTAR_READY_PIN A2
#define LED_SAFETY_ALTAR_PIN A3
#define LED_SAFETY_TABLE_PIN A4

#define NUM_ALTAR_READY_LEDS 180
#define NUM_ALTAR_SAFETY_LEDS 150
#define NUM_TABLE_SAFETY_LEDS 300

#define GO_SWITCH 2
#define SPELL_SWITCH 4
#define GO_BUTTON 7
#define SPELL_BUTTON 8

//http://alex.kathack.com/codes/tlc5940arduino/html_r011/group__CoreFunctions.html
//4095 is the max value for a channel on a tlc5940
#define TLC_ON 4095
#define TLC_OFF 0

// Sets up the scale
HX711 scale(DOUT, CLK);

// Adafruit_NeoPixel(number of pixels in strip, pin #, pixel type flags add as needed)
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//Adafruit_NeoPixel altar_ready = Adafruit_NeoPixel(NUM_ALTAR_READY_LEDS, LED_ALTAR_READY_PIN, NEO_GRB + NEO_KHZ800);
//Adafruit_NeoPixel altar_safety = Adafruit_NeoPixel(NUM_ALTAR_SAFETY_LEDS, LED_SAFETY_ALTAR_PIN, NEO_GRB + NEO_KHZ800); 
//Adafruit_NeoPixel table_safety = Adafruit_NeoPixel(NUM_TABLE_SAFETY_LEDS, LED_SAFETY_TABLE_PIN, NEO_GRB + NEO_KHZ800); 

//todo: very memory intensive, can we get an SD card for variable memory or something?
//FASTLED Code
CRGBArray<180> altar_ready_strip;
CRGBArray<1> altar_safety_strip;
CRGBArray<1> table_safety_strip;

CRGB leds[3][NUM_TABLE_SAFETY_LEDS];

void setup() {
  Serial.begin(9600);

//  pinMode(LED_ALTAR_READY, OUTPUT);     
//  pinMode(LED_SAFETY_ALTAR, OUTPUT);      
//  pinMode(LED_SAFETY_TABLE, OUTPUT);
  pinMode(GO_BUTTON, INPUT_PULLUP);      
  pinMode(SPELL_BUTTON, INPUT_PULLUP); 
  pinMode(GO_SWITCH, OUTPUT);  
  pinMode(SPELL_SWITCH, OUTPUT);  
  
  digitalWrite(GO_SWITCH, HIGH);
  digitalWrite(SPELL_SWITCH, HIGH);

  Tlc.init();
  Tlc.clear();
  Tlc.update();

  scale.set_scale(calibration_factor); 
  scale.tare();  

  //altar_ready.begin();
  //altar_safety.begin();
//  table_safety.begin();

  //altar_ready.show();
  //altar_safety.show();
//  table_safety.show();

  FastLED.addLeds<NEOPIXEL, LED_ALTAR_READY_PIN>(altar_ready_strip, NUM_ALTAR_READY_LEDS);
  FastLED.addLeds<NEOPIXEL, LED_SAFETY_ALTAR_PIN>(table_safety_strip, NUM_ALTAR_SAFETY_LEDS);
  FastLED.addLeds<NEOPIXEL, LED_SAFETY_TABLE_PIN>(table_safety_strip, NUM_TABLE_SAFETY_LEDS);
 
  FastLED.clear();
  FastLED.show();
}

void loop() {
  for(int i = 0; i < NUM_TABLE_SAFETY_LEDS; i++){
    if (i < NUM_ALTAR_SAFETY_LEDS){
//      altar_safety.setPixelColor(i, 0, 0, 0);
 
    }
    
    if (i < NUM_ALTAR_READY_LEDS){
//           altar_ready.setPixelColor(i, 0, 0, 125);
    }
    
    //todo: does this save any memory?
//      fill_solid(table_safety_strip, NUM_TABLE_SAFETY_LEDS, CRGB::Red);

    //todo: does this save any memory?
//    CLEDController *controllers[NUM_STRIPS];
//    
//    void setup() { 
//      controllers[0] = &FastLED.addLeds<WS2812,1>(leds, NUM_LEDS);
//      controllers[1] = &FastLED.addLeds<WS2812,2>(leds, NUM_LEDS);
//      controllers[2] = &FastLED.addLeds<WS2812,10>(leds, NUM_LEDS);
//      controllers[3] = &FastLED.addLeds<WS2812,11>(leds, NUM_LEDS);
//    }
//    
//    void loop() { 
//      // draw led data for the first strand into leds
//      fill_solid(leds, NUM_LEDS, CRGB::Red);
//      controllers[0]->showLeds(gBrightness);
//      fill_solid(leds, NUM_LEDS, CRGB::Red);
//      controllers[1]->showLeds(gBrightness);
    
      table_safety_strip[i] = CRGB::Red; 
//    table_safety.setPixelColor(i, 0, 0, 0);
  }
  
  FastLED.show();

  //altar_ready.show();
  //altar_safety.show();
//  table_safety.show();
  
  //scale.get_units() returns a float
//  Serial.print(scale.get_units(), 1); 
//  Serial.print(" lbs");
//  Serial.println();
//  
//  Serial.print("GO_BUTTON: ");
//  Serial.print(!digitalRead(GO_BUTTON));
//  Serial.print(" | SPELL_BUTTON: ");
//  Serial.print(!digitalRead(SPELL_BUTTON));
//  Serial.println();
  
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

