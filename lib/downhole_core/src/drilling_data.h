#pragma once

#include <stdint.h>

// One reading of every sensor, taken by the sampling task every 100 ms.
struct SensorSample {
	uint32_t timestamp_milliseconds;

	bool pressure_ok;
	int32_t temperature_centi_celsius;   // 2450 means 24.50 C
	uint32_t pressure_pascal;

	bool motion_ok;
	float acceleration_magnitude_g;      // 1.0 g when the tool sits still
	float rotation_rate_rpm;             // spin around the tool's long axis

	uint16_t mud_flow_gallons_per_minute;
	uint16_t gamma_ray_api;              // API units: 0 = clean sand, 150 = very "hot" shale
};

// One telemetry frame worth of data: ten samples squeezed into one record.
// Every field is a fixed size integer so it packs into bytes with no surprises.
struct WindowSummary {
	uint32_t timestamp_milliseconds;
	int16_t temperature_centi_celsius;
	uint32_t pressure_pascal;
	uint16_t peak_shock_milli_g;
	uint16_t peak_jerk_milli_g_per_second;
	int16_t average_rotation_rpm;
	uint8_t stick_slip_percent;
	uint16_t mud_flow_gallons_per_minute;
	uint16_t gamma_ray_api;
	uint16_t status_flags;
};

// Each condition owns one bit of WindowSummary::status_flags.
enum StatusFlag : uint16_t {
	STATUS_MANUAL_FAULT = 1u << 0,
	STATUS_PRESSURE_SENSOR_FAILED = 1u << 1,
	STATUS_MOTION_SENSOR_FAILED = 1u << 2,
	STATUS_OVER_TEMPERATURE = 1u << 3,
	STATUS_PRESSURE_KICK = 1u << 4,
	STATUS_HIGH_SHOCK = 1u << 5,
	STATUS_HIGH_JERK = 1u << 6,
	STATUS_STICK_SLIP = 1u << 7,
	STATUS_LOW_MUD_FLOW = 1u << 8,
	STATUS_SHALE_FORMATION = 1u << 9,
};

constexpr int STATUS_FLAG_COUNT = 10;

// Shale is information for the geologist, not a problem, so it does not light the alarm LED.
constexpr uint16_t STATUS_ALARM_MASK = (uint16_t)~STATUS_SHALE_FORMATION;

// Returns a short name such as "HIGH_SHOCK" for the flag stored at bit_position.
const char* status_flag_name(int bit_position);
