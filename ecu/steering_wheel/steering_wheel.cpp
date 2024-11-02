#include <Arduino.h>
#include "steering_wheel_datatypes.h"
#include "steering_wheel.h"
#include "serial.h"


// Function prototypes
static void handle_button_presses(SteeringWheelData_t &data);



// Function Definition

static void handle_button_presses(SteeringWheelData_t &data)
{
    if (data.left_dpad_button_sts_prev[TOP_BUTTON] == Button_Sts_e::Pressed 
        && data.left_dpad_button_sts[TOP_BUTTON] == Button_Sts_e::Pressed_Confirmed)
    {
        if (data.lcd_state != LCD_State_e::DEFAULT
            && data.lcd_state_confirm == true)
        {
            // start increasing the XX esuss position
            if (data.lcd_state == LCD_State_e::ALL_WHEELS)
            {
                data.sw_suspension_position[FRONT_LEFT] += INCREMENT_PER_CYCLE;
                data.swc_suspension_position[FRONT_RIGHT] += INCREMENT_PER_CYCLE;
                data.sw_suspension_position[REAR_LEFT] += INCREMENT_PER_CYCLE;
                data.swc_suspension_position[REAR_RIGHT] += INCREMENT_PER_CYCLE;
            }
            else if (data.lcd_state == LCD_State_e::FRONT_WHEELS)
            {
                data.sw_suspension_position[FRONT_LEFT] += INCREMENT_PER_CYCLE;
                data.swc_suspension_position[FRONT_RIGHT] += INCREMENT_PER_CYCLE;
            }
            else if (data.lcd_state == LCD_State_e::REAR_WHEELS)
            {
                data.sw_suspension_position[REAR_LEFT] += INCREMENT_PER_CYCLE;
                data.swc_suspension_position[REAR_RIGHT] += INCREMENT_PER_CYCLE;
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
            case LCD_State_e::DEFAULT:
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
                data.lcd_state = LCD_State_e::DEFAULT;
                break;
        }
        data.lcd_state_confirm = false;
        data.lcd_state_trigger_time = millis();
    }
    else if (data.left_dpad_button_sts[BOTTOM_BUTTON] == Button_Sts_e::Pressed_Confirmed)
    {
        if (data.lcd_state != LCD_State_e::DEFAULT
            && data.lcd_state_confirm == true)
        {
            if (data.lcd_state == LCD_State_e::ALL_WHEELS)
            {
                data.sw_suspension_position[FRONT_LEFT] -= INCREMENT_PER_CYCLE;
                data.swc_suspension_position[FRONT_RIGHT] -= INCREMENT_PER_CYCLE;
                data.sw_suspension_position[REAR_LEFT] -= INCREMENT_PER_CYCLE;
                data.swc_suspension_position[REAR_RIGHT] -= INCREMENT_PER_CYCLE;
            }
            else if (data.lcd_state == LCD_State_e::FRONT_WHEELS)
            {
                data.sw_suspension_position[FRONT_LEFT] -= INCREMENT_PER_CYCLE;
                data.swc_suspension_position[FRONT_RIGHT] -= INCREMENT_PER_CYCLE;
            }
            else if (data.lcd_state == LCD_State_e::REAR_WHEELS)
            {
                data.sw_suspension_position[REAR_LEFT] -= INCREMENT_PER_CYCLE;
                data.swc_suspension_position[REAR_RIGHT] -= INCREMENT_PER_CYCLE;
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
            case LCD_State_e::DEFAULT:
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
                data.lcd_state = LCD_State_e::DEFAULT;
                break;
        }
        data.lcd_state_confirm = false;
        data.lcd_state_trigger_time = millis();
    }
    else if (data.left_dpad_button_sts_prev[CENTER_BUTTON] == Button_Sts_e::Pressed
        && data.left_dpad_button_sts[CENTER_BUTTON] == Button_Sts_e::Pressed_Confirmed)
    {
        if (data.lcd_state != LCD_State_e::DEFAULT)
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
            data.lcd_state = LCD_State_e::DEFAULT;
            data.lcd_state_confirm = false;
        }
        else
        {
            // do nothing
        }
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

    // initialize LCD pins and set up libraries

    SerialMessage message = createSerialMessage(0x11, 0x50);

    sendSerialMessage(message);



}

void steering_wheel_handler(SteeringWheelData_t &data)
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


    // Read for serial communication from the PI



    // Handle button presses
    handle_button_presses(data);

    // Write to the LEDs

    // Write to the LCD

    

    // save the data
    for (int i = 0; i < NUM_DPAD_BUTTONS; i++)
    {
        data.left_dpad_button_sts_prev[i] = data.left_dpad_button_sts[i];
    }

    delay(100);
}