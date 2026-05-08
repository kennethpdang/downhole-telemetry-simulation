#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

// Centralization of pin numbers:
constexpr uint8_t  kVibrationAdcPin = 34;
constexpr uint8_t  kFaultButtonPin = 15;
constexpr uint32_t kFaultDebounceMs = 200;

volatile bool faultFlag = false;
volatile uint32_t lastFaultMillis = 0;

void IRAM_ATTR onFaultButtonPressed() {
	const uint32_t nowMs = millis();

	if (nowMs - lastFaultMillis < kFaultDebounceMs) {
		return;
	}

	lastFaultMillis = nowMs;
	faultFlag = true;
}

struct SensorReading {
	int32_t	temperatureCelsius;
	uint32_t pressurePascal; 	
	uint32_t timestampMilliseconds;
	uint32_t vibrationRaw;
};

// BME180 Device
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

// Potentiometer
class VibrationSensor {
	public:
		void begin(uint8_t adcPin) {
			pin_ = adcPin;
			analogReadResolution(12);
		}

		uint16_t read() {
			return (uint16_t)analogRead(pin_);
		}

	private:
		uint8_t pin_ = 0;
};

// Initialize Sensor Objects:
PressureSensor pressureSensor;
VibrationSensor vibrationSensor;

void setup() {
	Serial.begin(115200);
	delay(100);
	Serial.println("[BOOT] downhole-telemetry-sim starting!");

	if (!pressureSensor.begin()) {
		Serial.println("[FATAL] Sensor initialization failed!");
		while (true) {
			delay(1000);
		}
	}

	vibrationSensor.begin(kVibrationAdcPin);
	pinMode(kFaultButtonPin, INPUT_PULLUP);

	attachInterrupt(digitalPinToInterrupt(kFaultButtonPin), onFaultButtonPressed, FALLING);

	Serial.println("[BOOT] Sensors are okay!");
}

void loop() {
	SensorReading reading{};
	const bool pressureOk = pressureSensor.read(reading);
	reading.vibrationRaw  = vibrationSensor.read();

	Serial.print("t=");
	Serial.print(millis());
	Serial.print("ms ");

	if (pressureSensor.read(reading)) {

	} else {
		Serial.println("[WARNING] Pressure sensor read failed");
	}

	if (pressureOk) {
		Serial.print("temp=");
		Serial.print(reading.temperatureCelsius / 100.0f, 2);
		Serial.print("C  pressure=");
		Serial.print(reading.pressurePascal);
		Serial.print("Pa  ");
	} else {
		Serial.print("[pressure read failed]  ");
	}

	Serial.print("vibration=");
	Serial.print(reading.vibrationRaw);

	if (faultFlag) {
		faultFlag = false;
		Serial.println("[FAULT]");
	}

	Serial.println();
	delay(1000);
}