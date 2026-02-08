#ifndef OAST_VALIDATE_H
#define OAST_VALIDATE_H

#include <stdbool.h>
#include <stddef.h>

// Validate that a string is a valid OAST subdomain
// (20+ base32hex chars + 13 z-base-32 chars)
bool oast_is_valid_subdomain(const char *s, size_t len);

// Validate that a 20-char string is valid base32hex preamble
bool oast_is_valid_preamble(const char *s, size_t len);

// Validate that a string is a valid OAST domain (subdomain or FQDN)
bool oast_validate(const char *input, size_t input_len);

#endif // OAST_VALIDATE_H
