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

// Solenoid definitions (works on pin 13)
#define SOLENOID 13

// LED includes & definitions (pins 10-13 are unallocated)
#include <Adafruit_NeoPixel.h>
#define NEOPIXEL 11 // Works on pin 1, but may interfere with Serial1 stuff, try this
#define EYESTRIP 12

// Adafruit_NeoPixel(number of pixels in strip, pin #, pixel type flags add as needed)
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel eyes = Adafruit_NeoPixel(2, EYESTRIP, NEO_GRB + NEO_KHZ800); //todo: make sure we have the right version - might need different params for the eyes depending

// LED tracking values for the orb and the eyes for fading purposes. Everything starts white.
int stripFadeRate = 1000;
int stripGreenStart = 0;
int stripRedStart = 0;
int stripBlueStart = 0;
int stripGreenEnd = 255;
int stripRedEnd = 255;
int stripBlueEnd = 255;
int stripFadeProgress = -1;

int eyesGreenLeftStart = 0;
int eyesRedLeftStart = 255;
int eyesBlueLeftStart = 0;
int eyesGreenRightStart = 0;
int eyesRedRightStart = 255;
int eyesBlueRightStart = 0;
int eyesBrightnessStart = 10;

// Game model variables
//todo: test time calculations
bool isRoundActive = false;           // Whether or not there is a round in play
unsigned long baseTime = 0;           // The base value of ms to compare against elapsed ms for timeout
unsigned long roundTimeout = 5000;    // How long to wait after last touch before processing (ms)

// Get this party started
void setup() {
  // Set the output pins
  pinMode(SOLENOID, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Set up the Serial1 communication with the dispenser
//  todo: may actually need SoftwareSerial1 library if these pins aren't already configured
  Serial.begin(9600);
  Serial1.begin(9600);
  while (!Serial1) ; {} // uncomment when hooked up to Serial1
  
  // Command to fix card length (maybe not required?) 0xF0 = default, drops card | 0xF1 - F4 varying degrees of stoppage
  byte command[] = {0x02, 0xF4, 0x03, 0x00};
  command[3] = command[0] ^ command[1] ^ command[2];
  Serial1.write(command, 4);
  Serial.println("sent command to set stop distance");
//  
//  while(!Serial1.available()){};
//  
//  if (Serial1.available() > 0){
//    if (Serial1.read() == 6){
//      Serial.println("Received positive ack from stop distance command");
//    }
//  }

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
  //test fortune dispense on key press
  if (Serial.available() > 0){
    Serial.read();
    dispenseFortune();
  }
  
  // Fade the orb colors
  updateOrbColor();
  
  //todo: evaluate recently pressed buttons for easter egg purposes, nyan cat maybe?
  //        track pins pressed in an array, evaluate the array, set 'play easter egg' flag

  // If sufficient time has passed since the last touch, enter the completion mode, otherwise continue listening
  if (isRoundActive && hasRoundTimedOut()) {
//    completeRound();
  } else {
    listenForTouchInputs();
  }
}

/* SETUP FUNCTIONS */

// Touch Setup
void setupTouch() {
  if (!MPR121.begin(MPR121_ADDR)) Serial1.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);

  //TODO: Adjust touch sensitivity
  // This is the touch threshold - setting it low makes it more like a proximity trigger. Default value is 40 for touch
  MPR121.setTouchThreshold(2);

  // This is the release threshold - must ALWAYS be smaller than the touch threshold
  MPR121.setReleaseThreshold(1);
}

