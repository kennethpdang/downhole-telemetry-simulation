#pragma once

#include <Arduino.h>

// Several tasks print to the same UART. A mutex makes sure one task finishes
// its line before another task starts, so lines never get mixed together.
class SerialLogger {
	public:
		void begin(uint32_t baud_rate);

		// printf-style formatting, one complete line per call.
		void print_line(const char* format, ...) __attribute__((format(printf, 2, 3)));

	private:
		SemaphoreHandle_t serial_mutex_ = nullptr;
};

extern SerialLogger serial_logger;
