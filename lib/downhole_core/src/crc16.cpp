#include "crc16.h"

static constexpr uint16_t CRC16_POLYNOMIAL = 0x1021;
static constexpr uint16_t CRC16_INITIAL_VALUE = 0xFFFF;
static constexpr uint16_t CRC16_TOP_BIT = 0x8000;
static constexpr int BITS_PER_BYTE = 8;

uint16_t compute_crc16(const uint8_t* data, size_t length) {
	uint16_t crc_value = CRC16_INITIAL_VALUE;

	for (size_t byte_index = 0; byte_index < length; byte_index++) {
		crc_value ^= (uint16_t)(data[byte_index] << BITS_PER_BYTE);

		for (int bit_index = 0; bit_index < BITS_PER_BYTE; bit_index++) {
			if (crc_value & CRC16_TOP_BIT) {
				crc_value = (uint16_t)((crc_value << 1) ^ CRC16_POLYNOMIAL);
			} else {
				crc_value = (uint16_t)(crc_value << 1);
			}
		}
	}

	return crc_value;
}
