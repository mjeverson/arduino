/*******************************************************************************

 Francis the Fantastic
 Created by: Michael Everson
 Org: Site 3 Fire Arts
 ------------------------------

 This is the logic behind Francis the Fantastic, a fire fortune-teller being created as part of the 2015 Burning Man Honourarium installation, the Charnival.
*******************************************************************************/

// compiler error handling
#include "Compiler_Errors.h"

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
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel eyes = Adafruit_NeoPixel(2, EYESTRIP, NEO_GRB + NEO_KHZ800);

// Game model variables
int baseTime = 0;           // The base value of ms to compare against elapsed ms for timeout
bool isRoundActive = false; // Whether or not there is a round in play
int roundTimeout = 5000;    // How long to wait after last touch before processing (ms)

// Get this party started
void setup() {
  Serial.begin(57600);
  //while (!Serial) ; {} //uncomment when using the serial monitor
  
  // Set the output pins
  pinMode(SOLENOID, OUTPUT)
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
  // update the idle color for the orb
  updateIdlePulsePattern();
  
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
  eyes.setBrightness(10);

  for (int i = 0; i < eyes.numPixels(); i++) {
    eyes.setPixelColor(i, 0, 0, 0);
  }

  eyes.show();
}

/* UTILITY FUNCTIONS */

// Reads and handle the user's touch inputs
void listenForTouchInputs() {
  if (MPR121.touchStatusChanged()) {
    MPR121.updateTouchData();
    
    //todo: only make an action if we have one or fewer pins touched, or only do the first action
    //todo: nyan cat easter egg

    //todo: ignore or handle multi-touch - currently we only react if there is a single touch which can lead to false negatives. Probably should take first touched pin.
    if (MPR121.getNumTouches() <= 1) {
      
      // Iterate through all pins, and for each, check if we have a new touch on it. 
      //todo: There should be a better way, can't we just get informed of a specific touch?
      for (int i = firstPin; i <= lastPin; i++) {
        
        // If this is a new touch on this pin
        if (MPR121.isNewTouch(i)) {
          //pin i was just touched
          Serial.print("pin ");
          Serial.print(i);
          Serial.println(" was just touched");
          
          // Set the round as active and reset the timeout counter
          digitalWrite(LED_BUILTIN, HIGH);
          isRoundActive = true;
          resetTimer();

          // Change the colour at random for the touch
          updateOrbColor(true);

          //todo: instead of stoptrack is there a fade out?
          //todo: loop sounds while holding instead of play
          //todo: don't stop other tracks when touchdown

          if (MP3player.isPlaying()) {
              // if we're already playing a track, stop that one and play the newly requested one
              MP3player.stopTrack();
              MP3player.playTrack(i - firstPin);
              Serial.print("playing track ");
              Serial.println(i - firstPin);
            }
          } else {
            MP3player.playTrack(i - firstPin);
            Serial.print("playing track ");
            Serial.println(i - firstPin);
          }
        } else if (MPR121.isNewRelease(i)) {
          Serial.print("pin ");
          Serial.print(i);
          Serial.println(" is no longer being touched");
          digitalWrite(LED_BUILTIN, LOW);

          updateOrbColor(false);
        }
      }
    }
  }
}

// Handles the round completion
void completeRound(){
  isRoundActive = false;
  
  //todo: make eyes change colours, glow red or something
  
  //todo: make crystal ball change some colours, do something cool
  
  //todo: maybe play some mp3
  
  // fire the flame effect some number of times
  fireSolenoid();
  
  // dispense a card
  dispenseFortune();

  // delay for a bit before resetting everything
  delay(5000);
  resetTimer();
}

// Persist the background "pulsing" color patterns for the orb, whenever there is no touch change to the current idle color state
void updateIdlePulsePattern(){
  //todo: some algorithm to randomly pulse different colors
}

// Change the orb's color either to the idle state or to a random color if touched
void updateOrbColor(bool isTouched){
  //todo: different behaviour if touched
  strip.setBrightness(10);

  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }

  strip.show();
}

// eyes glow pattern
void updateEyeColor(){
  //todo: pulse, flow red/orange, random patterns
  eyes.setBrightness(10);

  for (int i = 0; i < strip.numPixels(); i++) {
    eyes.setPixelColor(i, 0, 0, 0);
  }

  eyes.show();
}

// Fire the flame effect
void fireSolenoid() {
  //todo: different solenoid firing patterns
  digitalWrite(SOLENOID, HIGH);
  delay(1000);
  digitalWrite(SOLENOID, LOW);
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
  currentMillis = millis();
}

