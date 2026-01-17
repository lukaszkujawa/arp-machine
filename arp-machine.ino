#include <ESP8266WiFi.h>
#include <Arduino.h>

#include "midi.h"
#include "arp.h"
#include "lcd.h"

Midi midi;
Arp arp(midi);
Lcd lcd(arp);

const uint32_t DEBOUNCE_MS = 200;
const uint8_t BTN_REGEN = 14;     // Regenerate sequence
const uint8_t BTN_ROOT = 12;      // Rotate root note
const uint8_t BTN_SCALE = 13;     // Rotate scale

volatile uint32_t lastIsrMs[3] = {0, 0, 0};
volatile uint8_t pendingMask = 0;

void IRAM_ATTR isrRegen() {
  uint32_t now = millis();
  if (now - lastIsrMs[0] < DEBOUNCE_MS) return;
  lastIsrMs[0] = now;
  pendingMask |= (1 << 0);
}

void IRAM_ATTR isrRoot() {
  uint32_t now = millis();
  if (now - lastIsrMs[1] < DEBOUNCE_MS) return;
  lastIsrMs[1] = now;
  pendingMask |= (1 << 1);
}

void IRAM_ATTR isrScale() {
  uint32_t now = millis();
  if (now - lastIsrMs[2] < DEBOUNCE_MS) return;
  lastIsrMs[2] = now;
  pendingMask |= (1 << 2);
}

void setup() {
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(10);

  pinMode(LED_BUILTIN, OUTPUT);  // Debug LED
  midi.setup();
  lcd.setup();

  pinMode(BTN_REGEN, INPUT_PULLUP);
  pinMode(BTN_ROOT, INPUT_PULLUP);
  pinMode(BTN_SCALE, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BTN_REGEN), isrRegen, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_ROOT), isrRoot, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_SCALE), isrScale, FALLING);
}

void loop() {
  unsigned long now = micros();

  // Check for button presses
  if (pendingMask & (1 << 0)) {
    pendingMask &= ~(1 << 0);
    arp.regenerate();
  }
  if (pendingMask & (1 << 1)) {
    pendingMask &= ~(1 << 1);
    arp.rotateRootNote();
  }
  if (pendingMask & (1 << 2)) {
    pendingMask &= ~(1 << 2);
    arp.rotateScale();
  }

  arp.tick(now);
  lcd.refresh(now);
}
