# DuckDB OAST Extension

A DuckDB extension for validating, decoding, and extracting OAST (Out-of-Band Application Security Testing) domains directly in SQL queries.

## Features

### Scalar Functions (JSON returns)

- `oast_validate(domain)` - Check if a string is a valid OAST domain (returns BOOLEAN)
- `oast_decode_json(domain)` - Decode OAST metadata (timestamp, machine ID, PID, counter, etc.) (returns JSON)
- `oast_extract(text)` - Find all OAST domains in arbitrary text (returns JSON array)
- `oast_extract_decode(text)` - Extract and decode OAST domains in one call (returns JSON array)

### SQL Macros (STRUCT returns)

Struct Macros (multi-field, efficient):
- `oast_struct(domain)` - Full decode to STRUCT with all fields
- `oast_summary(domain)` - Compact STRUCT with ksort, campaign, machine_id, ts

Field Accessor Macros (single-field convenience):
- `oast_timestamp(domain)` - Extract timestamp as TIMESTAMP WITH TIME ZONE
- `oast_campaign(domain)` - Extract campaign identifier (VARCHAR)
- `oast_ksort(domain)` - Extract K-sort prefix (VARCHAR)
- `oast_machine_id(domain)` - Extract machine ID (VARCHAR)

Extraction Helpers:
- `oast_count(text)` - Count OAST domains in text (BIGINT)
- `oast_has_oast(text)` - Check if text contains OAST domains (BOOLEAN)
- `oast_extract_structs(text)` - Extract and decode all domains to LIST(STRUCT)

Supports OAST domains from: oast.pro, oast.live, oast.site, oast.online, oast.fun, oast.me, interact.sh, interactsh.com. (More planned.)

## Installation

### Build from Source

```bash
git clone https://codeberg.org/hrbrmstr/go-roast
cd duckdb-oast
make configure
make release
```

The extension binary will be at `build/release/oast.duckdb_extension`.

### Load in DuckDB

```bash
duckdb -unsigned
```

```sql
LOAD './build/release/oast.duckdb_extension';
```

## Usage Examples

### Validate OAST Domains

```sql
-- Check if domain is valid OAST
SELECT oast_validate('c58bduhe008dovpvhvugcfemp9yyyyyyn.oast.pro');
-- Returns: true

SELECT oast_validate('invalid-domain.com');
-- Returns: false
```

### Decode OAST Metadata

Option 1: STRUCT output (recommended for most cases)

```sql
-- Decode to native STRUCT (ergonomic field access)
SELECT oast_struct('c58bduhe008dovpvhvugcfemp9yyyyyyn.oast.pro');

-- Access fields with dot notation
SELECT
    oast_struct(domain).ksort,
    oast_struct(domain).campaign,
    oast_struct(domain).timestamp
FROM sensor_logs;

-- Star expansion
SELECT oast_struct(domain).* FROM sensor_logs;
```

Option 2: JSON output (for compatibility)

```sql
-- Decode to JSON string
SELECT oast_decode_json('c58bduhe008dovpvhvugcfemp9yyyyyyn.oast.pro');
-- Returns JSON:
-- {
--   "original": "c58bduhe008dovpvhvugcfemp9yyyyyyn.oast.pro",
--   "valid": true,
--   "timestamp": 1632679674,
--   "machine_id": "2e:00:10",
--   "pid": 56447,
--   "counter": 4165629,
--   "ksort": "c58bdu",
--   "campaign": "he008",
--   "nonce": "cfemp9yyyyyyn"
-- }

-- Extract specific fields
SELECT
    json_extract_string(oast_decode_json(domain), '$.ksort') as ksort,
    json_extract_string(oast_decode_json(domain), '$.campaign') as campaign,
    json_extract(oast_decode_json(domain), '$.timestamp') as timestamp
FROM sensor_logs;
```

### Extract OAST Domains from Text

