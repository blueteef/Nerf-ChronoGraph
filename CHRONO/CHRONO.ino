//
//    FILE: timeMeasurement.ino
//  AUTHOR: Rob dot Tillaart at gmail dot com
// VERSION: 0.0.1
// PURPOSE: timeMeasurement Arduino UNO between 2 interrupts on pin 2 and 3 using timer1
//     URL: http://forum.arduino.cc/index.php?topic=473669
//
// HISTORY:
// 0.0.1 - 2017-04-28 initial version

#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"
#include "background.h"

#define TFT_DC 7
#define TFT_CS 10

Adafruit_GC9A01A tft(TFT_CS, TFT_DC);

volatile bool inprogress = false;
volatile bool dataReady = false;
volatile uint32_t ticks = 0;
volatile uint32_t overflows = 0;

const float ticksPerSecond = 16000000.0;
const float sensorDistance = 0.370064;
float avg;
long count = 1;
float cum_total = 0;
int num_samples = 3;
int aref = 5.255;
float R1 = 30000;
float R2 = 7500;
float voltage;
long deviation_count = 0;


void setup() {

  // Serial.begin(115200);

  tft_init();

  pinMode(2, INPUT);
  pinMode(3, INPUT);

  attachInterrupt(digitalPinToInterrupt(2), startTimer, RISING);
  attachInterrupt(digitalPinToInterrupt(3), stopTimer, RISING);
}


void loop() {
  if (dataReady && inprogress) {
    // MATH
    float duration = ticks / ticksPerSecond;    // seconds
    float speed = (sensorDistance / duration);  // feet / second

    tft_update(speed);

    // PREPARE NEXT MEASUREMENT
    inprogress = false;
  }
}

ISR(TIMER1_OVF_vect) {
  overflows++;
}

void startTimer() {
  // prevent start interrupt when in progress
  if (inprogress) return;
  inprogress = true;
  dataReady = false;

  TCCR1A = 0;           //
  TCCR1B = 0;           // stop
  TCNT1 = 0;            // reset counter, overflows at 65536
  overflows = 0;        // reset overflowCount
  OCR1A = 1;            // clock pulses per increment TCNT1
  TIMSK1 = bit(TOIE1);  // set overflow interrupt
  TCCR1B = bit(CS10);   // start
}

void stopTimer() {
  if (dataReady) return;  // prevent additional stop interrupt
  dataReady = true;

  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 = 0;
  ticks = overflows * 65536UL + TCNT1;  // calculate clockticks.
}

float sensor_read(int pin) {
  unsigned int x = 0;
  float val = 0.0, samples = 0.0, samp_avg = 0.0, final_val = 0.0, batt_percent = 0.0;

  for (int x = 0; x < num_samples; x++) {
    val = analogRead(pin);
    samples = samples + val;
    delay(3);
  }
  samp_avg = samples / num_samples;

  final_val = (samp_avg * aref) / 1024.0;
  final_val = final_val / (R2 / (R1 + R2));
  // batt_percent = map(final_val, 0, 100, 3.2, 4.2);

  return (final_val);
}

void tft_init() {

  tft.begin();
  tft.setRotation(2);
  tft.setTextWrap(false);
  tft.fillScreen(GC9A01A_BLACK);
  // tft.setTextSize(3);

  // tft.setTextColor(GC9A01A_WHITE);
  // tft.setCursor(50, 95);
  // tft.print("BOOTING");

  delay(500);

  // tft.fillScreen(GC9A01A_YELLOW);
  // tft.setTextColor(GC9A01A_RED, GC9A01A_YELLOW);
  // tft.setTextSize(3);
  // tft.setCursor(55, 30);
  // tft.print("  FPS:  ");
  // tft.setCursor(30, 60);
  // tft.print("    --    ");

  // tft.setCursor(20, 95);
  // tft.print("Avg: | VIN:");
  // tft.setCursor(30, 125);
  // tft.print("    --    ");

  // tft.setCursor(60, 160);
  // tft.print("Count: ");
  // tft.setCursor(30, 190);
  // tft.print("    --    ");

  (tft.drawBitmap(1, 0, background, 240, 240, GC9A01A_WHITE));
}

void tft_update(float FPS) {
  tft.setTextColor(GC9A01A_RED, GC9A01A_WHITE);
  tft.setTextSize(5);
  tft.setCursor(36, 73);
  tft.print(FPS, 1);

  avg = (cum_total + FPS) / count;

  tft.setTextSize(2);
  tft.setCursor(48, 144);
  tft.print(avg, 2);

  voltage = sensor_read(A6);

  tft.setTextSize(3);
  tft.setCursor(50, 177);
  tft.print(voltage, 1);

  if (count <= 9) {
    tft.setCursor(152, 176);
    tft.print(count);
  } else {
    tft.setCursor(142, 176);
    tft.print(count);
  }

  // setup standard deviation conditions.
  if (count >= 1) {
    if (FPS >= avg + 12) {
      deviation_count += 1;
    }
    if (FPS <= avg - 12) {
      deviation_count += 1;
    }
  }

  tft.setTextSize(2);
  tft.setCursor(166, 144);
  tft.print(deviation_count);
  count += 1;
}

// ---END OF FILE ---