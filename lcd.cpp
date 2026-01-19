#include <Arduino.h>
#include <Wire.h>
#include "lcd.h"
#include "arp.h"

// Define the u8g2 object
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Header layout
static const uint8_t HEADER_HEIGHT = 12;

// Grid layout constants
static const uint8_t STEP_WIDTH = 6;
static const uint8_t STEP_HEIGHT = 6;
static const uint8_t STEP_GAP = 1;
static const uint8_t BEAT_GAP = 2;  // Extra gap between quarter notes
static const uint8_t GRID_X_OFFSET = 5;  // Center the 118px grid on 128px screen
static const uint8_t GRID_HEIGHT = 4 * STEP_HEIGHT + 3 * STEP_GAP;  // 27px
static const uint8_t GRID_Y_OFFSET = 61 - GRID_HEIGHT;  // Start at bottom
static const uint8_t STEPS_PER_ROW = 16;

// Note names
static const char* NOTE_NAMES[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// Scale names
static const char* SCALE_NAMES[] = {"Major", "Minor", "Dorian", "Penta", "Harm Min"};

// Octave range names
static const char* OCT_RANGE_NAMES[] = {"3", "2-3", "3-4", "2-4"};

// Generator names (short form for header)
static const char* GEN_NAMES[] = {"D", "C"};

// Convert MIDI note to string (e.g., 60 -> "C4", 61 -> "C#4")
static void midiNoteToString(int8_t note, char* buf, size_t bufSize) {
  if (note <= 0) {
    snprintf(buf, bufSize, "---");
    return;
  }
  uint8_t octave = (note / 12) - 1;
  uint8_t noteInOctave = note % 12;
  snprintf(buf, bufSize, "%s%d", NOTE_NAMES[noteInOctave], octave);
}

// Step mod names
static const char* MOD_NAMES[] = {"N", "R2", "R3", "1:2", "1:3", "1:4", "10%", "25%", "50%", "75%"};

// Convert step mod to mode string
static const char* modToString(int8_t mod) {
  if (mod < 0 || mod >= STEP_MOD_COUNT) return "N";
  return MOD_NAMES[mod];
}

Lcd::Lcd(Arp& arp, MenuSelection& selection) : _arp(arp), _selection(selection) {
  // Initialize all cached state to invalid/different values to force initial draw
  _last_step = 255;
  _last_edit_step = 255;
  _last_edit_mode = false;
  _last_selection = SEL_COUNT;
  _last_length = 0;
  _last_generator = 255;
  _last_bpm = 0;
  _last_root_note = 255;
  _last_scale = 255;
  _last_octave_range = 255;
  _last_density = 0;
  _last_edit_note = -128;
  _last_edit_mod = -128;
  _last_edit_submode = 255;
}

void Lcd::setup() {
  Wire.begin();
  Wire.setClock(400000);  // 400kHz fast mode I2C
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
}

void Lcd::refresh(unsigned long now) {
  // Get current edit step values for comparison
  int8_t current_edit_note = _arp.editMode ? _arp.steps[_arp.editStep] : 0;
  int8_t current_edit_mod = _arp.editMode ? _arp.steps_mods[_arp.editStep] : 0;
  uint8_t current_edit_submode = _arp.editMode ? _arp.editSubMode : 0;

  // Redraw when any displayed state changes
  if (_arp.x == _last_step &&
      _selection == _last_selection &&
      _arp.editMode == _last_edit_mode &&
      _arp.editStep == _last_edit_step &&
      _arp.length == _last_length &&
      _arp.generator == _last_generator &&
      _arp.getBpm() == _last_bpm &&
      _arp.root_note == _last_root_note &&
      _arp.scale == _last_scale &&
      _arp.octaveRange == _last_octave_range &&
      _arp.density == _last_density &&
      current_edit_note == _last_edit_note &&
      current_edit_mod == _last_edit_mod &&
      current_edit_submode == _last_edit_submode) {
    return;
  }
  _last_step = _arp.x;
  _last_selection = _selection;
  _last_edit_mode = _arp.editMode;
  _last_edit_step = _arp.editStep;
  _last_length = _arp.length;
  _last_generator = _arp.generator;
  _last_bpm = _arp.getBpm();
  _last_root_note = _arp.root_note;
  _last_scale = _arp.scale;
  _last_octave_range = _arp.octaveRange;
  _last_density = _arp.density;
  _last_edit_note = current_edit_note;
  _last_edit_mod = current_edit_mod;
  _last_edit_submode = current_edit_submode;

  u8g2.clearBuffer();

  // Draw header bar (filled)
  u8g2.drawBox(0, 0, 128, HEADER_HEIGHT);

  // Draw header text (inverted)
  u8g2.setDrawColor(0);
  u8g2.drawStr(2, 10, "arpM");

  if (_arp.editMode) {
    u8g2.setDrawColor(1);
    u8g2.drawFrame( 0, 30, 127, 34 );
    u8g2.setDrawColor(0);
  }

  // Draw tempo indicator circle (blinks on quarter notes)
  if (_arp.x % 4 == 0) {
    u8g2.drawDisc(102, 6, 3);  // Filled circle when on beat
  } else {
    u8g2.drawCircle(102, 6, 3);  // Empty circle when off beat
  }

  // Draw BPM on right side (highlight if selected)
  char bpm_str[8];
  snprintf(bpm_str, sizeof(bpm_str), "%d", _arp.getBpm());
  uint8_t bpm_width = u8g2.getStrWidth(bpm_str);
  uint8_t bpm_x = 126 - bpm_width;

  if (_selection == SEL_BPM) {
    // Draw white box behind BPM to highlight it
    u8g2.setDrawColor(0);
    u8g2.drawBox(bpm_x - 2, 1, bpm_width + 4, 10);
    u8g2.setDrawColor(1);
    u8g2.drawStr(bpm_x, 10, bpm_str);
  } else {
    u8g2.drawStr(bpm_x, 10, bpm_str);
  }

  // Draw generator before BPM (highlight if selected)
  const char* gen_str = GEN_NAMES[_arp.generator];
  uint8_t gen_width = u8g2.getStrWidth(gen_str);
  uint8_t gen_x = bpm_x - gen_width - 14;

  u8g2.setDrawColor(0);
  if (_selection == SEL_GENERATOR) {
    u8g2.drawBox(gen_x - 2, 1, gen_width + 4, 10);
    u8g2.setDrawColor(1);
    u8g2.drawStr(gen_x, 10, gen_str);
  } else {
    u8g2.drawStr(gen_x, 10, gen_str);
  }

  // Draw length before generator (highlight if selected)
  char len_str[4];
  snprintf(len_str, sizeof(len_str), "%d", _arp.length);
  uint8_t len_width = u8g2.getStrWidth(len_str);
  uint8_t len_x = gen_x - len_width - 6;

  u8g2.setDrawColor(0);
  if (_selection == SEL_LENGTH) {
    u8g2.drawBox(len_x - 2, 1, len_width + 4, 10);
    u8g2.setDrawColor(1);
    u8g2.drawStr(len_x, 10, len_str);
  } else {
    u8g2.drawStr(len_x, 10, len_str);
  }

  u8g2.setDrawColor(1);

  // Draw settings row OR edit info box
  if (_arp.editMode) {
    // Draw edit info box (covers settings row area)
    char note_buf[6];
    midiNoteToString(_arp.steps[_arp.editStep], note_buf, sizeof(note_buf));
    const char* mode_str = modToString(_arp.steps_mods[_arp.editStep]);

    // Draw box background
    u8g2.drawFrame(0, 17, 127, 13);

    // Draw note label and value (highlight if EDIT_NOTE sub-mode)
    if (_arp.editSubMode == EDIT_NOTE) {
      u8g2.drawBox(4, 17, 56, 12);
      u8g2.setDrawColor(0);
    }
    u8g2.drawStr(6, 27, "Note:");
    u8g2.drawStr(42, 27, note_buf);
    u8g2.setDrawColor(1);

    // Draw mode label and value (highlight if EDIT_MODE sub-mode)
    if (_arp.editSubMode == EDIT_MODE) {
      u8g2.drawBox(64, 17, 60, 12);
      u8g2.setDrawColor(0);
    }
    u8g2.drawStr(68, 27, "Mode:");
    u8g2.drawStr(104, 27, mode_str);
    u8g2.setDrawColor(1);
  } else {
    // Draw normal settings row with highlighting for selected item
    uint8_t xPos = 2;
    char buf[12];

    // Root note
    snprintf(buf, sizeof(buf), "%s", NOTE_NAMES[_arp.root_note]);
    if (_selection == SEL_ROOT) {
      uint8_t w = u8g2.getStrWidth(buf);
      u8g2.drawBox(xPos - 1, 17, w + 2, 12);
      u8g2.setDrawColor(0);
    }
    u8g2.drawStr(xPos, 27, buf);
    u8g2.setDrawColor(1);
    xPos += u8g2.getStrWidth(buf) + 4;

    // Scale
    snprintf(buf, sizeof(buf), "%s", SCALE_NAMES[_arp.scale]);
    if (_selection == SEL_SCALE) {
      uint8_t w = u8g2.getStrWidth(buf);
      u8g2.drawBox(xPos - 1, 17, w + 2, 12);
      u8g2.setDrawColor(0);
    }
    u8g2.drawStr(xPos, 27, buf);
    u8g2.setDrawColor(1);
    xPos += u8g2.getStrWidth(buf) + 4;

    // Octave range
    snprintf(buf, sizeof(buf), "Oct%s", OCT_RANGE_NAMES[_arp.octaveRange]);
    if (_selection == SEL_OCTAVE) {
      uint8_t w = u8g2.getStrWidth(buf);
      u8g2.drawBox(xPos - 1, 17, w + 2, 12);
      u8g2.setDrawColor(0);
    }
    u8g2.drawStr(xPos, 27, buf);
    u8g2.setDrawColor(1);

    // Draw density on the right
    char density_str[8];
    snprintf(density_str, sizeof(density_str), "%d%%", _arp.density);
    uint8_t density_width = u8g2.getStrWidth(density_str);
    uint8_t density_x = 126 - density_width;
    if (_selection == SEL_DENSITY) {
      u8g2.drawBox(density_x - 1, 17, density_width + 2, 12);
      u8g2.setDrawColor(0);
    }
    u8g2.drawStr(density_x, 27, density_str);
    u8g2.setDrawColor(1);
  }

  // Draw 4 rows of 16 steps (64 steps total) at bottom
  for (uint8_t row = 0; row < 4; row++) {
    for (uint8_t col = 0; col < STEPS_PER_ROW; col++) {
      uint8_t step_index = row * STEPS_PER_ROW + col;

      // Add extra gap after every 4 steps for beat grouping
      uint8_t beat_gap = (col / 4) * BEAT_GAP;
      uint8_t x = GRID_X_OFFSET + col * (STEP_WIDTH + STEP_GAP) + beat_gap;
      uint8_t y = GRID_Y_OFFSET + row * (STEP_HEIGHT + STEP_GAP);

      bool is_disabled = (step_index >= _arp.length);

      if (is_disabled) {
        // Disabled step: just a bottom line
        u8g2.drawHLine(x, y + STEP_HEIGHT - 1, STEP_WIDTH);
      } else {
        bool has_note = (_arp.steps[step_index] > 0);
        bool is_current = (step_index == _arp.x);
        bool is_edit_cursor = (_arp.editMode && step_index == _arp.editStep);

        if (is_edit_cursor) {
          if (has_note) {
            // Edit cursor over note: filled box with black inner frame
            u8g2.drawBox(x, y, STEP_WIDTH, STEP_HEIGHT);
            u8g2.setDrawColor(0);
            u8g2.drawFrame(x + 2, y + 2, STEP_WIDTH - 4, STEP_HEIGHT - 4);
            u8g2.setDrawColor(1);
          } else {
            // Edit cursor over empty: double border frame
            u8g2.drawFrame(x, y, STEP_WIDTH, STEP_HEIGHT);
            u8g2.drawFrame(x + 1, y + 1, STEP_WIDTH - 2, STEP_HEIGHT - 2);
          }
        } else if (is_current) {
          // Current step: draw filled box with inverted inner if has note
          u8g2.drawBox(x, y, STEP_WIDTH, STEP_HEIGHT);
          if (has_note) {
            // Invert inner area to show note within current step
            u8g2.setDrawColor(0);
            u8g2.drawBox(x + 2, y + 2, STEP_WIDTH - 4, STEP_HEIGHT - 4);
            u8g2.setDrawColor(1);
          }
        } else if (has_note) {
          // Has note: filled box
          u8g2.drawBox(x, y, STEP_WIDTH, STEP_HEIGHT);
        } else {
          // Empty step: just outline
          u8g2.drawFrame(x, y, STEP_WIDTH, STEP_HEIGHT);
        }
      }
    }
  }

  u8g2.sendBuffer();
}
