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
#define ALTAR_READY_PIXEL_BIT 2
#define LED_RING_A 10
#define LED_RING_B 11
#define LED_RING_C 12

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip_a = Adafruit_NeoPixel(LED_RING_PIXELS, LED_RING_A, NEO_GRB + NEO_KHZ800);        
Adafruit_NeoPixel strip_b = Adafruit_NeoPixel(LED_RING_PIXELS, LED_RING_B, NEO_GRB + NEO_KHZ800);        
Adafruit_NeoPixel strip_c = Adafruit_NeoPixel(LED_RING_PIXELS, LED_RING_C, NEO_GRB + NEO_KHZ800);        
 
#define FADE_SPEED 5
#define PULSE_SPEED 8
float readyColors[3] = {127, 127, 127};
float globalFadeLevel = 255;
bool globalFadeIn;

// Pins the scale is using. Note, we're not actually using CLK for the clock pin, which is odd. Does it only work because of the crystal on the dmxfire?
//todo: double check these pins since we're using a mega now
#define CLK  A0
#define DOUT  A1

//Sets up the scale. This value is obtained using the SparkFun_HX711_Calibration sketch.
//todo: do we need to recalibrate?
#define calibration_factor -9890.0
HX711 scale(DOUT, CLK);

//todo: motor values
// Pins for all inputs, keep in mind the PWM defines must be on PWM pins
#define AIN1 2
//#define BIN1 7
#define AIN2 4
//#define BIN2 8
#define PWMA 5
//#define PWMB 6
#define STBY 9

// these constants are used to allow you to make your motor configuration 
// line up with function names like forward.  Value can be 1 or -1
const int offsetA = 1;
Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY);

void setup() {
  Serial.begin(9600);

  scale.set_scale(calibration_factor); 
  scale.tare();  

  ledsetup();  
}

// track the thread IDs for the lighting modes
int pulseThreadId;
int theatreThreadId;

void loop() {
  // Debug
  Serial.print(scale.get_units());
  Serial.println();

  //todo: test the motor
  //Use of the drive function which takes as arguements the speed
  //and optional duration.  A negative speed will cause it to go
  //backwards.  Speed can be from -255 to 255.  Also use of the 
  //brake function which takes no arguements.
  motor1.drive(255,1000);
  motor1.drive(-255,1000);
  motor1.brake();
  delay(1000);

 
  //while we're looping. if someone's on the scale, do the theater chase. if someone's not on the scale, do the ready pulse.
  if(scale.get_units() > 50){
    //todo: kill pulsing thread if it's running
    //todo: start a thread that does theatre chase if it's not already running
    //todo: start a thread that does motors if it's not already running
    //todo: brighten overhead lights
    //theaterChaseThread(readyColors[0], readyColors[1], readyColors[2], 50, ALTAR_READY_PIXEL_BIT, ALTAR_READY_PIXELS, 5);
  } else {
    //todo: kill theatre chase thread if it's running 
    //todo: start a thread for regular pulsing if it's not already running (setAltarListenLighting())
    //todo: kill "motors" thread
    //todo: darken overhead lights
    //doAltarReadyPulse();  
  }
}

//TODO: Might want to do this in a thread while someone is standing on the scale
// Sets the altar lights in a pattern that indicates it's listening, 
void setAltarListenLighting(){
  // Theatre lighting around the outside three times
  theaterChaseThread(readyColors[0], readyColors[1], readyColors[2], 50);
}

void doAltarReadyPulseThread(){
  if (globalFadeLevel >= 255) {
    globalFadeIn = false;
  } else if (globalFadeLevel <= 1) {
    globalFadeIn = true;     
  }

  if (globalFadeIn){
    globalFadeLevel += PULSE_SPEED;
  } else {
    globalFadeLevel -= PULSE_SPEED;
  }

  float factor = globalFadeLevel / 255.0;
  showColor(readyColors[0] * factor, readyColors[1] * factor, readyColors[2] * factor);
}

// Theatre-style crawling lights. Changes spacing to be dynamic based on string size
void theaterChaseThread(unsigned char r, unsigned char g, unsigned char b, unsigned char wait) {
  int theatre_spacing = ALTAR_READY_PIXELS / 10;
  
  for (int q = 0; q < theatre_spacing ; q++) {   
    unsigned int step = 0;
    
    cli();
    
    for (int i=0; i < ALTAR_READY_PIXELS ; i++) {
      if (step == q) {
        sendPixel( r , g , b);
      } else {
        sendPixel( 0 , 0 , 0);
      }
      
      step++;
      
      if (step == theatre_spacing) step = 0;
    }
    
    sei();
    show();
    //todo: probably can't do delay here
    delay(wait); 
  } 
}

// Display a single color on the whole string
void showColor(unsigned char r, unsigned char g, unsigned char b) {
  cli();  
  
  for(int p = 0; p < ALTAR_READY_PIXELS; p++) {
    sendPixel(r, g, b);
  }
  
  sei();
  show();
}


// **************************************************************************
// LED API
// **************************************************************************

// More info on how to find these at http://www.arduino.cc/en/Reference/PortManipulation
//todo: will need to figure this out again for the mega. Seriously though with the mega can probably just get away with regular neopixel stuff.
#define PIXEL_PORT  PORTC  // Port of the pin the pixels are connected to
#define PIXEL_DDR   DDRC   // Port of the pin the pixels are connected to

