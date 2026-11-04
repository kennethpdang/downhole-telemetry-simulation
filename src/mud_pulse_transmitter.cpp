#include "mud_pulse_transmitter.h"

// 0xAA is 10101010. A few of these before the frame let the surface lock onto the bit timing.
static constexpr uint8_t PREAMBLE_BYTE = 0xAA;
static constexpr size_t PREAMBLE_BYTE_COUNT = 2;
static constexpr int BITS_PER_BYTE = 8;

MudPulseTransmitter::MudPulseTransmitter(uint8_t valve_pin, uint32_t bit_period_milliseconds)
	: valve_pin_(valve_pin), half_bit_ticks_(pdMS_TO_TICKS(bit_period_milliseconds / 2)) {
}

void MudPulseTransmitter::begin() {
	pinMode(valve_pin_, OUTPUT);
	digitalWrite(valve_pin_, LOW);
}

uint32_t MudPulseTransmitter::bits_for_frame(size_t length) const {
	return (uint32_t)((PREAMBLE_BYTE_COUNT + length) * BITS_PER_BYTE);
}

void MudPulseTransmitter::transmit(const uint8_t* data, size_t length) {
	last_wake_tick_ = xTaskGetTickCount();

	for (size_t preamble_index = 0; preamble_index < PREAMBLE_BYTE_COUNT; preamble_index++) {
		send_byte(PREAMBLE_BYTE);
	}
	for (size_t byte_index = 0; byte_index < length; byte_index++) {
		send_byte(data[byte_index]);
	}

	digitalWrite(valve_pin_, LOW);
}

void MudPulseTransmitter::send_byte(uint8_t value) {
	for (int bit_index = BITS_PER_BYTE - 1; bit_index >= 0; bit_index--) {
		const bool bit_value = ((value >> bit_index) & 0x01) != 0;
		send_bit(bit_value);
	}
}

void MudPulseTransmitter::send_bit(bool bit_value) {
	if (bit_value) {
		hold_valve(false);
		hold_valve(true);
	} else {
		hold_valve(true);
		hold_valve(false);
	}
}

void MudPulseTransmitter::hold_valve(bool valve_closed) {
	digitalWrite(valve_pin_, valve_closed ? HIGH : LOW);

	// vTaskDelayUntil keeps every half-bit the same length, even if this task
	// was briefly interrupted, so the bit timing never drifts.
	vTaskDelayUntil(&last_wake_tick_, half_bit_ticks_);
}
