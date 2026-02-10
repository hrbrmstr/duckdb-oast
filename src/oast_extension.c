#include "duckdb_extension.h"

#include "oast_functions.h"
#include "oast_macros.h"

#include <stdio.h>
#include <string.h>

DUCKDB_EXTENSION_EXTERN

// Register a SQL macro, returning false on failure.
// On error, writes a diagnostic into err_buf so the caller can report it
// via access->set_error before returning false from the entrypoint.
static bool register_macro(duckdb_connection connection, const char *sql,
                           const char *macro_name, char *err_buf,
                           size_t err_buf_size) {
  duckdb_result result;
  duckdb_state state = duckdb_query(connection, sql, &result);
  if (state == DuckDBError) {
    const char *err = duckdb_result_error(&result);
    snprintf(err_buf, err_buf_size, "Failed to register macro '%s': %s",
             macro_name, err ? err : "unknown error");
    duckdb_destroy_result(&result);
    return false;
  }
  duckdb_destroy_result(&result);
  return true;
}

DUCKDB_EXTENSION_ENTRYPOINT(duckdb_connection connection,
                            duckdb_extension_info info,
                            struct duckdb_extension_access *access) {
  char err[512];


  // Register OAST scalar functions (must be registered before macros)
  RegisterOASTFunctions(connection);

  // Register SQL macros that wrap scalar functions.
  // These provide ergonomic STRUCT outputs and field accessors without
  // needing C API STRUCT creation.

  // Struct macros (multi-field, single JSON parse)
  if (!register_macro(connection, OAST_STRUCT_MACRO, "oast_struct", err,
                      sizeof(err)) ||
      !register_macro(connection, OAST_SUMMARY_MACRO, "oast_summary", err,
                      sizeof(err)) ||

      // Field accessor macros (single-field convenience)
      !register_macro(connection, OAST_TIMESTAMP_MACRO, "oast_timestamp", err,
                      sizeof(err)) ||
      !register_macro(connection, OAST_CAMPAIGN_MACRO, "oast_campaign", err,
                      sizeof(err)) ||
      !register_macro(connection, OAST_KSORT_MACRO, "oast_ksort", err,
                      sizeof(err)) ||
      !register_macro(connection, OAST_MACHINE_ID_MACRO, "oast_machine_id",
                      err, sizeof(err)) ||

      // Extraction helper macros
      !register_macro(connection, OAST_COUNT_MACRO, "oast_count", err,
                      sizeof(err)) ||
      !register_macro(connection, OAST_HAS_OAST_MACRO, "oast_has_oast", err,
                      sizeof(err)) ||
      !register_macro(connection, OAST_EXTRACT_STRUCTS_MACRO,
                      "oast_extract_structs", err, sizeof(err)) ||
      !register_macro(connection, OAST_FIRST_MACRO, "oast_first", err,
                      sizeof(err)) ||

      // Table macros (ergonomic SELECT * access)
      !register_macro(connection, OAST_DECODE_TBL_MACRO, "oast_decode_tbl",
                      err, sizeof(err)) ||
      !register_macro(connection, OAST_EXTRACT_TBL_MACRO, "oast_extract_tbl",
                      err, sizeof(err))) {
    access->set_error(info, err);
    return false;
  }

  return true;
}
