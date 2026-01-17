#pragma once
#include <Arduino.h>
#include "midi.h"

enum ScaleId : uint8_t { MAJOR=0, MINOR, DORIAN, PENTA, HARM_MINOR, SCALE_COUNT }; 

static const int8_t SCALE_STEPS[SCALE_COUNT][8] = { 
  /* MAJOR */ 
  {0,2,4,5,7,9,11,12}, 
  /* MINOR */ 
  {0,2,3,5,7,8,10,12}, 
  /* DORIAN */ 
  {0,2,3,5,7,9,10,12}, 
  /* PENTA */ 
  {0,3,5,7,10,12,15,19}, 
   /* HARM_MIN */ 
   {0,2,3,5,7,8,11,12} 
};

class Arp {

  public:

    int8_t steps[64];
    int8_t x = 0;
    uint8_t root_note;
    uint8_t octave;

    Arp(Midi& midi);

    void tick(unsigned long now);
    uint8_t getBpm() const { return _bpm; }


  private:
    Midi& _midi;
    unsigned long _next_note_on;
    unsigned long _next_note_off;
    uint8_t _note_playing;

    unsigned long _note_delays_ms;
    unsigned long _note_gate_ms;
    uint8_t _bpm;
    ScaleId _scale;

    void _generate_steps();

    void _update_bpm(uint8_t bpm);

};
