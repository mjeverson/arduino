/***************************************************
  We Are All Stardust 
  Michael Everson
  
  Designed for use with Teensy 3.6
  
  https://github.com/ftrias/TeensyThreads
  https://learn.sparkfun.com/tutorials/tb6612fng-hookup-guide?_ga=2.224845469.683491163.1533922223-1605071062.1518369019
 ****************************************************/
 
#include <Adafruit_NeoPixel.h>
#include <HX711.h>
#include <TeensyThreads.h>

// Number of pixels in the string
#define ALTAR_READY_PIXELS 180 
#define LED_RING_PIXELS 25

// Bit of the pins the pixels are connected to (see LED API below)
#define ALTAR_READY_PIXEL_BIT 2
#define LED_RING_A 33
#define LED_RING_B 34
#define LED_RING_C 35
#define LED_ALTAR 36

Adafruit_NeoPixel strip_a = Adafruit_NeoPixel(LED_RING_PIXELS, LED_RING_A, NEO_GRB + NEO_KHZ800);        
Adafruit_NeoPixel strip_b = Adafruit_NeoPixel(LED_RING_PIXELS, LED_RING_B, NEO_GRB + NEO_KHZ800);        
Adafruit_NeoPixel strip_c = Adafruit_NeoPixel(LED_RING_PIXELS, LED_RING_C, NEO_GRB + NEO_KHZ800);  
Adafruit_NeoPixel strip_altar = Adafruit_NeoPixel(ALTAR_READY_PIXELS, LED_ALTAR, NEO_RGB + NEO_KHZ800);      
 
#define FADE_SPEED 5
#define PULSE_SPEED 8

// Pins the scale is using. Note, we're not actually using CLK for the clock pin, which is odd. Does it only work because of the crystal on the dmxfire?
//todo: double check these pins since we're using a mega now
#define CLK  13
#define DOUT  14

//Sets up the scale. This value is obtained using the SparkFun_HX711_Calibration sketch.
//todo: do we need to recalibrate?
#define calibration_factor -9890.0
HX711 scale(DOUT, CLK);

// Pins for all inputs. Motors A & B are on the same driver so they use the same STBY. Keep in mind the PWM defines must be on PWM pins.
// Alternatively: STBYA 2 All As 3 4 5 All Bs 6 7 8 STBY C 9 All Cs 10 11 12
int STBYA = 7; //standby
int STBYC = 10; //standby

//Motor A
int PWMA = 2; //Speed control
int AIN1 = 5; //Direction
int AIN2 = 6; //Direction

//Motor B
int PWMB = 3; //Speed control
int BIN1 = 8; //Direction
int BIN2 = 9; //Direction

//Motor C
int PWMC = 4; //Speed control
int CIN1 = 11; //Direction
int CIN2 = 12; //Direction

// track the thread IDs for the lighting modes
int pulseThreadId;
int theatreThreadId;
int rainbowThreadId;

void setup() {
  Serial.begin(9600);

  // initialize scale
  scale.set_scale(calibration_factor); 
  scale.tare();  

  // initialize motor pins
  pinMode(STBYA, OUTPUT);
  pinMode(STBYC, OUTPUT);
  digitalWrite(STBYA, HIGH);
  digitalWrite(STBYC, HIGH);
  
  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  pinMode(PWMC, OUTPUT);
  pinMode(CIN1, OUTPUT);
  pinMode(CIN2, OUTPUT);

  // Initialize LEDs
  strip_a.begin();
  strip_a.show();
  
  strip_b.begin();
  strip_b.show();
  
  strip_c.begin();
  strip_c.show();
  
  strip_altar.begin();
  strip_altar.show(); 

  // Clear the serial buffer and prep for input
  Serial.flush();
  Serial.println("What time is it, party cat?");

//  pulseThreadId = threads.addThread(doAltarPulseThread);
  resetAltarLights();
}

void doAltarPulseThread(){
  while(true){
    doAltarPulse();
  }
}

void doAltarChaseThread(){
  while(true){
    theaterChaseStrandTest(strip_altar.Color(127, 127, 127), 50);
  }
}

void doRainbowThread(){
  while(true){
    Serial.println("doing rainbow thread");
    rainbowCycle(20);
  }
}

bool isActive = false;

