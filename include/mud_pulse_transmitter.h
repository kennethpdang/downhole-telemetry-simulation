#pragma once

#include <Arduino.h>

// Simulates a mud pulser: a valve inside the drill pipe that briefly blocks
// the mud flow, sending a pressure pulse up to sensors at the surface.
// Here the "valve" is an LED: lit = valve closed = pressure pulse.
//
// Bits are sent with Manchester encoding (IEEE 802.3 convention). Every bit
// is split into two halves and always has a change in the middle:
//   bit 1 = low then high
//   bit 0 = high then low
// The surface can recover the timing from those mid-bit changes, and a long
// run of zeros or ones never looks like a dead line.
class MudPulseTransmitter {
	public:
		MudPulseTransmitter(uint8_t valve_pin, uint32_t bit_period_milliseconds);
		void begin();

		// Blocks for the whole transmission, so call it from its own task.
		void transmit(const uint8_t* data, size_t length);

		uint32_t bits_for_frame(size_t length) const;

	private:
		void send_byte(uint8_t value);
		void send_bit(bool bit_value);
		void hold_valve(bool valve_closed);

		uint8_t valve_pin_;
		TickType_t half_bit_ticks_;
		TickType_t last_wake_tick_ = 0;
};
