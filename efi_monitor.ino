#include "FastLED.h"

// enable this to get output of status to serial at runtime - fine for slow RPM, but can slow things down too much to cope with high RPM.
#define SERIAL_DEBUG
// note that status is sent to serial during startup anyway, this only enables status during run.

#define DATA_PIN 13
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
static unsigned int const num_led_cols = 8;
static unsigned int const num_led_rows = 4;
static unsigned int const num_leds = num_led_rows * num_led_cols;
#define BRIGHTNESS 96

CRGB leds[num_leds];

uint8_t constexpr const num_cyls = 8;
uint8_t cylinder_pins[num_cyls]{2, 3, 4, 5, 6, 7, 8, 9}; // digital pin number corresponding to each cylinder
uint8_t cylinder_leds[num_cyls]{7, 6, 5, 4, 3, 2, 1, 0}; // first led index corresponding to cylinder
uint8_t cylinder_last_state[num_cyls];

// modify this to test different firing orders:
uint8_t firing_order[num_cyls]{1, 5, 4, 8, 6, 3, 7, 2};  // expected firing order, cylinders indexed from 1 for human-readable clarity
uint8_t firing_turn[num_cyls];  // the turn when each cylinder fires - generated from firing order

uint8_t pulse_in_cycle = num_cyls + 1; // where we are in the engine cycle - default "impossible" value for validation
uint8_t fired_last = 0;
uint8_t expected_to_fire_next = 0;

void setup() {
  delay(1000); // initial delay of a few seconds is recommended for FastLED
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, num_leds).setCorrection(UncorrectedColor); // initializes LED strip
  //FastLED.setBrightness(BRIGHTNESS);// global brightness

  // turn off the LEDs as soon as possible after start
  for(unsigned int led = 0; led != num_leds; ++led) {
    leds[led] = 0x000000;
  }
  FastLED.show();

  Serial.begin(9600);
  Serial.println("=== Restart ===");

  // firing order input is human-readable, firing order array from indexed from 1 to indexed from 0 for easier machine processing
  Serial.print("Firing order: ");
  for(unsigned int i = 0; i != num_cyls; ++i) {
    Serial.print(firing_order[i]);
    Serial.print(" ");
    --firing_order[i];
  }
  Serial.println();
  for(unsigned int cyl = 0; cyl != num_cyls; ++cyl) {
    for(unsigned int i = 0; i != num_cyls; ++i) {
      if(firing_order[i] == cyl) {
        firing_turn[cyl] = i;
        break;
      }
    }
  }
  
  for(unsigned int cyl = 0; cyl != num_cyls; ++cyl) {
    pinMode(cylinder_pins[cyl], INPUT_PULLUP);
  }
  delay(100);
  for(unsigned int cyl = 0; cyl != num_cyls; ++cyl) {
    cylinder_last_state[cyl] = digitalRead(cylinder_pins[cyl]);
    Serial.print("cyl ");
    Serial.print(cyl + 1);
    Serial.print(" has pin ");
    Serial.print(cylinder_pins[cyl]);
    Serial.print(", firing turn ");
    Serial.print(firing_turn[cyl]);
    Serial.print(", value ");
    Serial.println(cylinder_last_state[cyl]);

    if(cylinder_last_state[cyl] == HIGH) {
      leds[cylinder_leds[cyl]] = 0x000100;
    } else {
      leds[cylinder_leds[cyl]] = 0x010000;
    }
    FastLED.show();
  }
  delay(500);
  for(unsigned int led = 0; led != num_leds; ++led) {
    leds[led] = 0x000000;
  }
  FastLED.show();  
  Serial.println("=== Waiting for first pulse ===");

  while(pulse_in_cycle == num_cyls + 1) {
    for(unsigned int cyl = 0; cyl != num_cyls; ++cyl) {
      uint8_t cylinder_new_state = digitalRead(cylinder_pins[cyl]);
      if(cylinder_last_state[cyl] != cylinder_new_state) {
        cylinder_last_state[cyl] = cylinder_new_state;
        if(cylinder_new_state == LOW) {
          fired_last = cyl;
          
          // we've received a pulse - find out where we are in the cycle based on the firing order
          for(unsigned int i = 0; i != num_cyls; ++i) {
            if(firing_order[i] == cyl) {
              pulse_in_cycle = i;
              Serial.print("First pulse received on cyl ");
              Serial.print(cyl + 1);
              Serial.print(" so we're at ignition ");
              Serial.print(pulse_in_cycle);
              Serial.println(" in the cycle.");
              break;
            }
          }
          if(pulse_in_cycle == num_cyls + 1) {
            Serial.print("ERROR: Pulse received on cyl ");
            Serial.print(cyl + 1);
            Serial.println(" but that cylinder wasn't found in the firing order!  Fix the firing order definition.  Carrying on, but all cylinder fires will show wrong order.");
          } else {
            break;
          }
        }
      }
    }
  }

  uint8_t next_pulse = (pulse_in_cycle + 1) % num_cyls;
  expected_to_fire_next = firing_order[next_pulse];
  Serial.print("Next pulse ");
  Serial.print(next_pulse);
  Serial.print(", expect cyl ");
  Serial.print(expected_to_fire_next + 1);
  Serial.println(" to fire next.");

  Serial.println("=== Engine is running ===");
}

