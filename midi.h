#pragma once

#include <Arduino.h>

class Midi {

  public:
    Midi();

    void setup();

    void noteOn(uint8_t note, uint8_t velocity);
    void noteOff(uint8_t note);

    void allNotesOff();

  private:
    uint8_t _channel;

    void sendByte(uint8_t b);

};