// These are the timing constraints taken mostly from the WS2812 datasheets 
// These are chosen to be conservative and avoid problems rather than for maximum throughput 
#define T1H  900    // Width of a 1 bit in ns
#define T1L  600    // Width of a 1 bit in ns
#define T0H  400    // Width of a 0 bit in ns
#define T0L  900    // Width of a 0 bit in ns
#define RES 6000    // Width of the low gap between bits to cause a frame to latch

// Here are some convience defines for using nanoseconds specs to generate actual CPU delays
// Note that this has to be SIGNED since we want to be able to check for negative values of derivatives
#define NS_PER_SEC (1000000000L)          
#define CYCLES_PER_SEC (F_CPU)
#define NS_PER_CYCLE ( NS_PER_SEC / CYCLES_PER_SEC )
#define NS_TO_CYCLES(n) ( (n) / NS_PER_CYCLE )

// Actually send a bit to the string. We must to drop to asm to ensure that the compiler does
// not reorder things and make it so the delay happens in the wrong place.
inline void sendAltarReadyBit(bool bitVal) {
  if (bitVal) {        
    // 0 bit
    asm volatile (
      "sbi %[port], %[bit] \n\t"        // Set the output bit
      ".rept %[onCycles] \n\t"          // Execute NOPs to delay exactly the specified number of cycles
      "nop \n\t"
      ".endr \n\t"
      "cbi %[port], %[bit] \n\t"        // Clear the output bit
      ".rept %[offCycles] \n\t"         // Execute NOPs to delay exactly the specified number of cycles
      "nop \n\t"
      ".endr \n\t"
      ::
      [port]    "I" (_SFR_IO_ADDR(PIXEL_PORT)),
      [bit]   "I" (ALTAR_READY_PIXEL_BIT),
      [onCycles]  "I" (NS_TO_CYCLES(T1H) - 2),    // 1-bit width less overhead  for the actual bit setting, note that this delay could be longer and everything would still work
      [offCycles]   "I" (NS_TO_CYCLES(T1L) - 2)   // Minimum interbit delay. Note that we probably don't need this at all since the loop overhead will be enough, but here for correctness
    );
                                  
  } else {          
    // 1 bit
    
    // **************************************************************************
    // This line is really the only tight goldilocks timing in the whole program!
    // **************************************************************************
    asm volatile (
      "sbi %[port], %[bit] \n\t"      // Set the output bit
      ".rept %[onCycles] \n\t"        // Now timing actually matters. The 0-bit must be long enough to be detected but not too long or it will be a 1-bit
      "nop \n\t"                      // Execute NOPs to delay exactly the specified number of cycles
      ".endr \n\t"
      "cbi %[port], %[bit] \n\t"      // Clear the output bit
      ".rept %[offCycles] \n\t"       // Execute NOPs to delay exactly the specified number of cycles
      "nop \n\t"
      ".endr \n\t"
      ::
      [port]    "I" (_SFR_IO_ADDR(PIXEL_PORT)),
      [bit]   "I" (ALTAR_READY_PIXEL_BIT),
      [onCycles]  "I" (NS_TO_CYCLES(T0H) - 2),
      [offCycles] "I" (NS_TO_CYCLES(T0L) - 2)

    );
      
  }
  
  // Note that the inter-bit gap can be as long as you want as long as it doesn't exceed the 5us reset timeout (which is A long time)
  // Here I have been generous and not tried to squeeze the gap tight but instead erred on the side of lots of extra time.
  // This has thenice side effect of avoid glitches on very long strings becuase 
}  

inline void sendByte( unsigned char byte) {
    //TODO: There has to be a better way than having a different function for each pixel_bit
    //TODO: Get a weird "impossible constraint in 'asm'" error 
    for( unsigned char bit = 0 ; bit < 8 ; bit++ ) {
        sendAltarReadyBit(bitRead(byte, 7));

      // Neopixel wants bit in highest-to-lowest order, so send highest bit (bit #7 in an 8-bit byte since they start at 0)                                                                                 
      // and then shift left so bit 6 moves into 7, 5 moves into 6, etc
      byte <<= 1;                                   
    }           
} 

/*
  The following three functions are the public API:
  
  ledSetup() - set up the pin that is connected to the string. Call once at the begining of the program.  
  sendPixel( r g , b ) - send a single pixel to the string. Call this once for each pixel in a frame.
  show() - show the recently sent pixel on the LEDs . Call once per frame. 
*/

void ledsetup() {
  // Set the specified pin up as digital out
  // https://www.arduino.cc/en/Reference/bitSet
  bitSet(PIXEL_DDR, ALTAR_READY_PIXEL_BIT);
}

inline void sendPixel(unsigned char r, unsigned char g, unsigned char b)  {  
  // Neopixel wants colors in green then red then blue order
  sendByte(g);          
  sendByte(r);
  sendByte(b); 
}

void show() {
  // Just wait long enough without sending any bits to cause the pixels to latch and display the last sent frame
  // Round up since the delay must be _at_least_ this long (too short might not work, too long not a problem)
  _delay_us( (RES / 1000UL) + 1);       
}

