const int RX_PIN1 = A0;
const int RX_PIN2 = A1;
int THRESHOLD = 300;

void setup() {
  pinMode(RX_PIN1, INPUT_PULLUP);
  pinMode(RX_PIN2, INPUT_PULLUP);
  Serial.begin(115200);
  Serial.println("OPB100 phototransistor test - Dual Sensor");
}

void loop() {
  int v1 = analogRead(RX_PIN1);
  int v2 = analogRead(RX_PIN2);
  bool blocked1 = (v1 > THRESHOLD);
  bool blocked2 = (v2 > THRESHOLD);
  
  Serial.print("A0="); Serial.print(v1);
  Serial.print(" "); Serial.print(blocked1 ? "BLOCKED" : "CLEAR");
  Serial.print("  A1="); Serial.print(v2);
  Serial.print(" "); Serial.println(blocked2 ? "BLOCKED" : "CLEAR");
  delay(100);
}