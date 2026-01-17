# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP8266-based MIDI arpeggiator that sends MIDI note messages over serial at 31250 baud. The project generates rhythmic patterns and outputs them as MIDI Note On/Off messages using microsecond-precision timing for accurate tempo at fast speeds.

## Architecture

The codebase follows a simple class-based architecture with two main components:

### Midi Class (midi.h/midi.cpp)
- Handles low-level MIDI serial communication at 31250 baud
- Provides methods for noteOn(), noteOff(), and allNotesOff()
- Outputs MIDI messages on channel 1 via Serial
- Uses standard MIDI message format (status byte + data bytes)

### Arp Class (arp.h/arp.cpp)
- Core arpeggiator engine that manages timing and pattern generation
- Contains a 32-step pattern buffer (steps[32]) for note sequences
- Timing system uses micros() for precise microsecond-level timing
- Maintains two timing thresholds: _next_note_on and _next_note_off for gate control
- BPM determines both note delay (_note_delays_ms) and gate length (_note_gate_ms)
- Currently hardcoded to play note 48 at 120 BPM
- Scale definitions (MAJOR, MINOR, DORIAN, PENTA, HARM_MINOR) are defined but not yet used

### Main Loop (arp-machine.ino)
- Instantiates Midi and Arp objects
- setup() initializes serial MIDI communication
- loop() calls arp.tick() continuously to handle timing and note events

## Key Implementation Details

**Timing System**: The arpeggiator uses microsecond timing (micros()) with signed long comparisons to handle wraparound. Note events are triggered by comparing current time against scheduled times.

**Pattern Generation**: The _generate_steps() method in arp.cpp:13 creates a basic kick-snare pattern with notes at positions divisible by 4 (note 48) and positions where (x+2)%4==0 (note 60).

**BPM Calculation**: At arp.cpp:28, BPM is converted to microseconds with: 60000000UL / bpm / 4, representing 16th note divisions.

## Building and Uploading

This is an Arduino sketch (.ino) for ESP8266. Use the Arduino IDE or arduino-cli:
- Compile and upload: `arduino-cli compile --fqbn esp8266:esp8266:generic -u -p <port> arp-machine`
- Common ESP8266 boards: `esp8266:esp8266:nodemcuv2`, `esp8266:esp8266:d1_mini`
- The specific board FQBN and port depend on your hardware setup

## Hardware Requirements

- ESP8266 microcontroller (NodeMCU, D1 Mini, etc.)
- MIDI output circuit (serial TX to MIDI DIN via appropriate circuitry)
- Target device expects MIDI at 31250 baud on channel 1

## Important Implementation Notes

- **Reference passing**: The Arp class takes a reference to the Midi object (not by value) to ensure MIDI commands operate on the initialized Serial connection
- **Timing precision**: Uses `micros()` (unsigned long) for microsecond timing, with proper type casting in comparisons to handle wraparound
- **Pattern progression**: The step counter `x` increments through the 32-step pattern buffer on each note trigger
