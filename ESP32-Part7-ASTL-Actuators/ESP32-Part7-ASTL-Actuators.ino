//Currently this displays current AUC temperature and light from MCP9700 and photocell, respectively, and hardcoded AUC temperature and light
//The above is displayed on the OLED display
//Updating in progress -
//Issue at hand:
//1) A loadprohibited error is received when trying to make calls to the oled display address within the ISR
//
//Success integration of ASTL sensors and printing output to OLED display

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

//Temperature Sensor Variables
const int AUC_tempPin = 2;     //analog input pin constant
const int ASTL_tempPin = 35;     //analog input pin constant
int tempVal;    // temperature sensor raw readings
float volts;    // variable for storing voltage 
float AUC_currentTemp;     // the current(AUC) temperature variable
float ASTL_currentTemp;     // the current(ASTL) temperature variable
float targetTemp; // the target(AUC) temperature

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
const int modeSelectPin = 34;
bool pressed;
bool isEnoughTime;
const int eTime = 3000; //Elapsed time constant - interrupt service routine contents only occur if sufficient time occurs since last press
int nTime; //Next time (time at current press)

//Serial Connections
HardwareSerial &OLEDSerial = Serial2; //OLED

void IRAM_ATTR modeSwitch()
{
  if (millis()-nTime > eTime) //condition only allows body to execute every 3 sec at most(non-inclusively)
  {
    Serial.println(millis()-eTime); 
    nTime = millis(); //updates with a new time elapse each time ISR is invoked
    Serial.println("Hi"); 
    modeChange();

    Serial.println("Hi3"); 
  }


  //delay(10000);
//  char key = keypad.getKey();
//  while(key != '*')
//  {
//    key = keypad.getKey();
//    if (key){
//      Serial.println(key);
//    }
//  }
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
  pinMode(modeSelectPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(modeSelectPin),modeSwitch,FALLING); //changed from rising -> falling and connected pwr -> gnd
  modeInitialDisplay();
  Serial.println("Hitoo");   
}

void loop() {
  AUC_currentTemp = (getAUC_currentTemp(AUC_tempPin) + getAUC_currentTemp(AUC_tempPin) + getAUC_currentTemp(AUC_tempPin))/3; //Averaging to reduce error due to jitter
  ASTL_currentTemp = (getASTL_currentTemp(ASTL_tempPin) + getASTL_currentTemp(ASTL_tempPin) + getASTL_currentTemp(ASTL_tempPin))/3; //Averaging to reduce error due to jitter
  AUC_currentLight = (analogRead(AUC_lightPin)+analogRead(AUC_lightPin)+analogRead(AUC_lightPin))/3; //Averaging to reduce error due to jitter
  ASTL_currentLight = (analogRead(ASTL_lightPin)+analogRead(ASTL_lightPin)+analogRead(ASTL_lightPin))/3; //Averaging to reduce error due to jitter
  //If automatic mode do the following
  targetTemp = getTargetTemp();
  targetLight = getTargetLight();
  //If manual mode do the following
  //write function to get user input from keypad and display what is entered to OLED
  //set * = clear = start over text input completely
  //set # = enter = enter text input for each stage

  
  if(optMode == 1) //Manual Mode Display
  {
    //if changed from previous, then do initial display
    if(changed==true){
      changed=false;
      modeInitialDisplay();      
    }
    //setDisplay(AUC_currentTemp, targetTemp,ASTL_currentTemp, AUC_currentLight, targetLight, ASTL_currentLight);
    keyPadInput();
  }
  else  //Automatic Mode Display
  {
    if(changed==true){
      changed=false;
      modeInitialDisplay();
    }
    setDisplay(AUC_currentTemp, targetTemp,ASTL_currentTemp, AUC_currentLight, targetLight, ASTL_currentLight);
  }
  delay(2000);  
  if(AUC_currentTemp > targetTemp + 2)
  {
    //turn on fan and rotate blinds to reduce temp
  }
  else //<= AUC_currentTemp <= targetTemp
  {
    //
  }
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
      return 2000.00; //hardcoded light value
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
  int x = 0, y = 0;
  display.setCursor(y,x);
  display.println("Enter target temp:");
  display.display();
  char key = keypad.getKey();
  y+=10;
  while(key != '3')
  {
    key = keypad.getKey();
    x+=10;
    if(key == '3'){
      display.setTextSize(1);
      display.setTextColor(WHITE);
      int x = 0, y = 0;
      display.setCursor(y,x);
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
      display.setCursor(x,y);
      display.println(key);
      display.display();
    }
  }
}
