#ifndef STEERING_WHEEL_DATATYPES_H
#define STEERING_WHEEL_DATATYPES_H

#include <LiquidCrystal_I2C.h>

#define CONFIRMED_PRESS_TIME 500 //ms
#define LCD_STALE_TIME 20000 //ms
#define LED_BLINK_HOLD_ON 500 //ms
#define LED_BLINK_HOLD_OFF 300 //ms

#define MAX_SUSPENSION_STATE 100
#define MIN_SUSPENSION_STATE 0

#define NUM_DPAD_BUTTONS 5
#define NUM_LEDS 3
#define NUM_WHEELS 4
#define TOP_BUTTON 0
#define RIGHT_BUTTON 1
#define BOTTOM_BUTTON 2
#define LEFT_BUTTON 3
#define CENTER_BUTTON 4
#define GAS_LED 0
#define TEMPERATURE_LED 1
#define BATTERY_LED 2
#define FRONT_LEFT 0
#define FRONT_RIGHT 1
#define REAR_LEFT 2
#define REAR_RIGHT 3
#define INCREMENT_PER_CYCLE 1

enum class Button_Sts_e {
    SNA, 
    Not_Pressed,
    Pressed,
    Pressed_Confirmed,
};

enum class LED_Sts_e {
    SNA, 
    LIT, 
    BLINKING_ON,
    BLINKING_OFF,
};

enum class LCD_State_e {
    DEFAULT_NONE, 
    ALL_WHEELS,
    FRONT_WHEELS,
    REAR_WHEELS,
};

enum class Param_Sts_e {
    SNA, 
    GOOD,
    BAD,
};

typedef struct {
    int left_dpad_button_pins[NUM_DPAD_BUTTONS];
    int led_pins[NUM_LEDS];
    int left_dpad_button_data[NUM_DPAD_BUTTONS];
    Button_Sts_e left_dpad_button_sts[NUM_DPAD_BUTTONS];
    Button_Sts_e left_dpad_button_sts_prev[NUM_DPAD_BUTTONS];
    unsigned long left_dpad_button_press_time[NUM_DPAD_BUTTONS];
    LED_Sts_e leds_data[NUM_LEDS];
    LED_Sts_e leds_data_prev[NUM_LEDS];
    LCD_State_e lcd_state;
    unsigned long lcd_state_trigger_time;
    bool lcd_state_confirm;
    uint8_t sw_suspension_position[NUM_WHEELS];
    Param_Sts_e gas_ok;
    Param_Sts_e gas_ok_prev;
    Param_Sts_e battery_ok;
    Param_Sts_e battery_ok_prev;
    Param_Sts_e temperature_ok;
    Param_Sts_e temperature_ok_prev;
    unsigned long led_blinking_times[NUM_LEDS];
} SteeringWheelData_t;

#endif