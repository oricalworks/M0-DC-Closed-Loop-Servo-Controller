

/*
   
   https://github.com/JChristensen/extEEPROM - EEPROM LIBRARY
   
   read a rotary encoder with interrupts
   Encoder hooked up with common to GROUND,
   encoder0PinA to pin 2, encoder0PinB to pin 4 (or pin 3 see below)
   it doesn't matter which encoder pin you use for A or B 
   
   is possible to change PID costants by sending on SerialUSB interfaces the values separated by ',' in this order: KP,KD,KI
   example: 5.2,3.1,0 so we have  KP=5.2 KD=3.1 KI=0 is only for testing purposes, but i will leave this function with eeprom storage

*/ 
/*   -----------------To Do-----------------
 *    Implement code for physical EEPROM for V2 Board (Chip:24LC256 ADDRESS:0x50) (32Kb Storage) (I2c on SDA and SCL)
 *    PID Auto-Tune
 *    Error Reporting
 *    Force sensing
 *    Host Communincation
 *    Performance Reporting and Info for Osiliscope
 *    Switchless Homing(force sensing)
 *    Master and Slave Mirroring
 *    Configurable Limits
 *    Create Host Software
 *    
*/

// Set the pins used



// blink out an error code



//Hardware setup
#define encoder0PinA  2
#define encoder0PinB  3

#define SpeedPin     9
#define DirectionPin 8

//Step & Dir Interface 
#define STEP_PIN              3
#define DIR_PIN               12
#define ENABLE_PIN            13
// HS END

volatile long encoder0Pos = 0;

long target = 0;
long target1 = 0;
int amp=212;
//correction = Kp * error + Kd * (error - prevError) + kI * (sum of errors)
//PID controller constants
float KP = 6.0 ; //position multiplier (gain) 2.25
float KI = 0.1; // Intergral multiplier (gain) .25
float KD = 1.3; // derivative multiplier (gain) 1.0

int lastError = 0;
int sumError = 0;

//Integral term min/max (random value and not yet tested/verified)
int iMax = 100;
int iMin = 0;

long previousTarget = 0;
long previousMillis = 0;        // will store last time LED was updated
long interval = 5;           // interval at which to blink (milliseconds)

//for motor control ramps 1.4
bool newStep = false;
bool oldStep = false;
bool dir = false;

void setup() { 
  


  pinMode(encoder0PinA, INPUT_PULLUP); 
  pinMode(encoder0PinB, INPUT_PULLUP);  
  
  pinMode(DirectionPin, OUTPUT); 
  pinMode(SpeedPin, OUTPUT);
  
  //ramps 1.4 motor control
    pinMode(STEP_PIN, INPUT);
    pinMode(DIR_PIN, INPUT);

  attachInterrupt(2, doEncoderMotor0, CHANGE);  // encoder pin on interrupt 0 - pin 2
  attachInterrupt(3, countStep, RISING);  //on pin 3
  SerialUSB.begin(15200);
  SerialUSB.println("start");               

} 

void loop(){
  
  while (SerialUSB.available() > 0) {
    KP = SerialUSB.read();
    KD = SerialUSB.read();
    KI = SerialUSB.read();
    
    
    SerialUSB.println(KP);
    SerialUSB.println(KD);
    SerialUSB.println(KI);
}
  
  if(millis() - previousTarget > 500){ //enable this code only for test purposes
  SerialUSB.print(encoder0Pos);
  SerialUSB.print(',');
  SerialUSB.println(target1);
  previousTarget=millis();
  }
        
  target = target1;
  docalc();
}

void docalc() {
  
  if (millis() - previousMillis > interval) 
  {
    previousMillis = millis();   // remember the last time we blinked the LED
    
    long error = encoder0Pos - target ; // find the error term of current position - target    
    
    //generalized PID formula
    //correction = Kp * error + Kd * (error - prevError) + kI * (sum of errors)
    long ms = KP * error + KD * (error - lastError) +KI * (sumError);
       
    lastError = error;    
    sumError += error;
    
    //scale the sum for the integral term
    if(sumError > iMax) {
      sumError = iMax;
    } else if(sumError < iMin){
      sumError = iMin;
    }
    
    if(ms > 0){
      digitalWrite ( DirectionPin ,HIGH );      
    }
    if(ms < 0){
      digitalWrite ( DirectionPin , LOW );     
      ms = -1 * ms;
    }

    int motorspeed = map(ms,0,amp,0,255);
   if( motorspeed >= 255) motorspeed=255;
    //analogWrite ( SpeedPin, (255 - motorSpeed) );
    analogWrite ( SpeedPin,  motorspeed );
    //SerialUSB.print ( ms );
    //SerialUSB.print ( ',' );
    //SerialUSB.println ( motorspeed );
  }  
}

void doEncoderMotor0(){
  if (digitalRead(encoder0PinA) == HIGH) {   // found a low-to-high on channel A
    if (digitalRead(encoder0PinB) == LOW) {  // check channel B to see which way
                                             // encoder is turning
      encoder0Pos = encoder0Pos - 1;         // CCW
    } 
    else {
      encoder0Pos = encoder0Pos + 1;         // CW
    }
  }
  else                                        // found a high-to-low on channel A
  { 
    if (digitalRead(encoder0PinB) == LOW) {   // check channel B to see which way
                                              // encoder is turning  
      encoder0Pos = encoder0Pos + 1;          // CW
    } 
    else {
      encoder0Pos = encoder0Pos - 1;          // CCW
    }

  }
 
}

void countStep(){
  dir = digitalRead(DIR_PIN);
            if (dir) target1++;
            else target1--;
}

