#include "can_config.h"
#include "pin_config.h"
#include "sensor_common.h"
// #include "ECUs/SteeringControls/steering_controls.h"

// Setup function to initialize all components
void setup()
{
    // Initialize CAN bus
    CAN.begin( 500E3 );

    // Initialize sensors
    sensor_1_init();
    sensor_2_init();

    // Initialize steering controls
    steering_controls_init();
}

// Main loop to handle vehicle logic
void loop()
{
    // Read sensor values
    int sensor_1_value = sensor_1_read();

    // Handle CAN communication
    CAN_message_t message;
    if ( CAN.read( message ) )
    {
        process_CAN_message( message );
    }

    // Update steering control based on button states
    handle_steering_controls();

    // Add additional logic as needed
}