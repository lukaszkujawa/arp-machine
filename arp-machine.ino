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
const uint32_t ENC_DEBOUNCE_US = 1500;  // Microsecond debounce for encoder

// Encoder state - quadrature decoder
uint8_t encState = 0;           // 2-bit state: (CLK << 1) | DT
int8_t encAccum = 0;            // Accumulated pulses (need 4 for one detent)
uint32_t lastEncoderUs = 0;
uint32_t lastDetentUs = 0;      // Time of last completed detent (for acceleration)

// Quadrature lookup table: [oldState << 2 | newState] -> delta
// Valid transitions give +1 or -1, invalid give 0
const int8_t ENC_TABLE[16] = {
   0, -1,  1,  0,   // from state 0 (00)
   1,  0,  0, -1,   // from state 1 (01)
  -1,  0,  0,  1,   // from state 2 (10)
   0,  1, -1,  0    // from state 3 (11)
};

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
  uint32_t now = micros();
  if ((now - lastEncoderUs) < ENC_DEBOUNCE_US) return;

  uint8_t newState = (digitalRead(ENC_CLK) << 1) | digitalRead(ENC_DT);
  if (newState == encState) return;

  // Lookup delta from state transition table
  int8_t delta = ENC_TABLE[(encState << 2) | newState];
  encState = newState;
  lastEncoderUs = now;

  if (delta == 0) return;  // Invalid transition, ignore

  // Accumulate pulses - most encoders need 4 transitions per detent
  encAccum += delta;
  if (encAccum < 4 && encAccum > -4) return;

  // We have a full detent worth of movement
  int8_t dir = (encAccum > 0) ? 1 : -1;
  encAccum = 0;

  // Acceleration: faster rotation = bigger steps
  uint32_t interval = now - lastDetentUs;
  lastDetentUs = now;
  int8_t mult = 1;
  if (interval < 30000)      mult = 10;  // Very fast: <30ms
  else if (interval < 60000) mult = 5;   // Fast: <60ms
  else if (interval < 100000) mult = 2;  // Medium: <100ms
  dir *= mult;

  switch (currentSelection) {
    case SEL_CHANNEL:   arp.adjustChannel(dir); break;
    case SEL_SWING:     arp.adjustSwing(dir); break;
    case SEL_LENGTH:    arp.adjustLength(dir); break;
    case SEL_GENERATOR: arp.adjustGenerator(dir); break;
    case SEL_BPM:       arp.adjustBpm(dir); break;
    case SEL_ROOT:      arp.adjustRootNote(dir); break;
    case SEL_SCALE:     arp.adjustScale(dir); break;
    case SEL_OCTAVE:    arp.adjustOctaveRange(dir); break;
    case SEL_DENSITY:   arp.adjustDensity(dir); break;
    case SEL_EDIT:
      switch (arp.editSubMode) {
        case EDIT_SEQUENCE: arp.moveEditCursor(dir); break;
        case EDIT_NOTE:     arp.adjustCurrentStepNote(dir); break;
        case EDIT_MODE:     arp.adjustCurrentStepMod(dir); break;
      }
      break;
    default: break;
  }
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
  lcd.showIntro();
  delay(2000);

  // Encoder pins
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_BTN, INPUT_PULLUP);
  encState = (digitalRead(ENC_CLK) << 1) | digitalRead(ENC_DT);
  lastDetentUs = micros();

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
