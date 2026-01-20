#pragma once

#include <Arduino.h>

class Midi {

  public:
    Midi();

    void setup();

    void noteOn(uint8_t note, uint8_t velocity);
    void noteOn(uint8_t note, uint8_t velocity, uint8_t channel);
    void noteOff(uint8_t note);
    void noteOff(uint8_t note, uint8_t channel);

    void allNotesOff();
    void allNotesOff(uint8_t channel);
    void setChannel(uint8_t ch);

    // MIDI clock and transport
    void clock();      // Send MIDI clock pulse (0xF8)
    void start();      // Send MIDI Start (0xFA)
    void stop();       // Send MIDI Stop (0xFC)
    void midiContinue(); // Send MIDI Continue (0xFB)

  private:
    uint8_t _channel;

    void sendByte(uint8_t b);

};