#pragma once
#include <Arduino.h>
#include "midi.h"
#include "step_generator.h"

enum ScaleId : uint8_t { MAJOR=0, MINOR, DORIAN, PENTA, HARM_MINOR, SCALE_COUNT };

// Octave range settings: 0=oct3 only, 1=oct2-3, 2=oct3-4, 3=oct2-4
enum OctaveRange : uint8_t { OCT_3=0, OCT_2_3, OCT_3_4, OCT_2_4, OCT_RANGE_COUNT }; 

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
    uint8_t density;
    ScaleId scale;
    OctaveRange octaveRange;
    GeneratorId generator;
    uint8_t length;

    Arp(Midi& midi);

    void tick(unsigned long now);
    void regenerate();
    void togglePause();
    void adjustBpm(int8_t delta);
    void adjustRootNote(int8_t delta);
    void adjustScale(int8_t delta);
    void adjustOctaveRange(int8_t delta);
    void adjustDensity(int8_t delta);
    void adjustGenerator(int8_t delta);
    void adjustLength(int8_t delta);
    uint8_t getBpm() const { return _bpm; }
    bool isPaused() const { return _paused; }

    // Edit mode
    bool editMode;
    int8_t editStep;
    void toggleEditMode();
    void moveEditCursor(int8_t delta);
    void toggleCurrentStep();


  private:
    Midi& _midi;
    unsigned long _next_note_on;
    unsigned long _next_note_off;
    uint8_t _note_playing;
    bool _paused;

    unsigned long _note_delays_ms;
    unsigned long _note_gate_ms;
    uint8_t _bpm;

    int8_t _randomOctaveOffset();

    void _update_bpm(uint8_t bpm);

};
