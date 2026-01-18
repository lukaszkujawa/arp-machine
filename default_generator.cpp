#include "default_generator.h"
#include "chord_generator.h"
#include "arp.h"

static int8_t randomOctaveOffset(OctaveRange octaveRange) {
  switch (octaveRange) {
    case OCT_3:    return 0;
    case OCT_2_3:  return random(0, 2) == 0 ? -12 : 0;
    case OCT_3_4:  return random(0, 2) == 0 ? 0 : 12;
    case OCT_2_4:  return (random(0, 3) - 1) * 12;  // -12, 0, or 12
    default:       return 0;
  }
}

void generateDefault(const GeneratorParams& params) {
  uint8_t base_note = params.rootNote + (params.octave * 12);
  int8_t* steps = params.steps;
  int8_t* steps_mods = params.steps_mods;
  uint8_t density = params.density;
  ScaleId scale = params.scale;
  OctaveRange octaveRange = params.octaveRange;
  const int8_t (*scaleSteps)[8] = params.scaleSteps;

  // Initialize all step modifiers to 0 (normal)
  for (uint8_t i = 0; i < 64; i++) {
    steps_mods[i] = 0;
  }

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
      steps[i] = base_note + scaleSteps[scale][scale_degree] + randomOctaveOffset(octaveRange);
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
      steps[pos] = base_note + scaleSteps[scale][scale_degree] + randomOctaveOffset(octaveRange);
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
      steps[pos] = base_note + scaleSteps[scale][scale_degree] + randomOctaveOffset(octaveRange);
    }
  }
}

// Generator registry
StepGeneratorFn getGenerator(GeneratorId id) {
  switch (id) {
    case GEN_CHORD:
      return generateChord;
    case GEN_DEFAULT:
    default:
      return generateDefault;
  }
}
