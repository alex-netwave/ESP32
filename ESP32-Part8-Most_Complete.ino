//Lighting and Temperature Control System
//For use with Sparkfun ESP32 Thing DEV-13907
//Success integration of Sensors, Actuators, and Display
//Future: 
//-Assess the quality of temperature sensor code and hardware setup further
//-Revise the units used for light sensor output - make meaning of this

//OLED Libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Keypad Libraries
#include "Keypad.h"

//Servo Libraries
#include <ESP32Servo.h>

//OLED Definitions
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//Servo Object instantiation
Servo myservo;  // create servo object to control a servo

//Servo Variables
int pos = 0;    // variable to store the servo position
int servoPin = 13;

//Photocell/Light Sensor Variables
const int AUC_lightPin = 15;
const int ASTL_lightPin = 32;
int lightVal;
float AUC_currentLight; // the current (AUC) light variable
float ASTL_currentLight; // the current (ASTL) light variable
float targetLight; // the target (AUC) light variable
bool reduceLight; // if true there is a need to reduce light, if false there isn't one

//Temperature Sensor Variables
const int AUC_tempPin = 2;     //analog input pin constant
const int ASTL_tempPin = 35;     //analog input pin constant
int tempVal;    // temperature sensor raw readings
float volts;    // variable for storing voltage 
float AUC_currentTemp;     // the current(AUC) temperature variable
float ASTL_currentTemp;     // the current(ASTL) temperature variable
float targetTemp; // the target(AUC) temperature

//IR Sensor Variables
const int irfPin = 33; //int, char, byte works return 1 for no detect, 0 for detect
const int irrPin = 39;
int IRF, IRR; //IR forward blinds slide and IR reverse blinds slide, respectively


//Keypad Variables
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {23, 22, 3, 21}; 
byte colPins[COLS] = {19, 18, 5};

//Keypad Keymapping
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

//Mode Variables
bool optMode = 0; //default mode = automatic = 0, manual = 1
bool changed = false;
const int modeSelectPin = 12; //changed from 34 to 12 due to Sparkfun ESP32 Thing not having a pullup resistor on pin 34(0,4, 17 DONT WORK FOR THIS)
bool pressed;
bool isEnoughTime;
const int eTime = 3000; //Elapsed time constant - interrupt service routine contents only occur if sufficient time occurs since last press
int nTime; //Next time (time at current press)

//DC Motors Variables
const int M1 = 4;  //Fan control 1
const int M2 = 0;   //Fan control 2
const int M3 = 27;  //Fan control 1
const int M4 = 26;   //Fan control 2

//Serial Connections
HardwareSerial &OLEDSerial = Serial2; //OLED

void IRAM_ATTR modeSwitch()
{
  if (millis()-nTime > eTime) //condition only allows body to execute every 3 sec at most(non-inclusively)
  {
    Serial.println(millis()-eTime); 
    nTime = millis(); //updates with a new time elapse each time ISR is invoked
    modeChange();
  }
}

void setup() {
  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);    // standard 50 hz servo
  //myservo.attach(servoPin, 500, 2500); // attaches the servo on pin 13 to the servo object
  myservo.attach(servoPin);// using default min/max of 1000us and 2000us
  // different servos may require different min/max settings
  // for an accurate 0 to 180 sweep
  
  Serial.begin(9600);
  OLEDSerial.begin(115200);
  pinMode(modeSelectPin, INPUT_PULLDOWN); //changed from pullup -> pulldown
  attachInterrupt(digitalPinToInterrupt(modeSelectPin),modeSwitch,FALLING); //changed from rising -> falling and connected pwr -> gnd
  modeInitialDisplay();
  Serial.println("Hitoo");
  //Fan motor control
  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);  
  //Blinds slide motor control
  pinMode(M3, OUTPUT);
  pinMode(M4, OUTPUT);
  //IR Sensor reading
  pinMode(irfPin, INPUT); 
  pinMode(irrPin, INPUT);
}

