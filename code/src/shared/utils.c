#include "utils.h"

/*
static const uint32_t pow10_table[10] = {
    1000000000UL, 100000000UL, 10000000UL, 1000000UL, 100000UL,
    10000UL, 1000UL, 100UL, 10UL, 1UL
};
*/

uint32_t string_length(char *string)
{
    uint32_t i = 0;
    while (string[i])
        i++;
    return (i);
}

uint16_t number_to_string(char *buffer, uint16_t length, uint32_t number)
{
    if (!buffer || length == 0) return (0);

    // Handle zero explicitly
    if (number == 0) {
            if (length < 2) return (0);
            buffer[0] = '0';
        buffer[1] = '\0';
        return (1);
    }

    // Build digits in reverse
    uint8_t i = 0;
    while (number > 0 && i < (length - 1)) {
        buffer[i++] = '0' + (char)(number % 10);
        number /= 10;
    }

    // Number didn't fit
    if (number > 0) {
        buffer[0] = '\0';
        return (0);
    }

    buffer[i] = '\0';

    // Reverse the string in-place
    uint8_t left = 0, right = i - 1;
    while (left < right) {
        char tmp    = buffer[left];
        buffer[left]   = buffer[right];
        buffer[right]  = tmp;
        left++;
        right--;
    }

    return (i);
}

// Concatenates src onto the end of dst.
uint16_t string_concat(char *dst, const char *src, uint16_t dst_size)
{
    if (!dst || !src || dst_size == 0) return (0);

    uint16_t dst_length = 0;
    while (dst_length < dst_size && dst[dst_length] != '\0') dst_length++;

    if (dst_length == dst_size) return (0);

    uint16_t i = 0;
    while (src[i] != '\0') {
        if (dst_length + i + 1 >= dst_size) {
            dst[dst_length + i] = '\0';
            return (0);
        }
        dst[dst_length + i] = src[i];
        i++;
    }

    dst[dst_length + i] = '\0';
    return (dst_length + i);
}