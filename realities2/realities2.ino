/*
Adafruit Arduino - Lesson 3. RGB LED
*/

int bluePin = 9;
int redPin = 10;
int greenPin = 11;

 
void setup()
{
  pinMode(redPin, OUTPUT);
//  pinMode(greenPin, OUTPUT);
//  pinMode(bluePin, OUTPUT);
}
 
void loop()
{
  int r, rout;
  
  for (r = 0; r < 256; r++){
    setColor(r, 0, 0);
    delay(10);
  }

  for (rout = 255; rout >= 0; rout--){
    setColor(rout, 0, 0);
    delay(10);
  }

  delay(10);
}
 
void setColor(int red, int green, int blue)
{
  analogWrite(redPin, red);
//  analogWrite(greenPin, green);
//  analogWrite(bluePin, blue);
}
