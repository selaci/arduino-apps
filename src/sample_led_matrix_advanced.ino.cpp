#include <button.h>
#include <mpu6050.h>
#include <mpu_reading.h>
#include <shift_register.h>
#include <gwire.h>
#include <pin.h>
#include <system.h>

#include <Arduino.h>

const int SERIAL_BAUD_RATE = 19200;
const int ADDRESS          = 0x68;

// Button.
const int BUTTON_PIN       = 4;

// 74HC595
const int DATA_PIN         = 8;
const int CLOCK_PIN        = 9;
const int LATCH_PIN        = 10;

// TPIC6B595.
const int SER_IN_PIN       = 13;
const int SRCLK_PIN        = 11;
const int RCK_PIN          = 12;

ArduinoGear::GWire wire;
ArduinoGear::Pin pin;
ArduinoGear::System sys;

ArduinoCommon::Button button(&pin, &sys, BUTTON_PIN);
ArduinoCommon::Mpu6050 mpu6050(&wire, ADDRESS);
ArduinoCommon::MpuReading mpuReading;

ArduinoCommon::ShiftRegister sr74hc595(&pin, DATA_PIN, CLOCK_PIN, LATCH_PIN);
ArduinoCommon::ShiftRegister srtpic6b595(&pin, SER_IN_PIN, SRCLK_PIN, RCK_PIN);

uint8_t sequence_index = 1;
const uint8_t NUM_SEQUENCES = 7;
const uint8_t SIZE = 8;

int row_sequences[NUM_SEQUENCES][SIZE] = {
  {1, 2, 4, 8, 16, 32, 64, 128},
  {3, 7, 14, 28, 56, 112, 224, 192},
  {255, 255, 255, 255, 255, 255, 255, 255},
  {1, 2, 4, 8, 16, 32, 64, 128},
  {254, 253, 251, 247, 239, 223, 191, 127},
  {252, 248, 241, 227, 199, 143, 31, 127},
  {255, 255, 255, 255, 255, 255, 255, 255}
};

uint8_t column_sequences[NUM_SEQUENCES][SIZE] = {
  {1, 2, 4, 8, 16, 32, 64, 128},
  {3, 7, 14, 28, 56, 112, 224, 192},
  {1, 2, 4, 8, 16, 32, 64, 128},
  {255, 255, 255, 255, 255, 255, 255, 255},
  {254, 253, 251, 247, 239, 223, 191, 127},
  {252, 248, 241, 227, 199, 143, 31, 127},
  {255, 255, 255, 255, 255, 255, 255, 255}
};

// Mapping numbers.
float rowMapping;
float columnMapping;

// Rows and column values. If bit is "1" then "on", otherwise "off".
int rowEncoding = 0;
int columnEncoding = 0;

// Low pass filter (1st. order)
float alpha = 0.8;
float previousRowMapping = 3.0;    // Kind of the center.
float previousColumnMapping = 3.0; // Kind of the center.
uint8_t rowSlot, columnSlot;

uint8_t i; // A variable to ite  rate.
uint8_t j = 0; // A veriable that contains what row to start with.
uint8_t count = 0;
uint8_t result = 0;

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  mpu6050.init();

  sr74hc595.setup();
  srtpic6b595.setup();
  button.setup();

  Serial.println("Setup end.");
}

char buff[100];

int readingCounter = 0;

int timingCounter = 0;
unsigned long timing = 0;
unsigned long previousTiming = 0;

void loop() {
  if (button.has_pressed()) {
    // Change app. sequence.
    sequence_index = (sequence_index + 1) % NUM_SEQUENCES;
  }
  
  // Get a reading.
  if (readingCounter == 0) {
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
  
    rowMapping = 7 - (mpuReading.x + 90) / 22.625;
    columnMapping = 7 - (mpuReading.y + 90) / 22.625;
  
    previousRowMapping = (1 - alpha) * rowMapping + alpha * previousRowMapping;  
    previousColumnMapping = (1 - alpha) * columnMapping + alpha * previousColumnMapping;

    rowSlot = round(previousRowMapping);
    if (previousRowMapping < 0) {
      rowSlot = 0;
    } else if (previousRowMapping > SIZE) {
      rowSlot = SIZE - 1;
    }

    columnSlot = round(previousColumnMapping);
    if (previousColumnMapping < 0) {
      columnSlot = 0;
    } else if (previousColumnMapping > SIZE) {
      columnSlot = SIZE - 1;
    }
    
    columnEncoding = row_sequences[sequence_index][rowSlot];
    rowEncoding = column_sequences[sequence_index][columnSlot];
  
    // Turn on / off rows and columns. Multiple rows can be on at any given time.
    
    srtpic6b595.shiftOut(columnEncoding);
  }

  readingCounter = (readingCounter + 1) % 16;  
  /*
   * Turn on/off rows.
   *
   * Only a single row can be on at any given time. When a row sequence wants more than one row on,
   * a row will be turned on first then off before turning next row on.
   *
   * This logic has an issue though; the last row to be turned on will have more time on compared to
   * any other row. That's because every iteration needs time to get a reading, calculate next
   * sequence and other tasks. When a row has more time than the others on, that row will show more,
   * which is undesired.
   *
   * In order to workaround this, the next logic does not start with the same row or finishes with
   * the same row either in every single loop. Each loop iteration will have a different row to
   * start with and each iteration will go through all the rows. This means that in every single
   * loop iteration a different row will be left on last, thus giving the same brightness to every
   * row.
   *
   * For example, the first time the loop starts, it will start by row 0 and leave row 7 last.
   * The next time, will start by row 1 and leave row 0 last. The next time will start by row 2 and
   * leave row 1 last and so forth.
   *
   * Lastly, if a row happens to be off the logic moves on. As soon as one row is one, all the other
   * rows will be turned off, so there is no need to explicitely set a row off.
   */  
  while(true) {
    i = j;
    result = rowEncoding & (1 << i);

    //if (result != 0) {
      sr74hc595.shiftOut(result); // Only one row is on. All the others are off.
    //}

    count++;
    if (count == SIZE) {
      break;
    }

    i = (i + 1) % SIZE;
  }

  j = (j + 1) % SIZE; // Next loop iteration will start and finish with a different rows.
  count = 0;
  
  timingCounter++;

  if (timingCounter == 1000) {
    timing = sys.millis();
    Serial.print("Avg. loop (ms): "); Serial.println((timing - previousTiming) / 1000.0);
    previousTiming = timing;
    timingCounter = 0;    
  }
}