```sql
-- Count OAST domains in log files (fast)
SELECT line, oast_count(line) as num_domains
FROM read_csv_auto('logs/access.log', header=false, columns={'line': 'VARCHAR'})
WHERE oast_count(line) > 0;

-- Extract domains (returns JSON array)
SELECT oast_extract(line) as domains
FROM read_csv_auto('logs/access.log', header=false, columns={'line': 'VARCHAR'})
WHERE oast_count(line) > 0;

-- Returns: ["c58bduhe008dovpvhvugcfemp9yyyyyyn.oast.pro"]
```

### Extract and Decode in One Call

```sql
-- Extract and decode all OAST domains from text
SELECT oast_extract_decode('Log entry with c58bduhe008dovpvhvugcfemp9yyyyyyn.oast.pro and c5aov2fh0s0006ocs40gcfemp9yyyyyyn.oast.fun');
-- Returns JSON array with decoded metadata for both domains
```

## Performance Guide: Choosing the Right Function

When working with OAST domains in DuckDB, choose based on how many fields you need:

### Single field (filters, GROUP BY on one key)

Use the field accessor macros. They parse JSON once per call, which is fine for one field.

```sql
SELECT * FROM sessions WHERE oast_campaign(domain) = 'he008';

-- Time-based filtering
SELECT * FROM sessions
WHERE oast_timestamp(domain) > '2025-01-01'::TIMESTAMP;
```

### Multiple fields from the same domain

Use `oast_struct()` and subquery or alias to avoid repeated JSON parsing.

```sql
-- Good: parses once
SELECT d.campaign, d.ksort, d.machine_id
FROM (SELECT oast_struct(domain) AS d FROM sessions WHERE oast_validate(domain));

-- Bad: parses three times
SELECT oast_campaign(domain), oast_ksort(domain), oast_machine_id(domain)
FROM sessions WHERE oast_validate(domain);
```

### All fields

Use `oast_struct()` with star expansion or `oast_summary()` for just the key fields.

```sql
-- All fields
SELECT oast_struct(domain).* FROM sessions WHERE oast_validate(domain);

-- Just the commonly used fields
SELECT oast_summary(domain).* FROM sessions WHERE oast_validate(domain);
```

### Extracting from text

Use `oast_extract_structs()` for the full pipeline from raw text to decoded struct rows.

```sql
SELECT unnest(oast_extract_structs(payload)).*
FROM raw_logs WHERE oast_has_oast(payload);
```

## Threat Hunting Workflows

### Filter Valid OAST Hits from Sensor Data

```sql
-- Load sensor session data
CREATE TABLE sensor_sessions AS
SELECT * FROM read_csv_auto('sensor_data.csv');

-- Find all valid OAST callbacks (efficient with struct)
SELECT
    timestamp,
    client_ip,
    domain,
    oast_campaign(domain) as campaign
FROM sensor_sessions
WHERE oast_validate(domain)
ORDER BY timestamp DESC;
```

### Time-Based Analysis

```sql
-- Correlate OAST callbacks by timestamp (efficient with accessor)
SELECT
    date_trunc('hour', oast_timestamp(domain)) as hour,
    count(*) as callback_count,
    oast_campaign(domain) as campaign
FROM sensor_sessions
WHERE oast_validate(domain)
GROUP BY hour, campaign
ORDER BY hour DESC;
```

### K-Sort Deduplication

```sql
-- Deduplicate callbacks using K-sort identifier (efficient with summary)
WITH decoded AS (
    SELECT oast_summary(domain).* FROM sensor_sessions WHERE oast_validate(domain)
)
SELECT DISTINCT
    ksort,
    campaign,
    min(ts) as first_seen,
    max(ts) as last_seen,
    count(*) as total_hits
FROM decoded
GROUP BY ksort, campaign;
```

### Extract from Arbitrary Logs

```sql
-- Scan web server logs for embedded OAST domains
SELECT
    line,
    oast_extract(line) as found_domains
FROM read_csv_auto('logs/access.log', header=false, columns={'line': 'VARCHAR'})
WHERE oast_extract(line) != '[]';

-- Decode all found domains
SELECT
    line,
    oast_extract_decode(line) as decoded
FROM read_csv_auto('logs/access.log', header=false, columns={'line': 'VARCHAR'})
WHERE oast_extract(line) != '[]';
```

## Function Reference

### SQL Macros (Ergonomic Wrappers)

