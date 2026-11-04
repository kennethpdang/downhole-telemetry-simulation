#include "telemetry_report.h"

#include <stdio.h>

#include "serial_logger.h"

static constexpr size_t TEXT_LINE_CAPACITY = 160;
static constexpr float CENTI_PER_UNIT = 100.0f;
static constexpr float MILLI_PER_UNIT = 1000.0f;

static void report_values(const WindowSummary& summary, uint8_t sequence_number) {
	serial_logger.print_line(
		"[FRAME %3u] t=%lums T=%.2fC P=%luPa shock=%.2fg jerk=%.1fg/s rpm=%d slip=%u%% flow=%ugpm gamma=%uAPI flags=0x%04X",
		sequence_number,
		(unsigned long)summary.timestamp_milliseconds,
		summary.temperature_centi_celsius / CENTI_PER_UNIT,
		(unsigned long)summary.pressure_pascal,
		summary.peak_shock_milli_g / MILLI_PER_UNIT,
		summary.peak_jerk_milli_g_per_second / MILLI_PER_UNIT,
		summary.average_rotation_rpm,
		summary.stick_slip_percent,
		summary.mud_flow_gallons_per_minute,
		summary.gamma_ray_api,
		summary.status_flags);
}

static void report_hex(const uint8_t* frame_bytes, size_t frame_length) {
	char hex_line[TEXT_LINE_CAPACITY];
	size_t written_length = snprintf(hex_line, sizeof(hex_line), "[HEX]");

	for (size_t byte_index = 0; byte_index < frame_length; byte_index++) {
		if (written_length >= sizeof(hex_line)) {
			break;
		}
		written_length += snprintf(hex_line + written_length, sizeof(hex_line) - written_length, " %02X", frame_bytes[byte_index]);
	}

	serial_logger.print_line("%s", hex_line);
}

static void report_flags(uint16_t status_flags) {
	if (status_flags == 0) {
		return;
	}

	char flag_line[TEXT_LINE_CAPACITY];
	size_t written_length = snprintf(flag_line, sizeof(flag_line), "[FLAGS]");

	for (int bit_position = 0; bit_position < STATUS_FLAG_COUNT; bit_position++) {
		if (written_length >= sizeof(flag_line)) {
			break;
		}
		if (status_flags & (1u << bit_position)) {
			written_length += snprintf(flag_line + written_length, sizeof(flag_line) - written_length, " %s", status_flag_name(bit_position));
		}
	}

	serial_logger.print_line("%s", flag_line);
}

void report_telemetry_frame(const WindowSummary& summary, uint8_t sequence_number, const uint8_t* frame_bytes, size_t frame_length) {
	report_values(summary, sequence_number);
	report_hex(frame_bytes, frame_length);
	report_flags(summary.status_flags);
}
