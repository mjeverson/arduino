/*******************************************************************************

 Francis the Fantastic
 Created by: Michael Everson
 Org: Site 3 Fire Arts
 ------------------------------

 This is the logic behind Francis the Fantastic, a fire fortune-teller being created as part of the 2015 Burning Man Honourarium installation, the Charnival.
*******************************************************************************/

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
#define RX 0
#define TX 1

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

// LED tracking values for the orb and the eyes for fading purposes. Everything starts white.
int stripGreenStart = 255;
int stripRedStart = 255;
int stripBlueStart = 255;

int eyesGreenLeftStart = 0;
int eyesRedLeftStart = 255;
int eyesBlueLeftStart = 0;
int eyesGreenRightStart = 0;
int eyesRedRightStart = 255;
int eyesBlueRightStart = 0;
int eyesBrightnessStart = 10;

// Game model variables
//todo: test time calculations
bool isRoundActive = false; // Whether or not there is a round in play
unsigned long baseTime = 0;           // The base value of ms to compare against elapsed ms for timeout
unsigned long roundTimeout = 5000;    // How long to wait after last touch before processing (ms)

// Get this party started
void setup() {
  //todo: Set up the serial communication with the dispenser
  Serial.begin(57600);
  //while (!Serial) ; {} //uncomment when using the serial monitor
  //todo: command to fix card length (maybe not required?)

  // Set the output pins
  pinMode(RX, INPUT);
  pinMode(TX, OUTPUT);
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
  //todo: for fading of eyes or what have you, maybe set current state for looping purposes

  // If sufficient time has passed since the last touch, enter the completion mode, otherwise continue listening
  if (isRoundActive && hasRoundTimedOut()) {
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
  // This is the touch threshold - setting it low makes it more like a proximity trigger. Default value is 40 for touch
  MPR121.setTouchThreshold(20);

  // This is the release threshold - must ALWAYS be smaller than the touch threshold
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

// Crystal ball LED ring setup
void setupCrystalBall() {
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
  updateEyeColor(false);
}

/* UTILITY FUNCTIONS */

// Reads and handle the user's touch inputs
void listenForTouchInputs() {
  if (MPR121.touchStatusChanged()) {
    MPR121.updateTouchData();

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
        strip.setBrightness(50);

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
void completeRound() {
  isRoundActive = false;

  // Make eyes change colours
  updateEyeColor(true);

  // Make crystal ball get very bright on its current color
  updateOrbBrightness(true);

  //todo: maybe play some mp3

  // fire the flame effect some number of times
  fireSolenoid();

  // dispense a card
  dispenseFortune();

  // delay for a bit before resetting everything.
  delay(5000);
  updateOrbBrightness(false);
  updateEyeColor(false);
  resetTimer();
}

// Keep the orb's color shifting and changing
void updateOrbColor() {
  int Gend = random(0, 255);
  int Rend = random(0, 255);
  int Bend = random(0, 255);
  int rate = 50;

  // Pick a random color and fade to that color
  for (int i = 0; i < rate; i++) // larger values of 'rate' will give a smoother/slower transition.
  {
    int Gnew = stripGreenStart + (Gend - stripGreenStart) * i / rate;
    int Rnew = stripRedStart + (Rend - stripRedStart) * i / rate;
    int Bnew = stripBlueStart + (Bend - stripBlueStart) * i / rate;

    // set pixel color here
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Gnew, Rnew, Bnew);
    }
  }

  stripGreenStart = Gend;
  stripRedStart = Rend;
  stripBlueStart = Bend;

  strip.show();
}

// Brightens or darkens the orb
void updateOrbBrightness(bool isThinkingState) {
  int brightnessStart = 50;
  int brightnessEnd = 10;
  int rate = 50;

  if (isThinkingState) {
    brightnessStart = 10;
    brightnessEnd = 50;
  }

  // Larger values of 'rate' will give a smoother/slower transition.
  for (int i = 0; i < rate; i++)
  {
    strip.setBrightness(brightnessStart + (brightnessEnd - brightnessStart) * i / rate);
  }
}

// Eyes glow pattern
void updateEyeColor(bool isThinkingState) {
  // Pick a color for each eye and slowly fade them both, along with brightness
  int GLeftEnd = 0;
  int RLeftEnd = 255;
  int BLeftEnd = 0;
  int GRightEnd = 0;
  int RRightEnd = 255;
  int BRightEnd = 0;
  int brightnessStart = 50;
  int brightnessEnd = 10;
  int rate = 10;

  if (isThinkingState) {
    GLeftEnd = random(0, 255);
    RLeftEnd = random(0, 255);
    BLeftEnd = random(0, 255);
    GRightEnd = random(0, 255);
    RRightEnd = random(0, 255);
    BRightEnd = random(0, 255);
    brightnessStart = 10;
    brightnessEnd = 50;
  }

  // Larger values of 'rate' will give a smoother/slower transition.
  for (int i = 0; i < rate; i++)
  {
    int GLeftNew = eyesGreenLeftStart + (GLeftEnd - eyesGreenLeftStart) * i / rate;
    int RLeftNew = eyesRedLeftStart + (RLeftEnd - eyesRedLeftStart) * i / rate;
    int BLeftNew = eyesBlueLeftStart + (BLeftEnd - eyesBlueLeftStart) * i / rate;
    int GRightNew = eyesGreenRightStart + (GRightEnd - eyesGreenRightStart) * i / rate;
    int RRightNew = eyesRedRightStart + (RRightEnd - eyesRedRightStart) * i / rate;
    int BRightNew = eyesBlueRightStart + (BRightEnd - eyesBlueRightStart) * i / rate;

    // set pixel color & brightness
    eyes.setPixelColor(0, GLeftNew, RLeftNew, BLeftNew);
    eyes.setPixelColor(1, GRightNew, RRightNew, BRightNew);
    eyes.setBrightness(brightnessStart + (brightnessEnd - brightnessStart) * i / rate);
  }

  eyesGreenLeftStart = GLeftEnd;
  eyesRedLeftStart = RLeftEnd;
  eyesBlueLeftStart = BLeftEnd;
  eyesGreenLeftStart = GLeftEnd;
  eyesRedLeftStart = RLeftEnd;
  eyesBlueLeftStart = BLeftEnd;

  strip.show();
}

// Fire the flame effect
void fireSolenoid() {
  int firingSequence =  random(0, 3);

  switch (firingSequence) {
    case 1:
      digitalWrite(SOLENOID, HIGH);
      delay(500);
      digitalWrite(SOLENOID, LOW);
      delay(500);
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
      delay(500);
      digitalWrite(SOLENOID, LOW);
      delay(1000);
      digitalWrite(SOLENOID, HIGH);
      delay(500);
      digitalWrite(SOLENOID, LOW);
      delay(500);
      digitalWrite(SOLENOID, HIGH);
      delay(500);
      digitalWrite(SOLENOID, LOW);
      break;
    case 3:
      digitalWrite(SOLENOID, HIGH);
      delay(500);
      digitalWrite(SOLENOID, LOW);
      delay(500);
      digitalWrite(SOLENOID, HIGH);
      delay(500);
      digitalWrite(SOLENOID, LOW);
      delay(1000);
      digitalWrite(SOLENOID, HIGH);
      delay(500);
      digitalWrite(SOLENOID, LOW);
      break;
    default:
      digitalWrite(SOLENOID, HIGH);
      delay(1000);
      digitalWrite(SOLENOID, LOW);
      break;
  }
}

void dispenseFortune() {
  //todo: weird serial magic to trigger the card dispenser

  //todo: something like this...
  //   Issuing Command
  //   Status request command
  //    If busy flag, keep polling status
  //    If error bit, do some display warning like play sound/orb red/something else
  //   Clear Command
}

bool hasRoundTimedOut() {
  return  millis() - baseTime > roundTimeout;
}

// Set the baseline value for the timer
void resetTimer() {
  baseTime = millis();
}


