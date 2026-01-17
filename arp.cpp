#include "arp.h"

Arp::Arp(Midi& midi) : _midi(midi) {
  _next_note_on = 0;
  _next_note_off = 0;
  _note_playing = 0;

  octave = 3;  // Default octave
  randomSeed(RANDOM_REG32);  // Seed from ESP8266 hardware RNG

  _update_bpm(120);
  _generate_steps();
}

void Arp::_generate_steps() {
  // Pick random scale, root note, and density
  _scale = (ScaleId)random(0, SCALE_COUNT);
  root_note = random(0, 12);  // 0=C, 1=C#, ..., 11=B
  density = random(10, 71);   // 10-70% density
  uint8_t base_note = root_note + (octave * 12);

  // Generate first 16 steps (Page 1)
  for (uint8_t i = 0; i < 16; i++) {
    // Create structured rhythm with variations
    bool is_downbeat = (i % 16 == 0);  // Strong beats every bar
    bool is_beat = (i % 4 == 0);       // Beats every quarter note
    bool is_offbeat = (i % 2 == 1);    // Offbeats

    // Base probabilities scaled by density
    int chance = random(0, 100);
    bool should_play = false;

    if (is_downbeat) {
      should_play = (chance < density + 30);  // Downbeats favored
    } else if (is_beat) {
      should_play = (chance < density + 10);  // Beats slightly favored
    } else if (is_offbeat) {
      should_play = (chance < density - 10);  // Offbeats less likely
    } else {
      should_play = (chance < density - 20);  // 16th notes least likely
    }

    if (should_play) {
      // Pick a scale degree (0-7 for the scale)
      uint8_t scale_degree = random(0, 8);

      // Favor lower notes on downbeats
      if (is_downbeat && random(0, 100) < 70) {
        scale_degree = random(0, 3);  // Root, 2nd, or 3rd
      }

      // Calculate final MIDI note number
      steps[i] = base_note + SCALE_STEPS[_scale][scale_degree];
    } else {
      steps[i] = 0;  // Rest
    }
  }

  // Copy Page 1 to Pages 2, 3, and 4
  for (uint8_t page = 1; page < 4; page++) {
    for (uint8_t i = 0; i < 16; i++) {
      steps[page * 16 + i] = steps[i];
    }
  }

  // Add complexity to end of Page 2 (steps 24-31)
  for (uint8_t i = 24; i < 32; i++) {
    if (random(0, 100) < 60) {  // 60% chance to modify
      if (steps[i] > 0) {
        // Change to different scale degree
        uint8_t scale_degree = random(0, 8);
        steps[i] = base_note + SCALE_STEPS[_scale][scale_degree];
      } else {
        // Add a new note where there was a rest
        if (random(0, 100) < 50) {
          uint8_t scale_degree = random(0, 8);
          steps[i] = base_note + SCALE_STEPS[_scale][scale_degree];
        }
      }
    }
  }

  // Add even more complexity to end of Page 4 (steps 56-63)
  for (uint8_t i = 56; i < 64; i++) {
    if (random(0, 100) < 80) {  // 80% chance to modify
      if (steps[i] > 0) {
        // More dramatic changes - jump to different scale degrees
        uint8_t scale_degree = random(0, 8);
        steps[i] = base_note + SCALE_STEPS[_scale][scale_degree];

        // Sometimes add octave jumps for drama
        if (random(0, 100) < 30) {
          steps[i] += 12;  // Up one octave
        }
      } else {
        // Higher chance to fill in rests
        if (random(0, 100) < 70) {
          uint8_t scale_degree = random(0, 8);
          steps[i] = base_note + SCALE_STEPS[_scale][scale_degree];
        }
      }
    }
  }
}

void Arp::regenerate() {
  _generate_steps();
  x = 0;  // Reset step position
}

void Arp::_update_bpm(uint8_t bpm) {
  _bpm = bpm;
  _note_delays_ms = 60000000UL / bpm / 4;
  _note_gate_ms = _note_delays_ms / 2;
}

void Arp::tick(unsigned long now) {
  if(_note_playing > 0 && (long)(now - _next_note_off) >= 0) {
    _midi.noteOff(_note_playing);
    _note_playing = 0;
  }

  if(_note_playing == 0 && (long)(now - _next_note_on) >= 0) {
    _note_playing = steps[x];

    if(_note_playing > 0) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Toggle LED on each note
      _midi.noteOn(_note_playing, 100);
      _next_note_off = now + _note_gate_ms;
    }

    _next_note_on = now + _note_delays_ms;
    x = (x + 1) % 64;
  }

}