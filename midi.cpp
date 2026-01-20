#include "midi.h"

// MIDI baud rate is fixed
static const uint32_t MIDI_BAUD = 31250;

Midi::Midi() {
  _channel = 0;  // MIDI channel 1 (channels are 0-indexed in protocol)
}

void Midi::setup() {
  Serial.begin(MIDI_BAUD);
  delay(100);  // Longer delay for ESP8266 serial init
}

void Midi::sendByte(uint8_t b) {
  Serial.write(b);
}

void Midi::noteOn(uint8_t note, uint8_t velocity) {
  sendByte(0x90 | _channel);
  sendByte(note & 0x7F);
  sendByte(velocity & 0x7F);
}

void Midi::noteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
  sendByte(0x90 | (channel & 0x0F));
  sendByte(note & 0x7F);
  sendByte(velocity & 0x7F);
}

void Midi::noteOff(uint8_t note)  {
  sendByte(0x80 | _channel);
  sendByte(note & 0x7F);
  sendByte(0);
}

void Midi::noteOff(uint8_t note, uint8_t channel) {
  sendByte(0x80 | (channel & 0x0F));
  sendByte(note & 0x7F);
  sendByte(0);
}

void Midi::allNotesOff() {
  sendByte(0xB0 | _channel);
  sendByte(123); // CC 123 = all notes off
  sendByte(0);
}

void Midi::allNotesOff(uint8_t channel) {
  sendByte(0xB0 | (channel & 0x0F));
  sendByte(123); // CC 123 = all notes off
  sendByte(0);
}

void Midi::setChannel(uint8_t ch) {
  _channel = ch & 0x0F;  // Ensure 0-15 range
}
