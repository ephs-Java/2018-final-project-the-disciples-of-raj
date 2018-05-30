

#include <Wire.h>
#include <Adafruit_Trellis.h>
#include <Adafruit_UNTZtrument.h>
#include "MIDIUSB.h"

#define LED     13
#define CHANNEL 1  

#ifndef HELLA

Adafruit_Trellis     T[4];
Adafruit_UNTZtrument untztrument(&T[0], &T[1], &T[2], &T[3]);
const uint8_t        addr[] = { 0x70, 0x71,
                                0x72, 0x73 };
#else

Adafruit_Trellis     T[8];
Adafruit_UNTZtrument untztrument(&T[0], &T[1], &T[2], &T[3],
                                 &T[4], &T[5], &T[6], &T[7]);
const uint8_t        addr[] = { 0x70, 0x71, 0x72, 0x73,
                                0x74, 0x75, 0x76, 0x77 };
#endif 


#define WIDTH     ((sizeof(T) / sizeof(T[0])) * 2)
#define N_BUTTONS ((sizeof(T) / sizeof(T[0])) * 16)
#define LOWNOTE   ((128 - N_BUTTONS) / 2)

uint8_t       heart        = 0;  
unsigned long prevReadTime = 0L;
uint8_t  arr[8][8] = { {0,1,2,3,16,17,18,19},
                  {4,5,6,7,20,21,22,23},
                  {8,9,10,11,24,25,26,27},
                  {12,13,14,15,28,29,30,31},
                  {32,33,34,35,48,49,50,51},
                  {36,37,38,39,52,53,54,55},
                  {40,41,42,43,56,57,58,59},
                  {44,45,46,47,60,61,62,63} };

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
  TWBR = 12; // 400 KHz I2C on 16 MHz AVR
#endif
  untztrument.clear();
  untztrument.writeDisplay();
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

void loop() {
  unsigned long t = millis();
  if((t - prevReadTime) >= 20L) { // 20ms = min Trellis poll time
    if(untztrument.readSwitches()) { // Button state change?
      for(int i=0; i<sizeof(arr); i++) { // For each button...
        // Get column/row for button, convert to MIDI note number
        for(int j = 0; j< sizeof(arr[0]); j++){
        uint8_t x, y, note;
        untztrument.i2xy(arr[i][j], &x, &y);
        note = LOWNOTE + y * WIDTH + x;
        if(untztrument.justPressed(arr[x][y])) {
          
          noteOn(CHANNEL, note, 127);
          if(x==4&&y==0){
          untztrument.setLED(arr[x][y]);
          untztrument.setLED(arr[x+1][y]);
          untztrument.setLED(arr[x-1][y]);
          
                    Serial.println(x);
          Serial.println(y);
          }
          else{
          untztrument.setLED(arr[x][y]);
          untztrument.setLED(arr[x+1][y]);
          untztrument.setLED(arr[x-1][y]);
          untztrument.setLED(arr[x][y-1]);
          untztrument.setLED(arr[x][y+1]);
          Serial.println(x);
          Serial.println(y);
          }
           //cout << "note- "+ note + "x- " + x + "y- " + y);
          
          
        } else if(untztrument.justReleased(arr[x][y])) {
          noteOff(CHANNEL, note, 0);
                    if(x==4&&y==0){
          untztrument.clrLED(arr[x][y]);
          untztrument.clrLED(arr[x+1][y]);
          untztrument.clrLED(arr[x-1][y]);
                    Serial.println(x);
          Serial.println(y);
          }
          else{
          untztrument.clrLED(arr[x][y]);
          untztrument.clrLED(arr[x+1][y]);
          untztrument.clrLED(arr[x-1][y]);
          untztrument.clrLED(arr[x][y-1]);
          untztrument.clrLED(arr[x][y+1]);
                    Serial.println(x);
          Serial.println(y);
          }
        }
        }
      }
      untztrument.writeDisplay();
    }
    prevReadTime = t;
    digitalWrite(LED, ++heart & 32);
  }
}
