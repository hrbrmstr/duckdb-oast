#ifndef OAST_DECODE_H
#define OAST_DECODE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Decoded OAST metadata structure
typedef struct {
    char     original[256];   // Original input domain
    uint32_t timestamp;       // Unix timestamp (seconds since epoch)
    uint8_t  machine_id[3];   // 3-byte machine identifier
    uint16_t pid;             // Process ID
    uint32_t counter;         // 24-bit counter value
    char     nonce[128];      // z-base-32 nonce portion (if present)
    char     ksort[7];        // First 6 chars of preamble for K-sorting + null
    char     campaign[6];     // Chars 7-11 of preamble (campaign ID) + null
    bool     valid;           // Whether decode succeeded
    char     error[256];      // Error message if invalid
} oast_decoded_t;

// Decode an OAST domain (subdomain or FQDN)
// Returns 0 on success, -1 on failure
int oast_decode(const char *input, size_t input_len, oast_decoded_t *result);

#endif // OAST_DECODE_H