void loop() {
  // Debug
  Serial.print(scale.get_units());
  Serial.println();

  //while we're looping. if someone's on the scale, do the theater chase. if someone's not on the scale, do the ready pulse.
  if(scale.get_units() > 25){
    if(!isActive){
      threads.kill(pulseThreadId);
      rainbowThreadId = threads.addThread(doRainbowThread);
      theatreThreadId = threads.addThread(doAltarChaseThread);
    }
    
    // Start/Continue driving the motors
    driveAll(150);
    isActive = true;
  } else {
    if(isActive){
      threads.kill(theatreThreadId);
      threads.kill(rainbowThreadId);
//      pulseThreadId = threads.addThread(doAltarChaseThread);
      resetAltarLights();
      resetRainbowLights();
    }
    
    // Stops the motors
    stopAll();
    isActive = false;
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip_a.numPixels(); i++) {
      strip_a.setPixelColor(i, Wheel(((i * 256 / strip_a.numPixels()) + j) & 255));
      strip_b.setPixelColor(i, Wheel(((i * 256 / strip_b.numPixels()) + j) & 255));
      strip_c.setPixelColor(i, Wheel(((i * 256 / strip_c.numPixels()) + j) & 255));
    }
    strip_a.show();
    strip_b.show();
    strip_c.show();
    threads.delay(wait);
  }
}

void resetRainbowLights() {
  for(int i=0; i< strip_a.numPixels(); i++) {
    strip_a.setPixelColor(i, strip_a.Color(0, 0, 0));
    strip_b.setPixelColor(i, strip_b.Color(0, 0, 0));
    strip_c.setPixelColor(i, strip_c.Color(0, 0, 0));
  }
  
  strip_a.show();
  strip_b.show();
  strip_c.show();
}

void resetAltarLights() {
  for(int i=0; i< strip_altar.numPixels(); i++) {
    strip_altar.setPixelColor(i, strip_altar.Color(127, 127, 127));
  }
  
  strip_altar.show();
}

void driveAll(int speed) {
    driveMotor(1, speed);
    driveMotor(2, speed);
    driveMotor(3, speed);
}

void stopAll(){
    stopMotor(1);
    stopMotor(2);
    stopMotor(3);
}

void driveMotor(int motor, int speed){
  switch (motor) {
    case 1: {
      Serial.println("driving motor 1");
      digitalWrite(AIN1, HIGH);
      digitalWrite(AIN2, LOW);
      analogWrite(PWMA, speed);
      break;
    }
    case 2: {
      Serial.println("drive motor 2");
      digitalWrite(BIN1, HIGH);
      digitalWrite(BIN2, LOW);
      analogWrite(PWMB, speed);
      break;
    }
    case 3:{
      Serial.println("drive motor 3");
      digitalWrite(CIN1, HIGH);
      digitalWrite(CIN2, LOW);
      analogWrite(PWMC, speed);
      break;
    }
    default: {
      break;
    }
  }
}

void stopMotor(int motor){
  switch (motor) {
    case 1: {
//      Serial.println("stopping motor 1");
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, LOW);
      break;
    }
    case 2: {
//      Serial.println("stopping motor 2");
      digitalWrite(BIN1, LOW);
      digitalWrite(BIN2, LOW);
      break;
    }
    case 3:{
//      Serial.println("stopping motor 3");
      digitalWrite(CIN1, LOW);
      digitalWrite(CIN2, LOW);
      break;
    }
    default: {
      break;
    }
  }
}

// Intended to be run within a thread
void doAltarPulse(){
  Serial.println("pulsing altar");
  for (int i = 0; i <= 127; i+=1) {
    for(int j=0; j< strip_altar.numPixels(); j++) {
      strip_altar.setPixelColor(j, i,i,i);
    }

    strip_altar.show();      
    threads.delay(100);
  }

  for (int k = 127; k >=0; k-=1) {
    for(int l=0; l< strip_altar.numPixels(); l++) {
      strip_altar.setPixelColor(l, k,k,k);
    }

    strip_altar.show();      
    threads.delay(100);
  }
}

//Theatre-style crawling lights.
// Intended to be run within a thread
void theaterChaseStrandTest(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip_altar.numPixels(); i=i+3) {
        strip_altar.setPixelColor(i+q, 127,127,127);    //turn every third pixel on
      }
      strip_altar.show();
     
      threads.delay(100);
     
      for (int i=0; i < strip_altar.numPixels(); i=i+3) {
        strip_altar.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return strip_a.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
     WheelPos -= 85;
     return strip_a.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
     WheelPos -= 170;
     return strip_a.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


