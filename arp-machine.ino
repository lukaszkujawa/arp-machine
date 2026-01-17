#include <ESP8266WiFi.h>
#include <Arduino.h>

#include "midi.h"
#include "arp.h"
#include "lcd.h"

Midi midi;
Arp arp(midi);
Lcd lcd(arp);

const uint32_t DEBOUNCE_MS = 1000;
const uint8_t BTN1 = 14; 

volatile uint32_t lastIsrMs[1] = {0};
volatile uint8_t pendingMask = 0;

void IRAM_ATTR isrBtn1() {
  uint32_t now = millis();
  if (now - lastIsrMs[0] < DEBOUNCE_MS) return;
  lastIsrMs[0] = now;
  pendingMask |= (1 << 0);
}

void setup() {
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(10);

  pinMode(LED_BUILTIN, OUTPUT);  // Debug LED
  midi.setup();
  lcd.setup();

  pinMode(BTN1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN1), isrBtn1, FALLING);
}

void loop() {
  unsigned long now = micros();

  // Check for button press
  if (pendingMask & (1 << 0)) {
    pendingMask &= ~(1 << 0);
    arp.regenerate();
  }

  arp.tick(now);
  lcd.refresh(now);
}
