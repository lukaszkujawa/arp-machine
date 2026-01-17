#include "arp.h"

Arp::Arp(Midi& midi) : _midi(midi) {
  _next_note_on = 0;
  _next_note_off = 0;
  _note_playing = 0;

  octave = 3;  // Default octave
  root_note = 0;  // Default to C
  scale = MAJOR;  // Default to Major
  octaveRange = OCT_2_3;  // Default to octave 2-3 range
  randomSeed(RANDOM_REG32);  // Seed from ESP8266 hardware RNG

  _update_bpm(120);
  _generate_steps();
}

int8_t Arp::_randomOctaveOffset() {
  switch (octaveRange) {
    case OCT_3:    return 0;
    case OCT_2_3:  return random(0, 2) == 0 ? -12 : 0;
    case OCT_3_4:  return random(0, 2) == 0 ? 0 : 12;
    case OCT_2_4:  return (random(0, 3) - 1) * 12;  // -12, 0, or 12
    default:      return 0;
  }
}

void Arp::_generate_steps() {
  // Pick random density (root_note and scale are set by buttons)
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

      // Calculate final MIDI note number with octave variation
      steps[i] = base_note + SCALE_STEPS[scale][scale_degree] + _randomOctaveOffset();
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

  // Vary line 2 (steps 16-31): randomly add or remove 1-2 notes
  uint8_t changes_line2 = random(1, 3);
  for (uint8_t c = 0; c < changes_line2; c++) {
    uint8_t pos = 16 + random(0, 16);
    if (steps[pos] > 0) {
      steps[pos] = 0;  // Remove note
    } else {
      uint8_t scale_degree = random(0, 8);
      steps[pos] = base_note + SCALE_STEPS[scale][scale_degree] + _randomOctaveOffset();
    }
  }

  // Vary line 4 (steps 48-63): randomly add or remove 1-2 notes
  uint8_t changes_line4 = random(1, 3);
  for (uint8_t c = 0; c < changes_line4; c++) {
    uint8_t pos = 48 + random(0, 16);
    if (steps[pos] > 0) {
      steps[pos] = 0;  // Remove note
    } else {
      uint8_t scale_degree = random(0, 8);
      steps[pos] = base_note + SCALE_STEPS[scale][scale_degree] + _randomOctaveOffset();
    }
  }
}

void Arp::regenerate() {
  _generate_steps();
  x = 0;  // Reset step position
}

void Arp::rotateRootNote() {
  root_note = (root_note + 1) % 12;
}

void Arp::rotateScale() {
  scale = (ScaleId)((scale + 1) % SCALE_COUNT);
}

void Arp::rotateOctaveRange() {
  octaveRange = (OctaveRange)((octaveRange + 1) % OCT_RANGE_COUNT);
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