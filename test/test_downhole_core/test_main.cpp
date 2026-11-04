// Unit tests for the hardware-free logic. Run with: pio test -e native
#include <unity.h>

#include "alarm_evaluator.h"
#include "crc16.h"
#include "drilling_window.h"
#include "telemetry_frame.h"

static const AlarmThresholds TEST_THRESHOLDS = {
	6000,  // maximum_temperature_centi_celsius
	2000,  // maximum_pressure_change_pascal
	2000,  // maximum_shock_milli_g
	5000,  // maximum_jerk_milli_g_per_second
	50,    // maximum_stick_slip_percent
	200,   // minimum_mud_flow_gallons_per_minute
	75,    // shale_gamma_ray_api
};

void setUp() {
}

void tearDown() {
}

static SensorSample make_calm_sample(uint32_t timestamp_milliseconds) {
	SensorSample sample{};
	sample.timestamp_milliseconds = timestamp_milliseconds;
	sample.pressure_ok = true;
	sample.temperature_centi_celsius = 2400;
	sample.pressure_pascal = 101325;
	sample.motion_ok = true;
	sample.acceleration_magnitude_g = 1.0f;
	sample.rotation_rate_rpm = 30.0f;
	sample.mud_flow_gallons_per_minute = 500;
	sample.gamma_ray_api = 40;
	return sample;
}

static WindowSummary make_calm_summary() {
	WindowSummary summary{};
	summary.temperature_centi_celsius = 2400;
	summary.pressure_pascal = 101325;
	summary.peak_shock_milli_g = 1000;
	summary.mud_flow_gallons_per_minute = 500;
	summary.gamma_ray_api = 40;
	return summary;
}

// ----- CRC -------------------------------------------------------------------

