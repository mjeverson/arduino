/***************************************************
  Lucky Kitty Slot Machine

  Designed for use with Teensy 3.6

Notes:
  For Speed:
  - Run at 240mhz overclocked for ~500ms per screen
  - smaller bmps rendered in the center of the screen if possible
  - Or the _t3 lib if we can get it working
  - 565 raw file instead of bmp?
  - Raspberry pi?
  - Can't loop audio in a thread while rendering without huge slowdown

TODO: Threading!
  https://github.com/ftrias/TeensyThreads

TODO: Might have some threading issues wherever we use delay()
//todo: probably need a 2nd SD card to read in parallel with images. Test the SD card reader with nothing else on SPI1, if that works we can try SPI2.
test if changing the sdio flag for tensy allows spi1 access. if so gonna have a problem
//todo: alternatively, try buffering wav files in memory if there's room? <250kb per sound byte would work, maybe even try smaller wavs or mp3s or something?
// second SD card issue is a clock speed problem with the teensy, 24MHz/96MHz/192 works but higher seems to fail
// Sometimes fails to initialize SD card on upload, does resetting it always work?
//todo: audio crackling
//todo: better looping audio for reels or better track altogether
//todo: def some kind of weird memory leak going on after runs for a few mins. Inconsistently skipping final nyan cat
//todo: issue getting the sd card to initialize when just attaching 5V power

  References:
  //https://arduino.stackexchange.com/questions/26803/connecting-multiple-tft-panels-to-arduino-uno-via-spi
 ****************************************************/

#include <Adafruit_GFX.h> // Core graphics library
#include "Adafruit_HX8357.h"
//#include <HX8357_t3.h> // Hardware-specific library. Faster, but doesn't seem to render the image on the screen?
#include <SPI.h>
#include "SdFat.h" //not compatible with audio OOB may need to do some stuff
#include <Servo.h> // Tentacle & Coin
#include <Wire.h> // Amp controller
#include <TeensyThreads.h> // Threading
#include <Adafruit_NeoPixel.h> // LED Stuff
#include <Audio.h>

// Handle mechanism
#define HANDLE A1 

// Solenoids
#define SOL1 20
#define SOL2 21
#define SOL3 22
#define SOL4 23

//Speaker is on DAC0, or A21
#define MAX9744_I2CADDR 0x4B
AudioPlaySdWav playWav1;
AudioOutputAnalog audioOutput;
AudioConnection patchCord1(playWav1, 0, audioOutput, 0);

// Valid output pins: //2-10, 14, 16-17, 20-23, 29-30, 35-38
#define TENTACLE_SERVO 35
#define COIN_SERVO 36
#define NEOPIXEL 38
#define NUM_PIXELS 16
// Adafruit_NeoPixel(number of pixels in strip, pin #, pixel type flags add as needed)
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL, NEO_GRB + NEO_KHZ800);

Servo tentacleServo;
Servo coinServo;  
int tentacleServoPos = 0;
int coinServoPos = 0;

// screens 1 2 and 3 are on the spi1 busy (mosi 0, miso 1, sck 32)
#define TFT_DC 24
#define TFT_CS 25 
#define TFT_CS2 26
#define TFT_CS3 27

// Sound SD card is on spo0 bus (mosi 11, miso 12, sck 13) 
#define MOSI1 0
#define MISO1 1
#define SCK1 32

// Use soft SPI for TFTs, hardware for SPI
//HX8357_t3 tft = HX8357_t3(TFT_CS, TFT_DC);
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, MOSI1, SCK1, -1, MISO1);
Adafruit_HX8357 tft2 = Adafruit_HX8357(TFT_CS2, TFT_DC, MOSI1, SCK1, -1, MISO1);
Adafruit_HX8357 tft3 = Adafruit_HX8357(TFT_CS3, TFT_DC, MOSI1, SCK1, -1, MISO1);

