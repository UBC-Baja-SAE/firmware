#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "steering_wheel_datatypes.h"
#include "steering_wheel.h"
#include "lcd_handler.h"
#include "serial.h"


// Function prototypes
static void detect_button_presses(SteeringWheelData_t &data);
static void handle_button_presses(SteeringWheelData_t &data);



// Function Definition
static void detect_button_presses(SteeringWheelData_t &data)
{
    // Read for buttons pins being pressed
    for (int i = 0; i < NUM_DPAD_BUTTONS; i++)
    {
        data.left_dpad_button_data[i] = digitalRead(data.left_dpad_button_pins[i]);

        // Button is pressed
        if (data.left_dpad_button_data[i] == LOW)
        {   
            // prev was pressed
            if (data.left_dpad_button_sts_prev[i] == Button_Sts_e::Pressed)
            {
                // check if we should confirm the press
                if (millis() - data.left_dpad_button_press_time[i] >= CONFIRMED_PRESS_TIME)
                {
                    Serial.println("Button " + String(i) + " is confirmed pressed");

                    data.left_dpad_button_sts[i] = Button_Sts_e::Pressed_Confirmed;
                }
                else
                {
                    // keep button status as Pressed, so do nothing
                }
            }
            // prev was not pressed
            else if (data.left_dpad_button_sts_prev[i] == Button_Sts_e::Not_Pressed
                || data.left_dpad_button_sts_prev[i] == Button_Sts_e::SNA)
            {
                data.left_dpad_button_sts[i] = Button_Sts_e::Pressed;

                // start timer for the confirmed button press
                data.left_dpad_button_press_time[i] = millis();
            }
            else
            {
                // if the button press is confirmed and the button is still pressed, do nothing
            }
        }
        // Button is not pressed
        else if (data.left_dpad_button_data[i] == HIGH)
        {
            if (data.left_dpad_button_sts_prev[i] == Button_Sts_e::Pressed
                || data.left_dpad_button_sts_prev[i] == Button_Sts_e::Pressed_Confirmed)
            {
                Serial.println("Button " + String(i) + " is NO LONGER pressed");
            }
            else
            {
                // do nothing
            }

            data.left_dpad_button_sts[i] = Button_Sts_e::Not_Pressed;
            data.left_dpad_button_press_time[i] = 0;
        }
    }
}

static void increment_suspension_pos(SteeringWheelData_t &data, bool fl_sel, bool fr_sel, bool rl_sel, bool rr_sel)
{
    data.sw_suspension_position[FRONT_LEFT] = fl_sel ? (data.sw_suspension_position[FRONT_LEFT] + INCREMENT_PER_CYCLE) : data.sw_suspension_position[FRONT_LEFT];
    data.sw_suspension_position[FRONT_RIGHT] = fr_sel ? (data.sw_suspension_position[FRONT_RIGHT] + INCREMENT_PER_CYCLE) : data.sw_suspension_position[FRONT_RIGHT];
    data.sw_suspension_position[REAR_LEFT] = rl_sel ? (data.sw_suspension_position[REAR_LEFT] + INCREMENT_PER_CYCLE) : data.sw_suspension_position[REAR_LEFT];
    data.sw_suspension_position[REAR_RIGHT] = rr_sel ? (data.sw_suspension_position[REAR_RIGHT] + INCREMENT_PER_CYCLE) : data.sw_suspension_position[REAR_RIGHT];

    data.sw_suspension_position[FRONT_LEFT] = (data.sw_suspension_position[FRONT_LEFT] > MAX_SUSPENSION_STATE) ? MAX_SUSPENSION_STATE : data.sw_suspension_position[FRONT_LEFT];
    data.sw_suspension_position[FRONT_RIGHT] = (data.sw_suspension_position[FRONT_RIGHT] > MAX_SUSPENSION_STATE) ? MAX_SUSPENSION_STATE : data.sw_suspension_position[FRONT_RIGHT];
    data.sw_suspension_position[REAR_LEFT] = (data.sw_suspension_position[REAR_LEFT] > MAX_SUSPENSION_STATE) ? MAX_SUSPENSION_STATE : data.sw_suspension_position[REAR_LEFT];
    data.sw_suspension_position[REAR_RIGHT] = (data.sw_suspension_position[REAR_RIGHT] > MAX_SUSPENSION_STATE) ? MAX_SUSPENSION_STATE : data.sw_suspension_position[REAR_RIGHT];
}

