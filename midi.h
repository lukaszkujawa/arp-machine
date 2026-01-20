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

  private:
    uint8_t _channel;

    void sendByte(uint8_t b);

};