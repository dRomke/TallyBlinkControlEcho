#pragma once

#include <Arduino.h>

// Shared board timing and RF settings.
static const uint32_t BOARD_STARTUP_DELAY_MS = 500;
static const uint32_t LOOP_DELAY_MS          = 10;
static const uint32_t AIRTIME_REPORT_MS      = 10000;
static const uint32_t TALLY_REINJECT_MS      = 250;

static const float    LORA_FREQ_MHZ  = 863.4f;
static const float    LORA_BW_KHZ    = 500.0f;
static const uint8_t  LORA_SF        = 5;
static const uint32_t SERIAL_BAUD    = 115200;

static const size_t LORA_MAX_PAYLOAD = 255;
static const size_t LORA_MAX_RAW     = LORA_MAX_PAYLOAD - 1;

static const uint8_t LED_ACTIVE   = LOW;
static const uint8_t LED_INACTIVE = HIGH;

// LLCC68 module wiring (both boards).
static const uint8_t LORA_PIN_CS    = 10;
static const uint8_t LORA_PIN_DIO1  = 2;
static const uint8_t LORA_PIN_RESET = 3;
static const uint8_t LORA_PIN_BUSY  = 9;