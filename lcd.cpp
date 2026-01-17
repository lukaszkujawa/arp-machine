#include <Arduino.h>
#include <Wire.h>
#include "lcd.h"
#include "arp.h"

// Define the u8g2 object
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Header layout
static const uint8_t HEADER_HEIGHT = 12;

// Grid layout constants
static const uint8_t STEP_WIDTH = 7;
static const uint8_t STEP_HEIGHT = 6;
static const uint8_t STEP_GAP = 1;
static const uint8_t GRID_X_OFFSET = 0;
static const uint8_t GRID_HEIGHT = 4 * STEP_HEIGHT + 3 * STEP_GAP;  // 27px
static const uint8_t GRID_Y_OFFSET = 64 - GRID_HEIGHT;  // Start at bottom
static const uint8_t STEPS_PER_ROW = 16;

// Note names
static const char* NOTE_NAMES[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// Scale names
static const char* SCALE_NAMES[] = {"Major", "Minor", "Dorian", "Penta", "Harm Min"};

// Octave range names
static const char* OCT_RANGE_NAMES[] = {"3", "2-3", "3-4", "2-4"};

Lcd::Lcd(Arp& arp) : _arp(arp) {
  _last_update = 0;
  _last_step = -1;
}

void Lcd::setup() {
  Wire.begin();
  Wire.setClock(400000);  // 400kHz fast mode I2C
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
}

void Lcd::refresh(unsigned long now) {
  // Only redraw when step changes
  if (_arp.x == _last_step) {
    return;
  }
  _last_step = _arp.x;

  u8g2.clearBuffer();

  // Draw header bar (filled)
  u8g2.drawBox(0, 0, 128, HEADER_HEIGHT);

  // Draw header text (inverted)
  u8g2.setDrawColor(0);
  u8g2.drawStr(2, 10, "ARP Machine");

  // Draw BPM on right side
  char bpm_str[8];
  snprintf(bpm_str, sizeof(bpm_str), "%d BPM", _arp.getBpm());
  uint8_t bpm_width = u8g2.getStrWidth(bpm_str);
  u8g2.drawStr(126 - bpm_width, 10, bpm_str);

  u8g2.setDrawColor(1);

  // Draw root note, scale, and octave range
  char info_str[24];
  snprintf(info_str, sizeof(info_str), "%s %s  Oct%s",
           NOTE_NAMES[_arp.root_note],
           SCALE_NAMES[_arp.scale],
           OCT_RANGE_NAMES[_arp.octaveRange]);
  u8g2.drawStr(2, 24, info_str);

  // Draw density
  char density_str[8];
  snprintf(density_str, sizeof(density_str), "%d%%", _arp.density);
  uint8_t density_width = u8g2.getStrWidth(density_str);
  u8g2.drawStr(126 - density_width, 24, density_str);

  // Draw 4 rows of 16 steps (64 steps total) at bottom
  for (uint8_t row = 0; row < 4; row++) {
    for (uint8_t col = 0; col < STEPS_PER_ROW; col++) {
      uint8_t step_index = row * STEPS_PER_ROW + col;

      uint8_t x = GRID_X_OFFSET + col * (STEP_WIDTH + STEP_GAP);
      uint8_t y = GRID_Y_OFFSET + row * (STEP_HEIGHT + STEP_GAP);

      bool has_note = (_arp.steps[step_index] > 0);
      bool is_current = (step_index == _arp.x);

      if (is_current) {
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

  u8g2.sendBuffer();
}
