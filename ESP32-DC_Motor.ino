const int M1 = 16;
const int M2 = 4;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(M1,HIGH);
  digitalWrite(M2,LOW);
  delay(1000);
  digitalWrite(M1,LOW);
  digitalWrite(M2,LOW);
  delay(200);
}
