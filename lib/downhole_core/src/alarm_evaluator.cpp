#include "alarm_evaluator.h"

AlarmEvaluator::AlarmEvaluator(const AlarmThresholds& thresholds) : thresholds_(thresholds) {
}

uint16_t AlarmEvaluator::evaluate(const WindowSummary& summary) {
	uint16_t raised_flags = 0;
	const bool pressure_valid = (summary.status_flags & STATUS_PRESSURE_SENSOR_FAILED) == 0;
	const bool motion_valid = (summary.status_flags & STATUS_MOTION_SENSOR_FAILED) == 0;

	if (pressure_valid) {
		if (summary.temperature_centi_celsius > thresholds_.maximum_temperature_centi_celsius) {
			raised_flags |= STATUS_OVER_TEMPERATURE;
		}
		if (is_pressure_kick(summary)) {
			raised_flags |= STATUS_PRESSURE_KICK;
		}
	}

	if (motion_valid) {
		if (summary.peak_shock_milli_g > thresholds_.maximum_shock_milli_g) {
			raised_flags |= STATUS_HIGH_SHOCK;
		}
		if (summary.peak_jerk_milli_g_per_second > thresholds_.maximum_jerk_milli_g_per_second) {
			raised_flags |= STATUS_HIGH_JERK;
		}
		if (summary.stick_slip_percent > thresholds_.maximum_stick_slip_percent) {
			raised_flags |= STATUS_STICK_SLIP;
		}
	}

	if (summary.mud_flow_gallons_per_minute < thresholds_.minimum_mud_flow_gallons_per_minute) {
		raised_flags |= STATUS_LOW_MUD_FLOW;
	}
	if (summary.gamma_ray_api > thresholds_.shale_gamma_ray_api) {
		raised_flags |= STATUS_SHALE_FORMATION;
	}

	return raised_flags;
}

bool AlarmEvaluator::is_pressure_kick(const WindowSummary& summary) {
	const uint32_t current_pressure_pascal = summary.pressure_pascal;
	bool kick_detected = false;

	if (has_previous_pressure_) {
		uint32_t pressure_change_pascal = 0;
		if (current_pressure_pascal > previous_pressure_pascal_) {
			pressure_change_pascal = current_pressure_pascal - previous_pressure_pascal_;
		} else {
			pressure_change_pascal = previous_pressure_pascal_ - current_pressure_pascal;
		}
		kick_detected = pressure_change_pascal > thresholds_.maximum_pressure_change_pascal;
	}

	has_previous_pressure_ = true;
	previous_pressure_pascal_ = current_pressure_pascal;
	return kick_detected;
}
