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

  References:
  //https://arduino.stackexchange.com/questions/26803/connecting-multiple-tft-panels-to-arduino-uno-via-spi
 ****************************************************/

#include <Adafruit_GFX.h> // Core graphics library
#include "Adafruit_HX8357.h"
//#include <HX8357_t3.h> // Hardware-specific library. Faster, but doesn't seem to render the image on the screen?
#include <SPI.h>
#include "SdFat.h"
//#include <Adafruit_NeoPixel.h> // LED Stuff
//#include <Servo.h> // Tentacle & Coin
//#include <TMRpcm.h> // Play Wav Files
//#include <Wire.h> // Amp controller

//#define HANDLE A1 //handle mechanism

//#define SOL1 2-10, 14, 16-17, 20-23, 29-30, 35-38 // Solenoid Stuff
//#define SOL2 2-10, 14, 16-17, 20-23, 29-30, 35-38 // Solenoid Stuff
//#define SOL3 2-10, 14, 16-17, 20-23, 29-30, 35-38 // Solenoid Stuff
//#define SOL4 2-10, 14, 16-17, 20-23, 29-30, 35-38 // Solenoid Stuff

//#define SPEAKER 2-10, 14, 16-17, 20-23, 29-30, 35-38
//#define MAX9744_I2CADDR 0x4B
//int volume = 31; // 0-63
//TMRpcm tmrpcm; // wav Stuff

//#define TENTACLE_SERVO 2-10, 14, 16-17, 20-23, 29-30, 35-38
//#define COIN_SERVO 2-10, 14, 16-17, 20-23, 29-30, 35-38
//#define NEOPIXEL 2-10, 14, 16-17, 20-23, 29-30, 35-38 // LED Stuff

// Adafruit_NeoPixel(number of pixels in strip, pin #, pixel type flags add as needed)
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, NEOPIXEL, NEO_GRB + NEO_KHZ800);

//Servo tentacleServo;
//Servo coinServo;  
//int tentacleServoPos = 0;
//int coinServoPos = 0;

// screens 1 and 2 are on the spi0 bus (mosi 11, miso 12, sck 13) screen 3 is on spi1 (mosi 0, miso 1, sck 32)
#define TFT_DC 24
#define TFT_CS 25 
#define TFT_CS2 26
#define TFT_CS3 27

#define MOSI1 0
#define MISO1 1
#define TFT_DC2 28
#define SCK1 32

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
//HX8357_t3 tft = HX8357_t3(TFT_CS, TFT_DC);
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC);
Adafruit_HX8357 tft2 = Adafruit_HX8357(TFT_CS2, TFT_DC);
Adafruit_HX8357 tft3 = Adafruit_HX8357(TFT_CS3, TFT_DC2, MOSI1, SCK1, -1, MISO1);

#define NUM_SLOTS 6
int winState, slot1_current, slot2_current, slot3_current; 
char* images[] = {"nyanf.bmp", "tentf.bmp", "coinf.bmp", "firef.bmp", "cheesef.bmp", "pinchyf.bmp"};
char* sounds[] = {"nyan.wav", "scream.wav", "mario.wav", "hth.wav", "cheesy.wav", "pinchy.wav", "roll.wav"};

// Onboard Teensy 3.6 SD Slot
const uint8_t SD_CHIP_SELECT = SS;

// Faster library for SDIO
SdFatSdioEX sd;

void setup(void) {
  // Sets up the RNG
  randomSeed(analogRead(A0));
  
  Serial.begin(9600);
  Serial.println("SdFat version: ");
  Serial.print(SD_FAT_VERSION);

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
    Serial.println("\ncardBegin failed");
    return;
  }

  if (!sd.fsBegin()) {
    Serial.println("\nFile System initialization failed.\n");
    return;
  }

  t = millis() - t;
  
  Serial.println("\ninit time: ");
  Serial.print(t);

  // Set up sound player
//  tmrpcm.speakerPin = SPEAKER;
//  Wire.begin(); // Amp
//  setVolume(volume);

  // Set up handle listener