void test_crc16_matches_standard_check_value() {
	const uint8_t check_input[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	TEST_ASSERT_EQUAL_HEX16(0x29B1, compute_crc16(check_input, sizeof(check_input)));
}

// ----- Frame encode / decode -------------------------------------------------

void test_frame_round_trip_keeps_every_field() {
	WindowSummary original_summary = make_calm_summary();
	original_summary.timestamp_milliseconds = 123456;
	original_summary.temperature_centi_celsius = -1234;
	original_summary.peak_jerk_milli_g_per_second = 7777;
	original_summary.average_rotation_rpm = -42;
	original_summary.stick_slip_percent = 61;
	original_summary.status_flags = STATUS_HIGH_SHOCK | STATUS_SHALE_FORMATION;

	uint8_t frame_bytes[TELEMETRY_FRAME_LENGTH];
	const size_t frame_length = encode_telemetry_frame(original_summary, 7, frame_bytes, sizeof(frame_bytes));
	TEST_ASSERT_EQUAL_UINT32(TELEMETRY_FRAME_LENGTH, frame_length);
	TEST_ASSERT_EQUAL_HEX8(0xAA, frame_bytes[0]);
	TEST_ASSERT_EQUAL_HEX8(0x55, frame_bytes[1]);

	WindowSummary decoded_summary{};
	uint8_t decoded_sequence_number = 0;
	const FrameDecodeResult decode_result = decode_telemetry_frame(frame_bytes, frame_length, decoded_summary, decoded_sequence_number);

	TEST_ASSERT_TRUE(decode_result == FrameDecodeResult::OK);
	TEST_ASSERT_EQUAL_UINT8(7, decoded_sequence_number);
	TEST_ASSERT_EQUAL_UINT32(original_summary.timestamp_milliseconds, decoded_summary.timestamp_milliseconds);
	TEST_ASSERT_EQUAL_INT16(original_summary.temperature_centi_celsius, decoded_summary.temperature_centi_celsius);
	TEST_ASSERT_EQUAL_UINT32(original_summary.pressure_pascal, decoded_summary.pressure_pascal);
	TEST_ASSERT_EQUAL_UINT16(original_summary.peak_shock_milli_g, decoded_summary.peak_shock_milli_g);
	TEST_ASSERT_EQUAL_UINT16(original_summary.peak_jerk_milli_g_per_second, decoded_summary.peak_jerk_milli_g_per_second);
	TEST_ASSERT_EQUAL_INT16(original_summary.average_rotation_rpm, decoded_summary.average_rotation_rpm);
	TEST_ASSERT_EQUAL_UINT8(original_summary.stick_slip_percent, decoded_summary.stick_slip_percent);
	TEST_ASSERT_EQUAL_UINT16(original_summary.mud_flow_gallons_per_minute, decoded_summary.mud_flow_gallons_per_minute);
	TEST_ASSERT_EQUAL_UINT16(original_summary.gamma_ray_api, decoded_summary.gamma_ray_api);
	TEST_ASSERT_EQUAL_HEX16(original_summary.status_flags, decoded_summary.status_flags);
}

void test_frame_with_flipped_bit_fails_crc() {
	uint8_t frame_bytes[TELEMETRY_FRAME_LENGTH];
	const size_t frame_length = encode_telemetry_frame(make_calm_summary(), 1, frame_bytes, sizeof(frame_bytes));
	frame_bytes[10] ^= 0x04;

	WindowSummary decoded_summary{};
	uint8_t decoded_sequence_number = 0;
	const FrameDecodeResult decode_result = decode_telemetry_frame(frame_bytes, frame_length, decoded_summary, decoded_sequence_number);
	TEST_ASSERT_TRUE(decode_result == FrameDecodeResult::BAD_CRC);
}

void test_encode_refuses_small_buffer() {
	uint8_t small_buffer[TELEMETRY_FRAME_LENGTH - 1];
	TEST_ASSERT_EQUAL_UINT32(0, encode_telemetry_frame(make_calm_summary(), 0, small_buffer, sizeof(small_buffer)));
}

// ----- Drilling window -------------------------------------------------------

void test_window_measures_peak_shock_and_jerk() {
	DrillingWindow drilling_window;
	drilling_window.add_sample(make_calm_sample(0));

	SensorSample shock_sample = make_calm_sample(100);
	shock_sample.acceleration_magnitude_g = 3.0f;
	drilling_window.add_sample(shock_sample);

	const WindowSummary summary = drilling_window.build_summary();
	TEST_ASSERT_EQUAL_UINT16(2, drilling_window.sample_count());
	TEST_ASSERT_EQUAL_UINT16(3000, summary.peak_shock_milli_g);
	// A 2 g change in 0.1 s is 20 g/s.
	TEST_ASSERT_EQUAL_UINT16(20000, summary.peak_jerk_milli_g_per_second);
}

void test_window_measures_stick_slip_severity() {
	DrillingWindow drilling_window;
	const float rotation_pattern_rpm[] = { 0.0f, 60.0f, 0.0f, 60.0f };

	for (int sample_index = 0; sample_index < 4; sample_index++) {
		SensorSample sample = make_calm_sample(sample_index * 100);
		sample.rotation_rate_rpm = rotation_pattern_rpm[sample_index];
		drilling_window.add_sample(sample);
	}

	const WindowSummary summary = drilling_window.build_summary();
	// (60 - 0) / (2 x 30) = 100 %
	TEST_ASSERT_EQUAL_INT16(30, summary.average_rotation_rpm);
	TEST_ASSERT_EQUAL_UINT8(100, summary.stick_slip_percent);
}

void test_window_flags_failed_sensors() {
	DrillingWindow drilling_window;
	SensorSample broken_sample = make_calm_sample(0);
	broken_sample.pressure_ok = false;
	broken_sample.motion_ok = false;
	drilling_window.add_sample(broken_sample);

	const WindowSummary summary = drilling_window.build_summary();
	TEST_ASSERT_TRUE(summary.status_flags & STATUS_PRESSURE_SENSOR_FAILED);
	TEST_ASSERT_TRUE(summary.status_flags & STATUS_MOTION_SENSOR_FAILED);
}

// ----- Alarm evaluator -------------------------------------------------------

void test_calm_drilling_raises_no_flags() {
	AlarmEvaluator alarm_evaluator(TEST_THRESHOLDS);
	TEST_ASSERT_EQUAL_HEX16(0, alarm_evaluator.evaluate(make_calm_summary()));
	TEST_ASSERT_EQUAL_HEX16(0, alarm_evaluator.evaluate(make_calm_summary()));
}

void test_alarms_raise_matching_flags() {
	AlarmEvaluator alarm_evaluator(TEST_THRESHOLDS);
	WindowSummary summary = make_calm_summary();
	summary.temperature_centi_celsius = 7000;
	summary.peak_shock_milli_g = 2500;
	summary.peak_jerk_milli_g_per_second = 9000;
	summary.stick_slip_percent = 80;
	summary.mud_flow_gallons_per_minute = 50;
	summary.gamma_ray_api = 120;

	const uint16_t expected_flags = STATUS_OVER_TEMPERATURE | STATUS_HIGH_SHOCK | STATUS_HIGH_JERK | STATUS_STICK_SLIP | STATUS_LOW_MUD_FLOW | STATUS_SHALE_FORMATION;
	TEST_ASSERT_EQUAL_HEX16(expected_flags, alarm_evaluator.evaluate(summary));
}

void test_sudden_pressure_change_is_a_kick() {
	AlarmEvaluator alarm_evaluator(TEST_THRESHOLDS);
	WindowSummary summary = make_calm_summary();
	TEST_ASSERT_EQUAL_HEX16(0, alarm_evaluator.evaluate(summary));

	summary.pressure_pascal += 5000;
	TEST_ASSERT_EQUAL_HEX16(STATUS_PRESSURE_KICK, alarm_evaluator.evaluate(summary));
}

int main() {
	UNITY_BEGIN();
	RUN_TEST(test_crc16_matches_standard_check_value);
	RUN_TEST(test_frame_round_trip_keeps_every_field);
	RUN_TEST(test_frame_with_flipped_bit_fails_crc);
	RUN_TEST(test_encode_refuses_small_buffer);
	RUN_TEST(test_window_measures_peak_shock_and_jerk);
	RUN_TEST(test_window_measures_stick_slip_severity);
	RUN_TEST(test_window_flags_failed_sensors);
	RUN_TEST(test_calm_drilling_raises_no_flags);
	RUN_TEST(test_alarms_raise_matching_flags);
	RUN_TEST(test_sudden_pressure_change_is_a_kick);
	return UNITY_END();
}
