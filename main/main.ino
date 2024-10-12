// #include "can_config.h"
// #include "pin_config.h"
// #include "sensor_common.h"
// #include "ECUs/SteeringControls/steering_controls.h"

const int x_pin = A0;
const int y_pin = A1;
const int switch_pin = 2;


// global variables, let's default to 0 for now
int x_value = 0;
int y_value = 0;
int switch_state = 0;

// Setup function to initialize all components
void setup()
{
    // Initialize CAN bus
    // CAN.begin( 500E3 );

    // // Initialize sensors
    // sensor_1_init();
    // sensor_2_init();

    // // Initialize steering controls
    // steering_controls_init();

    pinMode(switch_pin, INPUT_PULLUP);

    Serial.begin(9600);


}

// Main loop to handle vehicle logic
void loop()
{
    // // Read sensor values
    // int sensor_1_value = sensor_1_read();

    // // Handle CAN communication
    // CAN_message_t message;
    // if ( CAN.read( message ) )
    // {
    //     process_CAN_message( message );
    // }

    // // Update steering control based on button states
    // handle_steering_controls();

    // // Add additional logic as needed

    x_value = analogRead(x_pin);
    y_value = analogRead(y_pin);

    switch_state = digitalRead(switch_pin);

    // Output the readings to the Serial Monitor
    Serial.print("Vx: ");
    Serial.print(x_value);
    Serial.print(" | Vy: ");
    Serial.print(y_value);
    Serial.print(" | Switch: ");
    Serial.println(switch_state == LOW ? "Pressed" : "Not Pressed");
  
    // Short delay for readability in the serial monitor
    delay(100);
}






// const int x_pin = A0;
// const int y_pin = A1;
// const int switch_pin = 2;

// enum Button_State {
//   SNA = 0,
//   ALL_WHEELS = 1,
//   FRONT_WHEELS = 2,
//   REAR_WHEELS = 3,
//   NUM_STATES
// };

// Button_State button_state = SNA;

// // global variables, let's default to 0 for now
// int x_value = 0;
// int y_value = 0;
// int prior_button_sts = 0;

// unsigned long lastDebounceTime = 0;
// unsigned long minHoldTime = 500; 

// bool change_in_button_state = false;

// // Setup function to initialize all components
// void setup()
// {

//     pinMode(switch_pin, INPUT_PULLUP);

//     button_state = ALL_WHEELS;

//     Serial.begin(9600);


// }

// // Main loop to handle vehicle logic
// void loop()
// {
//     x_value = analogRead(x_pin);
//     y_value = analogRead(y_pin);

//     int current_button_sts = digitalRead(switch_pin);
//     if (current_button_sts != prior_button_sts) {
//       lastDebounceTime = millis();
//       change_in_button_state = true;
//       Serial.println(lastDebounceTime);
//     }

//     // We want to only read a button press if it is held for over 100ms
//     if ((millis() - lastDebounceTime) > minHoldTime) {
//       bool button_press_falling_edge = (current_button_sts == LOW) & (prior_button_sts == HIGH);

//       if (change_in_button_state == true && current_button_sts == LOW) {
//         Serial.print("| change time: ");
//         Serial.println(millis());
//         switch (button_state) {
//           case SNA:
//             Serial.println("SNA to ALL_WHEELS");
//             button_state = ALL_WHEELS;
//             break;
//           case ALL_WHEELS:
//             Serial.println("ALL_WHEELS to FRONT_WHEELS");
//             button_state = FRONT_WHEELS;
//             break;
//           case FRONT_WHEELS:
//             Serial.println("FRONT_WHEELS to REAR_WHEELS");
//             button_state = REAR_WHEELS;
//             break;
//           case REAR_WHEELS:
//             Serial.println("REAR_WHEELS to ALL_WHEELS");
//             button_state = ALL_WHEELS;
//             break;
//         }
//       } 
//       change_in_button_state = false;
      
//     }

//     // // Output the readings to the Serial Monitor
//     // Serial.print("Vx: |");
//     // Serial.print(x_value);
//     // Serial.print(" | Vy: ");
//     // Serial.print(y_value);
//     // Serial.print(" | Switch: ");
//     // Serial.println(current_button_sts == LOW ? "Pressed" : "Not Pressed");
  
//     // // Short delay for readability in the serial monitor
//     // delay(100);

//     // save info
//     prior_button_sts = current_button_sts;
// }


#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  while (!Serial); // Wait for the serial monitor to open
  Serial.println("\nI2C Scanner");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }

  if (nDevices == 0) Serial.println("No I2C devices found\n");
  else Serial.println("done\n");

  delay(5000); // Wait 5 seconds for the next scan
}