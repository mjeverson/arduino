/*******************************************************************************

 Francis the Fantastic
 Created by: Michael Everson
 Org: Site 3 Fire Arts
 ------------------------------

 This is the logic behind Francis the Fantastic, a fire fortune-teller being created as part of the 2015 Burning Man Honourarium installation, the Charnival.
*******************************************************************************/

// compiler error handling
#include <Compiler_Errors.h>

// touch includes 
#include <MPR121.h>
#include <Wire.h>

// touch definitions. Note these pins refer to the touch electrodes and not output pins
#define MPR121_ADDR 0x5C
#define MPR121_INT 4
#define firstPin 0
#define lastPin 9

// mp3 includes
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <SFEMP3Shield.h>

// mp3 definitions
SFEMP3Shield MP3player;
SdFat sd;

// Serial stuff for dispenser
// RX is 0, TX is 1

// Solenoid definitions (works on pin 13)
#define SOLENOID 10

// LED includes & definitions (pins 10-13 are unallocated)
#include <Adafruit_NeoPixel.h>
#define NEOPIXEL 11 // Works on pin 1, but may interfere with serial stuff, try this
#define EYESTRIP 12 

// Adafruit_NeoPixel(number of pixels in strip, pin #, pixel type flags add as needed)
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel eyes = Adafruit_NeoPixel(2, EYESTRIP, NEO_GRB + NEO_KHZ800); //todo: make sure we have the right version - might need different params for the eyes depending

// Game model variables
//todo: test time calculations
unsigned long baseTime = 0;           // The base value of ms to compare against elapsed ms for timeout
unsigned long roundTimeout = 5000;    // How long to wait after last touch before processing (ms)
bool isRoundActive = false; // Whether or not there is a round in play

// Get this party started
void setup() {
  Serial.begin(57600);
  //while (!Serial) ; {} //uncomment when using the serial monitor
  
  // Set the output pins
  pinMode(SOLENOID, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // SD Card setup
  if (!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();

  // Initialize everything else
  setupTouch();
  setupMp3();
  setupCrystalBall();
  setupEyes();
  resetTimer();
}

void loop() {
  // If sufficient time has passed since the last touch, enter the completion mode, otherwise continue listening
  if (isRoundActive && hasRoundTimedOut()){
    completeRound();
  } else {
    listenForTouchInputs();
  }
}

/* SETUP FUNCTIONS */

// Touch Setup
void setupTouch() {
  if (!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);

  //TODO: Adjust touch sensitivity
  // Changes from Touch MP3. This is the touch threshold - setting it low makes it more like a proximity trigger
  // default value is 40 for touch
  MPR121.setTouchThreshold(20);

  // this is the release threshold - must ALWAYS be smaller than the touch threshold
  MPR121.setReleaseThreshold(10);
}

// Starts up the mp3 player
void setupMp3() {
  byte result = MP3player.begin();
  MP3player.setVolume(0, 0);

  if (result != 0) {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
  }
}

// Crystal ball ring setup
void setupCrystalBall() {
  // LED NeoPixel Ring Code
  strip.begin();
  strip.setBrightness(10);

  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }

  strip.show();
}

// LED Eyes setup
void setupEyes() {
  eyes.begin();
  resetEyeColor();
}

/* UTILITY FUNCTIONS */

// Reads and handle the user's touch inputs
void listenForTouchInputs() {
  if (MPR121.touchStatusChanged()) {
    MPR121.updateTouchData();
    
    //todo: only make an action if we have one or fewer pins touched, or only do the first action
    //todo: nyan cat easter egg

    //todo: ignore or handle multi-touch - currently we only react if there is a single touch which can lead to false negatives. 
    //      Current behaviour will play sounds for all touched pins. Leave this way for testing purposes, need to calibrate touch sensitivity and crossed wires
    //if (MPR121.getNumTouches() <= 1) {
      
      // Iterate through all pins, and for each, check if we have a new touch on it. 
      //todo: There should be a better way, can't we just get informed of a specific touch?
      for (int i = firstPin; i <= lastPin; i++) {
        
        // If this is a new touch on this pin
        if (MPR121.isNewTouch(i)) {
          //pin i was touched          
          digitalWrite(LED_BUILTIN, HIGH);
          strip.setBrightness(40);
          
          //todo: maybe have francis's eyes open at this point?
          // Set the round as active and reset the timeout counter
          isRoundActive = true;
          resetTimer();

          //todo: instead of stoptrack is there a fade out?
          //todo: loop sounds while holding instead of play?
          //todo: don't stop other tracks when touchdown?

          // if we're already playing a track, stop that one and play the newly requested one
          if (MP3player.isPlaying()) {
              MP3player.stopTrack();
              MP3player.playTrack(i - firstPin);
          } else {
            MP3player.playTrack(i - firstPin);
          }
        } else if (MPR121.isNewRelease(i)) {
          // Pin i was released
          digitalWrite(LED_BUILTIN, LOW);
          strip.setBrightness(10);
        }
      }
    //}
  }
}

// Handles the round completion
void completeRound(){
  isRoundActive = false;
  
  // Make eyes change colours
  updateEyeColor();
  
  //todo: make crystal ball change some colours, do something cool
  
  //todo: maybe play some mp3
  
  // fire the flame effect some number of times
  fireSolenoid();
  
  // dispense a card
  dispenseFortune();

  // delay for a bit before resetting everything
  delay(5000);
  resetTimer();
  resetEyeColor();
  //todo: reset eye color, orb color
}

//todo: keep the orb's color shifting and changing
void updateOrbColor(){
  int green;
  int red;
  int blue;
  
  //todo: some calculation based on millis() to pick color and brightness. Get current seconds (/ 1000?) value to dictate color, millis value dictate brightness?
  // millis / 1000 % 5 = which color to use, beware there will be some type issues here
  // millis / 10 % 50 = which brightness to use
  //todo: test if dividing two longs keeps the result a long
  unsigned long seconds = 1000;
  unsigned long tens = 10;
  unsigned long colorSequence = (millis() / seconds) % 5; //5 possible colors based on what second we're in
  unsigned long brightness = (millis() / tens) % 50; // 50 possible brightnesses based on what 10s we're in
  //todo: test decreasing once 49 is reached, some sort of negatives function
  
  switch (colorSequence){
    case 1:
      green = 0;
      red = 255;
      blue = 0;
      break;
    case 2:
      green = 0;
      red = 0;
      blue = 255;
      break;
    case 3:
      green = 127;
      red = 150;
      blue = 150;
      break;
    case 4:
      green = 255;
      red = 255;
      blue = 255;
      break;
    default:
      green = 255;
      red = 0;
      blue = 0;
      break;
    
    green = random(0, 254);
    red = random(0, 254);
    blue = random(0, 254);
  }

  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, green, red, blue);
  }

  strip.show();
}

