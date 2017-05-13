#include <HX711.h>
#include <Tlc5940.h>

//http://alex.kathack.com/codes/tlc5940arduino/html_r011/group__CoreFunctions.html
//4095 is the max value for a channel on a tlc5940
#define TLC_ON 4095
#define TLC_OFF 0

// Number of pixels in the string (180 altar ready, 150 altar safety, 300 table safety)
#define ALTAR_READY_PIXELS 180 
#define ALTAR_SAFETY_PIXELS 150  
#define TABLE_SAFETY_PIXELS 300  

// Bit of the pins the pixels are connected to (see LED API below)
#define ALTAR_READY_PIXEL_BIT 2      
#define ALTAR_SAFETY_PIXEL_BIT 3    
#define TABLE_SAFETY_PIXEL_BIT 4   
 
#define FADE_SPEED 5
#define PULSE_SPEED 8
float safetyColors[3] = {127, 0, 0};
float readyColors[3] = {127, 127, 127};
float globalFadeLevel = 255;
bool globalFadeIn;

// Pins the scale is using. Note, we're not actually using CLK for the clock pin, which is odd. Does it only work because of the crystal on the dmxfire?
#define CLK  A0
#define DOUT  A1

// Pins the buttons are connected to
#define GO_SWITCH 2
#define SPELL_SWITCH 4
#define GO_BUTTON 7
#define SPELL_BUTTON 8

//Sets up the scale. This value is obtained using the SparkFun_HX711_Calibration sketch.
#define calibration_factor -9890.0
HX711 scale(DOUT, CLK);

void setup() {
  Serial.begin(9600);

  pinMode(GO_BUTTON, INPUT_PULLUP);      
  pinMode(SPELL_BUTTON, INPUT_PULLUP); 
  pinMode(GO_SWITCH, OUTPUT);  
  pinMode(SPELL_SWITCH, OUTPUT);  
  
  digitalWrite(GO_SWITCH, HIGH);
  digitalWrite(SPELL_SWITCH, HIGH);

  ledsetup();  
  fadeLightingIn();

  Tlc.init();
  flameOff();

  scale.set_scale(calibration_factor); 
  scale.tare();  
}

void loop() {
  // Debug
  Serial.print(scale.get_units());
  Serial.println();
 
  // Button reads inverted because we're using the internal pullup resistor
  if (!digitalRead(GO_BUTTON)){
    // Go time! Do some nice LED prep stuff to show the altar is reading/waiting
    setAltarListenLighting();
    
    delay(2000);
    
    // Take a scale reading and trigger some fire and lights
    evaluateOffering();

    // Fade all lights out, then fade back in
    fadeLightingOutThenIn();

    delay(2000);
  } else if (!digitalRead(SPELL_BUTTON)) {
    // Invoke the great old ones
    fadeStrandIn(readyColors, ALTAR_READY_PIXEL_BIT, ALTAR_READY_PIXELS);
    
    castSpell();
    
    delay(1000);
    
    fadeStrandOutThenIn(readyColors, ALTAR_READY_PIXEL_BIT, ALTAR_READY_PIXELS);

    delay(2000);
  } else {
    // Pulsate the white light on the altar, track with a global factor each loop and reset when you do anything    
    doAltarReadyPulse();
  }

  // Always make sure fire is off unless we're actively doing something
  flameOff();
}

void castSpell() {
  //TODO: Custom lights?
  // Custom spell fire pattern
  long numberOfEffects = random(5, 10);

  for (int i = 0; i < numberOfEffects; i++){
    long effectNumber = random(0, 6);  

     Tlc.set(effectNumber, TLC_ON); 
     Tlc.update();
     delay(250);
  
     Tlc.set(effectNumber, TLC_OFF); 
     Tlc.update();
     delay(250);
  }

  candleFlames();
}

// Make fire and lights happen based on the scale reading!
void evaluateOffering() {  
  // Add in an element of random chance, but should always be the best when over a certain threshold
  long willOfTheCat = random(0, 100);

  //TODO: Probably want to reduce the thresholds, still do >100 or so for max effort, but give everything a better probability. Average offering probably won't be very heavy and we want moar fire.
  // Evaluate the weight thresholds and response patterns
  if (scale.get_units() > 100 || willOfTheCat > 95) {
    // Flame on!
    fireOutsideIn();
    candleFlames();
  } else if((scale.get_units() <= 100 && scale.get_units() > 30) || willOfTheCat > 85) {
    fireOutsideIn();
  } else if((scale.get_units() <= 30 && scale.get_units() > 20) || willOfTheCat > 75) {
    outerFlames();
    middleFlames();
    innerFlames();
  } else if((scale.get_units() <= 20 && scale.get_units() > 10) || willOfTheCat > 65) {
    outerFlames();
    middleFlames();
  } else if((scale.get_units() <= 10 && scale.get_units() > 5) || willOfTheCat > 55) {
    outerFlames();
  } else {
    catScoff();
  }

  // Flame off!
  flameOff(); 
}

