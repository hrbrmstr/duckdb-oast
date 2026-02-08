#include "oast_extract.h"
#include "oast_base32.h"
#include "oast_domains.h"
#include "oast_validate.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Check if character is valid for OAST subdomain (base32hex or z-base-32 or
// hyphen/underscore)
static bool is_subdomain_char(char c) {
  c = tolower(c);
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || c == '-' ||
         c == '_';
}

// Find start of subdomain by walking backwards from dot
// Returns start position, or -1 if invalid
static int find_subdomain_start(const char *text, size_t dot_pos) {
  if (dot_pos == 0) {
    return -1;
  }

  int pos = (int)dot_pos - 1;

  // Walk backwards while we have valid subdomain characters
  while (pos >= 0 && is_subdomain_char(text[pos])) {
    pos--;
  }

  // pos now points to the character before the subdomain, or -1 if at start
  int start = pos + 1;
  size_t subdomain_len = dot_pos - start;

  // Validate minimum length (20 chars for preamble + 13 for nonce = 33 minimum)
  if (subdomain_len < 33) {
    return -1;
  }

  // Validate first 20 chars are base32hex
  for (size_t i = 0; i < 20; i++) {
    if (!is_base32hex_char(text[start + i])) {
      return -1;
    }
  }

  // Validate remaining chars are valid (z-base-32 or hyphen/underscore)
  for (size_t i = 20; i < subdomain_len; i++) {
    char c = tolower(text[start + i]);
    if (!is_zbase32_char(c) && c != '-' && c != '_') {
      return -1;
    }
  }

  return start;
}

int oast_extract(const char *text, size_t text_len, oast_match_t **matches_out,
                 size_t *match_count_out) {
  *matches_out = NULL;
  *match_count_out = 0;

  if (text_len == 0 || !text) {
    return 0;
  }

  // Allocate initial capacity for matches
  size_t capacity = 16;
  oast_match_t *matches = malloc(sizeof(oast_match_t) * capacity);
  if (!matches) {
    return -1;
  }
  size_t match_count = 0;

  // Scan for known OAST domain suffixes
  for (int i = 0; KNOWN_OAST_DOMAINS[i] != NULL; i++) {
    const char *domain = KNOWN_OAST_DOMAINS[i];
    size_t domain_len = strlen(domain);

    // Skip if text is shorter than domain
    if (text_len < domain_len) {
      continue;
    }

    // Search for this domain in the text
    for (size_t pos = 0; pos <= text_len - domain_len; pos++) {
      // Case-insensitive comparison
      bool match = true;
      for (size_t j = 0; j < domain_len; j++) {
        if (tolower(text[pos + j]) != tolower(domain[j])) {
          match = false;
          break;
        }
      }

      if (!match) {
        continue;
      }

      // Found a domain match. Now check if there's a valid subdomain before it.
      // Need a dot immediately before the domain
      if (pos == 0 || text[pos - 1] != '.') {
        continue;
      }

      size_t dot_pos = pos - 1;

      // Find subdomain start
      int subdomain_start = find_subdomain_start(text, dot_pos);
      if (subdomain_start < 0) {
        continue;
      }

      // Check for boundary before subdomain (start of string, whitespace, or
      // non-subdomain char)
      if (subdomain_start > 0) {
        char before = text[subdomain_start - 1];
        if (is_subdomain_char(before)) {
          // Not a valid boundary
          continue;
        }
      }

      // Check for boundary after domain (end of string, whitespace, or
      // non-subdomain char)
      size_t domain_end = pos + domain_len;
      if (domain_end < text_len) {
        char after = text[domain_end];
        if (is_subdomain_char(after) || after == '.') {
          // Not a valid boundary (might be part of longer domain)
          continue;
        }
      }

      // Valid OAST match found!
      if (match_count >= capacity) {
        capacity *= 2;
        oast_match_t *new_matches =
            realloc(matches, sizeof(oast_match_t) * capacity);
        if (!new_matches) {
          free(matches);
          return -1;
        }
        matches = new_matches;
      }

      oast_match_t *m = &matches[match_count];
      m->start_idx = subdomain_start;
      m->end_idx = domain_end;
      m->full = text + subdomain_start;
      m->full_len = domain_end - subdomain_start;
      m->subdomain = text + subdomain_start;
      m->subdomain_len = dot_pos - subdomain_start;
      m->domain = domain; // Points to KNOWN_OAST_DOMAINS entry

      match_count++;

      // Skip past this match to avoid overlapping matches
      pos = domain_end - 1; // -1 because loop will increment
    }
  }

  *matches_out = matches;
  *match_count_out = match_count;
  return 0;
}
