#pragma once

#include <stddef.h>
#include <stdint.h>

#include "drilling_data.h"

// Prints a frame three ways: decoded values a person can read, the raw bytes
// in hex (what really goes up the wire), and the names of any raised flags.
void report_telemetry_frame(const WindowSummary& summary, uint8_t sequence_number, const uint8_t* frame_bytes, size_t frame_length);
