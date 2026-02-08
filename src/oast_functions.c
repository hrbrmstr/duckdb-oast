#include "oast_functions.h"
#include "duckdb_extension.h"
#include "oast_decode.h"
#include "oast_extract.h"
#include "oast_validate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DUCKDB_EXTENSION_EXTERN

// oast_validate(VARCHAR) -> BOOLEAN
static void OASTValidateFunction(duckdb_function_info info,
                                 duckdb_data_chunk input,
                                 duckdb_vector output) {
  idx_t count = duckdb_data_chunk_get_size(input);

  // Get input vector (VARCHAR)
  duckdb_vector input_vec = duckdb_data_chunk_get_vector(input, 0);
  duckdb_string_t *input_data =
      (duckdb_string_t *)duckdb_vector_get_data(input_vec);
  uint64_t *input_validity = duckdb_vector_get_validity(input_vec);

  // Get output data pointer (BOOLEAN)
  bool *output_data = (bool *)duckdb_vector_get_data(output);

  if (input_validity) {
    duckdb_vector_ensure_validity_writable(output);
    uint64_t *output_validity = duckdb_vector_get_validity(output);

    for (idx_t row = 0; row < count; row++) {
      if (duckdb_validity_row_is_valid(input_validity, row)) {
        duckdb_string_t str = input_data[row];
        const char *str_data = duckdb_string_t_data(&str);
        size_t str_len = duckdb_string_t_length(str);

        output_data[row] = oast_validate(str_data, str_len);
      } else {
        duckdb_validity_set_row_invalid(output_validity, row);
      }
    }
  } else {
    // No NULLs, process all rows
    for (idx_t row = 0; row < count; row++) {
      duckdb_string_t str = input_data[row];
      const char *str_data = duckdb_string_t_data(&str);
      size_t str_len = duckdb_string_t_length(str);

      output_data[row] = oast_validate(str_data, str_len);
    }
  }
}

// oast_decode_json(VARCHAR) -> VARCHAR (JSON)
static void OASTDecodeJSONFunction(duckdb_function_info info,
                                   duckdb_data_chunk input,
                                   duckdb_vector output) {
  idx_t count = duckdb_data_chunk_get_size(input);

  // Get input vector (VARCHAR)
  duckdb_vector input_vec = duckdb_data_chunk_get_vector(input, 0);
  duckdb_string_t *input_data =
      (duckdb_string_t *)duckdb_vector_get_data(input_vec);
  uint64_t *input_validity = duckdb_vector_get_validity(input_vec);

  // Handle potential NULL inputs
  if (input_validity) {
    duckdb_vector_ensure_validity_writable(output);
    uint64_t *output_validity = duckdb_vector_get_validity(output);

    for (idx_t row = 0; row < count; row++) {
      if (duckdb_validity_row_is_valid(input_validity, row)) {
        duckdb_string_t str = input_data[row];
        const char *str_data = duckdb_string_t_data(&str);
        size_t str_len = duckdb_string_t_length(str);

        oast_decoded_t result;
        oast_decode(str_data, str_len, &result);

        // Build JSON string
        char json[2048];
        snprintf(json, sizeof(json),
                 "{\"original\":\"%s\",\"valid\":%s,\"ts\":%u,"
                 "\"machine_id\":\"%02x:%02x:%02x\",\"pid\":%u,\"counter\":%u,"
                 "\"ksort\":\"%s\",\"campaign\":\"%s\",\"nonce\":\"%s\"",
                 result.original, result.valid ? "true" : "false",
                 result.timestamp, result.machine_id[0], result.machine_id[1],
                 result.machine_id[2], result.pid, result.counter, result.ksort,
                 result.campaign, result.nonce);

        if (!result.valid && result.error[0]) {
          size_t len = strlen(json);
          snprintf(json + len, sizeof(json) - len, ",\"error\":\"%s\"}",
                   result.error);
        } else {
          strcat(json, "}");
        }

        duckdb_vector_assign_string_element_len(output, row, json,
                                                strlen(json));
      } else {
        duckdb_validity_set_row_invalid(output_validity, row);
      }
    }
  } else {
    // No NULLs, process all rows
    for (idx_t row = 0; row < count; row++) {
      duckdb_string_t str = input_data[row];
      const char *str_data = duckdb_string_t_data(&str);
      size_t str_len = duckdb_string_t_length(str);

      oast_decoded_t result;
      oast_decode(str_data, str_len, &result);

      char json[2048];
      snprintf(json, sizeof(json),
               "{\"original\":\"%s\",\"valid\":%s,\"ts\":%u,"
               "\"machine_id\":\"%02x:%02x:%02x\",\"pid\":%u,\"counter\":%u,"
               "\"ksort\":\"%s\",\"campaign\":\"%s\",\"nonce\":\"%s\"",
               result.original, result.valid ? "true" : "false",
               result.timestamp, result.machine_id[0], result.machine_id[1],
               result.machine_id[2], result.pid, result.counter, result.ksort,
               result.campaign, result.nonce);

      if (!result.valid && result.error[0]) {
        size_t len = strlen(json);
        snprintf(json + len, sizeof(json) - len, ",\"error\":\"%s\"}",
                 result.error);
      } else {
        strcat(json, "}");
      }

      duckdb_vector_assign_string_element_len(output, row, json, strlen(json));
    }
  }
}

