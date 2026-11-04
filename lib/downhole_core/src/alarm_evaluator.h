#pragma once

#include "drilling_data.h"

// Every limit lives in one struct, so a different tool or well can use
// different numbers without touching the evaluation code.
struct AlarmThresholds {
	int16_t maximum_temperature_centi_celsius;
	uint32_t maximum_pressure_change_pascal;
	uint16_t maximum_shock_milli_g;
	uint16_t maximum_jerk_milli_g_per_second;
	uint8_t maximum_stick_slip_percent;
	uint16_t minimum_mud_flow_gallons_per_minute;
	uint16_t shale_gamma_ray_api;
};

// Compares one WindowSummary against the thresholds and returns the
// status bits that should be raised. It remembers the previous pressure so it
// can spot a sudden jump between two frames (a possible kick).
class AlarmEvaluator {
	public:
		explicit AlarmEvaluator(const AlarmThresholds& thresholds);
		uint16_t evaluate(const WindowSummary& summary);

	private:
		bool is_pressure_kick(const WindowSummary& summary);

		AlarmThresholds thresholds_;
		bool has_previous_pressure_ = false;
		uint32_t previous_pressure_pascal_ = 0;
};
