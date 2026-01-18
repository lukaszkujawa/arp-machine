# ARP Machine

A hardware MIDI arpeggiator built on the ESP8266 microcontroller. Generates rhythmic note patterns and outputs standard MIDI messages over serial at 31250 baud.

## Features

- **Variable-length sequencer** (1-64 steps) with 4 rows of 16 steps display
- **2 generation engines**: Default (rhythmic patterns) and Chord (arpeggio-based)
- **5 musical scales**: Major, Minor, Dorian, Pentatonic, Harmonic Minor
- **Adjustable parameters**: Length, BPM (40-240), root note, scale, octave range, note density
- **Sequence editing**: Toggle individual steps on/off with random scale-aware note generation
- **Real-time display**: 128x64 OLED shows all parameters and step grid with beat grouping
- **Microsecond-precision timing** for accurate tempo at high speeds

## Hardware Requirements

| Component | Description |
|-----------|-------------|
| ESP8266 | NodeMCU, D1 Mini, or similar |
| OLED Display | 128x64 SSD1306 I2C (0.96") |
| Rotary Encoder | With push button (e.g., KY-040) |
| 3x Buttons | Momentary push buttons |
| MIDI Output | Serial TX to MIDI DIN circuit |

### Pin Configuration

```
Rotary Encoder:
  ENC_CLK   → GPIO2  (D4)
  ENC_DT    → GPIO16 (D0)
  ENC_BTN   → GPIO0  (D3)

Buttons:
  BTN_PAUSE  → GPIO12 (D6)  - Pause/Resume
  BTN_SWITCH → GPIO13 (D7)  - Cycle menu selection
  BTN_REGEN  → GPIO14 (D5)  - Regenerate sequence

I2C (OLED):
  SDA → GPIO4 (D2)
  SCL → GPIO5 (D1)

MIDI:
  TX → GPIO1 (TX) → MIDI OUT circuit
```

## Controls

### Menu Navigation

Press **BTN_SWITCH** to cycle through parameters:

| Selection | Encoder Action | Display Location |
|-----------|----------------|------------------|
| Length | Adjust sequence length (1-64 steps) | Header |
| Generator | Switch generation engine (D/C) | Header |
| BPM | Adjust tempo (±1 BPM per click) | Header |
| Root | Change root note (C through B) | Settings row |
| Scale | Cycle through 5 scales | Settings row |
| Octave | Change octave range (3, 2-3, 3-4, 2-4) | Settings row |
| Density | Adjust note density (10-90%) | Settings row |
| Edit | Move cursor through sequence | Step grid |

### Generation Engines

Two pattern generation engines are available, selectable via the Generator parameter:

| Engine | Code | Description |
|--------|------|-------------|
| **Default** | D | Rhythmic pattern generator. Creates structured 16-step patterns with probability-based note placement favoring downbeats. Copies to 4 pages with subtle variations on pages 2 and 4. |
| **Chord** | C | Chord-based arpeggiator. Randomly selects a 7th chord from the scale, applies a random voicing (root, 1st, 2nd, or 3rd inversion), and plays in up/down/up-down pattern at 1/8 note intervals. |

### Sequence Length

The sequence length parameter (1-64) controls how many steps play before looping:
- Steps beyond the length are shown as thin lines in the display
- The playhead wraps at the set length
- Edit cursor is constrained to valid steps
- Useful for creating shorter loops or odd time signatures

### Sequence Editing

When in **Edit** mode:
- **Rotate encoder**: Move cursor left/right through active steps (respects length setting)
- **Press encoder**: Toggle step on/off
- Enabling a step generates a random note following current root, scale, and octave settings
- A frame appears around the step grid to indicate edit mode

### Other Controls

| Button | Action |
|--------|--------|
| BTN_PAUSE | Pause/resume playback |
| BTN_REGEN | Generate new random sequence |

## Building & Uploading

Requires Arduino IDE or arduino-cli with ESP8266 board support.

```bash
# Install ESP8266 board support
arduino-cli core install esp8266:esp8266

# Compile
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 arp-machine

# Upload (adjust port as needed)
arduino-cli upload --fqbn esp8266:esp8266:nodemcuv2 -p /dev/ttyUSB0 arp-machine
```

### Dependencies

- [U8g2](https://github.com/olikraus/u8g2) - Graphics library for OLED display

## Architecture

```
arp-machine/
├── arp-machine.ino        # Main entry point, input handling, event loop
├── arp.h / arp.cpp        # Arpeggiator engine (timing, state, playback)
├── midi.h / midi.cpp      # MIDI serial communication
├── lcd.h / lcd.cpp        # Display rendering
├── step_generator.h       # Generator interface and types
├── default_generator.h/cpp # Default rhythmic pattern generator
├── chord_generator.h/cpp  # Chord-based arpeggio generator
└── CLAUDE.md              # AI assistant context
```

### Component Overview

#### `arp-machine.ino` - Main Controller
- Initializes all components
- Handles rotary encoder with edge detection and debouncing
- Manages button interrupts (ISR) for responsive input
- Runs main loop: input → tick → display

**Key functions:**
- `handleEncoder()` - Rotary encoder rotation (polling, falling-edge detection)
- `handleEncButton()` - Encoder button (polling with debounce)
- `handleButtons()` - Process interrupt flags from ISRs
- `loop()` - Main event loop at ~µs resolution

#### `arp.h / arp.cpp` - Arpeggiator Engine
- 64-step pattern buffer (`steps[64]`) with variable length (1-64)
- Microsecond timing using `micros()` for note on/off scheduling
- Pluggable generator system for different pattern generation algorithms
- Edit mode for manual step toggling

**Key members:**
- `steps[64]` - Note values (0 = rest, >0 = MIDI note number)
- `x` - Current playhead position (0 to length-1)
- `length` - Active sequence length (1-64)
- `generator` - Current generation engine (GEN_DEFAULT or GEN_CHORD)
- `_next_note_on` / `_next_note_off` - Scheduled timing thresholds

#### `step_generator.h` - Generator Interface
Defines the interface for step generators:
- `GeneratorId` enum for selecting generators
- `GeneratorParams` struct with all parameters needed for generation
- `StepGeneratorFn` function pointer type
- `getGenerator()` returns generator function by ID

#### `default_generator.cpp` - Default Generator
Creates rhythmic patterns with:
- Probability-based note placement (density parameter)
- Downbeat emphasis
- 4-page structure with variations on pages 2 and 4

#### `chord_generator.cpp` - Chord Generator
Creates arpeggio patterns with:
- Random 7th chord selection from scale degrees
- Random voicing (root, 1st, 2nd, 3rd inversion)
- Three pattern modes: up, down, up-and-down
- Notes placed at 1/8 note intervals

**Timing system:**
```cpp
_note_delays_ms = 60000000UL / bpm / 4;  // 16th note interval in µs
_note_gate_ms = _note_delays_ms / 2;      // 50% gate time
```

#### `midi.h / midi.cpp` - MIDI Output
- Serial communication at 31250 baud (MIDI standard)
- Channel 1 output
- Note On/Off with velocity support

**API:**
```cpp
void noteOn(uint8_t note, uint8_t velocity);
void noteOff(uint8_t note);
void allNotesOff();
```

#### `lcd.h / lcd.cpp` - Display
- U8g2 library for SSD1306 OLED
- 400kHz I2C fast mode
- Efficient redraw (only on state change)

**Display layout:**
```
┌────────────────────────────┐
│arpM        64 D  ● 120     │  ← Header (title, length, generator, beat indicator, BPM)
│ C  Major  Oct2-3      45%  │  ← Settings row (root, scale, octave, density)
│ ■■□■ ■□■□ ■■□■ □■□■       │  ← Step grid (4 rows × 16 cols, grouped by quarter note)
│ ■■□■ ■□■□ ■■□■ □■□■       │     ■ = note, □ = empty, _ = disabled (beyond length)
│ ■■□■ ■□■□ ■■□■ □■□■       │
│ ■■□■ ■□■□ ■■□■ □■□■       │
└────────────────────────────┘
```
- Selected parameter is highlighted (inverted colors)
- Beat indicator circle fills on quarter notes
- Steps are visually grouped with extra spacing every 4 steps

### Data Flow

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Buttons   │────▶│  Main Loop  │────▶│     Arp     │
│   Encoder   │     │ (ino file)  │     │   Engine    │
└─────────────┘     └──────┬──────┘     └──────┬──────┘
                           │                   │
                           ▼                   ▼
                    ┌─────────────┐     ┌─────────────┐
                    │     LCD     │     │    MIDI     │
                    │   Display   │     │   Output    │
                    └─────────────┘     └─────────────┘
```

### Timing Architecture

The arpeggiator uses **microsecond-precision scheduling** to maintain accurate tempo:

1. `tick()` is called every loop iteration with current `micros()` value
2. Compares against `_next_note_on` and `_next_note_off` thresholds
3. Uses signed comparison `(long)(now - threshold) >= 0` to handle 32-bit wraparound
4. Schedules next events relative to current time

This approach ensures timing accuracy even at high BPM values where millisecond resolution would introduce noticeable jitter.

## Contributing

Feel free to submit issues and pull requests. Key areas for contribution:

- **New generators**: Add new pattern generation engines (euclidean rhythms, probability matrices, etc.)
- **Additional scales**: More scale definitions in `arp.h`
- **MIDI input**: External sync and note input
- **Persistence**: Save/load patterns to EEPROM
- **UI improvements**: Additional display modes or information

## License

MIT License - See LICENSE file for details.