// Sets the altar lights in a pattern that indicates it's listening, 
void setAltarListenLighting(){
  // TODO: Should take about 5s? how long should our delay be?
  // Theatre lighting around the outside three times
  theaterChase(readyColors[0], readyColors[1], readyColors[2], 50, ALTAR_READY_PIXEL_BIT, ALTAR_READY_PIXELS, 5);
  fadeStrandIn(readyColors, ALTAR_READY_PIXEL_BIT, ALTAR_READY_PIXELS);
}

void doAltarReadyPulse(){
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
  showColor(readyColors[0] * factor, readyColors[1] * factor, readyColors[2] * factor, ALTAR_READY_PIXEL_BIT, ALTAR_READY_PIXELS);
}

// Theatre-style crawling lights. Changes spacing to be dynamic based on string size
void theaterChase(unsigned char r, unsigned char g, unsigned char b, unsigned char wait, int strand, int num_pixels, int loops) {
  int theatre_spacing = num_pixels / 10;
  
  for (int j = 0; j < loops ; j++) {   
    for (int q = 0; q < theatre_spacing ; q++) {   
      unsigned int step = 0;
      
      cli();
      
      for (int i=0; i < num_pixels ; i++) {
        if (step == q) {
          sendPixel( r , g , b, strand);
        } else {
          sendPixel( 0 , 0 , 0, strand);
        }
        
        step++;
        
        if (step == theatre_spacing) step = 0;
      }
      
      sei();
      show();
      delay(wait);
    } 
  } 
}

void fadeLightingOutThenIn() {  
  fadeLightingOut();
  delay(3000);
  fadeLightingIn();
}

void fadeStrandOutThenIn(float colors[], int strand, int num_pixels) {  
  fadeStrandOut(colors, strand, num_pixels);
  delay(3000);
  fadeStrandIn(colors, strand, num_pixels);
}

void fadeLightingOut() {  
  for (float b = 255; b > 0; b -= FADE_SPEED) {
    float factor = b / 255;
    showColor(readyColors[0] * factor, readyColors[1] * factor, readyColors[2] * factor, ALTAR_READY_PIXEL_BIT, ALTAR_READY_PIXELS);
    showColor(safetyColors[0] * factor, safetyColors[1] * factor, safetyColors[2] * factor, ALTAR_SAFETY_PIXEL_BIT, ALTAR_SAFETY_PIXELS);
    showColor(safetyColors[0] * factor, safetyColors[1] * factor, safetyColors[2] * factor, TABLE_SAFETY_PIXEL_BIT, TABLE_SAFETY_PIXELS);
  }
}

void fadeLightingIn() {  
  for (float b = 0; b < 255; b += FADE_SPEED) {
    float factor = b / 255;
    showColor(readyColors[0] * factor, readyColors[1] * factor, readyColors[2] * factor, ALTAR_READY_PIXEL_BIT, ALTAR_READY_PIXELS);
    showColor(safetyColors[0] * factor, safetyColors[1] * factor, safetyColors[2] * factor, ALTAR_SAFETY_PIXEL_BIT, ALTAR_SAFETY_PIXELS);
    showColor(safetyColors[0] * factor, safetyColors[1] * factor, safetyColors[2] * factor, TABLE_SAFETY_PIXEL_BIT, TABLE_SAFETY_PIXELS);
  }
}

void fadeStrandIn(float colors[], int strand, int num_pixels) {  
  for (float b = 0; b < 255; b += FADE_SPEED) {
    float factor = b / 255;
    showColor(colors[0] * factor, colors[1] * factor, colors[2] * factor, strand, num_pixels);
  }
}

void fadeStrandOut(float colors[], int strand, int num_pixels) {  
  for (float b = 255; b > 0; b -= FADE_SPEED) {
    float factor = b / 255;
    showColor(colors[0] * factor, colors[1] * factor, colors[2] * factor, strand, num_pixels);
  }
}

// Display a single color on the whole string
void showColor(unsigned char r, unsigned char g, unsigned char b, int strand, int num_pixels) {
  cli();  
  
  for(int p = 0; p < num_pixels; p++) {
    sendPixel(r, g, b, strand);
  }
  
  sei();
  show();
}

// Fire patterns. Effects 0-6 are from right-to-left while facing the altar
void fireOutsideIn(){
  outerFlames();
  middleFlames();
  innerFlames();
  catFlames();
}

