#include <BleMouse.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

BleMouse bleMouse("ESP32 BLE Mouse");
Adafruit_MPU6050 mpu;

// Button pin for scroll
#define SCROLL_PIN 19
#define LEFT_CLICK_PIN  14
#define RIGHT_CLICK_PIN 18

// Base sensitivity and threshold
float baseSensitivity = 9.5;  // Adjust for overall speed
float threshold = 0.02;   // Deadzone to avoid jitter

// Smooth scrolling variables
bool mouseLocked = false;  // Tracks whether the mouse is locked for scrolling
float accumulatedMovementX = 0.0;  // Tracks horizontal movement for scrolling
float accumulatedMovementY = 0.0;  // Tracks vertical movement for scrolling
float scrollSpeed = 2.0;  // Controls the sensitivity of the scrolling

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Mouse with MPU6050 (Simple Version)");

  // Start BLE
  bleMouse.begin();

  // Start MPU
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found, check wiring!");
    while (1) delay(10);
  }

  // Configure MPU ranges
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Buttons as input with pullups
  pinMode(LEFT_CLICK_PIN, INPUT_PULLUP);
  pinMode(RIGHT_CLICK_PIN, INPUT_PULLUP);
  pinMode(SCROLL_PIN, INPUT_PULLUP);

  Serial.println("Setup complete, waiting for BLE connection...");
}

void loop() {
  if (bleMouse.isConnected()) {
    // --- MPU6050 Gyro Movement ---
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Raw gyro values directly
    float gyroX = g.gyro.x; // Roll
    float gyroY = g.gyro.y; // Pitch
    float gyroZ = g.gyro.z; // Yaw

    // Map gyro movement to cursor
    float moveX = gyroZ; // horizontal
    float moveY = gyroY; // vertical 

    // Deadzone filter
    if (fabs(moveX) < threshold) moveX = 0;
    if (fabs(moveY) < threshold) moveY = 0;

    // If mouse is locked, interpret movement as scrolling
    if (mouseLocked) {
      // Accumulate movement for scrolling
      accumulatedMovementY += moveY;  // vertical movement for scrolling

      // Smooth scrolling: gradually adjust speed based on accumulated movement
      if (fabs(accumulatedMovementY) >= threshold) {
        int scrollAmount = (int)(accumulatedMovementY * scrollSpeed);  // Adjust sensitivity based on movement
        bleMouse.move(0, 0, scrollAmount); // Scroll vertically
        accumulatedMovementY = 0;  // Reset accumulated movement after scrolling
      }
    } else {
      // Mouse movement for cursor (only if not locked)
      float speed = sqrt(moveX * moveX + moveY * moveY);
      float dynamicSensitivity = baseSensitivity * (1 + speed * 0.2);
      moveX *= dynamicSensitivity;
      moveY *= dynamicSensitivity + 2;

      // Apply scaling and move the mouse in the same direction as the movement
      if ((int)moveX != 0 || (int)moveY != 0) {
        bleMouse.move((int)moveX, (int)moveY);  // up and down of mouse
      }
    }

    // --- Button Actions ---
    if (digitalRead(LEFT_CLICK_PIN) == LOW) {
      bleMouse.press(MOUSE_LEFT);
    } else {
      bleMouse.release(MOUSE_LEFT);
    }

    if (digitalRead(RIGHT_CLICK_PIN) == LOW) {
      bleMouse.press(MOUSE_RIGHT);
    } else {
      bleMouse.release(MOUSE_RIGHT);
    }

    // Scroll Button: Lock Mouse when pressed
    if (digitalRead(SCROLL_PIN) == LOW) {
      if (!mouseLocked) {
        mouseLocked = true;  // Lock the mouse for scrolling
      }
    } else {
      mouseLocked = false;  // Unlock the mouse when the button is released
    }

    delay(5); // small delay for smoother control
  }
}
