#include "oast_base32.h"
#include <ctype.h>
#include <string.h>

// Base32hex alphabet (RFC 4648, lowercase)
const char BASE32HEX_ALPHABET[33] = "0123456789abcdefghijklmnopqrstuv";

// z-base-32 alphabet
const char ZBASE32_ALPHABET[33] = "ybndrfg8ejkmcpqxot1uwisza345h769";

bool is_base32hex_char(char c) {
  return strchr(BASE32HEX_ALPHABET, tolower(c)) != NULL;
}

bool is_zbase32_char(char c) {
  return strchr(ZBASE32_ALPHABET, tolower(c)) != NULL;
}

// Get base32hex character value (0-31), or -1 if invalid
static int base32hex_char_value(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'v') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'V') {
    return c - 'A' + 10;
  }
  return -1;
}

int base32hex_decode(const char *input, size_t input_len, uint8_t *output,
                     size_t *output_len) {
  // Base32hex: 5 bits per character
  // 20 chars × 5 bits = 100 bits = 12.5 bytes, use 12 bytes

  if (input_len != 20) {
    *output_len = 0;
    return -1;
  }

  uint64_t bit_buffer = 0;
  int bit_count = 0;
  size_t byte_index = 0;

  for (size_t i = 0; i < input_len; i++) {
    int val = base32hex_char_value(input[i]);
    if (val < 0) {
      *output_len = 0;
      return -1; // Invalid character
    }

    // Add 5 bits to buffer
    bit_buffer = (bit_buffer << 5) | (uint64_t)val;
    bit_count += 5;

    // Extract complete bytes
    while (bit_count >= 8 && byte_index < 12) {
      bit_count -= 8;
      output[byte_index] = (uint8_t)((bit_buffer >> bit_count) & 0xFF);
      byte_index++;
    }
  }

  *output_len = byte_index;
  return 0;
}
