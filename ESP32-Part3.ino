//Currently this displays current AUC temperature and light from MCP9700 and photocell, respectively, and hardcoded AUC temperature and light
//The above is displayed on the OLED display
//Updating in progress -
//Issue at hand: Clicking pushbutton connected to interrupt pin has bouncing that isn't account for - rectify this
//Afterward: Implement the key touch pad input when interrupted

//OLED Libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Keypad Libraries
#include "Keypad.h"

//OLED Definitions
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//Photocell/Light Sensor Variables
const int lightPin = 15;
int lightVal;
float currentLight; // the current (AUC) light variable
float targetLight; // the target (AUC) light variable

//Temperature Sensor Variables
const int tempPin = 2;     //analog input pin constant
int tempVal;    // temperature sensor raw readings
float volts;    // variable for storing voltage 
float currentTemp;     // the current(AUC) temperature variable
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
int opt = 0; // choice variable
const int modeSelectPin = 34;
bool pressed;

//Serial Connections
HardwareSerial &OLEDSerial = Serial2; //OLED

void IRAM_ATTR getKeyPadTouches()
{
  Serial.println("Hi");
//  pressed = true;
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
  // put your setup code here, to run once:
  Serial.begin(9600);
  OLEDSerial.begin(115200);
  pinMode(modeSelectPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(modeSelectPin),getKeyPadTouches,RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  currentTemp = getCurrentTemp(tempPin);
  targetTemp = getTargetTemp(); //edit this to get keypad input and use interrupt pin
  //Serial.println(String(currentTemp) + " degrees F"); //testing
  currentLight = analogRead(lightPin);
  targetLight = getTargetLight();
  setDisplay(currentTemp, targetTemp, currentLight, targetLight);
  delay(2000);  
}

//User-defined Functions Below
float getCurrentTemp(const int tPin)
{
  tempVal = analogRead(tPin);
  volts = tempVal/1024.0;             // normalize by the maximum temperature raw reading range, assumes 10-bit ADC(ESP32 is a 12-bit ADC)
  return volts * 100;   //MCP9700 outputs temp as fahrenheit(somewhat) as opposed to celsius
}

float getTargetTemp()
{
    if(opt == 1) //if mode is manual
    {
      
    }
    else if(opt == 0) //if mode is automatic
    {
      return 78.00; //hardcoded temperature value
    }
}

float getTargetLight()
{
    if(opt == 1) //if mode is manual
    {
      
    }
    else if(opt == 0) //if mode is automatic
    {
      return 2000.00; //hardcoded temperature value
    }
}

void setDisplay(float cTemp, float tTemp, float cLight, float tLight)
{
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println("Target temp:" + String(tTemp)+ " F");
  display.display();   
  // Display static text
  display.setCursor(0, 20);
  display.println("Current temp:" + String(cTemp)+ " F");
  display.display();
  // Display static text
  display.setCursor(0, 30);
  display.println("Target light:" + String(tLight));
  display.display();   
  // Display static text
  display.setCursor(0, 40);
  display.println("Current light:" + String(cLight));
  display.display();   
}
