#ifndef OAST_BASE32_H
#define OAST_BASE32_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Base32hex alphabet (RFC 4648, lowercase)
extern const char BASE32HEX_ALPHABET[33];

// z-base-32 alphabet
extern const char ZBASE32_ALPHABET[33];

// Decode base32hex string to bytes
// Returns number of bytes decoded, or -1 on error
int base32hex_decode(const char *input, size_t input_len, uint8_t *output, size_t *output_len);

// Check if character is valid base32hex
bool is_base32hex_char(char c);

// Check if character is valid z-base-32
bool is_zbase32_char(char c);

#endif // OAST_BASE32_H