#define WINSTATE_NONE 0
#define WINSTATE_NYAN 1
#define WINSTATE_TENTACLE 2
#define WINSTATE_COIN 3
#define WINSTATE_FIRE 4
#define WINSTATE_CHEESY 5
#define WINSTATE_PINCHY 6
#define NUM_SLOTS 6
int winState, slot1_current, slot2_current, slot3_current; 
char* images[] = {"nyanf.bmp", "tentf.bmp", "coinf.bmp", "firef.bmp", "cheesef.bmp", "pinchyf.bmp"};
//char* sounds[] = {"nyan.wav", "scream.wav", "mario.wav", "hth.wav", "cheesy.wav", "pinchy.wav", "roll.wav"};

// Onboard Teensy 3.6 SD Slot
const uint8_t SD_CHIP_SELECT = SS;
const uint8_t SOUND_SD_CHIP_SELECT = 29;

// Faster library for SDIO
SdFatSdioEX sd;
SdFat sd2;

// Get this party started!
void setup() {
  // Sets up the RNG
  randomSeed(analogRead(A0));
  
  Serial.begin(9600);

  // Sets up the TFT screens
  tft.begin(HX8357D);
  tft2.begin(HX8357D);
  tft3.begin(HX8357D);

  // Prove the screens are on and accepting commands
  tft.fillScreen(HX8357_BLUE);
  tft2.fillScreen(HX8357_BLUE);
  tft3.fillScreen(HX8357_BLUE);

  // Initialize the SD card
  Serial.println("Initializing SD card...");
  uint32_t t = millis();

  if (!sd.cardBegin()) {
    Serial.println("\nArt cardBegin failed");
    return;
  }

  if (!sd.fsBegin()) {
    Serial.println("\nArt File System initialization failed.\n");
    return;
  }

  // This didn't work when attached only to 5V power. All this was was the 3.3V & GND needed to connect directly to teensy & also to power supply ground 
  //todo: why does it need direct connection to power supply GND and teensy GND when both of those things are connected?
  if (!sd2.begin(SOUND_SD_CHIP_SELECT)) {
    Serial.println("\nSound cardBegin failed");
    return;
  }

  t = millis() - t;
  
  Serial.println("\ninit time: ");
  Serial.print(t);

  // Set up sound player
  Wire.begin(); 
  AudioMemory(8); //was 8

  // Set up handle listener
  pinMode(HANDLE, INPUT_PULLUP);

  // Set up solenoid
  pinMode(SOL1, OUTPUT);
  pinMode(SOL2, OUTPUT);
  pinMode(SOL3, OUTPUT);
  pinMode(SOL4, OUTPUT);

  //Set up servos
  tentacleServo.attach(TENTACLE_SERVO);
  coinServo.attach(COIN_SERVO);

  // Set up LEDs. Some default colour to indicate ready to go
  strip.begin();

  // Initializes the state of the peripherals
  resetState();

  // Initialize slot state.
  slot1_current = random(0,5);
  slot2_current = random(0,5);
  slot3_current = random(0,5);
  bmpDraw(images[slot1_current], 0, 0, tft);
  bmpDraw(images[slot2_current], 0, 0, tft2);
  bmpDraw(images[slot3_current], 0, 0, tft3);
}

void loop() {
//  Serial.print("\nPull handle to begin slots!\n");
//  while (digitalRead(HANDLE)){
//    SysCall::yield();
//  }

  // For now do this in a serial read. Normally will need to wait for handle mechanism
  Serial.print("\nPress any key to begin slots!\n");

  //TODO: Read any existing Serial data. (won't need this once we swap to handle)
  while (!Serial.available()) {}

  rollSlots();
  doWinState();
  resetState();

  //TODO: Clean up any available serial data (won't need this once we swap to handle)
  do {
    delay(10);
  } while (Serial.available() && Serial.read() >= 0);
}

File audioFile;

void playReelLoop(){
  while(true){
    //TODO: Change this to a while and then call out to the custom play function?
    while(!playWav1.isPlaying()){
      if (audioFile){
        audioFile.close();
      }
      
      if(audioFile = sd2.open("reel16.wav")){
        Serial.print("About to play reel!");
        playWav1.play(audioFile);
      } 
    }

    //TODO: might not even need this?
    threads.delay(10);
  }
}

