#include "arp.h"

Arp::Arp(Midi midi) {
  _midi = midi;
  _next_note_on = 0;
  _note_playing = 0;

  _update_bpm(120);
  _generate_steps();
}

void Arp::_generate_steps() {
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

void Arp::_update_bpm(uint8_t bpm) {
  _bpm = bpm;
  _note_delays_ms = 60000000UL / bpm / 4;
  _note_gate_ms = _note_delays_ms / 2;
}

void Arp::tick() {
  long now = micros();

  if(_note_playing > 0 && now - _next_note_off >= 0) {
    _midi.noteOff(_note_playing);
    _note_playing = 0;
  }

  if(_note_playing == 0 && now - _next_note_on >= 0) {
    _note_playing = 48;

    _midi.noteOn(_note_playing, 100);
    _next_note_off = now + _note_gate_ms;
    _next_note_on = now + _note_delays_ms;
  }

}