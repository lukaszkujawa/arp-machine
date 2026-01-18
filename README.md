# ARP Machine

A hardware MIDI arpeggiator built on the ESP8266 microcontroller. Generates rhythmic note patterns and outputs standard MIDI messages over serial at 31250 baud.

## Features

- **64-step sequencer** with 4 rows of 16 steps
- **5 musical scales**: Major, Minor, Dorian, Pentatonic, Harmonic Minor
- **Adjustable parameters**: BPM (40-240), root note, scale, octave range, note density
- **Sequence editing**: Toggle individual steps on/off with random scale-aware note generation
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
| BPM | Adjust tempo (±5 BPM per click) |
| Root | Change root note (C through B) |
| Scale | Cycle through 5 scales |
| Octave | Change octave range |
| Density | Adjust note density (10-90%) |
| Edit | Move cursor through sequence |

### Sequence Editing

When in **Edit** mode:
- **Rotate encoder**: Move cursor left/right through 64 steps
- **Press encoder**: Toggle step on/off
- Enabling a step generates a random note following current root, scale, and octave settings

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
├── arp-machine.ino    # Main entry point, input handling, event loop
├── arp.h / arp.cpp    # Arpeggiator engine (timing, patterns, playback)
├── midi.h / midi.cpp  # MIDI serial communication
├── lcd.h / lcd.cpp    # Display rendering
└── CLAUDE.md          # AI assistant context
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
- 64-step pattern buffer (`steps[64]`)
- Microsecond timing using `micros()` for note on/off scheduling
- Scale-aware note generation with configurable density
- Pattern generation creates structured rhythms with variations

**Key members:**
- `steps[64]` - Note values (0 = rest, >0 = MIDI note number)
- `x` - Current playhead position
- `_next_note_on` / `_next_note_off` - Scheduled timing thresholds
- `_generate_steps()` - Creates rhythmic patterns with probability-based placement

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
│ ARP Machine          ● 120 │  ← Header (title, beat indicator, BPM)
│ C  Major  Oct2-3      45%  │  ← Settings row (root, scale, octave, density)
│ ■■□■ ■□■□ ■■□■ □■□■       │  ← Step grid (4 rows × 16 cols)
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
