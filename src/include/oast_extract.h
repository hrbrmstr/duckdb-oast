#ifndef OAST_EXTRACT_H
#define OAST_EXTRACT_H

#include <stddef.h>

// Extracted OAST domain match
typedef struct {
    const char *full;         // Points into source text (not owned)
    size_t      full_len;
    const char *subdomain;    // Points into source text
    size_t      subdomain_len;
    const char *domain;       // The matched OAST domain suffix (points to KNOWN_OAST_DOMAINS)
    size_t      start_idx;
    size_t      end_idx;
} oast_match_t;

// Extract all OAST domains from text
// Caller must free the returned matches array with free()
// Returns number of matches found, or -1 on error
int oast_extract(const char *text, size_t text_len, oast_match_t **matches_out, size_t *match_count_out);

#endif // OAST_EXTRACT_H
