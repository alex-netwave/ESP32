#include "Keypad.h"
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns

char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

// For Arduino Microcontroller
//byte rowPins[ROWS] = {9, 8, 7, 6}; 
//byte colPins[COLS] = {5, 4, 3}; 

// For ESP8266 Microcontroller
//byte rowPins[ROWS] = {D1, D2, D3, D4}; 
//byte colPins[COLS] = {D5, D6, D7, D8}; 

// For ESP32 Microcontroller
byte rowPins[ROWS] = {23, 22, 3, 21}; 
byte colPins[COLS] = {19, 18, 5};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(9600);
}

void loop() {
  char key = keypad.getKey();

  if (key){
    Serial.println(key);
  }
}
