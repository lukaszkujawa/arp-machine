#include "midi.h"
#include "arp.h"

Midi midi;
Arp arp(midi);

void setup() {
  midi.setup();
}

void loop() {
  arp.tick(120);
}
