#pragma once

#include "drilling_data.h"

// Collects the fast 10 Hz samples between two telemetry frames and boils them
// down to one WindowSummary. Peaks matter more than averages downhole: one
// violent shock can crack a circuit board even if the average looks calm.
class DrillingWindow {
	public:
		void add_sample(const SensorSample& sample);
		uint16_t sample_count() const;
		WindowSummary build_summary() const;

		// Starts a new window. The last acceleration is kept so that jerk can
		// still be measured between the last sample of this window and the
		// first sample of the next one.
		void reset();

	private:
		void track_motion(const SensorSample& sample);

		uint16_t sample_count_ = 0;
		SensorSample latest_sample_{};
		bool pressure_failed_ = false;
		bool motion_failed_ = false;

		uint16_t motion_sample_count_ = 0;
		float peak_shock_g_ = 0.0f;
		float peak_jerk_g_per_second_ = 0.0f;
		float minimum_rotation_rpm_ = 0.0f;
		float maximum_rotation_rpm_ = 0.0f;
		float rotation_rpm_sum_ = 0.0f;

		bool has_previous_acceleration_ = false;
		float previous_acceleration_g_ = 0.0f;
		uint32_t previous_motion_timestamp_milliseconds_ = 0;
};