// oast_extract(VARCHAR) -> VARCHAR (JSON array of strings)
static void OASTExtractFunction(duckdb_function_info info,
                                duckdb_data_chunk input, duckdb_vector output) {
  idx_t count = duckdb_data_chunk_get_size(input);

  duckdb_vector input_vec = duckdb_data_chunk_get_vector(input, 0);
  duckdb_string_t *input_data =
      (duckdb_string_t *)duckdb_vector_get_data(input_vec);
  uint64_t *input_validity = duckdb_vector_get_validity(input_vec);

  if (input_validity) {
    duckdb_vector_ensure_validity_writable(output);
    uint64_t *output_validity = duckdb_vector_get_validity(output);

    for (idx_t row = 0; row < count; row++) {
      if (duckdb_validity_row_is_valid(input_validity, row)) {
        duckdb_string_t str = input_data[row];
        const char *str_data = duckdb_string_t_data(&str);
        size_t str_len = duckdb_string_t_length(str);

        // Extract matches
        oast_match_t *matches = NULL;
        size_t match_count = 0;
        oast_extract(str_data, str_len, &matches, &match_count);

        // Build JSON array
        char json[8192];
        size_t json_len = 0;
        json[json_len++] = '[';

        for (size_t i = 0; i < match_count; i++) {
          if (i > 0) {
            json[json_len++] = ',';
          }
          json[json_len++] = '"';

          // Copy full domain (subdomain.domain)
          size_t copy_len = matches[i].full_len;
          if (json_len + copy_len + 2 < sizeof(json)) {
            memcpy(json + json_len, matches[i].full, copy_len);
            json_len += copy_len;
          }

          json[json_len++] = '"';
        }

        json[json_len++] = ']';
        json[json_len] = '\0';

        duckdb_vector_assign_string_element_len(output, row, json, json_len);

        free(matches);
      } else {
        duckdb_validity_set_row_invalid(output_validity, row);
      }
    }
  } else {
    for (idx_t row = 0; row < count; row++) {
      duckdb_string_t str = input_data[row];
      const char *str_data = duckdb_string_t_data(&str);
      size_t str_len = duckdb_string_t_length(str);

      oast_match_t *matches = NULL;
      size_t match_count = 0;
      oast_extract(str_data, str_len, &matches, &match_count);

      char json[8192];
      size_t json_len = 0;
      json[json_len++] = '[';

      for (size_t i = 0; i < match_count; i++) {
        if (i > 0) {
          json[json_len++] = ',';
        }
        json[json_len++] = '"';

        size_t copy_len = matches[i].full_len;
        if (json_len + copy_len + 2 < sizeof(json)) {
          memcpy(json + json_len, matches[i].full, copy_len);
          json_len += copy_len;
        }

        json[json_len++] = '"';
      }

      json[json_len++] = ']';
      json[json_len] = '\0';

      duckdb_vector_assign_string_element_len(output, row, json, json_len);

      free(matches);
    }
  }
}

