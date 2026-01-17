#pragma once

#include <U8g2lib.h>

// Forward declaration
class Arp;

/**
Display used:
XTVTX 3PCS 0.96 Inch OLED Module 12864 128x64 Driver IIC I2C
*/

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;


class Lcd {

  public:

    Lcd(Arp& arp);
    void setup();
    void refresh(unsigned long now);

  private:
    Arp& _arp;
    unsigned long _last_update;
    int8_t _last_step;

};