static void decrement_suspension_pos(SteeringWheelData_t &data, bool fl_sel, bool fr_sel, bool rl_sel, bool rr_sel)
{
    data.sw_suspension_position[FRONT_LEFT] = fl_sel ? (data.sw_suspension_position[FRONT_LEFT] - INCREMENT_PER_CYCLE) : data.sw_suspension_position[FRONT_LEFT];
    data.sw_suspension_position[FRONT_RIGHT] = fr_sel ? (data.sw_suspension_position[FRONT_RIGHT] - INCREMENT_PER_CYCLE) : data.sw_suspension_position[FRONT_RIGHT];
    data.sw_suspension_position[REAR_LEFT] = rl_sel ? (data.sw_suspension_position[REAR_LEFT] - INCREMENT_PER_CYCLE) : data.sw_suspension_position[REAR_LEFT];
    data.sw_suspension_position[REAR_RIGHT] = rr_sel ? (data.sw_suspension_position[REAR_RIGHT] - INCREMENT_PER_CYCLE) : data.sw_suspension_position[REAR_RIGHT];

    data.sw_suspension_position[FRONT_LEFT] = (data.sw_suspension_position[FRONT_LEFT] < MIN_SUSPENSION_STATE) ? MIN_SUSPENSION_STATE : data.sw_suspension_position[FRONT_LEFT];
    data.sw_suspension_position[FRONT_RIGHT] = (data.sw_suspension_position[FRONT_RIGHT] < MIN_SUSPENSION_STATE) ? MIN_SUSPENSION_STATE : data.sw_suspension_position[FRONT_RIGHT];
    data.sw_suspension_position[REAR_LEFT] = (data.sw_suspension_position[REAR_LEFT] < MIN_SUSPENSION_STATE) ? MIN_SUSPENSION_STATE : data.sw_suspension_position[REAR_LEFT];
    data.sw_suspension_position[REAR_RIGHT] = (data.sw_suspension_position[REAR_RIGHT] < MIN_SUSPENSION_STATE) ? MIN_SUSPENSION_STATE : data.sw_suspension_position[REAR_RIGHT];
}

