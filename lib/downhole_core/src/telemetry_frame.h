#pragma once

#include <stddef.h>
#include <stdint.h>

#include "drilling_data.h"

// Binary uplink frame, all multi-byte fields big-endian (most significant byte first):
//
//   offset  size  field
//        0     2  sync word 0xAA 0x55
//        2     1  sequence number (wraps 255 -> 0)
//        3     4  timestamp, ms since boot
//        7     2  temperature, hundredths of a degree C (signed)
//        9     4  pressure, Pa
//       13     2  peak shock, milli-g
//       15     2  peak jerk, milli-g per second
//       17     2  average rotation, RPM (signed)
//       19     1  stick-slip severity, %
//       20     2  mud flow, gallons per minute
//       22     2  gamma ray, API units
//       24     2  status flags (see StatusFlag)
//       26     2  CRC-16/CCITT-FALSE of bytes 0..25
//
// 28 bytes total, versus roughly 150 bytes for the same data as text.
constexpr uint8_t TELEMETRY_SYNC_FIRST_BYTE = 0xAA;
constexpr uint8_t TELEMETRY_SYNC_SECOND_BYTE = 0x55;
constexpr size_t TELEMETRY_FRAME_LENGTH = 28;

enum class FrameDecodeResult {
	OK,
	TOO_SHORT,
	BAD_SYNC,
	BAD_CRC,
};

// Writes one frame into output_buffer. Returns the number of bytes written,
// or 0 if the buffer is smaller than TELEMETRY_FRAME_LENGTH.
size_t encode_telemetry_frame(const WindowSummary& summary, uint8_t sequence_number, uint8_t* output_buffer, size_t buffer_capacity);

// The surface side: checks sync and CRC, then unpacks the fields.
FrameDecodeResult decode_telemetry_frame(const uint8_t* input_buffer, size_t input_length, WindowSummary& summary, uint8_t& sequence_number);
