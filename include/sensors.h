#pragma once

#include <Adafruit_BMP085_U.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "drilling_data.h"

// Each sensor class hides its driver and fills only its own part of a SensorSample.
// If begin() failed, read() returns false without touching the bus, so one dead
// sensor never stops the others from reporting.

// BMP180 over I2C (address 0x77): temperature and pressure.
class PressureSensor {
	public:
		bool begin();
		bool read(SensorSample& sample);

	private:
		Adafruit_BMP085_Unified bmp_driver_{10085};
		bool is_ready_ = false;
};

// MPU6050 over I2C (address 0x68): 3-axis accelerometer and 3-axis gyroscope.
// Used for shock, jerk and rotation speed (stick-slip).
class MotionSensor {
	public:
		bool begin();
		bool read(SensorSample& sample);

	private:
		Adafruit_MPU6050 mpu_driver_;
		bool is_ready_ = false;
};

// Any analog input scaled linearly to 0..full_scale_value.
// inverted is for parts whose voltage falls as the quantity rises.
class AnalogChannel {
	public:
		AnalogChannel(uint8_t adc_pin, uint16_t full_scale_value, bool inverted);
		void begin();
		uint16_t read_scaled();

	private:
		uint8_t adc_pin_;
		uint16_t full_scale_value_;
		bool inverted_;
};
