#include <ESP32Servo.h>

Servo servo1, servo2, servo3, servo4, servo5;

// Servo pins
int servoPin1 = 13;
int servoPin2 = 12;
int servoPin3 = 14;
int servoPin4 = 27;
int servoPin5 = 26;

// Track current angles
int angle1 = 0;
int angle2 = 0;
int angle3 = 0;
int angle4 = 0;
int angle5 = 0;

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for serial connection
  Serial.println("Servo Toggle Controller Ready. Enter 1-5 to toggle servo between 0° and 90°.");

  servo1.attach(servoPin1);
  servo2.attach(servoPin2);
  servo3.attach(servoPin3);
  servo4.attach(servoPin4);
  servo5.attach(servoPin5);

  // Set all servos to initial angle (0°)
  servo1.write(angle1);
  servo2.write(angle2);
  servo3.write(angle3);
  servo4.write(angle4);
  servo5.write(angle5);
}

void loop() {
  if (Serial.available()) {
    char command = Serial.read();
    Serial.print("Received command: ");
    Serial.println(command);

    switch (command) {
      case '1':
        angle1 = (angle1 == 90) ? 0 : 90;
        servo1.write(angle1);
        Serial.print("Servo 1 moved to ");
        Serial.println(angle1);
        break;

      case '2':
        angle2 = (angle2 == 90) ? 0 : 90;
        servo2.write(angle2);
        Serial.print("Servo 2 moved to ");
        Serial.println(angle2);
        break;

      case '3':
        angle3 = (angle3 == 90) ? 0 : 90;
        servo3.write(angle3);
        Serial.print("Servo 3 moved to ");
        Serial.println(angle3);
        break;

      case '4':
        angle4 = (angle4 == 90) ? 0 : 90;
        servo4.write(angle4);
        Serial.print("Servo 4 moved to ");
        Serial.println(angle4);
        break;

      case '5':
        angle5 = (angle5 == 90) ? 0 : 90;
        servo5.write(angle5);
        Serial.print("Servo 5 moved to ");
        Serial.println(angle5);
        break;

      default:
        Serial.println("Invalid input. Enter a number from 1 to 5.");
        break;
    }
  }

  delay(20);
}
