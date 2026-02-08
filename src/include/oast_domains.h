#ifndef OAST_DOMAINS_H
#define OAST_DOMAINS_H

#include <stdbool.h>
#include <stddef.h>

// Known OAST domain suffixes (null-terminated array)
extern const char *KNOWN_OAST_DOMAINS[];

// Check if a domain ends with a known OAST suffix
// Returns the matching suffix or NULL if not an OAST domain
const char *is_known_oast_domain(const char *domain, size_t domain_len);

// Get the subdomain portion of a full OAST domain
// Returns length of subdomain, or 0 if not valid
size_t get_oast_subdomain(const char *full, size_t full_len, const char **subdomain_out);

#endif // OAST_DOMAINS_H
