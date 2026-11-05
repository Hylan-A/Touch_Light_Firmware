#pragma once
#include <Arduino.h>

/*
  Tech 49 Lab - Touch Light board  GPIO configuration.

   ESP32-C3 GPIO21 touch and GPIO4 PWM were verified in the
  Tech 49 Lab on Oct 30, 2026 (local touch/dimming/serial logs).

  Verified in the Rev1H KiCad PCB routing (U2 through R3 to Q1):
  XIAO ESP32-C3 D6/GPIO21 is the IQS228B touch input.
  XIAO ESP32-C3 D2/GPIO4 is the LED PWM output.
  An older schematic text note incorrectly said D3/GPIO5 for PWM;
  on the actual Rev1H PCB D3/GPIO5 is unconnected.

  If reusing this firmware on other hardware, verify the board wiring
  again before changing any GPIO settings.
*/
#define BOARD_0008_PINOUT_CONFIRMED 1
#if BOARD_0008_PINOUT_CONFIRMED != 1
  #error "Board 0008 ESP32-C3 pinout unverified. Check BoardConfig.h, set touch/PWM/status pins correctly, then set BOARD_0008_PINOUT_CONFIRMED to 1."
#endif

constexpr uint8_t TOUCH_PIN = 43;      // XIAO ESP32-C3 D6 <- IQS228B OUT (Rev1H PCB)
constexpr uint8_t LED_PWM_PIN = 3;     // Seeed XIAO ESP32-C3 D2 -> R3 -> Q1 gate (Rev1H PCB)
constexpr uint8_t STATUS_LED_PIN = 8;  // unused unless STATUS_LED_PRESENT is true
constexpr bool STATUS_LED_PRESENT = false; // Seeed XIAO C3 lacks user heartbeat LED

// IQS228B touch sensor active LOW, confirmed by Touch Light 0008 bench testing.
constexpr uint8_t TOUCH_INPUT_MODE = INPUT_PULLDOWN;

