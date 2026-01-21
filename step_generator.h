#pragma once
#include <Arduino.h>

// Forward declarations from arp.h
enum ScaleId : uint8_t;
enum OctaveRange : uint8_t;

// Generator type enum - add new generators here
enum GeneratorId : uint8_t {
  GEN_DEFAULT = 0,
  GEN_CHORD,
  GENERATOR_COUNT
};

// Parameters passed to step generators
struct GeneratorParams {
  int8_t* steps;           // Pointer to 64-step buffer to fill
  uint8_t* steps_fx;       // Pointer to 64-step effect buffer (StepFx)
  uint8_t* steps_cond;     // Pointer to 64-step condition buffer (StepCond)
  uint8_t stepCount;       // Number of steps (64)
  uint8_t rootNote;        // Root note (0-11)
  uint8_t octave;          // Base octave
  ScaleId scale;           // Scale to use
  OctaveRange octaveRange; // Octave range setting
  uint8_t density;         // Note density (10-90)
  const int8_t (*scaleSteps)[8]; // Pointer to scale definitions
};

// Function pointer type for step generators
typedef void (*StepGeneratorFn)(const GeneratorParams& params);

// Get generator function by ID
StepGeneratorFn getGenerator(GeneratorId id);