#### Struct Macros (efficient multi-field access)

#### `oast_struct(domain VARCHAR) -> STRUCT`

Decodes OAST domain and returns native STRUCT for ergonomic field access. Parses JSON once.

- Input: OAST domain (subdomain or FQDN)
- Returns: STRUCT with fields:
  - `original`: Original input
  - `valid`: Whether decode succeeded
  - `ts`: Unix timestamp (BIGINT)
  - `machine_id`: 3-byte machine identifier (VARCHAR hex)
  - `pid`: Process ID (INTEGER)
  - `counter`: Counter value (INTEGER)
  - `ksort`: First 6 chars for K-sorting (VARCHAR)
  - `campaign`: Campaign identifier (VARCHAR)
  - `nonce`: z-base-32 nonce (VARCHAR)
- NULL handling: Returns NULL for NULL input

Example:
```sql
-- Field access
SELECT oast_struct(domain).ksort, oast_struct(domain).ts FROM logs;

-- Star expansion
SELECT oast_struct(domain).* FROM logs;
```

#### `oast_summary(domain VARCHAR) -> STRUCT`

Returns compact STRUCT with just the commonly used fields. Parses JSON once.

- Input: OAST domain (subdomain or FQDN)
- Returns: STRUCT with fields:
  - `ksort`: K-sort prefix (VARCHAR)
  - `campaign`: Campaign identifier (VARCHAR)
  - `machine_id`: Machine ID (VARCHAR)
  - `ts`: Unix timestamp (BIGINT)
- NULL handling: Returns NULL for NULL input

Example:
```sql
SELECT oast_summary(domain).* FROM logs WHERE oast_validate(domain);
```

#### Field Accessor Macros (single-field convenience)

Each of these parses JSON independently. Use for single-field access in filters or sorting.

#### `oast_timestamp(domain VARCHAR) -> TIMESTAMP WITH TIME ZONE`

Returns decoded timestamp as a proper DuckDB TIMESTAMP.

Example:
```sql
SELECT * FROM logs WHERE oast_timestamp(domain) > '2025-01-01'::TIMESTAMP;
```

#### `oast_campaign(domain VARCHAR) -> VARCHAR`

Extracts the campaign identifier field.

Example:
```sql
SELECT * FROM logs WHERE oast_campaign(domain) = 'he008';
```

#### `oast_ksort(domain VARCHAR) -> VARCHAR`

Extracts the K-sort prefix (first 6 chars of preamble).

Example:
```sql
SELECT DISTINCT oast_ksort(domain) FROM logs WHERE oast_validate(domain);
```

#### `oast_machine_id(domain VARCHAR) -> VARCHAR`

Extracts the machine ID field.

Example:
```sql
SELECT oast_machine_id(domain), count(*) FROM logs GROUP BY oast_machine_id(domain);
```

#### Extraction Helper Macros

#### `oast_count(text VARCHAR) -> BIGINT`

Counts OAST domains in arbitrary text.

- Input: Text to search
- Returns: Number of OAST domains found
- NULL handling: Returns NULL for NULL input

Example:
```sql
-- Filter rows with OAST domains
SELECT * FROM logs WHERE oast_count(payload) > 0;

-- Aggregate by count
SELECT oast_count(payload) as n, count(*) as rows
FROM logs GROUP BY oast_count(payload);
```

#### `oast_has_oast(text VARCHAR) -> BOOLEAN`

Quick predicate for filtering rows that contain OAST domains. Equivalent to `oast_count(text) > 0`.

Example:
```sql
SELECT line FROM read_csv_auto('access.log', header=false, columns={'line':'VARCHAR'})
WHERE oast_has_oast(line);
```

#### `oast_extract_structs(text VARCHAR) -> LIST(STRUCT)`

Extract all OAST domains from text, decode each, return as list of structs. Full pipeline in one call.

- Input: Text to search
- Returns: LIST(STRUCT) with same fields as `oast_struct()`
- NULL handling: Returns NULL for NULL input

