const int irPin= 33; //int, char, byte works return 1 for no detect, 0 for detect
//bool doesnt
int IR;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(irPin, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  IR=digitalRead(irPin);
  Serial.println(IR);
  if(IR == 0){
    Serial.println("I see it!");
  }
  else{
    
  }
  delay(500);
}
