// buildin
#include <Arduino.h>

// third-party

// private, local
#include "uv_meter.h"
#include "smoothed_reader.h"
#include "debouncer.h"

/*------------Notes-----------*\
Credits: Modisch Fabrications

Testing should be possible on the digispark, as it has an ATTiny85 onboard. 
Make sure to exclude Pin 3 and 4 as these are used for USB.

Using an ATTiny45 might be possible if you shave of a few bytes somewhere, 
but why? Save your time, it's like 10ct.

\*----------------------------*/

// --- statics, constants & defines

const bool DEBUG = true;

const uint8_t PIN_LEDS = 1;
const uint8_t PIN_BTN = 2;
const uint8_t PIN_MIC = 4;

const uint8_t N_LEDS = 12;

const uint16_t DELAY_TO_SAVE_MS = (5 * 1000);

const uint8_t N_READINGS = 60;
const uint8_t N_MAXIMA = 100;

const uint16_t T_DEBOUNCE_MS = 50;

UV_Meter<PIN_LEDS, N_LEDS> uv_meter(DELAY_TO_SAVE_MS);

Smoothed_Reader<uint8_t, N_READINGS> reader;
Smoothed_Reader<uint8_t, N_MAXIMA> avg_max_reader;

Debouncer debouncer;

// --- functions

uint16_t waveform_to_amplitude(uint16_t raw_waveform)
{
  /*
    MAX4466 has VCC/2 base level and rail-to-rail output
    Input: 512 +/- 512
    Output: 0..512

     5V   |    -----
          |   /     \
    2.5V  | --       -
          |           \
    0V    |            ----
          ___________________
  */

  // 17 - 127 = -110 -> 110/255 = 0.43

  return abs((int16_t)raw_waveform - 512);
}

// --------------

void setup()
{
  // init hardware

  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_MIC, INPUT_PULLUP);

  // init subcomponents

  // all done
  uv_meter.startup();
}

void loop()
{
  bool reading = (digitalRead(PIN_BTN) == LOW); // inverted (pullup)
  if (debouncer.read(reading))
  {
    if (DEBUG)
    {
      uv_meter.flash(CRGB::Blue, 500);
    }
    uv_meter.next_mode();
  }

  // you could increase speed *a lot* by triggering ADC-read via a HW-timer 
  // or manually at the start of each loop after reading it's buffer
  uint16_t mic_reading = analogRead(PIN_MIC);  // 512 +/- 512
  // rescale to stay inside defined boundaries
  uint8_t scaled_amplitude = waveform_to_amplitude(mic_reading)/2;  // 0..255

  reader.read(scaled_amplitude);
  avg_max_reader.read(reader.get_rolling_avg());
  uint8_t final_value = map(avg_max_reader.get_rolling_max(), 0, 200, 0, 255);
  uv_meter.read(final_value);

  if (DEBUG)
  {
    uv_meter.flash(CRGB::DarkGreen, 100);
  }
}
