"""Surface-side decoder for one telemetry frame.

Copy a [HEX] line from the Wokwi serial monitor and run:

    python tools/decode_frame.py "AA 55 07 00 01 E2 40 ..."

It checks the sync word and CRC exactly like the receiver at the rig would,
then prints every field in readable units. Uses only the Python standard library.
"""

import struct
import sys

FRAME_LENGTH = 28
SYNC_WORD = b"\xAA\x55"

# Same layout as lib/downhole_core/src/telemetry_frame.h. ">" means big-endian.
FRAME_LAYOUT = ">2sBIhIHHhBHHHH"

STATUS_FLAG_NAMES = [
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
]


def compute_crc16(data):
    """CRC-16/CCITT-FALSE, identical to lib/downhole_core/src/crc16.cpp."""
    crc_value = 0xFFFF
    for byte_index in range(len(data)):
        crc_value ^= data[byte_index] << 8
        for bit_index in range(8):
            if crc_value & 0x8000:
                crc_value = ((crc_value << 1) ^ 0x1021) & 0xFFFF
            else:
                crc_value = (crc_value << 1) & 0xFFFF
    return crc_value


def flag_names(status_flags):
    raised_names = []
    for bit_position in range(len(STATUS_FLAG_NAMES)):
        if status_flags & (1 << bit_position):
            raised_names.append(STATUS_FLAG_NAMES[bit_position])
    return raised_names


def decode_frame(frame_bytes):
    if len(frame_bytes) != FRAME_LENGTH:
        raise ValueError(f"expected {FRAME_LENGTH} bytes, got {len(frame_bytes)}")

    fields = struct.unpack(FRAME_LAYOUT, frame_bytes)
    (sync_word, sequence_number, timestamp_milliseconds, temperature_centi_celsius,
     pressure_pascal, peak_shock_milli_g, peak_jerk_milli_g_per_second,
     average_rotation_rpm, stick_slip_percent, mud_flow_gallons_per_minute,
     gamma_ray_api, status_flags, received_crc) = fields

    if sync_word != SYNC_WORD:
        raise ValueError("bad sync word, this is not the start of a frame")

    computed_crc = compute_crc16(frame_bytes[:-2])
    if computed_crc != received_crc:
        raise ValueError(f"CRC mismatch: frame says {received_crc:04X}, data gives {computed_crc:04X}")

    print(f"sequence      {sequence_number}")
    print(f"time          {timestamp_milliseconds} ms")
    print(f"temperature   {temperature_centi_celsius / 100:.2f} C")
    print(f"pressure      {pressure_pascal} Pa")
    print(f"peak shock    {peak_shock_milli_g / 1000:.2f} g")
    print(f"peak jerk     {peak_jerk_milli_g_per_second / 1000:.1f} g/s")
    print(f"rotation      {average_rotation_rpm} RPM")
    print(f"stick-slip    {stick_slip_percent} %")
    print(f"mud flow      {mud_flow_gallons_per_minute} gal/min")
    print(f"gamma ray     {gamma_ray_api} API")
    print(f"flags         0x{status_flags:04X} {' '.join(flag_names(status_flags))}")
    print("CRC           OK")


def main():
    hex_text = " ".join(sys.argv[1:]).replace("[HEX]", "")
    decode_frame(bytes.fromhex(hex_text))


if __name__ == "__main__":
    main()
