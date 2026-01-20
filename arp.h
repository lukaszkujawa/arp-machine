#pragma once
#include <Arduino.h>
#include "midi.h"
#include "step_generator.h"

enum ScaleId : uint8_t { MAJOR=0, MINOR, DORIAN, PENTA, HARM_MINOR, SCALE_COUNT };

// Octave range settings: 0=oct3 only, 1=oct2-3, 2=oct3-4, 3=oct2-4
enum OctaveRange : uint8_t { OCT_3=0, OCT_2_3, OCT_3_4, OCT_2_4, OCT_RANGE_COUNT };

// Edit sub-modes: what the encoder controls in edit mode
enum EditSubMode : uint8_t { EDIT_SEQUENCE=0, EDIT_NOTE, EDIT_DIV, EDIT_COND, EDIT_SUBMODE_COUNT };

// Step division (ratchet): how many times a note triggers within its step
enum StepDiv : uint8_t {
  DIV_1=0,   // x1 - Normal single trigger
  DIV_2,     // x2 - Trigger twice (ratchet 2)
  DIV_3,     // x3 - Trigger three times (ratchet 3)
  STEP_DIV_COUNT
};

// Step trigger condition: when a note plays
enum StepCond : uint8_t {
  COND_ALWAYS=0, // Always trigger
  COND_1_2,      // 1:2 - Trigger every second time
  COND_1_3,      // 1:3 - Trigger every third time
  COND_1_4,      // 1:4 - Trigger every fourth time
  COND_PROB_10,  // 10% - 10% chance to trigger
  COND_PROB_25,  // 25% - 25% chance to trigger
  COND_PROB_50,  // 50% - 50% chance to trigger
  COND_PROB_75,  // 75% - 75% chance to trigger
  STEP_COND_COUNT
}; 

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
    uint8_t steps_div[64];   // Division/ratchet per step (StepDiv)
    uint8_t steps_cond[64];  // Trigger condition per step (StepCond)
    
    uint8_t x = 0;
    uint8_t root_note;
    uint8_t octave;
    uint8_t density;
    ScaleId scale;
    OctaveRange octaveRange;
    GeneratorId generator;
    uint8_t length;
    uint8_t channel;  // MIDI channel 0-15 (displayed as 1-16)
    uint8_t swing;    // Swing amount 50-75 (50=straight, 75=heavy swing)

    Arp(Midi& midi);

    void tick(unsigned long now);
    void regenerate();
    void clearSequence();
    void togglePause();
    void adjustBpm(int8_t delta);
    void adjustRootNote(int8_t delta);
    void adjustScale(int8_t delta);
    void adjustOctaveRange(int8_t delta);
    void adjustDensity(int8_t delta);
    void adjustGenerator(int8_t delta);
    void adjustLength(int8_t delta);
    void adjustChannel(int8_t delta);
    void adjustSwing(int8_t delta);
    uint8_t getBpm() const { return _bpm; }
    bool isPaused() const { return _paused; }

    // Edit mode
    bool editMode;
    uint8_t editStep;
    EditSubMode editSubMode;
    void toggleEditMode();
    void moveEditCursor(int8_t delta);
    void toggleCurrentStep();
    void cycleEditSubMode();
    void adjustCurrentStepNote(int8_t delta);
    void adjustCurrentStepDiv(int8_t delta);
    void adjustCurrentStepCond(int8_t delta);


  private:
    Midi& _midi;
    unsigned long _next_note_on;
    unsigned long _next_note_off;
    uint8_t _note_playing;
    bool _paused;

    // Ratchet state
    uint8_t _ratchet_count;        // How many ratchet hits remaining
    unsigned long _ratchet_gate;   // Gate time for ratchet notes
    unsigned long _ratchet_interval; // Time between ratchet hits
    unsigned long _next_ratchet_on;  // When to trigger next ratchet hit
    uint8_t _ratchet_note;         // Note to play for ratchet

    // Divisor mod state (counter per step for 1:2, 1:3, 1:4 mods)
    uint8_t _step_trigger_counter[64]; // Counter per step, play when counter % divisor == 0

    unsigned long _note_delays_ms;
    unsigned long _note_gate_ms;
    uint8_t _bpm;

    int8_t _randomOctaveOffset();

    void _update_bpm(uint8_t bpm);

};
