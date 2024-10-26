#ifndef STEERING_WHEEL_DATATYPES_H
#define STEERING_WHEEL_DATATYPES_H

#define CONFIRMED_PRESS_TIME 500 //ms

#define NUM_DPAD_BUTTONS 5
#define NUM_LEDS 3
#define TOP_BUTTON 0
#define RIGHT_BUTTON 1
#define BOTTOM_BUTTON 2
#define LEFT_BUTTON 3
#define CENTER_BUTTON 4
#define TOP_LED 0
#define MIDDLE_LED 1
#define BOTTOM_LED 2

enum class Button_Sts_e {
    SNA, 
    Not_Pressed,
    Pressed,
    Pressed_Confirmed,
};

enum class LED_Sts_e {
    SNA, 
    LIT, 
    UNLIT,
};

typedef struct {
    int left_dpad_button_pins[NUM_DPAD_BUTTONS];
    int led_pins[NUM_LEDS];
    int left_dpad_button_data[NUM_DPAD_BUTTONS];
    Button_Sts_e left_dpad_button_sts[NUM_DPAD_BUTTONS];
    Button_Sts_e left_dpad_button_sts_prev[NUM_DPAD_BUTTONS];
    unsigned long left_dpad_button_press_time[NUM_DPAD_BUTTONS];
    LED_Sts_e leds_data[NUM_LEDS];
} SteeringWheelData_t;

#endif