// Sets the slots rolling, picks an outcome and displays it
// On rollSlots, we should iterate through 0-6 and set the slot to be whatever its current state is +1 (rolling over, so use %)
// Also need to randomize a win state. States should be any of the 6 outcomes, or a total loss, or an almost loss (Trish has the odds)
// On a result, save the global state of the slots. 
// keep rolling the first slot til it gets where it needs to go. Then the second, then the third. (don't update the global state)
// OR just let the first slot start one or two early, then the second slot, then the third slot. let them roll a few times, then do it all again. Don't need global state
void rollSlots(){  
  // Start playing rolling sound
  int playReelLoopID = threads.addThread(playReelLoop);
  
  // Calculate win state
  int winRoll = random(1,20); 
  Serial.println("winRoll: ");
  Serial.print(winRoll);
  
  // Calcuate partial fail slot displays
  int falseWinSlot, falseWinSlotOdd;
    
  do {
    falseWinSlot = random(0,5);
    falseWinSlotOdd = random(0,5);
  } while (falseWinSlot == falseWinSlotOdd);
  
  int slot1_end, slot2_end, slot3_end;
  
  if (winRoll <= 2) {
    // nyancat
    winState = WINSTATE_NYAN;
    slot1_end = slot2_end = slot3_end = 0;
  } else if (winRoll <= 4){
    // tentacle
    winState = WINSTATE_TENTACLE;
    slot1_end = slot2_end = slot3_end = 1;
  } else if (winRoll == 5) {
    // coin
    winState = WINSTATE_COIN;
    slot1_end = slot2_end = slot3_end = 2;
  } else if (winRoll <= 7) {
    // fire
    winState = WINSTATE_FIRE;
    slot1_end = slot2_end = slot3_end = 3;
  } else if (winRoll <= 9) {
    // cheesy poofs
    winState = WINSTATE_CHEESY;
    slot1_end = slot2_end = slot3_end = 4;
  } else if (winRoll == 10){
    // pinchy
    winState = WINSTATE_PINCHY;
    slot1_end = slot2_end = slot3_end = 5;
  } else if (winRoll <= 12) {
    // partial fail
    winState = WINSTATE_NONE;
    slot1_end = slot2_end = falseWinSlot;
    slot3_end = falseWinSlotOdd;
  } else if (winRoll <= 14) {
    // Partial fail
    winState = WINSTATE_NONE;
    slot1_end = slot3_end = falseWinSlot;
    slot2_end = falseWinSlotOdd;
  }else if (winRoll <= 16) {
    // Partial fail
    winState = WINSTATE_NONE;
    slot2_end = slot3_end = falseWinSlot;
    slot1_end = falseWinSlotOdd;
  } else {
    // Total fail
    winState = WINSTATE_NONE;
    slot1_end = falseWinSlot;
    slot2_end = falseWinSlotOdd;
    slot3_end = random(0,5);
  }

  // Do the actual rolling slots
  int index = 0;
  int slot1_stoppedAt = -1;
  int slot2_stoppedAt = -1;
  int minRollsBeforeStopping = 5;

  // while the min number of changes hasn't happened AND the slots aren't in their final slots
  // only let the first slot move for the first iteration, then add the second for the next two, then start the third
  // After a min number of changes, let the first one go til it reaches its final state. two iterations later let the second go til it hits it. then two more later the third.
  while(index < minRollsBeforeStopping || slot1_current != slot1_end || slot2_current != slot2_end || slot3_current != slot3_end || index < slot2_stoppedAt + 2) {
    Serial.print("\n index: ");
    Serial.print(index + "\n");

    if (index < minRollsBeforeStopping || slot1_current != slot1_end){
      slot1_current++;
      slot1_current = slot1_current > 5 ? 0 : slot1_current;

      Serial.print("Loading slot 1, is: ");
      Serial.print(slot1_current);
      Serial.print(", should be: ");
      Serial.print(slot1_end);
      Serial.println("");
      
      bmpDraw(images[slot1_current], 0, 0, tft);
    }
    
    if (index >= minRollsBeforeStopping && slot1_current == slot1_end && slot1_stoppedAt == -1) {
      slot1_stoppedAt = index;
      Serial.println("slot 1 stopped at this index");
      
    }
  
    if (index >= 1 && (index < minRollsBeforeStopping || slot1_stoppedAt == -1 || (slot1_stoppedAt > -1 && index < slot1_stoppedAt + 2) || slot2_current != slot2_end)){
      slot2_current++;
      slot2_current = slot2_current > 5 ? 0 : slot2_current;

      Serial.print("Loading slot 2, is: ");
      Serial.print(slot2_current);
      Serial.print(" should be: ");
      Serial.print(slot2_end);
      Serial.println("");
      
      bmpDraw(images[slot2_current], 0, 0, tft2);
    } 
    
    if (index >= minRollsBeforeStopping && slot1_stoppedAt > -1 && index >= slot1_stoppedAt + 2 && slot2_current == slot2_end && slot2_stoppedAt == -1) {
      slot2_stoppedAt = index;
      Serial.println("slot 2 stopped at this index");
    }
    
    if (index >= 3 && (index < minRollsBeforeStopping || slot2_stoppedAt == -1  || (slot2_stoppedAt > -1 && index < slot2_stoppedAt + 2) || slot3_current != slot3_end)){
      slot3_current++;
      slot3_current = slot3_current > 5 ? 0 : slot3_current;

      Serial.print("Loading slot 3, is: ");
      Serial.print(slot3_current);
      Serial.print(" should be: ");
      Serial.print(slot3_end);
      Serial.println("");
      
      bmpDraw(images[slot3_current], 0, 0, tft3);
    }
  
    index++;
  }

  Serial.println("About to kill thread");
  threads.kill(playReelLoopID);
  playWav1.stop();

  //todo: can probably reduce or eliminate this delay
  delay(500);

  // This occasionally gets skipped, keep retrying til it plays
  //todo: this could probably be its own method playSound("rstop16.wav");
  while(!playWav1.isPlaying()){
    if(audioFile){
      Serial.println("closing redundant audioFile inside while loop");
      audioFile.close();
    }
    
    if(audioFile = sd2.open("rstop16.wav")){
      Serial.print("About to play rstop");
      playWav1.play(audioFile);
      delay(500);
    }
  }

  //TODO: Can remove this in final operation - maybe add a #debug flag for this sort of thing?
  while(playWav1.isPlaying()){
    Serial.println("Waiting for rstop to finish");
    delay(50);
  }

  playWav1.stop();

  // TODO: Might not need this, it should be handled by the while loop we're going to extract
  if (audioFile){
    Serial.println("closing audioFile from rstop");
    audioFile.close();
  }

  delay(500);
}

