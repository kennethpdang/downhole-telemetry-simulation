#pragma once

#include <stddef.h>
#include <stdint.h>

// CRC-16/CCITT-FALSE: polynomial 0x1021, starting value 0xFFFF.
// The same checksum is used by many radio and modem links. The sender
// appends it to every frame and the receiver recomputes it. If the two
// values differ, at least one bit was corrupted on the way.
uint16_t compute_crc16(const uint8_t* data, size_t length);
