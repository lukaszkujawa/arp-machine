#include "arp.h"

Arp::Arp(Midi midi) {
  _midi = midi;
  _last = micros();

  for(u_int x = 0 ; x < 32 ; x++) {
    if(x % 4 == 0) {
      steps[x] = 48;
    }
    else if((x+2) % 4 == 0) {
      steps[x] = 60;
    }
    else {
      steps[x] = 0;
    }
    
  }

}

/*

void updateBPM(uint16_t _bpm) {
  BPM = _bpm;
  NOTE_DIV_MS  = (60000UL / BPM) / NODE_DIV_1_16;
  NOTE_GATE_MS = NOTE_DIV_MS / 2;
}

*/

void Arp::tick(uint8_t bpm) {

}