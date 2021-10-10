#include <pin.h>
#include <system.h>
#include <button.h>

#include <Arduino.h>

const uint8_t PIN_NUMBER = 5;
const int SERIAL_BAUD_RATE = 115200;

ArduinoGear::Pin pin;
ArduinoGear::System sys;

// An elaspsed of 2 seconds, only for testing purposes. Usually 0 or 100 ms should be enough.
ArduinoCommon::Button button(&pin, &sys, PIN_NUMBER, 2000);

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  pin.pinMode(PIN_NUMBER, INPUT);
}

char buff[50];

char pressedMsg[] = "Button has been pressed";
char releasedMsg[] = "Button has been released";

void loop() {
  if (button.has_pressed()) {
    Serial.println(pressedMsg);
  } else if (button.has_released()) {
    Serial.println(releasedMsg);
  }
}