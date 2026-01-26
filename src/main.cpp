#include <Arduino.h>

// UPS Monitor for ESP32-C3 Super Mini
// VIA Energy Mini UPS - GPIO Test
// Version: 1.0

// INPUT pins (reading UPS signals)
#define LED_25   0   // Battery >= 25%
#define LED_50   1   // Battery >= 50%
#define LED_75   2   // Battery >= 75%
#define LED_100  3   // Battery = 100%
#define PWR_MODE 4   // Power mode: 1=AC, 0=Battery
#define EXTRA    5   // Extra pin for testing

// OUTPUT pin (3.3V source for testing)
#define VCC_TEST 8   // 3.3V output

const int inputPins[] = {LED_25, LED_50, LED_75, LED_100, PWR_MODE, EXTRA};
const int pinCount = 6;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Configure input pins
  for (int i = 0; i < pinCount; i++) {
    pinMode(inputPins[i], INPUT);
  }
  
  // Configure GPIO8 as OUTPUT with HIGH (3.3V source)
  pinMode(VCC_TEST, OUTPUT);
  digitalWrite(VCC_TEST, HIGH);
  
  Serial.println("UPS Monitor - GPIO Test Started");
  Serial.println("GPIO8 = 3.3V OUTPUT for testing");
  Serial.println();
}

void loop() {
  Serial.println("--- GPIO States ---");
  
  for (int i = 0; i < pinCount; i++) {
    Serial.print("GPIO");
    Serial.print(inputPins[i]);
    Serial.print(": ");
    Serial.println(digitalRead(inputPins[i]) ? "HIGH" : "LOW");
  }
  
  Serial.println();
  delay(1000);
}