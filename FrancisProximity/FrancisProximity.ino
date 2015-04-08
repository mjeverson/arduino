/*******************************************************************************

 Bare Conductive Proximity MP3 player
 ------------------------------

 proximity_mp3.ino - proximity triggered MP3 playback

 Based on code by Jim Lindblom and plenty of inspiration from the Freescale
 Semiconductor datasheets and application notes.

 Bare Conductive code written by Stefan Dzisiewski-Smith and Peter Krige.

 This work is licensed under a Creative Commons Attribution-ShareAlike 3.0
 Unported License (CC BY-SA 3.0) http://creativecommons.org/licenses/by-sa/3.0/

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

*******************************************************************************/

// compiler error handling
#include "Compiler_Errors.h"

// touch includes
#include <MPR121.h>
#include <Wire.h>
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

// mp3 includes
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <SFEMP3Shield.h>

// mp3 variables
SFEMP3Shield MP3player;

// sd card instantiation
SdFat sd;

// touch behaviour definitions. Note these pins refer to the touch electrodes and not output pins
#define firstPin 0
#define lastPin 9
int lastPlayed = 0;

//serial stuff for dispenser
// RX is 0, TX is 1

//solenoid stuff (works on pin 13)
#define SOLENOID 10

//led stuff (pins 10-13 are unallocated)
#include <Adafruit_NeoPixel.h>
#define NEOPIXEL 11 // Works on pin 1, but may interfere with serial stuff, try this
#define EYESTRIP 12 

// Adafruit_NeoPixel(number of pixels in strip, pin #, pixel type flags add as needed)
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel eyes = Adafruit_NeoPixel(2, EYESTRIP, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(57600);
  //while (!Serial) ; {} //uncomment when using the serial monitor
  
  pinMode(SOLENOID, OUTPUT)
  pinMode(LED_BUILTIN, OUTPUT);
  
  // SD Card setup
  if (!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();

  setupTouch();
  setupMp3();
  setupCrystalBall();
  setupEyes();
}

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

void loop() {
  // update the idle color for the orb
  updateIdlePulsePattern();
  
  //todo: update the timer, if the timer has hit timeout, run "complete" algorithm. otherwise, go into reading touch input
  bool timerHasCompleted = false;
  
  if(timerHasCompleted){
    // Move to the end of the round
    completeRound();
  } else {
    // Listen for touch inputs
    readTouchInputs();
  }
}

// Reads and handle the user's touch inputs
void readTouchInputs() {
  if (MPR121.touchStatusChanged()) {
    MPR121.updateTouchData();

    // only make an action if we have one or fewer pins touched, or only do the first action
    // ignore multiple touches
    // while touching change colors?
    // Eyes should glow
    // Game model
    // fire solenoid
    //todo: nyan cat easter egg
    //todo: maybe keep this in but also allow for more than one touch registering at once!
    if (MPR121.getNumTouches() <= 1) {
      
      // Ensure that we've touched a valid pin
      for (int i = firstPin; i <= lastPin; i++) {
        
        // If this is a new touch on this pin
        if (MPR121.isNewTouch(i)) {
          //pin i was just touched
          Serial.print("pin ");
          Serial.print(i);
          Serial.println(" was just touched");
          
          digitalWrite(LED_BUILTIN, HIGH);

          for (int i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, 0, 0, 255);
          }
          strip.show();

          //todo: instead of stoptrack is there a fade out?
          //todo: loop sounds while holding instead of play
          //todo: don't stop other tracks when touchdown

          if (i <= lastPin && i >= firstPin) {
            if (MP3player.isPlaying()) {

              if (lastPlayed == i) {
                // if we're already playing the requested track, stop it
                MP3player.stopTrack();
                MP3player.playTrack(i - firstPin);
                Serial.print("stopping track ");
                Serial.println(i - firstPin);
              } else {
                // if we're already playing a different track, stop that
                // one and play the newly requested one
                //MP3player.stopTrack();
                MP3player.stopTrack();
                MP3player.playTrack(i - firstPin);
                Serial.print("playing track ");
                Serial.println(i - firstPin);

                // don't forget to update lastPlayed - without it we don't
                // have a history
                lastPlayed = i;
              }
            } else {
              // if we're playing nothing, play the requested track
              // and update lastplayed
              MP3player.playTrack(i - firstPin);
              Serial.print("playing track ");
              Serial.println(i - firstPin);
              lastPlayed = i;
            }
          }
        } else if (MPR121.isNewRelease(i)) {
          //todo: fadeout the sound instead of just stop
          //            MP3player.stopTrack();
          //fadeOut(5);
          Serial.print("pin ");
          Serial.print(i);
          Serial.println(" is no longer being touched");
          digitalWrite(LED_BUILTIN, LOW);

          for (int i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, 255, 0, 255);
          }

          strip.show();
        }
        //        else{
        //           // loop whatever sound is playing
        //           //todo: maybe just long tracks and fade them out when stop touching
        //           //if(MP3player.isPlaying()){
        ////             MP3player.playTrack(i-firstPin);
        ////             lastPlayed = i;
        //           //}
        //        }
        //      }
      }
    }
    //  else {
    //    // touch status hasn't changed
    //    if(MP3player.isPlaying()){
    //      Serial.print("touch status hasn't changed, last played: " + lastPlayed);
    //       MP3player.playTrack(1);
    ////       lastPlayed = i;
    //    }
  }
}

// Persist the background "pulsing" color patterns for the orb, whenever there is no touch change to the current idle color state
void updateIdlePulsePattern(){
  //todo: some algorithm to randomly pulse different colors
}

// Change the orb's color either to the idle state or to a random color if touched
void updateOrbColor(bool isIdle){
  strip.setBrightness(10);

  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }

  strip.show();
}

// eyes glow pattern
void updateEyeColor(){
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
  delay(1000);
}

void dispenseFortune(){
  //todo: weird serial magic to trigger the card dispenser 
  //todo: some sort of error handling. make orb turn red or play a sound if out of cards or something?
}

