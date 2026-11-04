#include "telemetry_frame.h"

#include "crc16.h"

static constexpr size_t CRC_OFFSET = TELEMETRY_FRAME_LENGTH - 2;
static constexpr int BITS_PER_BYTE = 8;

// Appends integers to a byte buffer, most significant byte first.
class ByteWriter {
	public:
		explicit ByteWriter(uint8_t* buffer) : buffer_(buffer) {
		}

		void write_uint8(uint8_t value) {
			buffer_[position_] = value;
			position_++;
		}

		void write_uint16(uint16_t value) {
			write_uint8((uint8_t)(value >> BITS_PER_BYTE));
			write_uint8((uint8_t)(value & 0xFF));
		}

		void write_uint32(uint32_t value) {
			write_uint16((uint16_t)(value >> (2 * BITS_PER_BYTE)));
			write_uint16((uint16_t)(value & 0xFFFF));
		}

		size_t position() const {
			return position_;
		}

	private:
		uint8_t* buffer_;
		size_t position_ = 0;
};

// Reads integers back out in the same order ByteWriter wrote them.
class ByteReader {
	public:
		explicit ByteReader(const uint8_t* buffer) : buffer_(buffer) {
		}

		uint8_t read_uint8() {
			const uint8_t value = buffer_[position_];
			position_++;
			return value;
		}

		uint16_t read_uint16() {
			const uint16_t high_byte = read_uint8();
			const uint16_t low_byte = read_uint8();
			return (uint16_t)((high_byte << BITS_PER_BYTE) | low_byte);
		}

		uint32_t read_uint32() {
			const uint32_t high_half = read_uint16();
			const uint32_t low_half = read_uint16();
			return (high_half << (2 * BITS_PER_BYTE)) | low_half;
		}

	private:
		const uint8_t* buffer_;
		size_t position_ = 0;
};

size_t encode_telemetry_frame(const WindowSummary& summary, uint8_t sequence_number, uint8_t* output_buffer, size_t buffer_capacity) {
	if (buffer_capacity < TELEMETRY_FRAME_LENGTH) {
		return 0;
	}

	ByteWriter writer(output_buffer);
	writer.write_uint8(TELEMETRY_SYNC_FIRST_BYTE);
	writer.write_uint8(TELEMETRY_SYNC_SECOND_BYTE);
	writer.write_uint8(sequence_number);
	writer.write_uint32(summary.timestamp_milliseconds);
	writer.write_uint16((uint16_t)summary.temperature_centi_celsius);
	writer.write_uint32(summary.pressure_pascal);
	writer.write_uint16(summary.peak_shock_milli_g);
	writer.write_uint16(summary.peak_jerk_milli_g_per_second);
	writer.write_uint16((uint16_t)summary.average_rotation_rpm);
	writer.write_uint8(summary.stick_slip_percent);
	writer.write_uint16(summary.mud_flow_gallons_per_minute);
	writer.write_uint16(summary.gamma_ray_api);
	writer.write_uint16(summary.status_flags);
	writer.write_uint16(compute_crc16(output_buffer, writer.position()));

	return writer.position();
}

FrameDecodeResult decode_telemetry_frame(const uint8_t* input_buffer, size_t input_length, WindowSummary& summary, uint8_t& sequence_number) {
	if (input_length < TELEMETRY_FRAME_LENGTH) {
		return FrameDecodeResult::TOO_SHORT;
	}
	if (input_buffer[0] != TELEMETRY_SYNC_FIRST_BYTE || input_buffer[1] != TELEMETRY_SYNC_SECOND_BYTE) {
		return FrameDecodeResult::BAD_SYNC;
	}

	ByteReader crc_reader(input_buffer + CRC_OFFSET);
	const uint16_t received_crc = crc_reader.read_uint16();
	if (received_crc != compute_crc16(input_buffer, CRC_OFFSET)) {
		return FrameDecodeResult::BAD_CRC;
	}

	ByteReader reader(input_buffer + 2);
	sequence_number = reader.read_uint8();
	summary.timestamp_milliseconds = reader.read_uint32();
	summary.temperature_centi_celsius = (int16_t)reader.read_uint16();
	summary.pressure_pascal = reader.read_uint32();
	summary.peak_shock_milli_g = reader.read_uint16();
	summary.peak_jerk_milli_g_per_second = reader.read_uint16();
	summary.average_rotation_rpm = (int16_t)reader.read_uint16();
	summary.stick_slip_percent = reader.read_uint8();
	summary.mud_flow_gallons_per_minute = reader.read_uint16();
	summary.gamma_ray_api = reader.read_uint16();
	summary.status_flags = reader.read_uint16();

	return FrameDecodeResult::OK;
}
