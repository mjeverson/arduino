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
int stripFadeProgress = -1;
int stripGreen = 0;
int stripBlue = 0;
int stripRed = 0;

int eyesGreenLeft = 255;
int eyesRedLeft = 0;
int eyesBlueLeft = 0;
int eyesGreenRight = 0;
int eyesRedRight = 0;
int eyesBlueRight = 255;
int eyesBrightness = 150;

// Game model variables
bool isRoundActive = false;           // Whether or not there is a round in play
unsigned long baseTime = 0;           // The base value of ms to compare against elapsed ms for timeout
unsigned long originalTime = 0;       // The absolute start time of the round
unsigned long roundTimeout = 3000;    // How long to wait after last touch before processing (ms)
unsigned long maxRoundTime = 5000;    // Don't allow the round to go any longer than this

// Get this party started
void setup() {
  // Set the output pins
  pinMode(SOLENOID, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Set up the Serial1 communication with the dispenser
  Serial.begin(9600);
  Serial1.begin(9600);
  while (!Serial1) ; {} // uncomment when hooked up to Serial1
  
  // Command to fix card length (maybe not required?) 0xF0 = default, drops card | 0xF1 - F4 varying degrees of stoppage
  byte command[] = {0x02, 0xF4, 0x03, 0x00};
  command[3] = command[0] ^ command[1] ^ command[2];
  Serial1.write(command, 4);
  Serial.println("sent command to set stop distance");

  // SD Card setup
  if (!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();

  // Initialize everything else
  setupTouch();
  setupMp3();
  setupCrystalBall();
  setupEyes();
  resetTimer();
  originalTime = millis();
  
  randomSeed(analogRead(A0));
}

void loop() { 
  //test fortune dispense on key press
  if (Serial.available() > 0){
    Serial.read();
    dispenseFortune();
  }
  
  // Fade the orb colors
  updateOrbColor();
  
  // If sufficient time has passed since the last touch, enter the completion mode, otherwise continue listening
  if (isRoundActive && hasRoundTimedOut()) {
//    completeRound();
  } else {
//    listenForTouchInputs();
  }
}

/* SETUP FUNCTIONS */

// Touch Setup
void setupTouch() {
  if (!MPR121.begin(MPR121_ADDR)) Serial1.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);

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
  stripGreen = random(0,255);
  stripBlue = random(0,255);
  stripRed = random(0,255);
  strip.setBrightness(50);

  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, stripGreen, stripRed, stripBlue);
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
        strip.setBrightness(150);

        // Set the round as active and reset the timeout counter. Only set the original time if the round is just starting
        if (!isRoundActive){
            originalTime = millis();  
            isRoundActive = true;
        }
        
        resetTimer(
        );
        // if we're already playing a track, stop that one and play the newly requested one
        if (MP3player.isPlaying()) {
          MP3player.stopTrack();
          MP3player.playTrack(i - firstPin);
        } else {
          MP3player.playTrack(i - firstPin);
        }
      } else if (MPR121.isNewRelease(i)) {
        // Pin i was released
        strip.setBrightness(50);
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
  
  // fire the flame effect some number of times
  fireSolenoid();

  // dispense a card
  dispenseFortune();

  // delay for a bit before resetting everything.
  updateOrbBrightness(false);
  updateEyeColor(false);
  resetTimer();
}

// update the orb's colors every 5 secs
void updateOrbColor() {
  if (stripFadeProgress >= 0 && stripFadeProgress < 5000) {
        float percentProgress = (float)stripFadeProgress / 5000.0;
        int gProg = ((float)1 - percentProgress) * (float)stripGreen;
        int rProg = ((float)1 - percentProgress) * (float)stripRed;
        int bProg = ((float)1 - percentProgress) * (float)stripBlue;
        
      for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, gProg, rProg, bProg);
      } 
    
    stripFadeProgress = stripFadeProgress + 1;
  } else {
    stripRed = random(0,255);
    stripGreen = random(0,255);
    stripBlue = random(0,255);

    // pick new colors
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, stripGreen, stripRed, stripBlue);
    }

    // flip flag
    stripFadeProgress = 0;
  }

  strip.show();
}

// Brightens or darkens the orb
void updateOrbBrightness(bool isCompletingRound) {
  int brightnessStart = 50;
  int brightnessEnd = 150;
  
  if (isCompletingRound) {
    // Fade
    for (int i = 50; i < 150; i++){
      for (int j = 0; j < strip.numPixels(); j++) {
        strip.setPixelColor(j, stripGreen, stripRed, stripBlue);
      }
        
      strip.setBrightness(i);
      strip.show();
      delay(10);
    } 
  } else {
    // Fade
    for (int i = 150; i > 50; i--){
      for (int j = 0; j < strip.numPixels(); j++) {
        strip.setPixelColor(j, stripGreen, stripRed, stripBlue);
      }
        
      strip.setBrightness(i);
      strip.show();
      delay(10);
    } 
  }
}

// Eyes glow pattern
void updateEyeColor(bool isCompletingRound) {
  // Fade out
  for (int i = eyesBrightness; i > 0; i--){
    eyes.setPixelColor(0, eyesRedLeft, eyesGreenLeft, eyesBlueLeft);
    eyes.setPixelColor(1, eyesRedRight, eyesGreenRight, eyesBlueRight);
    eyes.setBrightness(i);
    eyes.show();
    delay(10);
  }
  
  // Pick new colours
  if (isCompletingRound) {
    eyesRedLeft = random(0, 255);
    eyesGreenLeft = random(0, 255);
    eyesBlueLeft = random(0, 255);
    eyesRedRight = random(0, 255);
    eyesGreenRight = random(0, 255);
    eyesBlueRight = random(0, 255);
  } else {
    eyesRedLeft = 0;
    eyesGreenLeft = 255;
    eyesBlueLeft = 0;
    eyesRedRight = 0;
    eyesGreenRight = 0;
    eyesBlueRight = 255;
  }
  
  // Fade back in with new colours
  for (int i = 0; i < eyesBrightness; i++){
    eyes.setPixelColor(0, eyesRedLeft, eyesGreenLeft, eyesBlueLeft);
    eyes.setPixelColor(1, eyesRedRight, eyesGreenRight, eyesBlueRight);
    eyes.setBrightness(i);
    eyes.show();
    
    if (isCompletingRound){
      strip.setPixelColor(i, stripGreen, stripRed, stripBlue);
      strip.setBrightness(i);
      strip.show();
    }
    
    delay(10);
  }
}

// Fire the flame effect
void fireSolenoid() {
  int firingSequence =  random(0, 3);

  switch (firingSequence) {
    case 1:
      digitalWrite(SOLENOID, HIGH);
      delay(1000);
      digitalWrite(SOLENOID, LOW);
      delay(500);
      digitalWrite(SOLENOID, HIGH);
      delay(1000);
      digitalWrite(SOLENOID, LOW);
      break;
    case 2:
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
    case 3:
      digitalWrite(SOLENOID, HIGH);
      delay(2000);
      digitalWrite(SOLENOID, LOW);
      delay(500);
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
      } else if (readByte == 3){
        Serial.println("received 3, next is checksum which well ignore");
        ignoreNextByte = true;
      }
    }
  }
}

bool hasRoundTimedOut() {
  return  (millis() - baseTime > roundTimeout) || (millis() - originalTime > maxRoundTime);
}

// Set the baseline value for the timer
void resetTimer() {
  baseTime = millis();
}


