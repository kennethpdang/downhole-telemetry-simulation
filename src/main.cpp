// Downhole telemetry simulation for an ESP32 in Wokwi.
//
// Three FreeRTOS tasks pass data down a pipeline:
//
//   sampling task  --(sample queue)-->  telemetry task  --(uplink mailbox)-->  mud pulse task
//   reads sensors at 10 Hz              packs 10 samples into                  blinks the frame out
//                                       one 28-byte frame, checks              at 20 bits/s, like a
//                                       alarms, prints it                      mud pulser valve
//
// The fault button interrupt runs outside all of them and only raises a flag.

#include <Arduino.h>
#include <Wire.h>
#include <atomic>

#include "alarm_evaluator.h"
#include "config.h"
#include "drilling_window.h"
#include "mud_pulse_transmitter.h"
#include "sensors.h"
#include "serial_logger.h"
#include "telemetry_frame.h"
#include "telemetry_report.h"

static constexpr uint32_t SERIAL_BAUD_RATE = 115200;

// What travels from the telemetry task to the mud pulse task.
struct TelemetryPacket {
	uint8_t sequence_number;
	size_t frame_length;
	uint8_t frame_bytes[TELEMETRY_FRAME_LENGTH];
};

static PressureSensor pressure_sensor;
static MotionSensor motion_sensor;
static AnalogChannel mud_flow_channel(MUD_FLOW_ADC_PIN, MAXIMUM_MUD_FLOW_GALLONS_PER_MINUTE, false);
static AnalogChannel gamma_ray_channel(GAMMA_RAY_ADC_PIN, MAXIMUM_GAMMA_RAY_API, true);
static MudPulseTransmitter mud_pulse_transmitter(MUD_PULSE_VALVE_PIN, MUD_PULSE_BIT_PERIOD_MILLISECONDS);
static AlarmEvaluator alarm_evaluator(DEFAULT_ALARM_THRESHOLDS);

static QueueHandle_t sample_queue = nullptr;
static QueueHandle_t uplink_mailbox = nullptr;

static std::atomic<bool> manual_fault_pending(false);
static volatile uint32_t last_fault_milliseconds = 0;
static std::atomic<uint32_t> dropped_sample_count(0);

// ----- Interrupt -------------------------------------------------------------

void IRAM_ATTR on_fault_button_pressed() {
	const uint32_t now_milliseconds = millis();

	if (now_milliseconds - last_fault_milliseconds < FAULT_DEBOUNCE_MILLISECONDS) {
		return;
	}

	last_fault_milliseconds = now_milliseconds;
	manual_fault_pending.store(true);
}

// ----- Tasks -----------------------------------------------------------------

static void sampling_task(void* task_parameters) {
	TickType_t last_wake_tick = xTaskGetTickCount();

	while (true) {
		SensorSample sample{};
		sample.timestamp_milliseconds = millis();
		sample.pressure_ok = pressure_sensor.read(sample);
		sample.motion_ok = motion_sensor.read(sample);
		sample.mud_flow_gallons_per_minute = mud_flow_channel.read_scaled();
		sample.gamma_ray_api = gamma_ray_channel.read_scaled();

		// Never block here: if the telemetry task falls behind, count the loss instead of drifting.
		if (xQueueSend(sample_queue, &sample, 0) != pdTRUE) {
			dropped_sample_count++;
		}

		vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(SAMPLE_PERIOD_MILLISECONDS));
	}
}