void doWinState(){
  //based on win state do sounds, fire, etc.
  //TODO: Probably gonna need to break some of this stuff out into threads. Lights, Fire, Servo 1 and servo 2 (sound's okay)
  //TODO: could change image on screen for victory if we want
  if (winState == WINSTATE_NYAN) {
    Serial.println("doWinState nyan");

    //TODO: Do the fire. Might need to swap to clock watching
//    doFire();
    
    //TODO: LEDs: nyancat rainbow marquee
//    doLights();
   
    // Sound: Nyancat. This occasionally gets skipped. Some weird timing thing? wait til it starts playing.
    playSound("nyan16.wav");
//    while(!playWav1.isPlaying()){
//      if(audioFile){
//        Serial.println("closing redundant audioFile inside while loop");
//        audioFile.close();
//      }
//      
//      if(audioFile = sd2.open("nyan16.wav")){
//        Serial.println("Playing nyancat");
//        playWav1.play(audioFile);
//        delay(500);
//      }
//    }
    
    //TODO: something like wait til all threads are done before continuing? threads.wait(n)
    //TODO: Could also do a while where we just poll until all threads have completed, while(threads.getState(n) == Threads::RUNNING)){}
    //TODO: Do we need to clear thread_func_id once the thread ends?
  } else if (winState == WINSTATE_TENTACLE) {
    Serial.println("doWinState tentacle");

    // Wave the tentacle
    //  doTentacle();
    
    //fire: all at once
    //  doFire();
    
    //TODO: LEDs: green
    //    doLights();
    
    //TODO: Sound: person screaming
    //playSound("scream16.wav");
    
  } else if (winState == WINSTATE_COIN) {
    Serial.println("doWinState coin");

    // Dispense a coin
    //  doCoin();
    
    // fire: 1-3-2-4-all
    //    doFire();
    
    //TODO: LEDs: Yellow
    //    doLights();
    
    //TODO: sound: mario 1up/coin
    //playSound("coin16.wav");
    
  } else if (winState == WINSTATE_FIRE) {
    Serial.println("doWinState fire");
    
    // fire all 4 x3
    //    doFire();
    
    //TODO: LEDs: Red
    //    doLights();
    
    //TODO: sound: highway to hell
    //playSound("hth16.wav");
  } else if (winState == WINSTATE_CHEESY) {
    Serial.println("doWinState cheesy");
    
    // no fire
    //    doFire();
    
    // LEDs: orange
    //    doLights();
    
    // Sound: cheesy poofs
    //playSound("cheesy16.wav");
    
  } else if (winState == WINSTATE_PINCHY) {
    Serial.println("doWinState pinchy");
    
    // fire all 4
    //    doFire();

    // LEDs: Red
    //    doLights();
    
    // Sound: PINCHAY
    //playSound("pinchy16.wav");
  } 

  //TODO: min amount of time before running off to resetState(). While sound is playing or some max time has reached or something
//  while(playWav1.isPlaying()){
//    Serial.println("Waiting for winstate audio to finish!");
//  }
  delay(5000);
  playWav1.stop();
  
  if(audioFile){
    Serial.println("closing audioFile from dowinstate");
    audioFile.close();
  }
  
  delay(500);
}

