
#include <Wire.h>
#include <Adafruit_Trellis.h>
#include <Adafruit_UNTZtrument.h>
#include "MIDIUSB.h"

#define LED 13 

#ifndef HELLA

Adafruit_Trellis     T[4];
Adafruit_UNTZtrument untztrument(&T[0], &T[1], &T[2], &T[3]);
const uint8_t        addr[] = { 0x70, 0x71, 0x72, 0x73 };
#else

Adafruit_Trellis     T[8];
Adafruit_UNTZtrument untztrument(&T[0], &T[1], &T[2], &T[3],
                                 &T[4], &T[5], &T[6], &T[7]);
const uint8_t        addr[] = { 0x70, 0x71, 0x72, 0x73,
                                0x74, 0x75, 0x76, 0x77 };
#endif // HELLA

#define WIDTH     ((sizeof(T) / sizeof(T[0])) * 2)
#define N_BUTTONS ((sizeof(T) / sizeof(T[0])) * 16)

enc e(4, 5);

uint8_t       grid[WIDTH];                
uint8_t       heart        = 0,           
              col          = WIDTH-1;     
unsigned int  bpm          = 240;          
unsigned long beatInterval = 60000L / bpm, 
              prevBeatTime = 0L,         
              prevReadTime = 0L;           


static const uint8_t PROGMEM
  note[8]    = {  72, 71, 69, 67, 65, 64, 62,  60 },
  channel[8] = {   1,  1,  1,  1,  1,  1,  1,   1 },
  bitmask[8] = {   1,  2,  4,  8, 16, 32, 64, 128 };

void setup() {
  pinMode(LED, OUTPUT);
#ifndef HELLA
  untztrument.begin(addr[0], addr[1], addr[2], addr[3]);
#else
  untztrument.begin(addr[0], addr[1], addr[2], addr[3],
                    addr[4], addr[5], addr[6], addr[7]);
#endif 
 
#ifdef ARDUINO_ARCH_SAMD
  Wire.setClock(400000L);
#endif
#ifdef __AVR__
  TWBR = 12; 
#endif
  untztrument.clear();
  untztrument.writeDisplay();
  memset(grid, 0, sizeof(grid));
  enc::begin();                     
  e.setBounds(60 * 4, 480 * 4 + 3); 
  e.setValue(bpm * 4);              
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}


void line(uint8_t x, boolean set) {
  for(uint8_t mask=1, y=0; y<8; y++, mask <<= 1) {
    uint8_t i = untztrument.xy2i(x, y);
    if(set || (grid[x] & mask)) untztrument.setLED(i);
    else                        untztrument.clrLED(i);
  }
}

void loop() {
  uint8_t       mask;
  boolean       refresh = false;
  unsigned long t       = millis();

  enc::poll(); 

  if((t - prevReadTime) >= 20L) { 
    if(untztrument.readSwitches()) {
      for(uint8_t i=0; i<N_BUTTONS; i++) { 
        uint8_t x, y;
        untztrument.i2xy(i, &x, &y);
        mask = pgm_read_byte(&bitmask[y]);
        if(untztrument.justPressed(i)) {
          if(grid[x] & mask) {
            grid[x] &= ~mask;
            untztrument.clrLED(i);
            noteOff(pgm_read_byte(&channel[y]), pgm_read_byte(&note[y]), 127);
          } else { // Turn on
            grid[x] |= mask;
            untztrument.setLED(i);
          }
          refresh = true;
        }
      }
    }
    prevReadTime = t;
    digitalWrite(LED, ++heart & 32); 
  }

  if((t - prevBeatTime) >= beatInterval) { // Next beat?
    // Turn off old column
    line(col, false);
    for(uint8_t row=0, mask=1; row<8; row++, mask <<= 1) {
      if(grid[col] & mask) {
        noteOff(pgm_read_byte(&channel[row]), pgm_read_byte(&note[row]), 127);
      }
    }
    // Advance column counter, wrap around
    if(++col >= WIDTH) col = 0;
    // Turn on new column
    line(col, true);
    for(uint8_t row=0, mask=1; row<8; row++, mask <<= 1) {
      if(grid[col] & mask) {
          noteOn(pgm_read_byte(&channel[row]), pgm_read_byte(&note[row]), 127);
      }
    }
    prevBeatTime = t;
    refresh      = true;
    bpm          = e.getValue() / 4; // Div for encoder detents
    beatInterval = 60000L / bpm;
  }

  if(refresh) untztrument.writeDisplay();

}
