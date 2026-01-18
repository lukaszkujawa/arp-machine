#pragma once
#include "step_generator.h"

// Default rhythmic pattern generator
// Creates structured rhythm with density-based probability,
// copies to 4 pages with variations on pages 2 and 4
void generateDefault(const GeneratorParams& params);
