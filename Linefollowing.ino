#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// ==========================================
// CONFIGURATION
// ==========================================
#define I2C_SDA 21
#define I2C_SCL 22
#define PCA9685_ADDRESS 0x40 // Default I2C address

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDRESS);

// Servo pulse length calibration (Tweak these if your servos jitter or don't reach full range)
// Standard MG90S/MG996R usually fall between 150 and 600.
#define SERVO_MIN 150  
#define SERVO_MAX 600  

#define NUM_SERVOS 6

// Default home position: [Waist, Shoulder, Elbow, Wrist Pitch, Wrist Roll, Gripper]
int current_angles[NUM_SERVOS] = {90, 90, 90, 90, 90, 45};

// ==========================================
// HELPER FUNCTIONS
// ==========================================
int angleToPulse(int angle) {
  // Maps 0-180 degrees to the PCA9685 pulse range
  return map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
}

void updateAllServos() {
  for (int i = 0; i < NUM_SERVOS; i++) {
    pwm.setPWM(i, 0, angleToPulse(current_angles[i]));
  }
}

void resetToHome() {
  current_angles[0] = 90;
  current_angles[1] = 90;
  current_angles[2] = 90;
  current_angles[3] = 90;
  current_angles[4] = 90;
  current_angles[5] = 45; // Gripper slightly open
  updateAllServos();
  Serial.println("OK: Reset to home position");
}

// ==========================================
// SETUP & LOOP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for serial monitor to initialize
  
  Serial.println("Initializing PCA9685...");
  Wire.begin(I2C_SDA, I2C_SCL);
  pwm.begin();
  pwm.setPWMFreq(50); // 50Hz is the standard frequency for analog servo motors
  delay(10);
  
  resetToHome();
  Serial.println("Ready. Awaiting commands...");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Remove trailing whitespace or carriage returns

    if (command.startsWith("SET,")) {
      // Parse: SET,a0,a1,a2,a3,a4,a5
      int angles[NUM_SERVOS];
      int parsed = sscanf(command.c_str(), "SET,%d,%d,%d,%d,%d,%d", 
                          &angles[0], &angles[1], &angles[2], 
                          &angles[3], &angles[4], &angles[5]);
      
      if (parsed == NUM_SERVOS) {
        // Validate and apply angles
        for (int i = 0; i < NUM_SERVOS; i++) {
          // Gripper (index 5) is clamped to 0-90 by Python, but we double-check here
          int max_angle = (i == 5) ? 90 : 180;
          current_angles[i] = constrain(angles[i], 0, max_angle);
        }
        updateAllServos();
        Serial.println("OK: Command executed");
      } else {
        Serial.println("ERR: Invalid SET format");
      }
    } 
    else if (command == "RESET") {
      resetToHome();
    } 
    else {
      Serial.println("ERR: Unknown command");
    }
  }
}