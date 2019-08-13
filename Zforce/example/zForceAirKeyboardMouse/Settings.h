#pragma once

#if (ARDUINO >= 100)
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

//#define DEBUG

#ifdef DEBUG
#define Sprintln(a) Serial.println(a)
#define Sprint(a) Serial.print(a)
#else
#define Sprintln(a)
#define Sprint(a)
#endif

typedef struct Location
{
    uint16_t x;
    uint16_t y;
} Location;

typedef struct TouchPoint
{
    Location loc;
    uint8_t state;
} TouchPoint;
