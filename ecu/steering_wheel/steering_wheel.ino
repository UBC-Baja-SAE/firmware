#include "steering_wheel.h"
#include "steering_wheel_datatypes.h"

//inputs from the bus

SteeringWheelData_t steering_wheel_data = { 0 };
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

void setup() {
    Serial.begin(9600);
    Serial.println("INIT");

    steering_wheel_init(steering_wheel_data, lcd);

    // Initialize the LCD

    lcd.init();          // Initialize LCD
    lcd.backlight();     // Turn on backlight
    lcd.clear();

    // // Clear the LCD and print "BAJA"
    // lcd.init(); // Initialize the LCD
    // lcd.backlight(); // Turn on the backlight
    // lcd.clear(); // Clear the display
    // lcd.setCursor(0, 0); // Set the cursor to the first column, first row
    // lcd.print("BAJA"); // Write text
}

void loop() {
    steering_wheel_handler(steering_wheel_data, lcd);

    update_lcd(lcd, steering_wheel_data.lcd_state);
}

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Initialize LCD (address 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Timing settings
const int animationDelay = 400;

void setup() {
    lcd.init();          // Initialize LCD
    lcd.backlight();     // Turn on backlight
    lcd.clear();
}

void loop() {
    // Run Dino animation
    animateDino();

}