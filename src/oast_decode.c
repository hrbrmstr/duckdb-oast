#include "oast_decode.h"
#include "oast_base32.h"
#include "oast_domains.h"
#include "oast_validate.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// Read big-endian uint32 from bytes
static uint32_t read_be32(const uint8_t *bytes) {
  return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
         ((uint32_t)bytes[2] << 8) | ((uint32_t)bytes[3]);
}

// Read big-endian uint16 from bytes
static uint16_t read_be16(const uint8_t *bytes) {
  return ((uint16_t)bytes[0] << 8) | ((uint16_t)bytes[1]);
}

// Read 24-bit big-endian value from bytes
static uint32_t read_be24(const uint8_t *bytes) {
  return ((uint32_t)bytes[0] << 16) | ((uint32_t)bytes[1] << 8) |
         ((uint32_t)bytes[2]);
}

int oast_decode(const char *input, size_t input_len, oast_decoded_t *result) {
  memset(result, 0, sizeof(*result));

  // Store original input
  size_t copy_len = input_len < sizeof(result->original) - 1
                        ? input_len
                        : sizeof(result->original) - 1;
  memcpy(result->original, input, copy_len);
  result->original[copy_len] = '\0';

  // Check for empty input
  if (input_len == 0) {
    strncpy(result->error, "empty input", sizeof(result->error) - 1);
    return -1;
  }

  // Extract subdomain (everything before first dot, or whole input if no dot)
  size_t subdomain_len = input_len;
  const char *subdomain = input;

  for (size_t i = 0; i < input_len; i++) {
    if (input[i] == '.') {
      subdomain_len = i;
      break;
    }
  }

  // Lowercase subdomain for comparison (using stack buffer)
  char subdomain_lower[256];
  if (subdomain_len >= sizeof(subdomain_lower)) {
    strncpy(result->error, "subdomain too long", sizeof(result->error) - 1);
    return -1;
  }

  for (size_t i = 0; i < subdomain_len; i++) {
    subdomain_lower[i] = tolower(subdomain[i]);
  }
  subdomain_lower[subdomain_len] = '\0';

  // Check minimum length (20 chars for preamble)
  if (subdomain_len < 20) {
    snprintf(result->error, sizeof(result->error),
             "subdomain too short: %zu chars (minimum 20)", subdomain_len);
    return -1;
  }

  // Extract preamble (first 20 chars)
  const char *preamble = subdomain_lower;

  // Validate preamble
  if (!oast_is_valid_preamble(preamble, 20)) {
    strncpy(result->error, "preamble contains invalid base32hex characters",
            sizeof(result->error) - 1);
    return -1;
  }

  // Extract nonce if present
  if (subdomain_len > 20) {
    size_t nonce_len = subdomain_len - 20;
    if (nonce_len < sizeof(result->nonce)) {
      memcpy(result->nonce, subdomain_lower + 20, nonce_len);
      result->nonce[nonce_len] = '\0';
    }
  }

  // Decode preamble
  uint8_t bytes[12];
  size_t decoded_len = 0;

  if (base32hex_decode(preamble, 20, bytes, &decoded_len) != 0 ||
      decoded_len != 12) {
    strncpy(result->error, "failed to decode preamble",
            sizeof(result->error) - 1);
    return -1;
  }

  // Extract fields from 12-byte array

  // Bytes 0-3: timestamp (big-endian uint32)
  result->timestamp = read_be32(bytes);

  // Bytes 4-6: machine ID (3 bytes)
  result->machine_id[0] = bytes[4];
  result->machine_id[1] = bytes[5];
  result->machine_id[2] = bytes[6];

  // Bytes 7-8: PID (big-endian uint16)
  result->pid = read_be16(bytes + 7);

  // Bytes 9-11: counter (24-bit big-endian)
  result->counter = read_be24(bytes + 9);

  // Extract K-sort and campaign identifiers
  memcpy(result->ksort, preamble, 6);
  result->ksort[6] = '\0';

  memcpy(result->campaign, preamble + 6, 5);
  result->campaign[5] = '\0';

  result->valid = true;
  return 0;
}
