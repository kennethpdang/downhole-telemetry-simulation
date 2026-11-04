#include "serial_logger.h"

#include <stdarg.h>

static constexpr size_t LOG_LINE_CAPACITY = 192;

SerialLogger serial_logger;

void SerialLogger::begin(uint32_t baud_rate) {
	Serial.begin(baud_rate);
	serial_mutex_ = xSemaphoreCreateMutex();
}

void SerialLogger::print_line(const char* format, ...) {
	char line_buffer[LOG_LINE_CAPACITY];

	va_list argument_list;
	va_start(argument_list, format);
	vsnprintf(line_buffer, sizeof(line_buffer), format, argument_list);
	va_end(argument_list);

	xSemaphoreTake(serial_mutex_, portMAX_DELAY);
	Serial.println(line_buffer);
	xSemaphoreGive(serial_mutex_);
}