// Starts up the mp3 player
void setupMp3() {
  byte result = MP3player.begin();
  MP3player.setVolume(0, 0);

  if (result != 0) {
    Serial1.print("Error code: ");
    Serial1.print(result);
    Serial1.println(" when trying to start MP3 player");
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
        strip.setBrightness(150);

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

//todo: maybe have it pulse white and only change to the color and get brighter, and freeze on it, with touch?
// Keep the orb's color shifting and changing
void updateOrbColor() {
  if (stripFadeProgress >= 0 && stripFadeProgress < stripFadeRate) {
    // next step of the fade
    int Gnew = stripGreenStart + (stripGreenEnd - stripGreenStart) * stripFadeProgress / stripFadeRate;
    int Rnew = stripRedStart + (stripRedEnd - stripRedStart) * stripFadeProgress / stripFadeRate;
    int Bnew = stripBlueStart + (stripBlueEnd - stripBlueStart) * stripFadeProgress / stripFadeRate;

    // set pixel color here
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Gnew, Rnew, Bnew);
    }

    stripFadeProgress++;
  } else {
    // pick new colors
    stripGreenEnd = random(0, 255);
    stripRedEnd = random(0, 255);
    stripBlueEnd = random(0, 255);

    // flip flag
    stripFadeProgress = 0;
  }

  strip.show();
}

// Brightens or darkens the orb
void updateOrbBrightness(bool isCompletingRound) {
  int brightnessStart = 50;
  int brightnessEnd = 10;
  int rate = 50;

  if (isCompletingRound) {
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
void updateEyeColor(bool isCompletingRound) {
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

  if (isCompletingRound) {
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

//todo: weird Serial1 magic to trigger the card dispenser
void dispenseFortune() {
  bool success = false;
  
  // Issuing Card Command
  byte command[] = {0x02, 0x40, 0x03, 0x00};
  command[3] = command[0] ^ command[1] ^ command[2];

  Serial1.write(command, 4);
  Serial.println("sent command to dispense");
  
  while(!Serial1.available()){};
  
  if (Serial1.available() > 0){
    if (Serial1.read() == 6){
      Serial.println("Received positive ack from dispense command");
    }
  }

  bool checkForStatus = true;

  // Wait for dispenser to complete: Status request command
  while (!success) {
    byte command[] = {0x02,0x31,0x03,0x02^0x31^0x03};
    Serial1.write(command, 4);
    Serial.println("sent status update");

    while(!Serial1.available()){
      Serial.println("waiting for data from status command");
      delay(1000);
    };

    bool readDataByte = false;
    bool ignoreNextByte = false;

    while (Serial1.available() > 0) {
      Serial.println("Got some data from status update");
      byte readByte = Serial1.read();
      Serial.println(readByte, HEX);

      if (ignoreNextByte == true){
        Serial.println("ignoring checksum");
        ignoreNextByte = false;
        checkForStatus = true;
        //continue;
      } else if (readByte == 2){
        //start of text
        Serial.println("received 2, next is data byte");
        readDataByte = true;
      } else if (readDataByte == true){
        Serial.println("reading data byte");
        Serial.println(readByte, DEC);
        Serial.println(readByte, BIN);
        
        byte busyByte = readByte & B01000000;
        byte motorByte = readByte & B00010000;
        byte sensorByte = readByte & B00000100;
        byte stackByte = readByte & B00000001;  
      
        if (busyByte > 0){
          //todo: do nothing
           Serial.println("BUSY");
        } else if (motorByte > 0){
          Serial.println("MOTOR ERROR");
          success = true;
          //todo: do a thing then send clear command
        } else if (stackByte > 0){
          Serial.println("STACK EMPTY");
          success = true;
          //todo: some notification
        } else {
          Serial.println("general success");
          success = true;
        }
      
        readDataByte = false;
      }
      else if (readByte == 6) {
        Serial.println("received positive ack");
         checkForStatus = false;
        //todo: if busy, keep polling status
        //continue;
      } else if (readByte == 3){
        Serial.println("received 3, next is checksum which well ignore");
        ignoreNextByte = true;
      }
    }
  }
}

bool hasRoundTimedOut() {
  return  millis() - baseTime > roundTimeout;
}

// Set the baseline value for the timer
void resetTimer() {
  baseTime = millis();
}