Example:
```sql
-- Extract, decode, and flatten to rows
SELECT unnest(oast_extract_structs(payload)).* FROM raw_logs WHERE oast_has_oast(payload);

-- Access specific fields
SELECT unnest(oast_extract_structs(payload)).campaign FROM raw_logs;
```

### Scalar Functions (JSON/Boolean Returns)

#### `oast_validate(domain VARCHAR) -> BOOLEAN`

Checks if the input is a valid OAST domain (subdomain + known OAST domain suffix).

- Input: Domain string (subdomain or FQDN)
- Returns: `true` if valid OAST domain, `false` otherwise
- NULL handling: Returns `NULL` for `NULL` input

### `oast_decode_json(domain VARCHAR) -> VARCHAR`

Decodes OAST preamble and returns metadata as JSON string.

- Input: OAST domain (subdomain or FQDN)
- Returns: JSON object with fields:
  - `original`: Original input string
  - `valid`: Whether decode succeeded
  - `ts`: Unix timestamp (seconds)
  - `machine_id`: 3-byte machine identifier (hex format)
  - `pid`: Process ID
  - `counter`: 24-bit counter value
  - `ksort`: First 6 chars of preamble (for K-sorting)
  - `campaign`: Chars 7-11 of preamble (campaign identifier)
  - `nonce`: z-base-32 nonce portion
  - `error`: Error message if invalid
- NULL handling: Returns `NULL` for `NULL` input

### `oast_extract(text VARCHAR) -> VARCHAR`

Extracts all OAST domains found in arbitrary text.

- Input: Text to search
- Returns: JSON array of OAST domain strings
- NULL handling: Returns `NULL` for `NULL` input

### `oast_extract_decode(text VARCHAR) -> VARCHAR`

Extracts and decodes all OAST domains in text (combines extract + decode).

- Input: Text to search
- Returns: JSON array of decoded OAST objects
- NULL handling: Returns `NULL` for `NULL` input

## Build System

### Requirements

- CMake 3.5+
- C99 compiler
- Python 3 (for build scripts)
- Git (for version metadata)

### Make Targets

- `make configure` - Set up build environment (Python venv, dependencies)
- `make debug` - Build debug version with symbols
- `make release` - Build optimized release version
- `make test_debug` - Run SQLLogicTest suite (debug)
- `make test_release` - Run SQLLogicTest suite (release)
- `make clean` - Remove build artifacts
- `make clean_all` - Full clean including configure

### Build Variables

Set in `Makefile`:

- `EXTENSION_NAME` - Extension name (default: oast)
- `USE_UNSTABLE_C_API` - Use unstable DuckDB C API (default: 0)
- `TARGET_DUCKDB_VERSION` - DuckDB version to target (default: v1.2.0)

## Architecture

### C Source Layout

```
src/
├── oast_extension.c      # Extension entry point
├── oast_functions.c      # DuckDB function registration
├── oast_decode.c         # Preamble decoding (base32hex -> XID fields)
├── oast_extract.c        # Domain extraction (hand-rolled matcher)
├── oast_validate.c       # Domain validation
├── oast_base32.c         # Base32hex encoding utilities
├── oast_domains.c        # Known OAST domain list
└── include/              # Header files
```

### Design Decisions

- No external dependencies - Pure C implementation, no regex libraries
- Hand-rolled extractor - Avoids POSIX regex or PCRE2 dependencies
- JSON string returns - Stable C API compatible, works with DuckDB's JSON functions
- Stable C API only - Uses DuckDB stable C API (v1.2.0+)

## Testing

### Run Test Suite

```bash
make test_debug
make test_release
```

### Test Coverage

- Validation with valid/invalid domains
- Decoding with test vectors from Go reference implementation
- Extraction from text with embedded domains
- NULL input handling
- Batch processing with CSV data

## Performance

Binary sizes:
- Debug build: ~55KB
- Release build: ~51KB (stripped)

The extension uses vectorized processing for batch operations, processing entire DuckDB data chunks at once.


## Related Projects

This extension ports the OAST decode/extract logic from [go-roast](https://codeberg.org/hrbrmstr/go-roast), a Go-based MCP server for OAST domain analysis.

## License

MIT

## Contributing

Issues and pull requests welcome.