void loop() {
  AUC_currentTemp = (getAUC_currentTemp(AUC_tempPin) + getAUC_currentTemp(AUC_tempPin) + getAUC_currentTemp(AUC_tempPin))/3; //Averaging to reduce error due to jitter
  ASTL_currentTemp = (getASTL_currentTemp(ASTL_tempPin) + getASTL_currentTemp(ASTL_tempPin) + getASTL_currentTemp(ASTL_tempPin))/3; //Averaging to reduce error due to jitter
  ASTL_currentTemp+= 4; //Calibration/adjusting for what appears to manufacturing differences: IMPORTANT - needs further investigation prior to implementation in the field
  AUC_currentLight = (analogRead(AUC_lightPin)+analogRead(AUC_lightPin)+analogRead(AUC_lightPin))/3; //Averaging to reduce error due to jitter
  ASTL_currentLight = (analogRead(ASTL_lightPin)+analogRead(ASTL_lightPin)+analogRead(ASTL_lightPin))/3; //Averaging to reduce error due to jitter
  ASTL_currentLight+= 400; //Calibration/adjusting for what appears to manufacturing differences: IMPORTANT - needs further investigation prior to implementation in the field
  //If automatic mode do the following

 Serial.println(millis());
  
  if(optMode == 1) //Manual Mode Display
  {
    //if changed from previous, then do initial display
    if(changed==true){
      changed=false;
      modeInitialDisplay();    
      keyPadInput();        
    }
    setDisplay(AUC_currentTemp, targetTemp,ASTL_currentTemp, AUC_currentLight, targetLight, ASTL_currentLight);

  }
  else  //Automatic Mode Display
  {
    if(changed==true){
      changed=false;
      modeInitialDisplay();  
    }
    targetTemp = getTargetTemp();
    targetLight = getTargetLight();
    setDisplay(AUC_currentTemp, targetTemp,ASTL_currentTemp, AUC_currentLight, targetLight, ASTL_currentLight);
  }
  delay(2000);  
  fanControl();
  rotateBlinds();
}

//Function Definitions Below
float getASTL_currentTemp(const int tPin)
{
  tempVal = analogRead(tPin);
  volts = tempVal/1024.0;             // normalize by the maximum temperature raw reading range, assumes 10-bit ADC(ESP32 is a 12-bit ADC)
  return volts * 100;   //MCP9700 outputs temp as fahrenheit(somewhat) as opposed to celsius
}

float getAUC_currentTemp(const int tPin)
{
  tempVal = analogRead(tPin);
  volts = tempVal/1024.0;             // normalize by the maximum temperature raw reading range, assumes 10-bit ADC(ESP32 is a 12-bit ADC)
  return volts * 100;   //MCP9700 outputs temp as fahrenheit(somewhat) as opposed to celsius
}

float getTargetTemp()
{
    if(optMode == 1) //if mode is manual
    {
      return 75.00; //user entered - REPLACE LATER
    }
    else if(optMode == 0) //if mode is automatic
    {
      return 78.00; //hardcoded temperature value
    }
}

float getTargetLight()
{
    if(optMode == 1) //if mode is manual
    {
      return 1750.00; //user entered - REPLACE LATER
    }
    else if(optMode == 0) //if mode is automatic
    {
      return 1500.00; //hardcoded light value
    }
}

//Temperature and Light Display Screen
void setDisplay(float AUC_cTemp, float tTemp,float ASTL_cTemp, float AUC_cLight, float tLight, float ASTL_cLight)
{
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    //for(;;);
  }
  delay(2000);
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  //AUC status and targets below
  display.setCursor(0,0);
  // Display static text
  display.println("Target temp:" + String(tTemp)+ " F");
  display.display();   
  // Display static text
  display.setCursor(0, 10);
  display.println("AUC temp:" + String(AUC_cTemp)+ " F");
  display.display();
  // Display static text
  display.setCursor(0, 20);
  display.println("Target light:" + String(tLight));
  display.display();   
  // Display static text
  display.setCursor(0, 30);
  display.println("AUC light:" + String(AUC_cLight));
  display.display();
  
  //ASTL status below
  display.setCursor(0, 40);
  display.println("ASTL temp:" + String(ASTL_cTemp)+ " F");
  display.display();
  
  display.setCursor(0, 50);
  display.println("ASTL light:" + String(ASTL_cLight));
  display.display();         
}


void modeChange()
{
  changed=true;
  Serial.println(changed);
  if(optMode == 0)
    optMode = 1; 
  else
    optMode = 0;
}

void modeInitialDisplay()
{
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  if(optMode == 0)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 10);
    // Display static text
    display.println("Automatic mode");
    display.display(); 
    display.setCursor(0, 20);
    display.println("initiated.");
    display.display();
    delay(2000);  
  }
  else
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 10);
    // Display static text
    display.println("Manual mode");
    display.display();
    display.setCursor(0, 20);
    display.println("initiated.");
    display.display();
    delay(2000);     
  }
}

