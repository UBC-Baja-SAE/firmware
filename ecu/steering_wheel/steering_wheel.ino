// #include "steering_wheel.h"
// #include "steering_wheel_datatypes.h"

// //inputs from the bus

// SteeringWheelData_t steering_wheel_data = { 0 };
// LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

// void setup() {
//     Serial.begin(9600);
//     Serial.println("INIT");

//     steering_wheel_init(steering_wheel_data, lcd);

//     // Initialize the LCD
//     lcd.begin(16, 2);       // Set up the LCD's dimensions (16 columns, 2 rows)
//     lcd.backlight();        // Turn on the backlight

//     // Clear the LCD and print "BAJA"
//     lcd.init(); // Initialize the LCD
//     lcd.backlight(); // Turn on the backlight
//     lcd.clear(); // Clear the display
//     lcd.setCursor(0, 0); // Set the cursor to the first column, first row
//     lcd.print("BAJA"); // Write text
// }

// void loop() {
//     // steering_wheel_handler(steering_wheel_data, lcd);
// }

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Initialize LCD (address 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

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

void animateDino() {
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