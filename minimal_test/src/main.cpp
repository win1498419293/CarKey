#include <Arduino.h>

void setup() {
    Serial.begin(115200); delay(100);
    Serial.println("=== GPIO FINAL TEST ===");

    // S3 replacement pins - INPUT_PULLUP
    Serial.print("GPIO41 (handbrake) INPUT_PULLUP... "); pinMode(41, INPUT_PULLUP); Serial.println("OK");
    Serial.print("GPIO42 (neutral)   INPUT_PULLUP... "); pinMode(42, INPUT_PULLUP); Serial.println("OK");

    // S3 replacement pins - OUTPUT
    Serial.print("GPIO6  (IGN)     OUTPUT... "); pinMode(6, OUTPUT); digitalWrite(6, LOW); Serial.println("OK");
    Serial.print("GPIO7  (HORN)    OUTPUT... "); pinMode(7, OUTPUT); digitalWrite(7, LOW); Serial.println("OK");
    Serial.print("GPIO40 (START)   OUTPUT... "); pinMode(40, OUTPUT); digitalWrite(40, LOW); Serial.println("OK");
    Serial.print("GPIO5  (RELAY)   OUTPUT... "); pinMode(5, OUTPUT); digitalWrite(5, LOW); Serial.println("OK");

    Serial.println("=== ALL GPIO PASSED ===");
}

void loop() { Serial.println("alive"); delay(3000); }
