
/*
 * LilyPad tutorial: sensing (sensors)
 *
 * Reads data from a LilyPad light sensor module
 * and then sends that data to the computer
 * so that you can see the sensor values
 */
 
int ledPin = 6;	// LED is connected to digital pin 13
int sensorPin = A6;	// light sensor is connected to analog pin 0
int sensorValue;	// variable to store the value coming from the sensor

int redPin = 9;	// R petal on RGB LED module connected to digital pin 11
int greenPin = 11;	// G petal on RGB LED module connected to digital pin 9
int bluePin = 10;	// B petal on RGB LED module connected to digital pin 10
  
void setup()	 
{	 
         pinMode(ledPin, OUTPUT);	// sets the ledPin to be an output
         Serial.begin(9600);	//initialize the serial port
}	 
 	 
void loop()	// run over and over again
{	 
          sensorValue = analogRead(sensorPin);	// read the value from the sensor
          Serial.println(sensorValue);	// send that value to the computer
          
          color(sensorValue * 10, 0,  255 - (sensorValue * 10));
          
          delay(500);	// delay for 1/10 of a second
}

void color (unsigned char red, unsigned char green, unsigned char blue)     // the color generating function
{	 
          analogWrite(redPin, 255-red);	 
          analogWrite(bluePin, 255-blue);
   //       analogWrite(greenPin, 255-green);
}