//TODO: Set everything back to normal state for another round
void resetState(){
  Serial.println("Round over, state reset!");

  // Reset the win state
  winState = WINSTATE_NONE;
  
  // Reset coin and tentacle servo positions
  tentacleServo.write(0);
  coinServo.write(0);
  
  // Stop audio
  playWav1.stop();
  if(audioFile){
    audioFile.close();
  }
  
  //TODO: reset LEDs
  doLights();
  
  //TODO: Make sure fire is off
  doFire();
  
  //TODO: Any other variables that need resetting
}

// Stops any existing sound, makes sure the file is closed, then keeps attempting to play the sound until it actually starts
void playSound(char* filename){
  playWav1.stop();

  //TODO: Might not need this
  delay(500);
  
  while(!playWav1.isPlaying()){
    if(audioFile){
      Serial.println("closing redundant audioFile inside while loop");
      audioFile.close();
    }
    
    if(audioFile = sd2.open(filename)){
      Serial.println("Playing nyancat");
      playWav1.play(filename);
      delay(500);
    }
  }
}

//TODO: Do the fire. Might need to swap to clock watching
void doFire(){
  Serial.println("doing fire");

  switch (winState) {
    case WINSTATE_NYAN: 
      //1-2-3-4
      fireSequential(false);
      fireSequential(true);
      break;
     case WINSTATE_TENTACLE:
      // all at once
      fireAll();
      break;
    case WINSTATE_COIN:
      // fire: 1-3-2-4-all
      fireSequential(false);
      fireAll();
      break;
    case WINSTATE_FIRE:
      // fire all 4 x3
      for (int i = 0; i< 3; i++){
          fireAll();
      }
      break;
    case WINSTATE_CHEESY:
      // No fire
      fireOff();
      break;
    case WINSTATE_PINCHY:
      // fire all 4
      fireAll();
      break;
    default:
      // No fire
      fireOff();
      break;
  }
}

// Triggers all four solenoids
fireAll(){
  digitalWrite(SOL1, HIGH);
  digitalWrite(SOL2, HIGH);
  digitalWrite(SOL3, HIGH);
  digitalWrite(SOL4, HIGH);
  delay(500);
  digitalWrite(SOL1, LOW);
  digitalWrite(SOL2, LOW);
  digitalWrite(SOL3, LOW);
  digitalWrite(SOL4, LOW);
  delay(250);
}