// eyes glow pattern
void updateEyeColor(){
  int eyeSequence =  random(0, 2);
  
  //todo: write a function to generate sequences. randomize brightness, randomize color, always fade
  switch (eyeSequence) {
    case 1:
      eyes.setBrightness(25);
    
      for (int i = 0; i < strip.numPixels(); i++) {
        eyes.setPixelColor(i, 255, 0, 0);
      }      
      break;
    case 2:
      eyes.setBrightness(25);
    
      for (int i = 0; i < strip.numPixels(); i++) {
        eyes.setPixelColor(i, 0, 0, 255);
      }       
      break;
    default:
      //todo: pulse to a bright red
      eyes.setBrightness(50);
    
      for (int i = 0; i < strip.numPixels(); i++) {
        eyes.setPixelColor(i, 0, 255, 0);
      }
      break;
  }

  eyes.show();
}

// Fire the flame effect
void fireSolenoid() {
  int firingSequence =  random(0, 2);
  
  //todo: write a function to generate sequences. randomize delay up to a max, randomize number of shots to a max
  switch (firingSequence) {
    case 1:
      digitalWrite(SOLENOID, HIGH);
      delay(500);
      digitalWrite(SOLENOID, LOW);
      delay(500);
      digitalWrite(SOLENOID, HIGH);
      delay(500);
      digitalWrite(SOLENOID, LOW);      
      break;
    case 2:
      digitalWrite(SOLENOID, HIGH);
      delay(1000);
      digitalWrite(SOLENOID, LOW);
      delay(1000);
      digitalWrite(SOLENOID, HIGH);
      delay(1000);
      digitalWrite(SOLENOID, LOW);  
      break;
    default:
      digitalWrite(SOLENOID, HIGH);
      delay(1000);
      digitalWrite(SOLENOID, LOW);
      break;
  }
}

void dispenseFortune(){
  //todo: weird serial magic to trigger the card dispenser 
  //todo: some sort of error handling. make orb turn red or play a sound if out of cards or something?
}

bool hasRoundTimedOut(){
  return  millis() - baseTime > roundTimeout;
}

// Set the baseline value for the timer
void resetTimer(){
  baseTime = millis();
}

void resetEyeColor(){
  eyes.setBrightness(10);

  for (int i = 0; i < eyes.numPixels(); i++) {
    eyes.setPixelColor(i, 0, 0, 0);
  }

  eyes.show();
}

