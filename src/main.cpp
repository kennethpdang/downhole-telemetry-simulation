#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

// Centralization of pin numbers:
constexpr uint8_t  kVibrationAdcPin = 34;
constexpr uint8_t  kFaultButtonPin = 15;
constexpr uint32_t kFaultDebounceMs = 200;

volatile bool faultFlag = false;
volatile uint32_t lastFaultMillis = 0;

struct SensorReading {
	int32_t	temperatureCelsius;
	uint32_t pressurePascal; 	
	uint32_t timestampMilliseconds;
	uint32_t vibrationRaw;
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
			out.pressurePascal = (uint32_t)(event.pressure * 100.0f);
			out.timestampMilliseconds = millis();
			return true;
		}

	private:
		Adafruit_BMP085_Unified bmp_{10085};

		float readTemperatureCelsius() {
			float temperatureCelsius = 0.0f;
			bmp_.getTemperature(&temperatureCelsius);
			return temperatureCelsius;
		}
};

// Initialize Pressure Sensor Object:
PressureSensor testSensor;

void setup() {
	Serial.begin(115200);
	delay(100);
	Serial.println("[BOOT] downhole-telemetry-sim starting!");

	if (!testSensor.begin()) {
		Serial.println("[FATAL] Sensor initialization failed!");
		while (true) {
			delay(1000);
		}
	}

	Serial.println("[BOOT] Sensor is okay!");
}

void loop() {
	SensorReading reading;
	if (testSensor.read(reading)) {
		Serial.print("t=");
		Serial.print(reading.timestampMilliseconds);
		Serial.print("ms temp=");
		Serial.print(reading.temperatureCelsius / 100.0f, 2);
		Serial.print("C	pressure=");
		Serial.print(reading.pressurePascal);
		Serial.println("Pa");
	} else {
		Serial.println("[WARNING] sensor read failed");
	}
	delay(1000);
}