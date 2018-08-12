/******************************************************************************
TestRun.ino
TB6612FNG H-Bridge Motor Driver Example code
Michelle @ SparkFun Electronics
8/20/16
https://github.com/sparkfun/SparkFun_TB6612FNG_Arduino_Library
Uses 2 motors to show examples of the functions in the library.  This causes
a robot to do a little 'jig'.  Each movement has an equal and opposite movement
so assuming your motors are balanced the bot should end up at the same place it
started.
Resources:
TB6612 SparkFun Library
Development environment specifics:
Developed on Arduino 1.6.4
Developed with ROB-9457
******************************************************************************/

// This is the library for the TB6612 that contains the class Motor and all the
// functions
#include <SparkFun_TB6612.h>

// Pins for all inputs, keep in mind the PWM defines must be on PWM pins
// the default pins listed are the ones used on the Redbot (ROB-12097) with
// the exception of STBY which the Redbot controls with a physical switch
#define AIN1 2
#define BIN1 7 //todo: why does setting these to 23-25 cause problems?
#define CIN1 12 //todo: why does setting these to 23-25 cause problems?
#define AIN2 3
#define BIN2 8
#define CIN2 13
#define PWMA 4
#define PWMB 5
#define PWMC 6
#define STBYA 9
#define STBYB 10
#define STBYC 11

// these constants are used to allow you to make your motor configuration 
// line up with function names like forward.  Value can be 1 or -1
const int offsetA = 1;
const int offsetB = 1;

// Initializing motors.  The library will allow you to initialize as many
// motors as you have memory for.  If you are using functions like forward
// that take 2 motors as arguements you can either write new functions or
// call the function more than once.
Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBYA);
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB, STBYB);

void setup()
{
 //Nothing here
 Serial.begin(9600);
}


void loop()
{
   //Use of the drive function which takes as arguements the speed
   //and optional duration.  A negative speed will cause it to go
   //backwards.  Speed can be from -255 to 255.  Also use of the 
   //brake function which takes no arguements.
   
   
   //Use of the drive function which takes as arguements the speed
   //and optional duration.  A negative speed will cause it to go
   //backwards.  Speed can be from -255 to 255.  Also use of the 
   //brake function which takes no arguements.
   Serial.println("Doing motor2 drive pos");
   motor2.drive(255,1000);
   Serial.println("Doing motor2 drive neg");
   motor2.drive(-255,1000);
   Serial.println("Doing motor2 break");
   motor2.brake();
   delay(1000);

Serial.println("Doing motor1 drive pos");
   motor1.drive(255,1000);
   Serial.println("Doing motor1 drive neg");
   motor1.drive(-255,1000);
   Serial.println("Doing motor1 break");
   motor1.brake();
   delay(1000);
   
//   //Use of the forward function, which takes as arguements two motors
//   //and optionally a speed.  If a negative number is used for speed
//   //it will go backwards
//   Serial.println("Doing motor1-2 forward pos");
//   forward(motor1, motor2, 150);
//   delay(1000);
//   
//   //Use of the back function, which takes as arguments two motors 
//   //and optionally a speed.  Either a positive number or a negative
//   //number for speed will cause it to go backwards
//   Serial.println("Doing motor1-2 back neg");
//   back(motor1, motor2, -150);
//   delay(1000);
//   
//   //Use of the brake function which takes as arguments two motors.
//   //Note that functions do not stop motors on their own.
//   Serial.println("Doing motor1-2 brake");
//   brake(motor1, motor2);
//   delay(1000);
//   
//   //Use of the left and right functions which take as arguements two
//   //motors and a speed.  This function turns both motors to move in 
//   //the appropriate direction.  For turning a single motor use drive.
//   Serial.println("Doing motor1-2 left");
//   left(motor1, motor2, 100);
//   delay(1000);
//   Serial.println("Doing motor1-2 right");
//   right(motor1, motor2, 100);
//   delay(1000);
//   
//   //Use of brake again.
//   Serial.println("Doing motor1-2 brake");
//   brake(motor1, motor2);
//   delay(1000);
   
}