static void telemetry_task(void* task_parameters) {
	DrillingWindow drilling_window;
	uint8_t sequence_number = 0;

	while (true) {
		SensorSample sample;
		if (xQueueReceive(sample_queue, &sample, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		drilling_window.add_sample(sample);
		if (drilling_window.sample_count() < SAMPLES_PER_FRAME) {
			continue;
		}

		WindowSummary summary = drilling_window.build_summary();
		drilling_window.reset();

		summary.status_flags |= alarm_evaluator.evaluate(summary);
		if (manual_fault_pending.exchange(false)) {
			summary.status_flags |= STATUS_MANUAL_FAULT;
		}

		TelemetryPacket packet{};
		packet.sequence_number = sequence_number;
		packet.frame_length = encode_telemetry_frame(summary, sequence_number, packet.frame_bytes, sizeof(packet.frame_bytes));

		report_telemetry_frame(summary, sequence_number, packet.frame_bytes, packet.frame_length);
		digitalWrite(ALARM_LED_PIN, (summary.status_flags & STATUS_ALARM_MASK) ? HIGH : LOW);

		// The mailbox holds one packet. Overwriting means the slow mud pulser
		// always sends the newest frame and older ones are simply skipped.
		xQueueOverwrite(uplink_mailbox, &packet);

		const uint32_t dropped_samples = dropped_sample_count.exchange(0);
		if (dropped_samples > 0) {
			serial_logger.print_line("[WARNING] %lu samples dropped, telemetry task is behind", (unsigned long)dropped_samples);
		}

		sequence_number++;
	}
}

static void mud_pulse_task(void* task_parameters) {
	while (true) {
		TelemetryPacket packet;
		if (xQueueReceive(uplink_mailbox, &packet, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		const uint32_t bit_count = mud_pulse_transmitter.bits_for_frame(packet.frame_length);
		serial_logger.print_line("[UPLINK] pulsing frame %u to surface: %lu bits at %lu bit/s",
			packet.sequence_number,
			(unsigned long)bit_count,
			(unsigned long)(1000 / MUD_PULSE_BIT_PERIOD_MILLISECONDS));

		const uint32_t start_milliseconds = millis();
		mud_pulse_transmitter.transmit(packet.frame_bytes, packet.frame_length);
		const uint32_t elapsed_milliseconds = millis() - start_milliseconds;

		serial_logger.print_line("[UPLINK] frame %u delivered in %lu ms", packet.sequence_number, (unsigned long)elapsed_milliseconds);
	}
}

// ----- Startup -----------------------------------------------------------------

static void report_sensor_start(const char* sensor_name, bool started) {
	if (started) {
		serial_logger.print_line("[BOOT] %s OK", sensor_name);
	} else {
		serial_logger.print_line("[BOOT] %s NOT FOUND, its data will be flagged as failed", sensor_name);
	}
}

void setup() {
	serial_logger.begin(SERIAL_BAUD_RATE);
	delay(100);
	serial_logger.print_line("[BOOT] downhole-telemetry-sim starting");

	Wire.begin(I2C_DATA_PIN, I2C_CLOCK_PIN);
	report_sensor_start("BMP180 pressure/temperature (I2C 0x77)", pressure_sensor.begin());
	report_sensor_start("MPU6050 accelerometer/gyroscope (I2C 0x68)", motion_sensor.begin());

	mud_flow_channel.begin();
	gamma_ray_channel.begin();
	mud_pulse_transmitter.begin();

	pinMode(ALARM_LED_PIN, OUTPUT);
	digitalWrite(ALARM_LED_PIN, LOW);

	pinMode(FAULT_BUTTON_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(FAULT_BUTTON_PIN), on_fault_button_pressed, FALLING);

	sample_queue = xQueueCreate(SAMPLE_QUEUE_LENGTH, sizeof(SensorSample));
	uplink_mailbox = xQueueCreate(1, sizeof(TelemetryPacket));

	xTaskCreatePinnedToCore(sampling_task, "sampling", TASK_STACK_BYTES, nullptr, SAMPLING_TASK_PRIORITY, nullptr, 1);
	xTaskCreatePinnedToCore(telemetry_task, "telemetry", TASK_STACK_BYTES, nullptr, TELEMETRY_TASK_PRIORITY, nullptr, 1);
	xTaskCreatePinnedToCore(mud_pulse_task, "mud_pulse", TASK_STACK_BYTES, nullptr, MUD_PULSE_TASK_PRIORITY, nullptr, 1);

	serial_logger.print_line("[BOOT] tasks running");
}

void loop() {
	// All work happens in the tasks above, so the Arduino loop task can go away.
	vTaskDelete(nullptr);
}
