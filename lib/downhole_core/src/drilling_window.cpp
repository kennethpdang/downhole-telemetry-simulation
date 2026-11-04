#include "drilling_window.h"

#include <math.h>

static constexpr float MILLISECONDS_PER_SECOND = 1000.0f;
static constexpr float MILLI_PER_UNIT = 1000.0f;
static constexpr float MINIMUM_ROTATING_RPM = 5.0f;
static constexpr float MAXIMUM_STICK_SLIP_PERCENT = 255.0f;

// Rounds a non-negative value and clamps it into a uint16_t field.
static uint16_t to_clamped_uint16(float value) {
	if (value <= 0.0f) {
		return 0;
	}
	if (value >= 65535.0f) {
		return 65535;
	}
	return (uint16_t)lroundf(value);
}

void DrillingWindow::add_sample(const SensorSample& sample) {
	sample_count_++;
	latest_sample_ = sample;

	if (!sample.pressure_ok) {
		pressure_failed_ = true;
	}

	if (sample.motion_ok) {
		track_motion(sample);
	} else {
		motion_failed_ = true;
	}
}

void DrillingWindow::track_motion(const SensorSample& sample) {
	const float acceleration_g = sample.acceleration_magnitude_g;
	const float rotation_rpm = sample.rotation_rate_rpm;

	if (acceleration_g > peak_shock_g_) {
		peak_shock_g_ = acceleration_g;
	}

	// Jerk is how fast acceleration changes: (change in g) / (change in seconds).
	if (has_previous_acceleration_) {
		const uint32_t elapsed_milliseconds = sample.timestamp_milliseconds - previous_motion_timestamp_milliseconds_;

		if (elapsed_milliseconds > 0) {
			const float elapsed_seconds = elapsed_milliseconds / MILLISECONDS_PER_SECOND;
			const float jerk_g_per_second = fabsf(acceleration_g - previous_acceleration_g_) / elapsed_seconds;

			if (jerk_g_per_second > peak_jerk_g_per_second_) {
				peak_jerk_g_per_second_ = jerk_g_per_second;
			}
		}
	}

	has_previous_acceleration_ = true;
	previous_acceleration_g_ = acceleration_g;
	previous_motion_timestamp_milliseconds_ = sample.timestamp_milliseconds;

	if (motion_sample_count_ == 0) {
		minimum_rotation_rpm_ = rotation_rpm;
		maximum_rotation_rpm_ = rotation_rpm;
	}
	if (rotation_rpm < minimum_rotation_rpm_) {
		minimum_rotation_rpm_ = rotation_rpm;
	}
	if (rotation_rpm > maximum_rotation_rpm_) {
		maximum_rotation_rpm_ = rotation_rpm;
	}

	rotation_rpm_sum_ += rotation_rpm;
	motion_sample_count_++;
}

uint16_t DrillingWindow::sample_count() const {
	return sample_count_;
}

WindowSummary DrillingWindow::build_summary() const {
	WindowSummary summary{};
	summary.timestamp_milliseconds = latest_sample_.timestamp_milliseconds;
	summary.temperature_centi_celsius = (int16_t)latest_sample_.temperature_centi_celsius;
	summary.pressure_pascal = latest_sample_.pressure_pascal;
	summary.mud_flow_gallons_per_minute = latest_sample_.mud_flow_gallons_per_minute;
	summary.gamma_ray_api = latest_sample_.gamma_ray_api;
	summary.peak_shock_milli_g = to_clamped_uint16(peak_shock_g_ * MILLI_PER_UNIT);
	summary.peak_jerk_milli_g_per_second = to_clamped_uint16(peak_jerk_g_per_second_ * MILLI_PER_UNIT);

	if (motion_sample_count_ > 0) {
		const float average_rpm = rotation_rpm_sum_ / motion_sample_count_;
		summary.average_rotation_rpm = (int16_t)lroundf(average_rpm);

		// Industry stick-slip severity: (max RPM - min RPM) / (2 x average RPM).
		// 0% is a smooth spin. 100% means the bit stops dead and then whips around.
		const float absolute_average_rpm = fabsf(average_rpm);
		if (absolute_average_rpm >= MINIMUM_ROTATING_RPM) {
			float severity_percent = (maximum_rotation_rpm_ - minimum_rotation_rpm_) / (2.0f * absolute_average_rpm) * 100.0f;
			if (severity_percent > MAXIMUM_STICK_SLIP_PERCENT) {
				severity_percent = MAXIMUM_STICK_SLIP_PERCENT;
			}
			summary.stick_slip_percent = (uint8_t)lroundf(severity_percent);
		}
	}

	if (pressure_failed_) {
		summary.status_flags |= STATUS_PRESSURE_SENSOR_FAILED;
	}
	if (motion_failed_) {
		summary.status_flags |= STATUS_MOTION_SENSOR_FAILED;
	}

	return summary;
}

void DrillingWindow::reset() {
	sample_count_ = 0;
	latest_sample_ = SensorSample{};
	pressure_failed_ = false;
	motion_failed_ = false;
	motion_sample_count_ = 0;
	peak_shock_g_ = 0.0f;
	peak_jerk_g_per_second_ = 0.0f;
	minimum_rotation_rpm_ = 0.0f;
	maximum_rotation_rpm_ = 0.0f;
	rotation_rpm_sum_ = 0.0f;
}
