/***************************************************
  This is our Bitmap drawing example for the Adafruit HX8357 Breakout
  ----> http://www.adafruit.com/products/2050

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
//https://arduino.stackexchange.com/questions/26803/connecting-multiple-tft-panels-to-arduino-uno-via-spi


//for speed:
// not full size bmps, only the pixels we need to draw. background transparent doesn't even matter, no background for that.
// check the better IL library optimized for teensy
// draw one screen at a time so each screen upates faster, 1-2-3-1-2-3

#include <Adafruit_GFX.h>    // Core graphics library
#include "Adafruit_HX8357.h"
#include <SPI.h>
#include "SdFat.h"

// Set USE_SDIO to zero for SPI card access. 
#define USE_SDIO 1

// TFT display and SD card will share the hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno,
// Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.

#define TFT_DC 24//9
#define TFT_CS 25//10
#define TFT_CS2 26//11
#define TFT_CS3 27//12
// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC);
Adafruit_HX8357 tft2 = Adafruit_HX8357(TFT_CS2, TFT_DC);
Adafruit_HX8357 tft3 = Adafruit_HX8357(TFT_CS3, TFT_DC);

// NEW
/*
 * SD chip select pin.  Common values are:
 *
 * Arduino Ethernet shield, pin 4.
 * SparkFun SD shield, pin 8.
 * Adafruit SD shields and modules, pin 10.
 * Default SD chip select is the SPI SS pin.
 */
const uint8_t SD_CHIP_SELECT = SS;

#if USE_SDIO
// Use faster SdioCardEX
SdFatSdioEX sd;
#else // USE_SDIO
SdFat sd;
#endif  // USE_SDIO

// global for card size
uint32_t cardSize;

//------------------------------------------------------------------------------
// store error strings in flash
#define sdErrorMsg(msg) sd.errorPrint(F(msg));

// end NEW

void setup(void) {
  Serial.begin(9600);

  // NEW
    // Wait for USB Serial 
    while (!Serial) {
      SysCall::yield();
    }

    Serial.println("SdFat version: ");
    Serial.print(SD_FAT_VERSION);
  // END NEW

  tft.begin(HX8357D);
  tft2.begin(HX8357D);
  tft3.begin(HX8357D);
  
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

  t = millis() - t;

  cardSize = sd.card()->cardSize();
  if (cardSize == 0) {
    Serial.println("cardSize failed");
    return;
  }

  if (!sd.fsBegin()) {
    sdErrorMsg("\nFile System initialization failed.\n");
    return;
  }
  
  Serial.println("\ninit time: ");
  Serial.print(t);

  bmpDraw("nyanf.bmp", 0, 0, tft);
  bmpDraw("nyanf.bmp", 0, 0, tft2);
  bmpDraw("nyanf.bmp", 0, 0, tft3);
}

void loop() {
  bmpDraw("nyanh.bmp", 0, 0, tft);
  bmpDraw("nyanh.bmp", 0, 0, tft2);
  bmpDraw("nyanh.bmp", 0, 0, tft3);
  bmpDraw("nyanf.bmp", 0, 0, tft);
  bmpDraw("nyanf.bmp", 0, 0, tft2);
  bmpDraw("nyanf.bmp", 0, 0, tft3);
}

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 20

void bmpDraw(char *filename, uint8_t x, uint16_t y, Adafruit_HX8357 screen) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

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
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        screen.setAddrWindow(x, y, x+w-1, y+h-1);
//        tft2.setAddrWindow(x, y, x+w-1, y+h-1);
//        tft3.setAddrWindow(x, y, x+w-1, y+h-1);

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
            screen.pushColor(tft.color565(r,g,b));
//            tft2.pushColor(tft2.color565(r,g,b));
//            tft3.pushColor(tft3.color565(r,g,b));
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