// oast_extract_decode(VARCHAR) -> VARCHAR (JSON array of decoded objects)
static void OASTExtractDecodeFunction(duckdb_function_info info,
                                      duckdb_data_chunk input,
                                      duckdb_vector output) {
  idx_t count = duckdb_data_chunk_get_size(input);

  duckdb_vector input_vec = duckdb_data_chunk_get_vector(input, 0);
  duckdb_string_t *input_data =
      (duckdb_string_t *)duckdb_vector_get_data(input_vec);
  uint64_t *input_validity = duckdb_vector_get_validity(input_vec);

  if (input_validity) {
    duckdb_vector_ensure_validity_writable(output);
    uint64_t *output_validity = duckdb_vector_get_validity(output);

    for (idx_t row = 0; row < count; row++) {
      if (duckdb_validity_row_is_valid(input_validity, row)) {
        duckdb_string_t str = input_data[row];
        const char *str_data = duckdb_string_t_data(&str);
        size_t str_len = duckdb_string_t_length(str);

        // Extract matches
        oast_match_t *matches = NULL;
        size_t match_count = 0;
        oast_extract(str_data, str_len, &matches, &match_count);

        // Build JSON array of decoded objects
        char *json = malloc(16384);
        if (!json) {
          duckdb_validity_set_row_invalid(output_validity, row);
          free(matches);
          continue;
        }

        size_t json_len = 0;
        json[json_len++] = '[';

        for (size_t i = 0; i < match_count; i++) {
          if (i > 0 && json_len < 16383) {
            json[json_len++] = ',';
          }

          // Decode this match
          oast_decoded_t decoded;
          oast_decode(matches[i].full, matches[i].full_len, &decoded);

          // Append decoded JSON
          size_t remaining = 16384 - json_len;
          if (remaining < 2)
            break; // Need at least room for "]" and null terminator

          int written = snprintf(
              json + json_len, remaining,
              "{\"original\":\"%.*s\",\"valid\":%s,\"ts\":%u,"
              "\"machine_id\":\"%02x:%02x:%02x\",\"pid\":%u,\"counter\":%u,"
              "\"ksort\":\"%s\",\"campaign\":\"%s\",\"nonce\":\"%s\"}",
              (int)matches[i].full_len, matches[i].full,
              decoded.valid ? "true" : "false", decoded.timestamp,
              decoded.machine_id[0], decoded.machine_id[1],
              decoded.machine_id[2], decoded.pid, decoded.counter,
              decoded.ksort, decoded.campaign, decoded.nonce);

          // Check if snprintf succeeded and didn't truncate
          if (written > 0 && (size_t)written < remaining) {
            json_len += written;
          } else {
            // Output was truncated, stop adding more entries
            break;
          }
        }

        if (json_len < 16383) {
          json[json_len++] = ']';
        }
        if (json_len < 16384) {
          json[json_len] = '\0';
        }

        duckdb_vector_assign_string_element_len(output, row, json, json_len);

        free(json);
        free(matches);
      } else {
        duckdb_validity_set_row_invalid(output_validity, row);
      }
    }
  } else {
    for (idx_t row = 0; row < count; row++) {
      duckdb_string_t str = input_data[row];
      const char *str_data = duckdb_string_t_data(&str);
      size_t str_len = duckdb_string_t_length(str);

      oast_match_t *matches = NULL;
      size_t match_count = 0;
      oast_extract(str_data, str_len, &matches, &match_count);

      char *json = malloc(16384);
      if (!json) {
        free(matches);
        continue;
      }

      size_t json_len = 0;
      json[json_len++] = '[';

      for (size_t i = 0; i < match_count; i++) {
        if (i > 0 && json_len < 16383) {
          json[json_len++] = ',';
        }

        oast_decoded_t decoded;
        oast_decode(matches[i].full, matches[i].full_len, &decoded);

        size_t remaining = 16384 - json_len;
        if (remaining < 2)
          break; // Need at least room for "]" and null terminator

        int written = snprintf(
            json + json_len, remaining,
            "{\"original\":\"%.*s\",\"valid\":%s,\"ts\":%u,"
            "\"machine_id\":\"%02x:%02x:%02x\",\"pid\":%u,\"counter\":%u,"
            "\"ksort\":\"%s\",\"campaign\":\"%s\",\"nonce\":\"%s\"}",
            (int)matches[i].full_len, matches[i].full,
            decoded.valid ? "true" : "false", decoded.timestamp,
            decoded.machine_id[0], decoded.machine_id[1], decoded.machine_id[2],
            decoded.pid, decoded.counter, decoded.ksort, decoded.campaign,
            decoded.nonce);

        // Check if snprintf succeeded and didn't truncate
        if (written > 0 && (size_t)written < remaining) {
          json_len += written;
        } else {
          // Output was truncated, stop adding more entries
          break;
        }
      }

      if (json_len < 16383) {
        json[json_len++] = ']';
      }
      if (json_len < 16384) {
        json[json_len] = '\0';
      }

      duckdb_vector_assign_string_element_len(output, row, json, json_len);

      free(json);
      free(matches);
    }
  }
}

