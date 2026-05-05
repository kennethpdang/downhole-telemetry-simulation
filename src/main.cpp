#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

struct SensorReading {
	int32_t	temperatureCelsius;		 // hundredths of a degree C
	uint32_t pressurePascal;
	uint32_t timestampMilliseconds;
};

class PressureSensor {
public:
	bool begin() {
		if (!bmp_.begin()) {
			return false;
		}
		return true;
	}

	bool read(SensorReading& out) {
		sensors_event_t event;
		bmp_.getEvent(&event);

		if (!event.pressure) {
			return false;
		}

		out.temperatureCelsius = (int32_t)(readTemperatureCelsius() * 100.0f);
		out.pressurePascal = (uint32_t)(event.pressure * 100.0f); // hPa -> Pa
		out.timestampMilliseconds = millis();
		return true;
	}

private:
	float readTemperatureCelsius() {
		float temperatureCelsius = 0.0f;
		bmp_.getTemperature(&temperatureCelsius);
		return temperatureCelsius;
	}

	Adafruit_BMP085_Unified bmp_{10085};
};

// Initialize Pressure Sensor Object:
PressureSensor testSensor;

void setup() {
	Serial.begin(115200);
	delay(100);
	Serial.println(F("[boot] downhole-telemetry-sim starting"));

	if (!testSensor.begin()) {
		Serial.println(F("[fatal] sensor initialization failed!"));
		while (true) {
			delay(1000);
		}
	}

	Serial.println(F("[boot] sensor OK"));
}

void loop() {
	SensorReading reading;
	if (testSensor.read(reading)) {
		Serial.print(F("t="));
		Serial.print(reading.timestampMilliseconds);
		Serial.print(F("ms	temp="));
		Serial.print(reading.temperatureCelsius / 100.0f, 2);
		Serial.print(F("C	pressure="));
		Serial.print(reading.pressurePascal);
		Serial.println(F("Pa"));
	} else {
		Serial.println(F("[warn] sensor read failed"));
	}
	delay(1000);
}