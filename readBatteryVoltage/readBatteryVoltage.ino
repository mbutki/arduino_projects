void setup() {
  // put your setup code here, to run once:

}

void loop() {
  float value = (analogRead(35)/4095.0)*2*3.3*1.1;
  Serial.begin(115200);
  Serial.println("Battery");
  Serial.println(value);
  delay(1000);
}
