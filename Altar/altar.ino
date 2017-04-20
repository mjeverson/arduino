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

// Pins the scale is using
//TODO: Does the scale still work with the LED code hooked up? Shouldn't we need to use the CLK pin? 
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
  setSafetyLighting();

  Tlc.init();
  Tlc.clear();
  Tlc.update();

  scale.set_scale(calibration_factor); 
  scale.tare();  
}

void loop() {
  Serial.print(scale.get_units());
  Serial.println();
  
  // Button reads inverted because we're using the internal pullup resistor
  if (!digitalRead(GO_BUTTON)){
    // Go time! Start listening to the scale
    
    // Do some nice LED prep stuff to show the altar is reading/waiting
    beginAltarCountdown();
    
    //TODO: Wait for a few seconds, then take a scale reading
    evaluateOffering();
  } else if (!digitalRead(SPELL_BUTTON)) {
    // Invoking the great old ones
    castSpell();
  } else {
    // Reset the altar to default lighting
    resetAltarLighting();
  }
}

void beginAltarCountdown(){
  //TODO: Sets the altar lights in a pattern that indicates it's listening, 5s?
  for (int i=0; i < ALTAR_READY_PIXELS; i++) {
    cli();
    
    //TODO: Altar's "evaluating" light patterns
    // For every pixel, update the entire strand 
    int p = 0;
    
    while (p++ <= i) {
        sendPixel(0, 0, 127, ALTAR_READY_PIXEL_BIT);
    } 
     
    while (p++ <= ALTAR_READY_PIXELS) {
        sendPixel(127, 127, 127, ALTAR_READY_PIXEL_BIT);  
      
    }

    sei();
    show();    
    delay(10);
  }
}

void castSpell() {
  //TODO: Custom spell fire and light pattern
}

void evaluateOffering() {
  //TODO: Make fire and lights happen based on the scale reading!
  
  if (scale.get_units() > 100) {
    Tlc.set(0, TLC_ON); // Flame on!
  } else {
    Tlc.set(0, TLC_OFF); // Flame off!
  }
  
  Tlc.update();
}

void setSafetyLighting(){
  cli();

  //TODO: Try messing around with the timing here so we can set this in a single loop
  for (int i=0; i < ALTAR_SAFETY_PIXELS; i++) {
    sendPixel(127, 0, 0, ALTAR_SAFETY_PIXEL_BIT);
  }

  for (int i=0; i < TABLE_SAFETY_PIXELS; i++) {
    sendPixel(127, 0, 0, TABLE_SAFETY_PIXEL_BIT);
  }

  sei();
  show();
}

void resetAltarLighting(){
  cli();
  
  for (int i=0; i < ALTAR_READY_PIXELS; i++) {
    //TODO: do something nice like make the lights fade in here
    sendPixel(127, 127, 127, ALTAR_READY_PIXEL_BIT);
  }
  
  sei();
  show();
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
    //TODO: Get a weird impossible constraint in 'asm' error 
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

