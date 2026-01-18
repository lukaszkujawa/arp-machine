#pragma once
#include "step_generator.h"

// Chord-based arpeggio generator
// Selects a random 4-note chord from the scale, applies random voicing,
// and plays in up/down/up&down pattern at 1/8 note intervals
void generateChord(const GeneratorParams& params);
