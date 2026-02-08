#include "oast_domains.h"
#include <string.h>

// Known OAST domain suffixes
const char *KNOWN_OAST_DOMAINS[] = {
    "oast.pro", "oast.live",   "oast.site",      "oast.online", "oast.fun",
    "oast.me",  "interact.sh", "interactsh.com", NULL};

const char *is_known_oast_domain(const char *domain, size_t domain_len) {
  for (int i = 0; KNOWN_OAST_DOMAINS[i] != NULL; i++) {
    const char *suffix = KNOWN_OAST_DOMAINS[i];
    size_t suffix_len = strlen(suffix);

    if (domain_len >= suffix_len) {
      const char *end = domain + domain_len - suffix_len;
      if (memcmp(end, suffix, suffix_len) == 0) {
        // Check boundary (must be start of string or after a dot)
        if (domain_len == suffix_len || end[-1] == '.') {
          return suffix;
        }
      }
    }
  }
  return NULL;
}

size_t get_oast_subdomain(const char *full, size_t full_len,
                          const char **subdomain_out) {
  const char *suffix = is_known_oast_domain(full, full_len);
  if (!suffix) {
    return 0;
  }

  size_t suffix_len = strlen(suffix);
  if (full_len == suffix_len) {
    // Just the domain, no subdomain
    return 0;
  }

  // Extract subdomain (everything before the domain)
  size_t subdomain_len = full_len - suffix_len - 1; // -1 for the dot
  *subdomain_out = full;
  return subdomain_len;
}
