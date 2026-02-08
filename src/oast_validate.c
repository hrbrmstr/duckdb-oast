#include "oast_validate.h"
#include "oast_base32.h"
#include "oast_domains.h"
#include <ctype.h>

bool oast_is_valid_preamble(const char *s, size_t len) {
  if (len != 20) {
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    if (!is_base32hex_char(s[i])) {
      return false;
    }
  }

  return true;
}

bool oast_is_valid_subdomain(const char *s, size_t len) {
  // OAST subdomain format:
  // - 20 chars base32hex (preamble)
  // - 13+ chars z-base-32 (nonce)

  if (len < 33) { // 20 + 13 minimum
    return false;
  }

  // Check preamble (first 20 chars)
  if (!oast_is_valid_preamble(s, 20)) {
    return false;
  }

  // Check nonce (remaining chars)
  for (size_t i = 20; i < len; i++) {
    if (!is_zbase32_char(s[i])) {
      return false;
    }
  }

  return true;
}

bool oast_validate(const char *input, size_t input_len) {
  // Check if it's a known OAST domain (subdomain or FQDN)
  const char *suffix = is_known_oast_domain(input, input_len);
  if (!suffix) {
    return false;
  }

  // Extract subdomain
  const char *subdomain = NULL;
  size_t subdomain_len = get_oast_subdomain(input, input_len, &subdomain);

  if (subdomain_len == 0) {
    // Just a domain without subdomain, not valid
    return false;
  }

  // Validate subdomain structure
  return oast_is_valid_subdomain(subdomain, subdomain_len);
}
