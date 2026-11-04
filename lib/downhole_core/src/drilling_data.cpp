#include "drilling_data.h"

static const char* const STATUS_FLAG_NAMES[STATUS_FLAG_COUNT] = {
	"MANUAL_FAULT",
	"PRESSURE_SENSOR_FAILED",
	"MOTION_SENSOR_FAILED",
	"OVER_TEMPERATURE",
	"PRESSURE_KICK",
	"HIGH_SHOCK",
	"HIGH_JERK",
	"STICK_SLIP",
	"LOW_MUD_FLOW",
	"SHALE_FORMATION",
};

const char* status_flag_name(int bit_position) {
	if (bit_position < 0 || bit_position >= STATUS_FLAG_COUNT) {
		return "UNKNOWN";
	}
	return STATUS_FLAG_NAMES[bit_position];
}
