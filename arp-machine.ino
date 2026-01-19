#include <ESP8266WiFi.h>
#include <Arduino.h>

#include "midi.h"
#include "arp.h"
#include "lcd.h"  // Includes MenuSelection enum

Midi midi;
Arp arp(midi);
MenuSelection currentSelection = SEL_CHANNEL;
Lcd lcd(arp, currentSelection);

// Rotary encoder pins
const uint8_t ENC_CLK = 2;
const uint8_t ENC_DT = 16;
const uint8_t ENC_BTN = 0;

// Button pins
const uint8_t BTN_PAUSE = 12;     // Pause/resume ARP
const uint8_t BTN_SWITCH = 13;    // Switch menu selection
const uint8_t BTN_REGEN = 14;     // Regenerate sequence

// Timing
const uint32_t BTN_DEBOUNCE_MS = 200;
const uint32_t ENC_DEBOUNCE_MS = 2;

// Encoder state
uint8_t lastClk;
uint32_t lastEncoderMs = 0;

// Button state
volatile uint32_t lastIsrMs[3] = {0, 0, 0};
volatile uint8_t pendingMask = 0;

// Encoder button polling state
uint8_t lastEncBtn = HIGH;
uint32_t lastEncBtnMs = 0;

void IRAM_ATTR isrRegen() {
  uint32_t now = millis();
  if (now - lastIsrMs[0] < BTN_DEBOUNCE_MS) return;
  lastIsrMs[0] = now;
  pendingMask |= 0x01;
}

void IRAM_ATTR isrPause() {
  uint32_t now = millis();
  if (now - lastIsrMs[1] < BTN_DEBOUNCE_MS) return;
  lastIsrMs[1] = now;
  pendingMask |= 0x02;
}

void IRAM_ATTR isrSwitch() {
  uint32_t now = millis();
  if (now - lastIsrMs[2] < BTN_DEBOUNCE_MS) return;
  lastIsrMs[2] = now;
  pendingMask |= 0x04;
}

void handleEncoder() {
  uint8_t clk = digitalRead(ENC_CLK);
  if (clk == lastClk) return;

  uint32_t now = millis();
  if (now - lastEncoderMs < ENC_DEBOUNCE_MS) {
    lastClk = clk;
    return;
  }

  // Only act on falling edge
  if (clk == LOW) {
    int8_t delta = (digitalRead(ENC_DT) == HIGH) ? 1 : -1;

    switch (currentSelection) {
      case SEL_LENGTH:    arp.adjustLength(delta); break;
      case SEL_GENERATOR: arp.adjustGenerator(delta); break;
      case SEL_BPM:       arp.adjustBpm(delta); break;
      case SEL_CHANNEL:   arp.adjustChannel(delta); break;
      case SEL_ROOT:      arp.adjustRootNote(delta); break;
      case SEL_SCALE:     arp.adjustScale(delta); break;
      case SEL_OCTAVE:    arp.adjustOctaveRange(delta); break;
      case SEL_DENSITY:   arp.adjustDensity(delta); break;
      case SEL_EDIT:
        switch (arp.editSubMode) {
          case EDIT_SEQUENCE: arp.moveEditCursor(delta); break;
          case EDIT_NOTE:     arp.adjustCurrentStepNote(delta); break;
          case EDIT_MODE:     arp.adjustCurrentStepMod(delta); break;
        }
        break;
      default: break;
    }
    lastEncoderMs = now;
  }
  lastClk = clk;
}

void handleEncButton() {
  uint8_t btn = digitalRead(ENC_BTN);
  if (btn == lastEncBtn) return;

  uint32_t now = millis();
  if (now - lastEncBtnMs < BTN_DEBOUNCE_MS) {
    lastEncBtn = btn;
    return;
  }

  // Act on falling edge (button press)
  if (btn == LOW) {
    if (currentSelection == SEL_EDIT) {
      // Toggle step on/off in edit mode
      arp.toggleCurrentStep();
    }
  }

  lastEncBtn = btn;
  lastEncBtnMs = now;
}

void handleButtons() {
  uint8_t mask = pendingMask;
  if (!mask) return;

  if (mask & 0x01) {
    pendingMask &= ~0x01;
    if (arp.editMode) {
      // In edit mode, cycle through sub-modes (sequence -> note -> mode -> sequence)
      arp.cycleEditSubMode();
    } else {
      arp.regenerate();
    }
  }
  if (mask & 0x02) {
    pendingMask &= ~0x02;
    arp.togglePause();
  }
  if (mask & 0x04) {
    pendingMask &= ~0x04;
    // Cycle through menu selections (including edit mode)
    currentSelection = (MenuSelection)((currentSelection + 1) % SEL_COUNT);
    // Sync edit mode state with selection
    arp.editMode = (currentSelection == SEL_EDIT);
    if (arp.editMode) {
      arp.editStep = 0;  // Start at step 0
      arp.editSubMode = EDIT_SEQUENCE;  // Reset to sequence navigation
    }
  }
}

void setup() {
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(10);

  pinMode(LED_BUILTIN, OUTPUT);
  midi.setup();
  lcd.setup();

  // Encoder pins
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_BTN, INPUT_PULLUP);
  lastClk = digitalRead(ENC_CLK);

  // Button pins
  pinMode(BTN_PAUSE, INPUT_PULLUP);
  pinMode(BTN_SWITCH, INPUT_PULLUP);
  pinMode(BTN_REGEN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BTN_REGEN), isrRegen, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_PAUSE), isrPause, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_SWITCH), isrSwitch, FALLING);
}

void loop() {
  unsigned long now = micros();
  handleEncoder();
  handleEncButton();
  handleButtons();
  arp.tick(now);
  lcd.refresh(now);
}
