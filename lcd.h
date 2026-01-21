#pragma once

#include <U8g2lib.h>

// Forward declaration
class Arp;

// Menu selection enum (must match arp-machine.ino)
enum MenuSelection : uint8_t { SEL_PAGE = 0, SEL_CHANNEL, SEL_SWING, SEL_LENGTH, SEL_GENERATOR, SEL_BPM, SEL_ROOT, SEL_SCALE, SEL_OCTAVE, SEL_DENSITY, SEL_EDIT, SEL_COUNT };

/**
Display used:
XTVTX 3PCS 0.96 Inch OLED Module 12864 128x64 Driver IIC I2C
*/

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;


class Lcd {

  public:

    Lcd(Arp* arp, MenuSelection& selection, uint8_t& currentPage);
    void setup();
    void showIntro();
    void refresh(unsigned long now);
    void setArp(Arp* arp);

  private:
    Arp* _arp;
    MenuSelection& _selection;
    uint8_t& _currentPage;

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
    uint8_t _last_channel;
    uint8_t _last_swing;
    int8_t _last_edit_note;
    uint8_t _last_edit_fx;
    uint8_t _last_edit_cond;
    uint8_t _last_edit_submode;
    uint8_t _last_page;

};