void candleFlames() {
  Tlc.set(0, TLC_ON);
  Tlc.set(1, TLC_ON);
  Tlc.set(2, TLC_ON);
  Tlc.set(4, TLC_ON);
  Tlc.set(5, TLC_ON);
  Tlc.set(6, TLC_ON);
  Tlc.update();
  delay(1000);
  
  flameOff();
  delay(500);
}

void outerFlames(){
  Tlc.set(0, TLC_ON); 
  Tlc.set(6, TLC_ON); 
  Tlc.update();
  delay(500);
  
  flameOff();
  delay(500);
}

void middleFlames(){
  Tlc.set(1, TLC_ON); 
  Tlc.set(5, TLC_ON); 
  Tlc.update();
  delay(500);
  
  flameOff();
  delay(500);
}

void innerFlames(){
  Tlc.set(2, TLC_ON); 
  Tlc.set(4, TLC_ON); 
  Tlc.update();
  delay(500);
  
  flameOff();
  delay(500);
}

void catFlames(){
  Tlc.set(3, TLC_ON);
  Tlc.update();
  delay(1500);
  
  flameOff();
  delay(500);
}

void catScoff(){
  Tlc.set(3, TLC_ON);
  Tlc.update();
  delay(500);
  
  flameOff();
  delay(500);
}

void flameOff(){
  Tlc.clear();
  Tlc.update();
}


// **************************************************************************
// LED API
// **************************************************************************

// More info on how to find these at http://www.arduino.cc/en/Reference/PortManipulation
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

//TODO: There has to be a way to get rid of these guys.
inline void sendAltarSafetyBit(bool bitVal) {
  if (bitVal) {
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
      [bit]   "I" (ALTAR_SAFETY_PIXEL_BIT),
      [onCycles]  "I" (NS_TO_CYCLES(T1H) - 2),    // 1-bit width less overhead  for the actual bit setting, note that this delay could be longer and everything would still work
      [offCycles]   "I" (NS_TO_CYCLES(T1L) - 2)   // Minimum interbit delay. Note that we probably don't need this at all since the loop overhead will be enough, but here for correctness
    );                            
  } else {          
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
      [bit]   "I" (ALTAR_SAFETY_PIXEL_BIT),
      [onCycles]  "I" (NS_TO_CYCLES(T0H) - 2),
      [offCycles] "I" (NS_TO_CYCLES(T0L) - 2)
    );   
  }
}  

inline void sendTableSafetyBit(bool bitVal) {
  if (bitVal) {
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
      [bit]   "I" (TABLE_SAFETY_PIXEL_BIT),
      [onCycles]  "I" (NS_TO_CYCLES(T1H) - 2),    // 1-bit width less overhead  for the actual bit setting, note that this delay could be longer and everything would still work
      [offCycles]   "I" (NS_TO_CYCLES(T1L) - 2)   // Minimum interbit delay. Note that we probably don't need this at all since the loop overhead will be enough, but here for correctness
    );                            
  } else {          
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
      [bit]   "I" (TABLE_SAFETY_PIXEL_BIT),
      [onCycles]  "I" (NS_TO_CYCLES(T0H) - 2),
      [offCycles] "I" (NS_TO_CYCLES(T0L) - 2)
    );   
  }
}  

inline void sendByte( unsigned char byte, int strand ) {
    //TODO: There has to be a better way than having a different function for each pixel_bit
    //TODO: Get a weird "impossible constraint in 'asm'" error 
    for( unsigned char bit = 0 ; bit < 8 ; bit++ ) {
      if (strand == ALTAR_READY_PIXEL_BIT){
        // If this works, why doesn't parameterizing it?
        sendAltarReadyBit(bitRead(byte, 7));
      } else if (strand == ALTAR_SAFETY_PIXEL_BIT){
        sendAltarSafetyBit(bitRead(byte, 7));
      } else if (strand == TABLE_SAFETY_PIXEL_BIT){ 
        sendTableSafetyBit(bitRead(byte, 7));
      } 

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
  bitSet(PIXEL_DDR, ALTAR_SAFETY_PIXEL_BIT);
  bitSet(PIXEL_DDR, TABLE_SAFETY_PIXEL_BIT);
}

inline void sendPixel(unsigned char r, unsigned char g, unsigned char b, int strand)  {  
  // Neopixel wants colors in green then red then blue order
  sendByte(g, strand);          
  sendByte(r, strand);
  sendByte(b, strand); 
}

void show() {
  // Just wait long enough without sending any bits to cause the pixels to latch and display the last sent frame
  // Round up since the delay must be _at_least_ this long (too short might not work, too long not a problem)
  _delay_us( (RES / 1000UL) + 1);       
}

