const int inputPin = D5;
int lastState = HIGH;

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(inputPin, INPUT_PULLUP);
  lastState = digitalRead(inputPin);
}

void loop() {
  int currentState = digitalRead(inputPin);
  if (currentState != lastState) {
    Serial.println(currentState == HIGH ? "HIGH" : "LOW");
    lastState = currentState;
  }
}
