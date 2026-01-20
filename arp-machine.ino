#include <ESP8266WiFi.h>
#include <Arduino.h>

#include "midi.h"
#include "arp.h"
#include "lcd.h"  // Includes MenuSelection enum

Midi midi;

// 4 simultaneous sequencers
Arp arp0(midi);
Arp arp1(midi);
Arp arp2(midi);
Arp arp3(midi);
Arp* arps[4] = {&arp0, &arp1, &arp2, &arp3};

uint8_t currentPage = 0;
MenuSelection currentSelection = SEL_PAGE;
Lcd lcd(&arp0, currentSelection, currentPage);

// Helper to get current arp
#define arp (*arps[currentPage])

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
const uint32_t LONG_PRESS_MS = 3000;    // Long press threshold for clear sequence

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

// Regen button polling state (for long-press detection)
uint8_t lastRegenBtn = HIGH;
uint32_t regenPressStartMs = 0;
uint32_t regenLastChangeMs = 0;
bool regenLongPressTriggered = false;

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
    case SEL_PAGE: {
      int8_t newPage = currentPage + (dir > 0 ? 1 : -1);
      if (newPage < 0) newPage = 3;
      if (newPage > 3) newPage = 0;
      currentPage = newPage;
      lcd.setArp(arps[currentPage]);
      break;
    }
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
        case EDIT_DIV:      arp.adjustCurrentStepDiv(dir); break;
        case EDIT_COND:     arp.adjustCurrentStepCond(dir); break;
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

void handleRegenButton() {
  uint8_t btn = digitalRead(BTN_REGEN);
  uint32_t now = millis();

  // Debounce: ignore changes within debounce window
  if (btn != lastRegenBtn && (now - regenLastChangeMs) < BTN_DEBOUNCE_MS) {
    return;
  }

  if (btn == LOW && lastRegenBtn == HIGH) {
    // Button just pressed - start tracking
    regenPressStartMs = now;
    regenLastChangeMs = now;
    regenLongPressTriggered = false;
  } else if (btn == LOW) {
    // Button held down - check for long press
    if (!regenLongPressTriggered && (now - regenPressStartMs >= LONG_PRESS_MS)) {
      // Long press detected - clear sequence
      arp.clearSequence();
      regenLongPressTriggered = true;
    }
  } else if (btn == HIGH && lastRegenBtn == LOW) {
    // Button released
    regenLastChangeMs = now;
    if (!regenLongPressTriggered) {
      // Short press - regenerate or cycle edit sub-mode
      if (arp.editMode) {
        arp.cycleEditSubMode();
      } else {
        arp.regenerate();
      }
    }
  }

  lastRegenBtn = btn;
}

void handleButtons() {
  uint8_t mask = pendingMask;
  if (!mask) return;

  if (mask & 0x02) {
    pendingMask &= ~0x02;
    // Pause/unpause all sequencers
    for (uint8_t i = 0; i < 4; i++) {
      arps[i]->togglePause();
    }
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

  // Initialize 4 sequencers with different channels
  // Arp 0: CH1 (default), has generated sequence
  // Arps 1-3: CH2-4, empty sequences
  for (uint8_t i = 1; i < 4; i++) {
    arps[i]->channel = i;
    arps[i]->clearSequence();
  }

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

  attachInterrupt(digitalPinToInterrupt(BTN_PAUSE), isrPause, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_SWITCH), isrSwitch, FALLING);
}

void loop() {
  unsigned long now = micros();
  handleEncoder();
  handleEncButton();
  handleRegenButton();
  handleButtons();

  // Tick all 4 sequencers
  for (uint8_t i = 0; i < 4; i++) {
    arps[i]->tick(now);
  }

  lcd.refresh(now);
}
