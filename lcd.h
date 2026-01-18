#pragma once

#include <U8g2lib.h>

// Forward declaration
class Arp;

// Menu selection enum (must match arp-machine.ino)
enum MenuSelection : uint8_t { SEL_LENGTH = 0, SEL_GENERATOR, SEL_BPM, SEL_ROOT, SEL_SCALE, SEL_OCTAVE, SEL_DENSITY, SEL_EDIT, SEL_COUNT };

/**
Display used:
XTVTX 3PCS 0.96 Inch OLED Module 12864 128x64 Driver IIC I2C
*/

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;


class Lcd {

  public:

    Lcd(Arp& arp, MenuSelection& selection);
    void setup();
    void refresh(unsigned long now);

  private:
    Arp& _arp;
    MenuSelection& _selection;

    // Cached state for change detection
    uint8_t _last_step;
    uint8_t _last_edit_step;
    bool _last_edit_mode;
    MenuSelection _last_selection;
    uint8_t _last_length;
    uint8_t _last_generator;
    uint8_t _last_bpm;
    uint8_t _last_root_note;
    uint8_t _last_scale;
    uint8_t _last_octave_range;
    uint8_t _last_density;
    int8_t _last_edit_note;
    int8_t _last_edit_mod;
    uint8_t _last_edit_submode;

};