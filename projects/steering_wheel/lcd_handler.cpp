#include <Arduino.h>
#include "lcd_handler.h"
#include "steering_wheel_datatypes.h"

void update_lcd(LiquidCrystal_I2C &lcd, LCD_State_e state)
{
    lcd.clear(); // Clear the screen before writing
    switch (state) {
        case LCD_State_e::DEFAULT_NONE:
            lcd.setCursor(0, 0);
            lcd.print("BAJA...");
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