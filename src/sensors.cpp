#include "sensors.h"

#include <Arduino.h>
#include <Wire.h>
#include <math.h>

static constexpr float HECTOPASCAL_TO_PASCAL = 100.0f;
static constexpr float CENTI_PER_UNIT = 100.0f;
static constexpr float STANDARD_GRAVITY_METERS_PER_SECOND_SQUARED = 9.80665f;
static constexpr float RADIANS_PER_SECOND_TO_RPM = 60.0f / (2.0f * (float)M_PI);
static constexpr uint8_t ADC_RESOLUTION_BITS = 12;
static constexpr uint16_t ADC_MAXIMUM_RAW = 4095;

// ----- PressureSensor -------------------------------------------------------

bool PressureSensor::begin() {
	is_ready_ = bmp_driver_.begin();
	return is_ready_;
}

bool PressureSensor::read(SensorSample& sample) {
	if (!is_ready_) {
		return false;
	}

	sensors_event_t pressure_event;
	bmp_driver_.getEvent(&pressure_event);
	if (pressure_event.pressure <= 0.0f) {
		return false;
	}

	float temperature_celsius = 0.0f;
	bmp_driver_.getTemperature(&temperature_celsius);

	sample.temperature_centi_celsius = (int32_t)lroundf(temperature_celsius * CENTI_PER_UNIT);
	sample.pressure_pascal = (uint32_t)lroundf(pressure_event.pressure * HECTOPASCAL_TO_PASCAL);
	return true;
}

// ----- MotionSensor ---------------------------------------------------------

bool MotionSensor::begin() {
	is_ready_ = mpu_driver_.begin(MPU6050_I2CADDR_DEFAULT, &Wire);
	if (!is_ready_) {
		return false;
	}

	mpu_driver_.setAccelerometerRange(MPU6050_RANGE_8_G);
	mpu_driver_.setGyroRange(MPU6050_RANGE_500_DEG);
	mpu_driver_.setFilterBandwidth(MPU6050_BAND_21_HZ);
	return true;
}

bool MotionSensor::read(SensorSample& sample) {
	if (!is_ready_) {
		return false;
	}

	sensors_event_t acceleration_event;
	sensors_event_t gyroscope_event;
	sensors_event_t chip_temperature_event;
	if (!mpu_driver_.getEvent(&acceleration_event, &gyroscope_event, &chip_temperature_event)) {
		return false;
	}

	const float acceleration_x_g = acceleration_event.acceleration.x / STANDARD_GRAVITY_METERS_PER_SECOND_SQUARED;
	const float acceleration_y_g = acceleration_event.acceleration.y / STANDARD_GRAVITY_METERS_PER_SECOND_SQUARED;
	const float acceleration_z_g = acceleration_event.acceleration.z / STANDARD_GRAVITY_METERS_PER_SECOND_SQUARED;

	// Total acceleration regardless of direction. Sitting still, this is 1 g (gravity).
	sample.acceleration_magnitude_g = sqrtf(acceleration_x_g * acceleration_x_g + acceleration_y_g * acceleration_y_g + acceleration_z_g * acceleration_z_g);

	// The z axis points along the drill string, so z rotation is how fast the bit turns.
	sample.rotation_rate_rpm = gyroscope_event.gyro.z * RADIANS_PER_SECOND_TO_RPM;
	return true;
}

// ----- AnalogChannel --------------------------------------------------------

AnalogChannel::AnalogChannel(uint8_t adc_pin, uint16_t full_scale_value, bool inverted)
	: adc_pin_(adc_pin), full_scale_value_(full_scale_value), inverted_(inverted) {
}

void AnalogChannel::begin() {
	analogReadResolution(ADC_RESOLUTION_BITS);
	pinMode(adc_pin_, INPUT);
}

uint16_t AnalogChannel::read_scaled() {
	uint32_t raw_value = (uint32_t)analogRead(adc_pin_);
	if (raw_value > ADC_MAXIMUM_RAW) {
		raw_value = ADC_MAXIMUM_RAW;
	}
	if (inverted_) {
		raw_value = ADC_MAXIMUM_RAW - raw_value;
	}
	return (uint16_t)((raw_value * full_scale_value_) / ADC_MAXIMUM_RAW);
}
