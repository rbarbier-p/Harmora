#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>
#include "mos.h"

/**
 * Send debug string via MIDI SysEx
 */
void debug_send_string(const char *msg);

/**
 * Send debug packet via MIDI SysEx
 */
void debug_send_packet(MPacket packet);

/**
 * Send debug value with label
 */
void debug_send_value(const char *label, uint16_t value);

#endif // DEBUG_H
