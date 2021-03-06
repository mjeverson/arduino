/***************************************************
  This is our Bitmap drawing example for the Adafruit ILI9341 Breakout and Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/


#include <Adafruit_GFX.h>    // Core graphics library
//#include "Adafruit_ILI9341.h" // Hardware-specific library
//#include <ILI9341_t3.h>
#include "Adafruit_HX8357.h"
#include <SPI.h>
#include <SD.h>

// TFT display and SD card will share the hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno,
// Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.

#define TFT_DC 9
#define TFT_CS 10
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
//ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, 8);
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC);

#define SD_CS 53

int pUp = 0;
int pDown = 1;
int pLeft = 2;
int pRight = 3;
int pCenter = 5;

char *mains[] = {"nyanf.bmp","nyanh.bmp"};
bool main_page = true;
bool run_page = false;
bool menu_page = false;
int mainn = 0;

void setup(void) {

  Serial.begin(9600);
  Serial.println("OKOK");

  delay(1000);

  tft.begin();
  tft.fillScreen(HX8357_BLUE);

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed!");
  }else{
    Serial.println("OK!");
  }


  pinMode(pUp,INPUT_PULLUP);
  pinMode(pDown,INPUT_PULLUP);
  pinMode(pLeft,INPUT_PULLUP);
  pinMode(pRight,INPUT_PULLUP);
  pinMode(pCenter,INPUT_PULLUP);
  
  bmpDraw(mains[0], 0, 0);
  //bmpDraw("test.bmp", 0, 0);
}


void loop() {

  if(digitalRead(pUp)==LOW){
    delay(50);

    if(main_page==true){
      mainn--;
      if(mainn<0){mainn=3;}
      //tft.fillScreen(ILI9341_BLACK);
      bmpDraw(mains[mainn], 0, 0);
    }
  }
  if(digitalRead(pDown)==LOW){
    delay(50);

    if(main_page==true){
      mainn++;
      if(mainn>2){mainn=0;}
      //tft.fillScreen(ILI9341_BLACK);
      bmpDraw(mains[mainn], 0, 0);
    }
  }

  if(digitalRead(pCenter)==LOW){
    if(main_page==true){
      if(mainn==0){
        main_page=false;
        //tft.fillScreen(ILI9341_BLACK);
        bmpDraw("run.bmp", 0, 0);
      }
      if(mainn==2){
        main_page=false;
        //tft.fillScreen(ILI9341_BLACK);
        bmpDraw("menu.bmp", 0, 0);
      }
    }else{
      bmpDraw(mains[0], 0, 0);
      mainn = 0;
      main_page=true;
    }
  }
  
}


#define BUFFPIXEL 240
//===========================================================
// Try Draw using writeRect
void bmpDraw(char *filename, uint8_t x, uint16_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint16_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  uint16_t awColors[320];  // hold colors for one row at a time...

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("File not found"));
    return;
  }

  uint32_t usec;
  uint32_t us;
  uint32_t total_seek = 0;
  uint32_t total_read = 0;
  uint32_t total_parse = 0;
  uint32_t total_draw = 0;

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

        usec = 0;
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
          us = usec;
          usec -= us;
          total_seek += us;

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              us = usec;
              usec -= us;
              total_parse += us;
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
              us = usec;
              usec -= us;
              total_read += us;
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            awColors[col] = tft.color565(r,g,b);
          } // end pixel
          us = usec;
          usec -= us;
          total_parse += us;
          tft.writeRect(0, row, w, 1, awColors);
          us = usec;
          usec -= us;
          total_draw += us;
        } // end scanline
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
        Serial.print("Seek: ");
        Serial.println(total_seek);
        Serial.print("Read: ");
        Serial.println(total_read);
        Serial.print("Parse: ");
        Serial.println(total_parse);
        Serial.print("Draw: ");
        Serial.println(total_draw);
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
