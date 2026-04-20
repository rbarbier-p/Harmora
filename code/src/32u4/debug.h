#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

/**
 * Send debug string via MIDI SysEx
 */
void debug_send_string(const char *msg);

/**
 * Send debug value with label
 */
void debug_send_value(const char *label, uint16_t value);

#endif // DEBUG_H
