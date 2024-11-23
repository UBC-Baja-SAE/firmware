#ifndef LCD_HANDLER_DATATYPES_H
#define LCD_HANDLER_DATATYPES_H

#include <LiquidCrystal_I2C.h>

#define ANIMATION_DELAY 400 //ms

// Dino character definitions
byte chardino1[8] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B10000,
    B10000,
    B11000,
    B11110
};

byte chardino2[8] = {
    B00000,
    B00000,
    B00001,
    B00001,
    B00001,
    B00011,
    B01111,
    B11111
};

byte chardino3[8] = {
    B11111,
    B11111,
    B10111,
    B11111,
    B11100,
    B11111,
    B11100,
    B11100
};

byte chardino4[8] = {
    B10000,
    B11000,
    B11000,
    B11000,
    B00000,
    B10000,
    B00000,
    B00000
};

byte chardino5[8] = {
    B11111,
    B11111,
    B01111,
    B00111,
    B00011,
    B00011,
    B00010,
    B00011
};

byte chardino6[8] = {
    B11111,
    B11111,
    B11111,
    B11111,
    B10110,
    B00010,
    B00010,
    B00011
};

byte chardino7[8] = {
    B11111,
    B11001,
    B10000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000
};

#endif