static void handle_button_presses(SteeringWheelData_t &data)
{
    if (data.left_dpad_button_sts_prev[TOP_BUTTON] == Button_Sts_e::Pressed 
        && data.left_dpad_button_sts[TOP_BUTTON] == Button_Sts_e::Pressed_Confirmed)
    {
        if (data.lcd_state != LCD_State_e::DEFAULT_NONE
            && data.lcd_state_confirm == true)
        {
            // start increasing the XX esus position
            if (data.lcd_state == LCD_State_e::ALL_WHEELS)
            {
                (void) increment_suspension_pos(data, true, true, true, true);
            }
            else if (data.lcd_state == LCD_State_e::FRONT_WHEELS)
            {
                (void) increment_suspension_pos(data, true, true, false, false);
            }
            else if (data.lcd_state == LCD_State_e::REAR_WHEELS)
            {
                (void) increment_suspension_pos(data, false, false, true, true);
            }
            else
            {
                // not possible, do nothing
            }
        }
        else
        {
            // do nothing
        }
        data.lcd_state_trigger_time = millis();
    }
    else if (data.left_dpad_button_sts_prev[RIGHT_BUTTON] == Button_Sts_e::Pressed
        && data.left_dpad_button_sts[RIGHT_BUTTON] == Button_Sts_e::Pressed_Confirmed)
    {
        switch (data.lcd_state)
        {
            case LCD_State_e::DEFAULT_NONE:
                data.lcd_state = LCD_State_e::ALL_WHEELS;
                break;
            case LCD_State_e::ALL_WHEELS:
                data.lcd_state = LCD_State_e::FRONT_WHEELS;
                break;
            case LCD_State_e::FRONT_WHEELS:
                data.lcd_state = LCD_State_e::REAR_WHEELS;
                break;
            case LCD_State_e::REAR_WHEELS:
                data.lcd_state = LCD_State_e::ALL_WHEELS;
                break;
            default:
                data.lcd_state = LCD_State_e::DEFAULT_NONE;
                break;
        }
        data.lcd_state_confirm = false;
        data.lcd_state_trigger_time = millis();
    }
    else if (data.left_dpad_button_sts[BOTTOM_BUTTON] == Button_Sts_e::Pressed_Confirmed)
    {
        if (data.lcd_state != LCD_State_e::DEFAULT_NONE
            && data.lcd_state_confirm == true)
        {
            if (data.lcd_state == LCD_State_e::ALL_WHEELS)
            {
                decrement_suspension_pos(data, true, true, true, true);
            }
            else if (data.lcd_state == LCD_State_e::FRONT_WHEELS)
            {
                decrement_suspension_pos(data, true, true, false, false);
            }
            else if (data.lcd_state == LCD_State_e::REAR_WHEELS)
            {
                decrement_suspension_pos(data, false, false, true, true);
            }
            else
            {
                // not possible, do nothing
            }
        }
        else
        {
            // do nothing
        }
        data.lcd_state_trigger_time = millis();
    }
    else if (data.left_dpad_button_sts_prev[LEFT_BUTTON] == Button_Sts_e::Pressed
        && data.left_dpad_button_sts[LEFT_BUTTON] == Button_Sts_e::Pressed_Confirmed)
    {
        switch (data.lcd_state)
        {
            case LCD_State_e::DEFAULT_NONE:
                data.lcd_state = LCD_State_e::REAR_WHEELS;
                break;
            case LCD_State_e::REAR_WHEELS:
                data.lcd_state = LCD_State_e::FRONT_WHEELS;
                break;
            case LCD_State_e::FRONT_WHEELS:
                data.lcd_state = LCD_State_e::ALL_WHEELS;
                break;
            case LCD_State_e::ALL_WHEELS:
                data.lcd_state = LCD_State_e::REAR_WHEELS;
                break;
            default:
                data.lcd_state = LCD_State_e::DEFAULT_NONE;
                break;
        }
        data.lcd_state_confirm = false;
        data.lcd_state_trigger_time = millis();
    }
    else if (data.left_dpad_button_sts_prev[CENTER_BUTTON] == Button_Sts_e::Pressed
        && data.left_dpad_button_sts[CENTER_BUTTON] == Button_Sts_e::Pressed_Confirmed)
    {
        if (data.lcd_state != LCD_State_e::DEFAULT_NONE)
        {
            data.lcd_state_confirm = true;
        }
        else
        {
            data.lcd_state_confirm = false;
        }
        data.lcd_state_trigger_time = millis();
    }
    else
    {
        //no press detected, let's check if we should return to a default LCD state
        if (millis() - data.lcd_state_trigger_time >= LCD_STALE_TIME)
        {
            data.lcd_state = LCD_State_e::DEFAULT_NONE;
            data.lcd_state_confirm = false;
        }
        else
        {
            // do nothing
        }
    }
}

static void handle_led_flags(Param_Sts_e parameter_sts, Param_Sts_e parameter_sts_prev, LED_Sts_e &led_sts)
{
    if (parameter_sts == Param_Sts_e::GOOD
        && parameter_sts_prev != parameter_sts)
    {
        led_sts = LED_Sts_e::LIT;
    }
    else if (parameter_sts == Param_Sts_e::BAD
        && parameter_sts_prev != parameter_sts)
    {
        led_sts = LED_Sts_e::BLINKING_ON;
    }
    else if (parameter_sts == Param_Sts_e::SNA)
    {
        led_sts = LED_Sts_e::SNA;
    }
    else
    {
        // do nothing
    }
}


static void handle_led_out(LED_Sts_e &led_sts, LED_Sts_e led_sts_prev, int led_pin, unsigned long &led_blinking_time)
{
    if (led_sts == LED_Sts_e::SNA)
    {
        digitalWrite(led_pin, LOW);
    }
    else if (led_sts == LED_Sts_e::LIT)
    {
        digitalWrite(led_pin, HIGH);
    }
    else if (led_sts == LED_Sts_e::BLINKING_ON)
    {
        digitalWrite(led_pin, HIGH);

        if (millis() - led_blinking_time >= LED_BLINK_HOLD_ON)
        {
            // Serial.println("LED " + String(led_pin) + " IS BLINKING OFF");
            led_sts = LED_Sts_e::BLINKING_OFF;
            led_blinking_time = millis();
        }

    }
    else if (led_sts == LED_Sts_e::BLINKING_OFF)
    {
        digitalWrite(led_pin, LOW);

        if (millis() - led_blinking_time >= LED_BLINK_HOLD_OFF)
        {
            // Serial.println("LED " + String(led_pin) + " IS BLINKING ON");
            led_sts = LED_Sts_e::BLINKING_ON;
            led_blinking_time = millis();
        }
    }
    else
    {
        // not possible, do nothing
    }
}

