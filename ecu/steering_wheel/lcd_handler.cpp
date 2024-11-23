#include <Arduino.h>
#include "lcd_handler.h"
#include "steering_wheel_datatypes.h"
#include "lcd_handler_datatypes.h"

void init_lcd(LiquidCrystal_I2C &lcd)
{
    lcd.init();          // Initialize LCD
    lcd.backlight();     // Turn on backlight
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("INITIALIZING");
}

void update_lcd(LiquidCrystal_I2C &lcd, LCD_State_e state)
{
    lcd.clear(); // Clear the screen before writing
    switch (state) {
        case LCD_State_e::DEFAULT_NONE:
            animateDino(lcd);
            break;
        case LCD_State_e::ALL_WHEELS:
            lcd.setCursor(0, 0);
            lcd.print("ALL_WHEELS");
            break;
        case LCD_State_e::FRONT_WHEELS:
            lcd.setCursor(0, 0);
            lcd.print("FRONT_WHEELS");
            break;
        case LCD_State_e::REAR_WHEELS:
            lcd.setCursor(0, 0);
            lcd.print("REAR_WHEELS");
            break;
    }
}


static void animateDino(LiquidCrystal_I2C &lcd) {
    // Load custom characters into LCD memory
    lcd.createChar(1, chardino1);
    lcd.createChar(2, chardino2);
    lcd.createChar(3, chardino3);
    lcd.createChar(4, chardino4);
    lcd.createChar(5, chardino5);
    lcd.createChar(6, chardino6);
    lcd.createChar(7, chardino7);

    for (int position = 0; position <= 18; position++) {
        // Display dino's top row
        if (position >= 3) {
            lcd.setCursor(position - 3, 0);
            lcd.write(1);
        }
        if (position >= 2) {
            lcd.setCursor(position - 2, 0);
            lcd.write(2);
        }
        if (position >= 1) {
            lcd.setCursor(position - 1, 0);
            lcd.write(3);
        }
        lcd.setCursor(position, 0);
        lcd.write(4);

        // Display dino's bottom row
        if (position >= 3) {
            lcd.setCursor(position - 3, 1);
            lcd.write(5);
        }
        if (position >= 2) {
            lcd.setCursor(position - 2, 1);
            lcd.write(6);
        }
        if (position >= 1) {
            lcd.setCursor(position - 1, 1);
            lcd.write(7);
        }

        // Wait and clear screen for next frame
        delay(animationDelay);
        lcd.clear();
    }
}