void display_and_scroll() {
  FastLED.show();
  for(unsigned int led_row = num_led_rows - 1; led_row != 0; --led_row) {
    for(unsigned int led_col = 0; led_col != num_led_cols; ++led_col) {
      leds[led_col + (led_row * num_led_cols)] = leds[led_col + ((led_row - 1) * num_led_cols)]; // cascade LED colours along the matrix
    }
  }
  for(unsigned int led_col = 0; led_col != num_led_cols; ++led_col) {
    leds[led_col] = 0x000000; // clear the first column
  }
}

void loop() {
  for(unsigned int cyl = 0; cyl != num_cyls; ++cyl) {
    uint8_t cylinder_new_state = digitalRead(cylinder_pins[cyl]);
    if(cylinder_last_state[cyl] != cylinder_new_state) {
      if(cylinder_new_state == LOW) {
        // only process events once output has been released, so we do nothing when it goes low
        leds[cylinder_leds[cyl]] = 0x010101;
        FastLED.show();
        
      } else {
        // output has been released, so process the event
        if(cyl == expected_to_fire_next) {
          pulse_in_cycle = (pulse_in_cycle + 1) % num_cyls;
          expected_to_fire_next = firing_order[pulse_in_cycle];
          leds[cylinder_leds[cyl]] = 0x000100; // mark this cylinder as successfully tested, green
          display_and_scroll();
          leds[cylinder_leds[expected_to_fire_next]] = 0x010100; // mark next expected cylinder as yellow

          #ifdef SERIAL_DEBUG
            Serial.print(cyl + 1);
            Serial.print("* -> ");
            Serial.println(expected_to_fire_next); // correct, plus what we expect next
          #endif
        } else if(cyl == fired_last) {
          leds[cylinder_leds[cyl]] = 0x000100; // mark this cylinder as successfully tested, green
          
          #ifdef SERIAL_DEBUG
            Serial.print(cyl + 1);
            Serial.println("R"); // repeat fire, as expected
          #endif
        } else {
          leds[cylinder_leds[cyl]] = 0x010000; // mark this cylinder as fired out of order, red
          display_and_scroll();

          #ifdef SERIAL_DEBUG
            Serial.print(cyl + 1);
            Serial.print("X"); // incorrect
            Serial.println(expected_to_fire_next + 1);
          #endif
        }
        
        fired_last = cyl;

      }
        
      cylinder_last_state[cyl] = cylinder_new_state;
    }
  }
}
