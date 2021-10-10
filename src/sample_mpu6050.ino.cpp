#include <mpu6050.h>
#include <gwire.h>

#include <Arduino.h>

const int ADDRESS = 0x68;
const int SERIAL_BAUD_RATE = 115200;


ArduinoGear::GWire gwire;
ArduinoCommon::Mpu6050 mpu6050(&gwire, ADDRESS);
ArduinoCommon::MpuReading reading;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
}

char buff[50];

void loop() {
  reading = mpu6050.getAccReading();
  sprintf(buff, "X: %d, Y: %d\n", (int) reading.x, (int) reading.y);
  Serial.print(buff);
}