//  pinMode(HANDLE, INPUT_PULLUP);

  // Set up solenoid
//  pinMode(SOL1, OUTPUT);
//  pinMode(SOL2, OUTPUT);
//  pinMode(SOL3, OUTPUT);
//  pinMode(SOL4, OUTPUT);

  //Set up servos
//  tentacleServo.attach(TENTACLE_SERVO);
//  coinServo.attach(COIN_SERVO);

  // Set up LEDs
//  strip.begin();

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

  // Read any existing Serial data.

  while (!Serial.available()) {
    SysCall::yield();
  }
  
  rollSlots();
  doWinState();

  do {
    delay(10);
  } while (Serial.available() && Serial.read() >= 0);
}

// Sets the slots rolling, picks an outcome and displays it
void rollSlots(){  
  // On rollSlots, we should iterate through 0-6 and set the slot to be whatever its current state is +1 (rolling over, so use %)
  // Also need to randomize a win state. States should be any of the 6 outcomes, or a total loss, or an almost loss (Trish has the odds)
  // On a result, save the global state of the slots. 
  // keep rolling the first slot til it gets where it needs to go. Then the second, then the third. (don't update the global state)
  // OR just let the first slot start one or two early, then the second slot, then the third slot. let them roll a few times, then do it all again. Don't need global state

  // Calculate win state
  winState = random(1,20); 
  Serial.println("winState");
  Serial.print(winState);
  
  // Calcuate partial fail slot displays
  int falseWinSlot, falseWinSlotOdd;
    
  do {
    falseWinSlot = random(0,5);
    falseWinSlotOdd = random(0,5);
  } while (falseWinSlot == falseWinSlotOdd);
  
  int slot1_end, slot2_end, slot3_end;
  
  if (winState <= 2) {
    // nyancat
    slot1_end = slot2_end = slot3_end = 0;
  } else if (winState <= 4){
    // tentacle
    slot1_end = slot2_end = slot3_end = 1;
  } else if (winState == 5) {
    // coin
    slot1_end = slot2_end = slot3_end = 2;
  } else if (winState <= 7) {
    // fire
    slot1_end = slot2_end = slot3_end = 3;
  } else if (winState <= 9) {
    // cheesy poofs
    slot1_end = slot2_end = slot3_end = 4;
  } else if (winState == 10){
    // pinchy
    slot1_end = slot2_end = slot3_end = 5;
  } else if (winState <= 12) {
    // partial fail
    slot1_end = slot2_end = falseWinSlot;
    slot3_end = falseWinSlotOdd;
  } else if (winState <= 14) {
    // Partial fail
    slot1_end = slot3_end = falseWinSlot;
    slot2_end = falseWinSlotOdd;
  }else if (winState <= 16) {
    // Partial fail
    slot2_end = slot3_end = falseWinSlot;
    slot1_end = falseWinSlotOdd;
  } else {
    // Total fail
    slot1_end = falseWinSlot;
    slot2_end = falseWinSlotOdd;
    slot3_end = random(0,5);
  }

  //TODO: Start playing rolling sound? Loop in the background until after?
  
  int index = 0;
  int slot1_stoppedAt = -1;
  int slot2_stoppedAt = -1;
  int minRollsBeforeStopping = 5;

  // while the min number of changes hasn't happened AND the slots aren't in their final slots
  while(index < minRollsBeforeStopping || slot1_current != slot1_end || slot2_current != slot2_end || slot3_current != slot3_end || index < slot2_stoppedAt + 2) {
    // only let the first slot move for the first two iterations, then add the second for the next two, then start the third
    // After a min number of changes, let the first one go til it reaches its final state. two iterations later let the second go til it hits it. then two more later the third.

    Serial.print("index: ");
    Serial.println(index);

    if (index < minRollsBeforeStopping || slot1_current != slot1_end){
      slot1_current++;
      slot1_current = slot1_current > 5 ? 0 : slot1_current;

      Serial.print("Loading slot 1, is: ");
      Serial.print(slot1_current);
      Serial.print(" should be: ");
      Serial.print(slot1_end);
      Serial.println("");
      
      
      bmpDraw(images[slot1_current], 0, 0, tft);
    }
    
    if (index >= minRollsBeforeStopping && slot1_current == slot1_end && slot1_stoppedAt == -1) {
      slot1_stoppedAt = index;
      Serial.println("slot 1 stopped at this index");
    }
  
    if (index > 1 && (index < minRollsBeforeStopping || slot1_stoppedAt == -1 || (slot1_stoppedAt > -1 && index < slot1_stoppedAt + 2) || slot2_current != slot2_end)){
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
    
    if (index > 3 && (index < minRollsBeforeStopping || slot2_stoppedAt == -1  || (slot2_stoppedAt > -1 && index < slot2_stoppedAt + 2) || slot3_current != slot3_end)){
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

  //TODO: Stop playing rolling sound?
}

void doWinState(){
  //based on win state do sounds, fire, etc.
  //TODO: same if/else as above, maybe consolidate somehow?
  //TODO: could change image on screen for victory if we want
  if (winState <= 2) {
    // nyancat
    Serial.println("doWinState nyan");
    //fire: 1-2-3-4-4-3-2-1
    //LEDs: nyancat rainbow marquee
    //Sound: nyancat
  } else if (winState <= 4){
    Serial.println("doWinState tentacle");
    // tentacle
    //fire: all at once
    // LEDs: green
    //Sound: person screaming
  } else if (winState == 5) {
    Serial.println("doWinState coin");
    // coin
    // fire: 1-3-2-4-all
    //LEDs: Yellow
    // sound: mario 1up/coin
  } else if (winState <= 7) {
    Serial.println("doWinState fire");
    // fire
    // fire all 4 x3
    // LEDs: Red
    // sound: highway to hell
  } else if (winState <= 9) {
    Serial.println("doWinState cheesy");
    // cheesy poofs
    // no fire
    // LEDs: white
    // Sound: cheesy poofs
  } else if (winState == 10){
    Serial.println("doWinState pinchy");
    // pinchy
    // fire all 4
    // LEDs: Red
    // Sound: PINCHAY
  } 
}

void playSound(){
  //sounds needed: nyancat, pinchy, person screaming (homer?), super mario coin/1up, highway to hell, cartman cheesy poofs, slot rolling sound mario kart?
  //tmrpcm.play("nyancat.wav");
}

void doFire(){
  //trigger the solenoids
//  digitalWrite(SOL1, HIGH);
//  delay(1000);
//  digitalWrite(SOL1, LOW);
//  delay(500);
//  digitalWrite(SOL2, HIGH);
//  delay(1000);
//  digitalWrite(SOL2, LOW);
}

void doTentacle(){
  // Servo to trigger tentacle
//  for(tentacleServoPos = 10; tentacleServoPos < 170; tentacleServoPos += 1) 
//  {                                  
//    tentacleServo.write(tentacleServoPos); 
//    delay(15);                      
//  } 
}

void doCoin(){
  // servo to trigger coin dispenser
//  for(coinServoPos = 10; coinServoPos < 170; coinServoPos += 1)  // goes from 10 degrees to 170 degrees 
//  {                                  // in steps of 1 degree 
//    coinServo.write(coinServoPos);              // tell servo to go to position in variable 'pos' 
//    delay(15);                       // waits 15ms for the servo to reach the position 
//  } 
}

void doLEDs(){
  //RGB LED patterns
//  int stripRed = random(0,255);
//  int stripGreen = random(0,255);
//  int stripBlue = random(0,255);
//
//  // pick new colors
//  for (int i = 0; i < strip.numPixels(); i++) {
//    strip.setPixelColor(i, stripGreen, stripRed, stripBlue);
//  }
}

// Setting the volume is very simple! 
void setVolume(int v) {
  // cant be higher than 63 or lower than 0
//  if (v > 63) v = 63;
//  if (v < 0) v = 0;
//  
//  Serial.print("Setting volume to ");
//  Serial.println(v);
//  Wire.beginTransmission(MAX9744_I2CADDR);
//  Wire.write(v);
//  Wire.endTransmission();
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
