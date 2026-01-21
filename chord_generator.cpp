#include "chord_generator.h"
#include "arp.h"

// Pattern types
enum ChordPattern : uint8_t {
  PATTERN_UP = 0,
  PATTERN_DOWN,
  PATTERN_UP_DOWN,
  PATTERN_COUNT
};

// Chord degree offsets within scale (0=root, 2=3rd, 4=5th, 6=7th)
static const uint8_t CHORD_DEGREES[4] = {0, 2, 4, 6};

static int8_t randomOctaveOffset(OctaveRange octaveRange) {
  switch (octaveRange) {
    case OCT_3:    return 0;
    case OCT_2_3:  return random(0, 2) == 0 ? -12 : 0;
    case OCT_3_4:  return random(0, 2) == 0 ? 0 : 12;
    case OCT_2_4:  return (random(0, 3) - 1) * 12;
    default:       return 0;
  }
}

void generateChord(const GeneratorParams& params) {
  int8_t* steps = params.steps;
  uint8_t* steps_fx = params.steps_fx;
  uint8_t* steps_cond = params.steps_cond;
  uint8_t base_note = params.rootNote + (params.octave * 12);
  ScaleId scale = params.scale;
  OctaveRange octaveRange = params.octaveRange;
  const int8_t (*scaleSteps)[8] = params.scaleSteps;

  // Clear all steps and initialize effects/conditions to defaults
  for (uint8_t i = 0; i < 64; i++) {
    steps[i] = 0;
    steps_fx[i] = 0;    // FX_1 (normal)
    steps_cond[i] = 0;  // COND_ALWAYS
  }

  // Select random chord root (scale degree 0-6 for I, ii, iii, IV, V, vi, vii)
  uint8_t chordRoot = random(0, 7);

  // Build 4-note chord (7th chord) from scale
  int8_t chordNotes[4];
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t scaleDegree = (chordRoot + CHORD_DEGREES[i]) % 8;
    int8_t noteOffset = scaleSteps[scale][scaleDegree];

    // If scale degree wraps around, add octave
    if (chordRoot + CHORD_DEGREES[i] >= 8) {
      noteOffset += 12;
    }
    chordNotes[i] = base_note + noteOffset;
  }

  // Select random voicing (inversion)
  uint8_t inversion = random(0, 4);

  // Apply inversion by rotating chord notes and adjusting octaves
  int8_t voicedChord[4];
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t srcIdx = (i + inversion) % 4;
    voicedChord[i] = chordNotes[srcIdx];

    // Notes that wrapped need octave adjustment to maintain ascending order
    if (srcIdx < inversion) {
      voicedChord[i] += 12;
    }
  }

  // Apply octave offset to entire chord
  int8_t octOffset = randomOctaveOffset(octaveRange);
  for (uint8_t i = 0; i < 4; i++) {
    voicedChord[i] += octOffset;
  }

  // Select random pattern
  ChordPattern pattern = (ChordPattern)random(0, PATTERN_COUNT);

  // Generate sequence indices based on pattern
  // Up&Down: 0,1,2,3,2,1 repeating (6 notes per cycle)
  // Up: 0,1,2,3 repeating
  // Down: 3,2,1,0 repeating

  // Fill 64 steps at 1/8 note intervals (every 2nd step)
  // That gives us 32 notes to place
  uint8_t noteIndex = 0;
  uint8_t upDownPos = 0;  // Position in up&down cycle
  bool ascending = true;

  for (uint8_t step = 0; step < 64; step += 2) {
    uint8_t chordIdx;

    switch (pattern) {
      case PATTERN_UP:
        chordIdx = noteIndex % 4;
        break;

      case PATTERN_DOWN:
        chordIdx = 3 - (noteIndex % 4);
        break;

      case PATTERN_UP_DOWN:
        // 0,1,2,3,2,1,0,1,2,3,2,1...
        chordIdx = upDownPos;
        if (ascending) {
          upDownPos++;
          if (upDownPos >= 3) {
            ascending = false;
          }
        } else {
          upDownPos--;
          if (upDownPos <= 0) {
            ascending = true;
          }
        }
        break;

      default:
        chordIdx = 0;
    }

    steps[step] = voicedChord[chordIdx];
    noteIndex++;
  }
}
