#ifndef OAST_MACROS_H
#define OAST_MACROS_H

// Struct macros (efficient multi-field access via single JSON parse)

#define OAST_STRUCT_MACRO \
    "CREATE OR REPLACE MACRO oast_struct(domain) AS " \
    "json_transform(" \
    "  oast_decode_json(domain)," \
    "  '{" \
    "    \"original\": \"VARCHAR\"," \
    "    \"valid\": \"BOOLEAN\"," \
    "    \"ts\": \"BIGINT\"," \
    "    \"machine_id\": \"VARCHAR\"," \
    "    \"pid\": \"INTEGER\"," \
    "    \"counter\": \"INTEGER\"," \
    "    \"ksort\": \"VARCHAR\"," \
    "    \"campaign\": \"VARCHAR\"," \
    "    \"nonce\": \"VARCHAR\"" \
    "  }'" \
    ")"

#define OAST_SUMMARY_MACRO \
    "CREATE OR REPLACE MACRO oast_summary(domain) AS " \
    "json_transform(" \
    "  oast_decode_json(domain)," \
    "  '{" \
    "    \"ksort\": \"VARCHAR\"," \
    "    \"campaign\": \"VARCHAR\"," \
    "    \"machine_id\": \"VARCHAR\"," \
    "    \"ts\": \"BIGINT\"" \
    "  }'" \
    ")"

// Field accessor macros (single-field convenience, each calls oast_decode_json independently)

#define OAST_TIMESTAMP_MACRO \
    "CREATE OR REPLACE MACRO oast_timestamp(domain) AS " \
    "to_timestamp(" \
    "  CAST(json_extract(oast_decode_json(domain), '$.ts') AS BIGINT)" \
    ")"

#define OAST_CAMPAIGN_MACRO \
    "CREATE OR REPLACE MACRO oast_campaign(domain) AS " \
    "json_extract_string(oast_decode_json(domain), '$.campaign')"

#define OAST_KSORT_MACRO \
    "CREATE OR REPLACE MACRO oast_ksort(domain) AS " \
    "json_extract_string(oast_decode_json(domain), '$.ksort')"

#define OAST_MACHINE_ID_MACRO \
    "CREATE OR REPLACE MACRO oast_machine_id(domain) AS " \
    "json_extract_string(oast_decode_json(domain), '$.machine_id')"

// Extraction helper macros

#define OAST_COUNT_MACRO \
    "CREATE OR REPLACE MACRO oast_count(text) AS " \
    "json_array_length(oast_extract(text))"

#define OAST_HAS_OAST_MACRO \
    "CREATE OR REPLACE MACRO oast_has_oast(text) AS " \
    "oast_count(text) > 0"

#define OAST_EXTRACT_STRUCTS_MACRO \
    "CREATE OR REPLACE MACRO oast_extract_structs(text) AS " \
    "json_transform(" \
    "  oast_extract_decode(text)," \
    "  '[{" \
    "    \"original\": \"VARCHAR\"," \
    "    \"valid\": \"BOOLEAN\"," \
    "    \"ts\": \"BIGINT\"," \
    "    \"machine_id\": \"VARCHAR\"," \
    "    \"pid\": \"INTEGER\"," \
    "    \"counter\": \"INTEGER\"," \
    "    \"ksort\": \"VARCHAR\"," \
    "    \"campaign\": \"VARCHAR\"," \
    "    \"nonce\": \"VARCHAR\"" \
    "  }]'" \
    ")"

#endif // OAST_MACROS_H
