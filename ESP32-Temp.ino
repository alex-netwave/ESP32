//Functional Code

const int tempPin = 2;     //analog input pin constant
int tempVal;    // temperature sensor raw readings
float volts;    // variable for storing voltage 
float temp;     // actual temperature variable

void setup()
{
  // start the serial port at 9600 baud
  Serial.begin(9600);
}
void loop()
{
  //read the temp sensor and store it in tempVal
  tempVal = analogRead(tempPin);
  volts = tempVal/1024.0;             // normalize by the maximum temperature raw reading range, assumes 10-bit ADC(ESP32 is a 12-bit ADC)
  temp = volts * 100;                 //MCP9700 outputs temp as fahrenheit(somewhat) as opposed to celsius
  Serial.print(" Temperature is:   "); // print out the following string to the serial monitor
  Serial.print(temp);                  // in the same line print the temperature
  Serial.println (" degrees F");       // still in the same line print degrees F, then go to next line.
  delay(2000);                         // wait for 2 second or 2000 milliseconds before taking the next reading. 
}
