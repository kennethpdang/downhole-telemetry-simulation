#pragma once

#include <stdint.h>

#include "alarm_evaluator.h"

// ---------------------------------------------------------------------------
// Pins. Change the wiring here and nowhere else.
// ---------------------------------------------------------------------------
constexpr uint8_t I2C_DATA_PIN = 21;          // shared by BMP180 (0x77) and MPU6050 (0x68)
constexpr uint8_t I2C_CLOCK_PIN = 22;
constexpr uint8_t MUD_FLOW_ADC_PIN = 34;      // potentiometer: mud flow meter stand-in
constexpr uint8_t GAMMA_RAY_ADC_PIN = 35;     // photoresistor: gamma ray scintillator stand-in
constexpr uint8_t FAULT_BUTTON_PIN = 15;      // manual fault injection, falling-edge interrupt
constexpr uint8_t MUD_PULSE_VALVE_PIN = 4;    // yellow LED: lit = pulser valve closed (pressure pulse)
constexpr uint8_t ALARM_LED_PIN = 5;          // red LED: lit while any alarm flag is raised

// ---------------------------------------------------------------------------
// Timing.
// ---------------------------------------------------------------------------
constexpr uint32_t SAMPLE_PERIOD_MILLISECONDS = 100;     // 10 Hz, fast enough to see jerk
constexpr uint16_t SAMPLES_PER_FRAME = 10;               // one telemetry frame per second
constexpr uint32_t MUD_PULSE_BIT_PERIOD_MILLISECONDS = 50; // 20 bits per second, a fast real mud pulser
constexpr uint32_t FAULT_DEBOUNCE_MILLISECONDS = 200;

// ---------------------------------------------------------------------------
// Sensor scaling. The ADC returns 0..4095; these are the values at full scale.
// ---------------------------------------------------------------------------
constexpr uint16_t MAXIMUM_MUD_FLOW_GALLONS_PER_MINUTE = 1000;
constexpr uint16_t MAXIMUM_GAMMA_RAY_API = 150;

// ---------------------------------------------------------------------------
// Alarm limits. Scaled to what the Wokwi parts can produce, not to a real well
// (real tools are rated to roughly 150 to 175 C and 20,000+ psi).
// ---------------------------------------------------------------------------
constexpr AlarmThresholds DEFAULT_ALARM_THRESHOLDS = {
	6000,  // maximum_temperature_centi_celsius: 60.00 C
	2000,  // maximum_pressure_change_pascal: 20 hPa jump between two frames
	2000,  // maximum_shock_milli_g: 2 g
	5000,  // maximum_jerk_milli_g_per_second: 5 g/s
	50,    // maximum_stick_slip_percent
	200,   // minimum_mud_flow_gallons_per_minute
	75,    // shale_gamma_ray_api
};

// ---------------------------------------------------------------------------
// FreeRTOS task setup. Higher number = higher priority.
// ---------------------------------------------------------------------------
constexpr uint32_t TASK_STACK_BYTES = 4096;
constexpr uint8_t SAMPLING_TASK_PRIORITY = 3;
constexpr uint8_t TELEMETRY_TASK_PRIORITY = 2;
constexpr uint8_t MUD_PULSE_TASK_PRIORITY = 1;
constexpr uint8_t SAMPLE_QUEUE_LENGTH = 20;