String ledStateToString(LED_Sts_e state) {
    switch (state) {
        case LED_Sts_e::SNA: return "SNA";
        case LED_Sts_e::LIT: return "LIT";
        case LED_Sts_e::BLINKING_ON: return "BLINKING_ON";
        case LED_Sts_e::BLINKING_OFF: return "BLINKING_OFF";
        default: return "UNKNOWN";
    }
}


void steering_wheel_init(SteeringWheelData_t &data)
{
    Serial.println("INIT");

    // default button presses to off and
    data = {
        .left_dpad_button_pins = {2, 3, 4, 5, 6},
        .led_pins = {7, 8, 9},
        .left_dpad_button_data = {0, 0, 0, 0, 0},
        .left_dpad_button_sts = {Button_Sts_e::SNA, Button_Sts_e::SNA, Button_Sts_e::SNA, Button_Sts_e::SNA, Button_Sts_e::SNA},
        .left_dpad_button_sts_prev = {Button_Sts_e::SNA, Button_Sts_e::SNA, Button_Sts_e::SNA, Button_Sts_e::SNA, Button_Sts_e::SNA},
        .left_dpad_button_press_time = {0, 0, 0, 0, 0},
        .leds_data = {LED_Sts_e::SNA, LED_Sts_e::SNA, LED_Sts_e::SNA},
        .leds_data_prev = {LED_Sts_e::SNA, LED_Sts_e::SNA, LED_Sts_e::SNA},
        .lcd_state = LCD_State_e::DEFAULT_NONE,
        .lcd_state_trigger_time = 0,
        .lcd_state_confirm = false,
        .sw_suspension_position = {0, 0, 0, 0},
        .gas_ok = Param_Sts_e::GOOD,
        .gas_ok_prev = Param_Sts_e::SNA,
        .battery_ok = Param_Sts_e::BAD,
        .battery_ok_prev = Param_Sts_e::SNA,
        .temperature_ok = Param_Sts_e::BAD,
        .temperature_ok_prev = Param_Sts_e::SNA,
        .led_blinking_times = {0, 0, 0},
    };

    // initialize button pins
    for (int i = 0; i < NUM_DPAD_BUTTONS; i++)
    {
        pinMode(data.left_dpad_button_pins[i], INPUT_PULLUP);
    }

    // initialize led pins and turn all LEDS OFF
    for (int j = 0; j < NUM_LEDS; j++)
    {
        pinMode(data.led_pins[j], OUTPUT);
        digitalWrite(data.led_pins[j], LOW);
    }

    SerialMessage message = createSerialMessage(0x11, 0x50);
    sendSerialMessage(message);
}

void steering_wheel_handler(SteeringWheelData_t &data)
{
    detect_button_presses(data);


    // Read for serial communication from the PI



    // Handle button presses
    handle_button_presses(data);

    // Write to the LEDs

    handle_led_flags(data.gas_ok, data.gas_ok_prev, data.leds_data[GAS_LED]);
    handle_led_flags(data.temperature_ok, data.temperature_ok_prev, data.leds_data[TEMPERATURE_LED]);
    handle_led_flags(data.battery_ok, data.battery_ok_prev, data.leds_data[BATTERY_LED]);

    handle_led_out(data.leds_data[GAS_LED], data.leds_data_prev[GAS_LED], data.led_pins[GAS_LED], data.led_blinking_times[GAS_LED]);
    handle_led_out(data.leds_data[BATTERY_LED], data.leds_data_prev[BATTERY_LED], data.led_pins[BATTERY_LED], data.led_blinking_times[BATTERY_LED]);
    handle_led_out(data.leds_data[TEMPERATURE_LED], data.leds_data_prev[TEMPERATURE_LED], data.led_pins[TEMPERATURE_LED], data.led_blinking_times[TEMPERATURE_LED]);

    // Serial.println("LED " + String(data.led_pins[GAS_LED]) + " is " + ledStateToString(data.leds_data[GAS_LED]));
    

    // save the data
    for (int i = 0; i < NUM_DPAD_BUTTONS; i++)
    {
        data.left_dpad_button_sts_prev[i] = data.left_dpad_button_sts[i];
    }

    for (int j = 0; j < NUM_LEDS; j++)
    {
        data.leds_data_prev[j] = data.leds_data[j];
    }

    data.gas_ok_prev = data.gas_ok;
    data.battery_ok_prev = data.battery_ok;
    data.temperature_ok_prev = data.temperature_ok;

    delay(100);
}