// Triggers a  sequential pattern of 1-2-3-4, or reverse
fireSequential(boolean reverse){
  if(!reverse){
    digitalWrite(SOL1, HIGH);
    delay(250);
    digitalWrite(SOL1, LOW);
    delay(250);
    digitalWrite(SOL2, HIGH);
    delay(250);
    digitalWrite(SOL2, LOW);
    delay(250);
    digitalWrite(SOL3, HIGH);
    delay(250);
    digitalWrite(SOL3, LOW);
    delay(250);
    digitalWrite(SOL4, HIGH);
    delay(250);
    digitalWrite(SOL4, LOW);
    delay(250);
  } else {
    digitalWrite(SOL4, HIGH);
    delay(250);
    digitalWrite(SOL4, LOW);
    delay(250);
    digitalWrite(SOL3, HIGH);
    delay(250);
    digitalWrite(SOL3, LOW);
    delay(250);
    digitalWrite(SOL2, HIGH);
    delay(250);
    digitalWrite(SOL2, LOW);
    delay(250);
    digitalWrite(SOL1, HIGH);
    delay(250);
    digitalWrite(SOL1, LOW);
    delay(250);
  }
}

// Turns off all fire
void fireOff(){
  digitalWrite(SOL1, LOW);
  digitalWrite(SOL2, LOW);
  digitalWrite(SOL3, LOW);
  digitalWrite(SOL4, LOW);
}

// Sets the LED colours based on the win state
void doLights(){
  Serial.println("doing Lights");
  
  switch (winState) {
    case WINSTATE_NYAN: 
      // nyan rainbow colours
      rainbowCycle(50);
      break;
     case WINSTATE_TENTACLE:
      // green 
      setStripColor(255, 0, 0);
      break;
    case WINSTATE_COIN:
      // yellow
      setStripColor(255, 255, 0);
      break;
    case WINSTATE_FIRE:
      // red
      setStripColor(0, 255, 0);
      break;
    case WINSTATE_CHEESY:
      // orange
      setStripColor(170, 255, 0);
      break;
    case WINSTATE_PINCHY:
      // Red
      setStripColor(0, 255, 0);
      break;
    default:
      // White
      setStripColor(255, 255, 255);
      break;
  }
}

// Sets the LED strip all to one colour
void setStripColor(int g, int r, int b, int wait){
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, g, r, b);
  }

  strip.show();
  delay(wait);
}

// Makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
 
  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    
    strip.show();
    delay(50);
  }
}
 
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

// Makes the tentacle pop out, wiggle around, and go away
void doTentacle(){
  //TODO: Might have to become a clock watcher for stuff like this if it messes with threads
  //eg. take time t = millis() here, put a while loop after this until n ms have passed
  tentacleServo.write(180); 
  delay(500);

  tentacleServo.write(90); 
  delay(300);

  tentacleServo.write(180); 
  delay(300);

  tentacleServo.write(90); 
  delay(300);

  tentacleServo.write(180); 
  delay(500);
  
  tentacleServo.write(0);
}


// Triggers the coin dispenser to dispense a coin
void doCoin(){
  coinServo.write(180); 
  delay(500);
  coinServo.write(0);
}

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 80

void bmpDraw(char *filename, uint8_t x, uint16_t y, Adafruit_HX8357 screen) {
  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint16_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = false;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= screen.width()) || (y >= screen.height())) return;

  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if((bmpFile = sd.open(filename)) == NULL){
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= screen.width())  w = screen.width()  - x;
        if((y+h-1) >= screen.height()) h = screen.height() - y;

        // Set TFT address window to clipped image bounds
        screen.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
            
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            
            screen.pushColor(screen.color565(r,g,b));

          } // end pixel
        } // end scanline
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}

