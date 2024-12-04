/* Display  | ESP32-WROOM 
 * GND      | GND
 * 5V       | VIN
 * TX       | RX2 (IO16)
 * RX       | TX2 (IO17)
 */
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
}

void loop() {
  // Read and send to Serial2 anything that Serial received
  if (Serial.available()) {
    delay(5);
    while(Serial.available()){
      Serial2.write(Serial.read());
    }
  }

  // Read and show to Serial anything that Serial2 received
  if (Serial2.available()) {
    Serial.write(Serial2.read());
  }
}
