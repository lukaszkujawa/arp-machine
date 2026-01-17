#include "midi.h"
#include "arp.h"

Midi midi;
Arp arp(midi);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);  // Debug LED
  midi.setup();
}

void loop() {
  arp.tick();
}
