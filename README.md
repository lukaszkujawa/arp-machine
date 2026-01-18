# ARP Machine

A hardware MIDI arpeggiator built on the ESP8266 microcontroller. Generates rhythmic note patterns and outputs standard MIDI messages over serial at 31250 baud.

## Features

- **64-step sequencer** with 4 rows of 16 steps, adjustable length (1-64)
- **5 musical scales**: Major, Minor, Dorian, Pentatonic, Harmonic Minor
- **Adjustable parameters**: BPM (40-240), root note, scale, octave range, note density
- **Step modifiers**: Normal, Ratchet x2, Ratchet x3, Half-time (1:2)
- **Sequence editing**: Full control over step notes and modifiers
- **Multiple generators**: Default rhythmic patterns, Chord arpeggios
- **Real-time display**: 128x64 OLED shows all parameters and step grid
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

| Selection | Encoder Action |
|-----------|----------------|
| Length | Adjust sequence length (1-64 steps) |
| Generator | Switch pattern generator (D=Default, C=Chord) |
| BPM | Adjust tempo (40-240 BPM) |
| Root | Change root note (C through B) |
| Scale | Cycle through 5 scales |
| Octave | Change octave range (3, 2-3, 3-4, 2-4) |
| Density | Adjust note density (10-90%) |
| Edit | Enter sequence edit mode |

### Sequence Editing

When in **Edit** mode, press **BTN_REGEN** to cycle through sub-modes:

| Sub-mode | Display | Encoder Action |
|----------|---------|----------------|
| Sequence | Neither highlighted | Move cursor through steps |
| Note | "Note:" highlighted | Change note pitch (octave 2-4, follows scale) |
| Mode | "Mode:" highlighted | Change step modifier (N, R2, R3, 1:2) |

- **Press encoder**: Toggle step on/off (in Sequence sub-mode)
- Enabling a step generates a random note following current root, scale, and octave settings
- Edit cursor always starts at step 0

### Step Modifiers

| Modifier | Name | Effect |
|----------|------|--------|
| N | Normal | Single note trigger |
| R2 | Ratchet x2 | Two 1/32 notes within the 1/16 step |
| R3 | Ratchet x3 | Three notes within the 1/16 step |
| 1:2 | Half-time | Note triggers every other time |

### Other Controls

| Button | Action |
|--------|--------|
| BTN_PAUSE | Pause/resume playback |
| BTN_REGEN | Generate new sequence (or cycle edit sub-mode when editing) |

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
├── arp.h / arp.cpp        # Arpeggiator engine (timing, patterns, playback)
├── midi.h / midi.cpp      # MIDI serial communication
├── lcd.h / lcd.cpp        # Display rendering
├── step_generator.h       # Generator interface and registry
├── default_generator.h/cpp # Default rhythmic pattern generator
├── chord_generator.h/cpp  # Chord arpeggio pattern generator
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
- 64-step pattern buffer with note values and modifiers
- Microsecond timing using `micros()` for note on/off scheduling
- Scale-aware note generation with configurable density
- Step modifiers for ratchets and half-time effects
- Edit mode with sub-modes for sequence, note, and modifier editing

**Key members:**
- `steps[64]` - Note values (0 = rest, >0 = MIDI note number)
- `steps_mods[64]` - Step modifiers (N, R2, R3, 1:2)
- `x` - Current playhead position
- `editMode`, `editStep`, `editSubMode` - Edit state
- `_next_note_on` / `_next_note_off` - Scheduled timing thresholds
- `_ratchet_count`, `_ratchet_interval` - Ratchet playback state

**Timing system:**
```cpp
_note_delays_ms = 60000000UL / bpm / 4;  // 16th note interval in µs
_note_gate_ms = _note_delays_ms / 2;      // 50% gate time

// Ratchet timing (R2 example):
_ratchet_interval = _note_delays_ms / 2;  // 1/32 note
_ratchet_gate = _ratchet_interval / 2;    // 1/64 note (50% gate)
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

**Display layout (normal mode):**
```
┌────────────────────────────┐
│ arpM      64 D       ● 120 │  ← Header (title, length, generator, beat, BPM)
│ C  Major  Oct2-3      45%  │  ← Settings row (root, scale, octave, density)
│ ■■□■ ■□■□ ■■□■ □■□■       │  ← Step grid (4 rows × 16 cols)
│ ■■□■ ■□■□ ■■□■ □■□■       │
│ ■■□■ ■□■□ ■■□■ □■□■       │
│ ■■□■ ■□■□ ■■□■ □■□■       │
└────────────────────────────┘
```

**Display layout (edit mode):**
```
┌────────────────────────────┐
│ arpM      64 D       ● 120 │  ← Header
│ Note: C3      Mode: N      │  ← Edit info (note pitch, step modifier)
│ ■■□■ ■□■□ ■■□■ □■□■       │  ← Step grid with edit cursor
│ ■■□■ ■□■□ ■■□■ □■□■       │
│ ■■□■ ■□■□ ■■□■ □■□■       │
│ ■■□■ ■□■□ ■■□■ □■□■       │
└────────────────────────────┘
```

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

- Additional scales and modes
- MIDI input for external sync
- Save/load patterns to EEPROM
- Additional arpeggio patterns (up, down, random, etc.)

## License

MIT License - See LICENSE file for details.
