#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>
#include <avr/pgmspace.h>

uint32_t string_length(char *string);
uint16_t number_to_string(char *buffer, uint16_t length, uint32_t number);
uint16_t string_concat(char *dst, const char *src, uint16_t dst_size);

#endif
