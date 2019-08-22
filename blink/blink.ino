int low = 16;
int high = 17;
void setup() {
  delay(1000);
  Serial.begin(115200);
  for (int i = low; i < high+1; i++) {
    pinMode(1, OUTPUT);
  }
}

void loop() {
  Serial.println("Loop start");
  for (int i = low; i < high+1; i++) {
    analogWrite(1, HIGH);
  }
  delay(1000);
  for (int i = low; i < high+1; i++) {
    digitalWrite(1, LOW);
  }
  delay(1000);
}
