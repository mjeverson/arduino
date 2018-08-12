#include <Adafruit_NeoPixel.h>
#include <HX711.h>
#include <Thread.h>
//todo: arduinothreads https://github.com/ivanseidel/ArduinoThread
#include <SparkFun_TB6612.h>
//todo: motor waklthrough https://learn.sparkfun.com/tutorials/tb6612fng-hookup-guide?_ga=2.224845469.683491163.1533922223-1605071062.1518369019

// Number of pixels in the string
#define ALTAR_READY_PIXELS 180 
#define LED_RING_PIXELS 24

// Bit of the pins the pixels are connected to (see LED API below)
//todo: dont need anymore only one set of leds run this way
//todo: will need pin #s for the neopixel rings though
//todo: must be pwm pins
#define ALTAR_READY_PIXEL_BIT 2
#define LED_RING_A 10
#define LED_RING_B 11
#define LED_RING_C 12
#define LED_ALTAR 7

Adafruit_NeoPixel strip_a = Adafruit_NeoPixel(LED_RING_PIXELS, LED_RING_A, NEO_GRB + NEO_KHZ800);        
Adafruit_NeoPixel strip_b = Adafruit_NeoPixel(LED_RING_PIXELS, LED_RING_B, NEO_GRB + NEO_KHZ800);        
Adafruit_NeoPixel strip_c = Adafruit_NeoPixel(LED_RING_PIXELS, LED_RING_C, NEO_GRB + NEO_KHZ800);  
Adafruit_NeoPixel strip_altar = Adafruit_NeoPixel(180, LED_ALTAR, NEO_GRB + NEO_KHZ800);      
 
#define FADE_SPEED 5
#define PULSE_SPEED 8

// Pins the scale is using. Note, we're not actually using CLK for the clock pin, which is odd. Does it only work because of the crystal on the dmxfire?
//todo: double check these pins since we're using a mega now
#define CLK  A0
#define DOUT  A1

//Sets up the scale. This value is obtained using the SparkFun_HX711_Calibration sketch.
//todo: do we need to recalibrate?
#define calibration_factor -9890.0
HX711 scale(DOUT, CLK);

// Pins for all inputs, keep in mind the PWM defines must be on PWM pins
// the default pins listed are the ones used on the Redbot (ROB-12097) with
// the exception of STBY which the Redbot controls with a physical switch
#define AIN1 2
#define AIN2 3
#define PWMA 4

#define BIN1 7 //todo: why does setting these to 23-25 cause problems?
#define BIN2 8
#define PWMB 5

#define CIN1 12 //todo: why does setting these to 23-25 cause problems?
#define CIN2 13
#define PWMC 6

#define STBYA 9
#define STBYB 10
#define STBYC 11

// these constants are used to allow you to make your motor configuration 
// line up with function names like forward.  Value can be 1 or -1
Motor motor_a = Motor(AIN1, AIN2, PWMA, 1, STBYA);
Motor motor_b = Motor(BIN1, BIN2, PWMB, 1, STBYB);
//Motor motor_c = Motor(BIN1, BIN2, PWMB, 1, STBYB);


///todo: new thread class for motors or lights
// Create a new Class, called SensorThread, that inherits from Thread
class MotorThread: public Thread
{
public:
  int value;
  int pin;

  // No, "run" cannot be anything...
  // Because Thread uses the method "run" to run threads,
  // we MUST overload this method here. using anything other
  // than "run" will not work properly...
  void run(){
    // Reads the analog pin, and saves it localy
    value = map(analogRead(pin), 0,1023,0,255);
    runned();
  }
};

MotorThread analog1 = MotorThread();
MotorThread analog2 = MotorThread();

void setup() {
  Serial.begin(9600);

  scale.set_scale(calibration_factor); 
  scale.tare();  

  analog1.pin = A1;
  analog1.setInterval(100);

  // Configures Thread analog2
  analog2.pin = A2;
  analog2.setInterval(100);

  Serial.flush();
  Serial.println("What time is it, party cat?");
}

// track the thread IDs for the lighting modes
int pulseThreadId;
int theatreThreadId;

void loop() {
  //todo: keep this it works
//  theaterChaseStrandTest(strip_altar.Color(127, 127, 127), 50); // White

  // Debug
//  Serial.print(scale.get_units());
//  Serial.println();

  if (Serial.available()) {    
    // check if a number was received
    if (Serial.read() == '0') {
      Serial.println("IT'S PARTY TIME!");
      //todo: why does motor_a not run until after motor_b.brake has been called?
      //todo: will need to test how this all works with threads...
      Serial.println("Doing motor2 drive pos");
      motor_b.drive(255,1000);
      Serial.println("Doing motor2 drive neg");
      motor_b.drive(-255,1000);
      Serial.println("Doing motor2 break");
      motor_b.brake();
      delay(1000);
    
      Serial.println("Doing motor1 drive pos");
      motor_a.drive(255,1000);
      Serial.println("Doing motor1 drive neg");
      motor_a.drive(-255,1000);
      Serial.println("Doing motor1 break");
      motor_a.brake();
      delay(1000);

      analog1.run();
      analog2.run();
    }
    else {
      Serial.println("Party's over... disengaging motor locomotion protocol :(");
      forward(motor_a, motor_b, 150);
//      motor_a.drive(150);
    }
  } 

  // Get the fresh readings
  //todo: keep this it seems to work
//  Serial.print("Analog1 Thread: ");
//  Serial.println(analog1.value);
//
//  Serial.print("Analog2 Thread: ");
//  Serial.println(analog2.value);
  //todo: need a way to stop threads from running. threadcontroller.clear()?
 
  //while we're looping. if someone's on the scale, do the theater chase. if someone's not on the scale, do the ready pulse.
  if(scale.get_units() > 50){
    //todo: kill pulsing thread if it's running
    //todo: start a thread that does theatre chase if it's not already running
    //todo: start a thread that does motors if it's not already running
    //todo: brighten overhead lights
//    theaterChaseStrandTest(strip.Color(127, 127, 127), 50); // White
  } else {
    //todo: kill theatre chase thread if it's running 
    //todo: start a thread for regular pulsing if it's not already running (setAltarListenLighting())
    //todo: kill "motors" thread
    //todo: darken overhead lights
    //doAltarReadyPulse();  
  }
}

//void doAltarReadyPulseThread(){
//  if (globalFadeLevel >= 255) {
//    globalFadeIn = false;
//  } else if (globalFadeLevel <= 1) {
//    globalFadeIn = true;     
//  }
//
//  if (globalFadeIn){
//    globalFadeLevel += PULSE_SPEED;
//  } else {
//    globalFadeLevel -= PULSE_SPEED;
//  }
//
//  float factor = globalFadeLevel / 255.0;
//  showColor(readyColors[0] * factor, readyColors[1] * factor, readyColors[2] * factor);
//}

//Theatre-style crawling lights.
void theaterChaseStrandTest(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip_altar.numPixels(); i=i+3) {
        strip_altar.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip_altar.show();
     
      delay(wait);
     
      for (int i=0; i < strip_altar.numPixels(); i=i+3) {
        strip_altar.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}