////===========================================================
//// Try Draw using writeRect
//void bmpDraw2(char *filename, uint8_t x, uint16_t y, HX8357_t3 screen) {
//
//  File     bmpFile;
//  int      bmpWidth, bmpHeight;   // W+H in pixels
//  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
//  uint32_t bmpImageoffset;        // Start of image data in file
//  uint32_t rowSize;               // Not always = bmpWidth; may have padding
//  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
//  uint16_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
//  boolean  goodBmp = false;       // Set to true on valid header parse
//  boolean  flip    = true;        // BMP is stored bottom-to-top
//  int      w, h, row, col;
//  uint8_t  r, g, b;
//  uint32_t pos = 0, startTime = millis();
//
//  uint16_t awColors[320];  // hold colors for one row at a time...
//
//  if((x >= screen.width()) || (y >= screen.height())) return;
//
//  Serial.print(F("Loading image '"));
//  Serial.print(filename);
//  Serial.println('\'');
//
//  // Open requested file on SD card
//  if ((bmpFile = sd.open(filename)) == NULL) {
//    Serial.print(F("File not found"));
//    return;
//  }
//
//  elapsedMicros usec;
//  uint32_t us;
//  uint32_t total_seek = 0;
//  uint32_t total_read = 0;
//  uint32_t total_parse = 0;
//  uint32_t total_draw = 0;
//
//  // Parse BMP header
//  if(read16(bmpFile) == 0x4D42) { // BMP signature
//    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
//    (void)read32(bmpFile); // Read & ignore creator bytes
//    bmpImageoffset = read32(bmpFile); // Start of image data
//    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
//    // Read DIB header
//    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
//    bmpWidth  = read32(bmpFile);
//    bmpHeight = read32(bmpFile);
//    if(read16(bmpFile) == 1) { // # planes -- must be '1'
//      bmpDepth = read16(bmpFile); // bits per pixel
//      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
//      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed
//
//        goodBmp = true; // Supported BMP format -- proceed!
//        Serial.print(F("Image size: "));
//        Serial.print(bmpWidth);
//        Serial.print('x');
//        Serial.println(bmpHeight);
//
//        // BMP rows are padded (if needed) to 4-byte boundary
//        rowSize = (bmpWidth * 3 + 3) & ~3;
//
//        // If bmpHeight is negative, image is in top-down order.
//        // This is not canon but has been observed in the wild.
//        if(bmpHeight < 0) {
//          bmpHeight = -bmpHeight;
//          flip      = false;
//        }
//
//        // Crop area to be loaded
//        w = bmpWidth;
//        h = bmpHeight;
//        if((x+w-1) >= screen.width())  w = screen.width()  - x;
//        if((y+h-1) >= screen.height()) h = screen.height() - y;
//
//        usec = 0;
//        for (row=0; row<h; row++) { // For each scanline...
//
//          // Seek to start of scan line.  It might seem labor-
//          // intensive to be doing this on every line, but this
//          // method covers a lot of gritty details like cropping
//          // and scanline padding.  Also, the seek only takes
//          // place if the file position actually needs to change
//          // (avoids a lot of cluster math in SD library).
//          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
//            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
//          else     // Bitmap is stored top-to-bottom
//            pos = bmpImageoffset + row * rowSize;
//          if(bmpFile.position() != pos) { // Need seek?
//            bmpFile.seek(pos);
//            buffidx = sizeof(sdbuffer); // Force buffer reload
//          }
//          us = usec;
//          usec -= us;
//          total_seek += us;
//
//          for (col=0; col<w; col++) { // For each pixel...
//            // Time to read more pixel data?
//            if (buffidx >= sizeof(sdbuffer)) { // Indeed
//              us = usec;
//              usec -= us;
//              total_parse += us;
//              bmpFile.read(sdbuffer, sizeof(sdbuffer));
//              buffidx = 0; // Set index to beginning
//              us = usec;
//              usec -= us;
//              total_read += us;
//            }
//
//            // Convert pixel from BMP to TFT format, push to display
//            b = sdbuffer[buffidx++];
//            g = sdbuffer[buffidx++];
//            r = sdbuffer[buffidx++];
//            awColors[col] = screen.color565(r,g,b);
//          } // end pixel
//          us = usec;
//          usec -= us;
//          total_parse += us;
//          screen.writeRect(0, row, w, 1, awColors);
//          us = usec;
//          usec -= us;
//          total_draw += us;
//        } // end scanline
//        Serial.print(F("Loaded in "));
//        Serial.print(millis() - startTime);
//        Serial.println(" ms");
//        Serial.print("Seek: ");
//        Serial.println(total_seek);
//        Serial.print("Read: ");
//        Serial.println(total_read);
//        Serial.print("Parse: ");
//        Serial.println(total_parse);
//        Serial.print("Draw: ");
//        Serial.println(total_draw);
//      } // end goodBmp
//    }
//  }
//
//  bmpFile.close();
//  if(!goodBmp) Serial.println(F("BMP format not recognized."));
//}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

