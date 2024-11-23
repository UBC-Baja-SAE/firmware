#ifndef LCD_HANDLER_H
#define LCD_HANDLER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "steering_wheel_datatypes.h"

// Function prototype
void update_lcd(LiquidCrystal_I2C &lcd, LCD_State_e state);

// void trigger_animation(LiquidCrystal_I2C &lcd, LCD_State_e state);

#endif // LCD_HANDLER_H