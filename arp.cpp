#include "arp.h"
#include "default_generator.h"

Arp::Arp(Midi& midi) : _midi(midi) {
  _next_note_on = 0;
  _next_note_off = 0;
  _note_playing = 0;
  _paused = false;

  // Ratchet state
  _ratchet_count = 0;
  _ratchet_gate = 0;
  _ratchet_interval = 0;
  _next_ratchet_on = 0;
  _ratchet_note = 0;

  // Divisor mod state - all steps start at counter 0 (will play first time)
  memset(_step_trigger_counter, 0, sizeof(_step_trigger_counter));

  octave = 4;  // Default octave (C4 = MIDI 48)
  root_note = 0;  // Default to C
  scale = MAJOR;  // Default to Major
  octaveRange = OCT_2_3;  // Default to octave 2-3 range
  density = 45;  // Default 45% density
  generator = GEN_DEFAULT;  // Default generator
  length = 64;  // Default full sequence length
  channel = 0;  // Default MIDI channel 1 (0-indexed)
  editMode = false;  // Start in normal mode
  editStep = 0;  // Edit cursor at step 0
  editSubMode = EDIT_SEQUENCE;  // Default to sequence navigation
  randomSeed(RANDOM_REG32);  // Seed from ESP8266 hardware RNG

  _update_bpm(120);
  regenerate();
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

void Arp::regenerate() {
  GeneratorParams params = {
    steps,
    steps_mods,
    64,
    root_note,
    octave,
    scale,
    octaveRange,
    density,
    SCALE_STEPS
  };

  StepGeneratorFn genFn = getGenerator(generator);
  genFn(params);
  x = 0;  // Reset step position
}

void Arp::togglePause() {
  _paused = !_paused;
  if (_paused && _note_playing > 0) {
    _midi.noteOff(_note_playing);
    _note_playing = 0;
  }
}

void Arp::adjustBpm(int8_t delta) {
  int16_t newBpm = _bpm + delta;
  if (newBpm < 40) newBpm = 40;
  if (newBpm > 240) newBpm = 240;
  _update_bpm((uint8_t)newBpm);
}

void Arp::adjustRootNote(int8_t delta) {
  int8_t newRoot = root_note + delta;
  if (newRoot < 0) newRoot = 11;
  if (newRoot > 11) newRoot = 0;
  root_note = (uint8_t)newRoot;
}

void Arp::adjustScale(int8_t delta) {
  int8_t newScale = scale + delta;
  if (newScale < 0) newScale = SCALE_COUNT - 1;
  if (newScale >= SCALE_COUNT) newScale = 0;
  scale = (ScaleId)newScale;
}

void Arp::adjustOctaveRange(int8_t delta) {
  int8_t newRange = octaveRange + delta;
  if (newRange < 0) newRange = OCT_RANGE_COUNT - 1;
  if (newRange >= OCT_RANGE_COUNT) newRange = 0;
  octaveRange = (OctaveRange)newRange;
}

void Arp::adjustDensity(int8_t delta) {
  int16_t newDensity = density + delta * 5;
  if (newDensity < 10) newDensity = 10;
  if (newDensity > 90) newDensity = 90;
  density = (uint8_t)newDensity;
}

void Arp::adjustGenerator(int8_t delta) {
  int8_t newGen = generator + delta;
  if (newGen < 0) newGen = GENERATOR_COUNT - 1;
  if (newGen >= GENERATOR_COUNT) newGen = 0;
  generator = (GeneratorId)newGen;
}

void Arp::adjustLength(int8_t delta) {
  int16_t newLen = length + delta;
  if (newLen < 1) newLen = 1;
  if (newLen > 64) newLen = 64;
  length = (uint8_t)newLen;
  // Ensure playhead and edit cursor are within bounds
  if (x >= length) x = 0;
  if (editStep >= length) editStep = length - 1;
}

void Arp::adjustChannel(int8_t delta) {
  int8_t newCh = channel + delta;
  if (newCh < 0) newCh = 15;
  if (newCh > 15) newCh = 0;
  channel = (uint8_t)newCh;
  _midi.setChannel(channel);
}

void Arp::_update_bpm(uint8_t bpm) {
  _bpm = bpm;
  _note_delays_ms = 60000000UL / bpm / 4;
  _note_gate_ms = _note_delays_ms / 2;
}

void Arp::tick(unsigned long now) {
  // Handle note off
  if(_note_playing > 0 && (long)(now - _next_note_off) >= 0) {
    _midi.noteOff(_note_playing);
    _note_playing = 0;
  }

  // Handle ratchet note on (separate from main step timing)
  if (_ratchet_count > 0 && _note_playing == 0 && (long)(now - _next_ratchet_on) >= 0) {
    _ratchet_count--;
    _note_playing = _ratchet_note;
    _midi.noteOn(_note_playing, 100);
    _next_note_off = now + _ratchet_gate;
    _next_ratchet_on = now + _ratchet_interval;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  if (_paused) return;

  if(_note_playing == 0 && _ratchet_count == 0 && (long)(now - _next_note_on) >= 0) {
    uint8_t note = steps[x];
    int8_t mod = steps_mods[x];

    bool should_play = true;

    // Handle divisor mods (1:2, 1:3, 1:4) - play when counter hits 0
    if (note > 0) {
      if (mod == MOD_HALF) {
        should_play = (_step_trigger_counter[x] % 2) == 0;
        _step_trigger_counter[x]++;
      } else if (mod == MOD_THIRD) {
        should_play = (_step_trigger_counter[x] % 3) == 0;
        _step_trigger_counter[x]++;
      } else if (mod == MOD_QUARTER) {
        should_play = (_step_trigger_counter[x] % 4) == 0;
        _step_trigger_counter[x]++;
      }
      // Handle probability mods
      else if (mod == MOD_PROB_10) {
        should_play = random(100) < 10;
      } else if (mod == MOD_PROB_25) {
        should_play = random(100) < 25;
      } else if (mod == MOD_PROB_50) {
        should_play = random(100) < 50;
      } else if (mod == MOD_PROB_75) {
        should_play = random(100) < 75;
      }
    }

    if (note > 0 && should_play) {
      _note_playing = note;
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Toggle LED on each note

      // Set up gate time and ratchet based on mod
      unsigned long gate = _note_gate_ms;
      _ratchet_count = 0;

      if (mod == MOD_RATCHET2) {
        // 2 hits: divide 1/16 step into 2x 1/32 notes
        // Gate = 50% of 1/32 = 1/64 note duration
        _ratchet_interval = _note_delays_ms / 2;
        _ratchet_gate = _ratchet_interval / 2;
        gate = _ratchet_gate;
        _ratchet_count = 1;
        _ratchet_note = note;
        _next_ratchet_on = now + _ratchet_interval;
      } else if (mod == MOD_RATCHET3) {
        // 3 hits: divide 1/16 step into 3 equal sub-notes
        // Gate = 50% of each sub-note duration
        _ratchet_interval = _note_delays_ms / 3;
        _ratchet_gate = _ratchet_interval / 2;
        gate = _ratchet_gate;
        _ratchet_count = 2;
        _ratchet_note = note;
        _next_ratchet_on = now + _ratchet_interval;
      }

      _midi.noteOn(_note_playing, 100);
      _next_note_off = now + gate;
    }

    _next_note_on = now + _note_delays_ms;
    x = (x + 1) % length;
  }

}

void Arp::toggleEditMode() {
  editMode = !editMode;
  if (editMode) {
    editStep = 0;  // Start editing at step 0
    editSubMode = EDIT_SEQUENCE;  // Reset to sequence navigation
  }
}

void Arp::cycleEditSubMode() {
  EditSubMode nextMode = (EditSubMode)((editSubMode + 1) % EDIT_SUBMODE_COUNT);

  // If entering note or mode editing and current step is inactive, activate it first
  if ((nextMode == EDIT_NOTE || nextMode == EDIT_MODE) && steps[editStep] == 0) {
    uint8_t base_note = root_note + (octave * 12);
    uint8_t scale_degree = random(0, 8);
    steps[editStep] = base_note + SCALE_STEPS[scale][scale_degree] + _randomOctaveOffset();
    steps_mods[editStep] = 0;  // Normal modifier
  }

  editSubMode = nextMode;
}

void Arp::moveEditCursor(int8_t delta) {
  int16_t newStep = (int16_t)editStep + delta;
  if (newStep < 0) newStep = length - 1;
  if (newStep >= length) newStep = 0;
  editStep = (uint8_t)newStep;
}

void Arp::toggleCurrentStep() {
  if (steps[editStep] > 0) {
    // Turn step off
    steps[editStep] = 0;
  } else {
    // Turn step on with a random note following scale
    uint8_t base_note = root_note + (octave * 12);
    uint8_t scale_degree = random(0, 8);
    steps[editStep] = base_note + SCALE_STEPS[scale][scale_degree] + _randomOctaveOffset();
    steps_mods[editStep] = 0;  // Normal modifier
  }
}

void Arp::adjustCurrentStepNote(int8_t delta) {
  // Only adjust if step has a note
  if (steps[editStep] <= 0) return;

  int8_t currentNote = steps[editStep];

  // Define bounds: display octave 2 to octave 4
  // Display octave = (MIDI note / 12) - 1
  // So display octave 2 starts at MIDI 36, octave 4 starts at MIDI 60
  int8_t minNote = root_note + 3 * 12;  // Root in display octave 2
  int8_t maxNote = root_note + 5 * 12 + SCALE_STEPS[scale][6];  // 7th degree in display octave 4

  // Use 7 degrees per octave (indices 0-6), index 7 is the next octave's root
  const uint8_t DEGREES_PER_OCTAVE = 7;

  // Find current position as absolute scale index
  int8_t noteOffset = currentNote - root_note;
  int16_t absIndex = 0;

  // Calculate which octave and find the scale degree
  int8_t octave = 0;
  while (noteOffset >= 12) {
    noteOffset -= 12;
    octave++;
  }
  while (noteOffset < 0) {
    noteOffset += 12;
    octave--;
  }

  // Find the scale degree (0-6) for this note within the octave
  int8_t degree = 0;
  for (int8_t i = 6; i >= 0; i--) {
    if (noteOffset >= SCALE_STEPS[scale][i]) {
      degree = i;
      break;
    }
  }

  // Calculate absolute index: octave * 7 + degree
  absIndex = octave * DEGREES_PER_OCTAVE + degree;

  // Apply delta
  absIndex += delta;

  // Convert back to octave and degree
  int8_t newOctave = absIndex / DEGREES_PER_OCTAVE;
  int8_t newDegree = absIndex % DEGREES_PER_OCTAVE;
  if (newDegree < 0) {
    newDegree += DEGREES_PER_OCTAVE;
    newOctave--;
  }

  // Calculate new MIDI note
  int8_t newNote = root_note + (newOctave * 12) + SCALE_STEPS[scale][newDegree];

  // Clamp to allowed range
  if (newNote < minNote) newNote = minNote;
  if (newNote > maxNote) newNote = maxNote;

  steps[editStep] = newNote;
}

void Arp::adjustCurrentStepMod(int8_t delta) {
  // Only adjust if step has a note
  if (steps[editStep] <= 0) return;

  int8_t newMod = steps_mods[editStep] + delta;

  // Wrap around
  if (newMod < 0) newMod = STEP_MOD_COUNT - 1;
  if (newMod >= STEP_MOD_COUNT) newMod = 0;

  steps_mods[editStep] = newMod;
}