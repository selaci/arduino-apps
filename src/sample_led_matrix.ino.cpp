#include <mpu6050.h>
#include <mpu_reading.h>
#include <shift_register.h>
#include <gwire.h>
#include <pin.h>

#include <Arduino.h>

const int SERIAL_BAUD_RATE = 115200;
const int ADDRESS          = 0x68;

// 74HC595
const int DATA_PIN         = 5;
const int CLOCK_PIN        = 6;
const int LATCH_PIN        = 7;

// TPIC6B595.
const int SER_IN_PIN       = 8;
const int SRCLK_PIN        = 9;
const int RCK_PIN          = 10;

ArduinoGear::GWire gwire;
ArduinoGear::Pin pin;

ArduinoCommon::Mpu6050 mpu6050(&gwire, ADDRESS);
ArduinoCommon::MpuReading mpuReading;

ArduinoCommon::ShiftRegister sr74hc595(&pin, DATA_PIN, CLOCK_PIN, LATCH_PIN);
ArduinoCommon::ShiftRegister srtpic6b595(&pin, SER_IN_PIN, SRCLK_PIN, RCK_PIN);

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  mpu6050.init();
}

// Mapping numbers.
int rowMappingNumber;
int columnMappingNumber;

// Rows and column values. If bit is "1" then "on", otherwise "off".
int rowEncoding = 0;
int columnEncoding = 0;

// Low pass filter (1st. order)
float alpha = 0.5;
int previousrowMappingNumber = 3;    // Kind of the center.
int previouscolumnMappingNumber = 3; // Kind of the center.

void loop()
{
  // Get a reading.
  mpuReading = mpu6050.getAccReading();

  // Calculate row and column numbers.
  /*
   * This maps angle readings to row numbers. Each row is numbered from 0 to 7.
   * The angle reading ranges from "-90" to "90".
   *
   * Each row covers a "22.5" (degrees / row) angle range (180 (desgrees)/ 8 (rows)).
   *
   * Row 7 ranges from "-90" to "-67.5".
   * Row 0 ranges from "67.5" to "90".
   *
   * Now, instead of using "22.5", I use "22.625". This guarantees than "-90" and "90" are within
   * a valid rage number. Otherwise, we'd have this:
   * 
   *    90 / 22.5 =>  4
   *   -90 / 22.6 => -4
   * 
   *   The overall range of row numbers is -4, -3, -2, -1, 0, 1, 2, 3, 4, which is 9 numbers instead
   *   of 8.
   *
   * Roughly, this is the mapping values:
   *
   *  [-90  , -67.5) => 7
   *  [-67.5, -45  ) => 6
   *  [-45  , -22.5) => 5
   *  [-22.5,  0   ) => 4
   *  [  0  , 22.5 ) => 3
   *  [ 22.5, 45   ) => 2
   *  [ 45  , 67.5 ) => 1
   *  [ 67.5, 90   ) => 0
   */
  rowMappingNumber = (int) (7 - (mpuReading.x + 90) / 22.625);
  columnMappingNumber = (int) (7 - (mpuReading.y + 90) / 22.625);

  previousrowMappingNumber = (int) (rowMappingNumber + alpha * previousrowMappingNumber);
  previouscolumnMappingNumber = (int) (columnMappingNumber + alpha * previouscolumnMappingNumber);

  // Get row and column on/off.
  /* Right now it's a direct relationship of 1 to 1. In the future, there will be different
   * applications that light rows and columns in different ways.
   */
  columnEncoding = 1 << previousrowMappingNumber;
  rowEncoding = 1 << previouscolumnMappingNumber;

  // Turn on / off rows and columns.
  sr74hc595.shiftOut(rowEncoding);
  srtpic6b595.shiftOut(columnEncoding);
}