void keyPadInput()
{
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  //Get temp
  display.setCursor(0,0);
  display.println("Enter target temp:");
  display.display();
  char out[] = "00";
  char key = keypad.getKey();
  int i;
  while(out[1] == '0')
  {
    if(out[0] == '0')
      i=0;
    else if(out[1] == '0')
      i=1; 
    if(key == '3'){
      display.setTextSize(1);
      display.setTextColor(WHITE);
      int x = 0, y = 0;
      display.setCursor(0,0);
      display.println("Enter target temp:");
      display.display();
    }
    else if (key == '9') 
    {
      display.clearDisplay(); 
      display.display();     
      break;
    }
    else if (key){
      Serial.println(key);
      out[i]=key;
      if(i==0)
      {
        display.setCursor(0,10);
        display.println(key);
        display.display();        
      }
    }      
    key = keypad.getKey();
  }
  display.setCursor(0,10);
  display.println(String(out));
  display.display();    
  Serial.println(String(out));
  targetTemp = atoi(out);
  //Get light
  display.setCursor(0,20);
  display.println("Enter target light:");
  display.display();
  char out2[] = "0000";
  key = keypad.getKey();
  int j;
  while(out2[3] == '0')
  {
    if(out2[0] == '0')
      j=0;
    else if(out2[1] == '0')
      j=1; 
    else if(out2[2] == '0')
      j=2;
    else if(out2[3] == '0')
      j=3;
    if(key == '3'){
      display.setTextSize(1);
      display.setTextColor(WHITE);
      int x = 0, y = 0;
      display.setCursor(0,20);
      display.println("Enter target light:");
      display.display();
    }
    else if (key == '9') 
    {
      display.clearDisplay(); 
      display.display();     
      break;
    }
    else if (key){
      Serial.println(key);
      out2[j]=key;
      if(j==0)
      {
        display.setCursor(0,30);
        display.println(key);
        display.display();        
      }
      else if(j==1)
      {
        display.setCursor(6,30);
        display.println(key);
        display.display();    
      }
      else if(j==2)
      {
        display.setCursor(12,30);
        display.println(key);
        display.display();    
      }
    }      
    key = keypad.getKey();
  }
  display.setCursor(0,30);
  display.println(String(out2));
  display.display();    
  Serial.println(String(out2));
  targetLight = atoi(out2);
}

//Function that rotates the blinds if there is too much light
void rotateBlinds()
{
  if(AUC_currentLight + 500 < targetLight)
  {
    pos = 0; //Close blinds
    myservo.write(pos); 
    drawBlinds();       
  }

  else if(AUC_currentLight - 500 > targetLight)
  { 
    pos = 90; //Open blinds
    myservo.write(pos);
    reverseDrawBlinds();
  }
  else if(AUC_currentLight + 300 < targetLight || targetLight < AUC_currentLight + 300)
    stopBlinds();
  else{}
}

//Function that turns fan on or off depending on the temperature
void fanControl()
{
  if(AUC_currentTemp > targetTemp + 5) //Initial 'fan on' condition
  {
    //Turn on fan
    digitalWrite(M1,HIGH);
    digitalWrite(M2,LOW);
    delay(1000);

  }
  else if(AUC_currentTemp < targetTemp + 2) //Termination of 'fan on' duration
  {
    //Turn off fan
    digitalWrite(M1, LOW);
    digitalWrite(M2, LOW);    
  }
  else // //Continued 'fan on' duration
  {}
}

//Function that draws the blinds open or closed depending on the lighting
void drawBlinds()
{
  if(irFCheck())
  {
    //Turn on motor - forward
    digitalWrite(M3,HIGH);
    digitalWrite(M4,LOW);
  }
  else
  {
    //Turn off motor
    stopBlinds();    
  }
}

void stopBlinds()
{
  digitalWrite(M3, LOW);
  digitalWrite(M4, LOW);
}

void reverseDrawBlinds()
{
  if(irRCheck())
  {
    //Turn on motor - reverse
    digitalWrite(M3,LOW);
    digitalWrite(M4,HIGH);
  }
  else
  {
    //Turn off motor
    stopBlinds();    
  }
}
//IR Forward Blinds Slide sensor check
int irFCheck()
{
  IRF=digitalRead(irfPin);
  return IRF;
}

//IR Reverse Blinds Slide sensor check
int irRCheck()
{
  IRR=digitalRead(irrPin);
  return IRR;
}
