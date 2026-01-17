#include <ESP8266WiFi.h>
#include <Arduino.h>

#include "midi.h"
#include "arp.h"
#include "lcd.h"

Midi midi;
Arp arp(midi);
Lcd lcd(arp);

void setup() {
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(10);

  pinMode(LED_BUILTIN, OUTPUT);  // Debug LED
  midi.setup();
  lcd.setup();
}

void loop() {
  unsigned long now = micros();

  arp.tick(now);
  lcd.refresh(now);
}