void RegisterOASTFunctions(duckdb_connection connection) {
  duckdb_logical_type varchar_type =
      duckdb_create_logical_type(DUCKDB_TYPE_VARCHAR);
  duckdb_logical_type bool_type =
      duckdb_create_logical_type(DUCKDB_TYPE_BOOLEAN);

  // Register oast_validate(VARCHAR) -> BOOLEAN
  {
    duckdb_scalar_function function = duckdb_create_scalar_function();
    duckdb_scalar_function_set_name(function, "oast_validate");
    duckdb_scalar_function_add_parameter(function, varchar_type);
    duckdb_scalar_function_set_return_type(function, bool_type);
    duckdb_scalar_function_set_function(function, OASTValidateFunction);
    duckdb_register_scalar_function(connection, function);
    duckdb_destroy_scalar_function(&function);
  }

  // Register oast_decode_json(VARCHAR) -> VARCHAR
  {
    duckdb_scalar_function function = duckdb_create_scalar_function();
    duckdb_scalar_function_set_name(function, "oast_decode_json");
    duckdb_scalar_function_add_parameter(function, varchar_type);
    duckdb_scalar_function_set_return_type(function, varchar_type);
    duckdb_scalar_function_set_function(function, OASTDecodeJSONFunction);
    duckdb_register_scalar_function(connection, function);
    duckdb_destroy_scalar_function(&function);
  }

  // Register oast_extract(VARCHAR) -> VARCHAR (JSON array)
  {
    duckdb_scalar_function function = duckdb_create_scalar_function();
    duckdb_scalar_function_set_name(function, "oast_extract");
    duckdb_scalar_function_add_parameter(function, varchar_type);
    duckdb_scalar_function_set_return_type(function, varchar_type);
    duckdb_scalar_function_set_function(function, OASTExtractFunction);
    duckdb_register_scalar_function(connection, function);
    duckdb_destroy_scalar_function(&function);
  }

  // Register oast_extract_decode(VARCHAR) -> VARCHAR (JSON array)
  {
    duckdb_scalar_function function = duckdb_create_scalar_function();
    duckdb_scalar_function_set_name(function, "oast_extract_decode");
    duckdb_scalar_function_add_parameter(function, varchar_type);
    duckdb_scalar_function_set_return_type(function, varchar_type);
    duckdb_scalar_function_set_function(function, OASTExtractDecodeFunction);
    duckdb_register_scalar_function(connection, function);
    duckdb_destroy_scalar_function(&function);
  }

  duckdb_destroy_logical_type(&varchar_type);
  duckdb_destroy_logical_type(&bool_type);
}
