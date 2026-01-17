#include "midi.h"

// MIDI baud rate is fixed
static const uint32_t MIDI_BAUD = 31250;

Midi::Midi() {
  _channel = 1;
}

void Midi::setup() {
  Serial.begin(MIDI_BAUD, SERIAL_8N1);
  delay(10);
}

void Midi::sendByte(uint8_t b) {
  Serial.write(b);
}

void Midi::noteOn(uint8_t note, uint8_t velocity) {
  sendByte(0x90 | _channel);
  sendByte(note & 0x7F);
  sendByte(velocity & 0x7F);
}

void Midi::noteOff(uint8_t note)  {
  sendByte(0x80 | _channel);
  sendByte(note & 0x7F);
  sendByte(0);
}

void Midi::allNotesOff() {
  sendByte(0xB0 | _channel);
  sendByte(123); // CC 123 = all notes off
  sendByte(0);
}
