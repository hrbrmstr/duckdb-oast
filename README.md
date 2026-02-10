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
- `oast_first(text)` - Extract and decode first OAST domain from text (STRUCT)

Table Macros (use with `SELECT * FROM`):
- `oast_decode_tbl(domain)` - Decode a domain into a row with all fields + timestamp
- `oast_extract_tbl(text)` - Extract and decode all domains from text into rows

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

Option 1: Table macro (recommended -- use with `SELECT *` or LATERAL joins)

```sql
-- All decoded fields as columns, including a proper TIMESTAMP
SELECT * FROM oast_decode_tbl('c58bduhe008dovpvhvugcfemp9yyyyyyn.oast.pro');

-- Join decoded fields alongside existing table columns
SELECT t.client_ip, d.campaign, d.ksort, d.timestamp
FROM sensor_logs t, LATERAL oast_decode_tbl(t.domain) d;
```

Option 2: STRUCT output (for expressions and filters)

```sql
-- Access individual fields with dot notation
SELECT
    oast_struct(domain).ksort,
    oast_struct(domain).campaign
FROM sensor_logs;

-- All fields via unnest
SELECT unnest(oast_struct(domain)) FROM sensor_logs;
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
FROM read_csv('logs/access.log', header=false, columns={'line': 'VARCHAR'})
WHERE oast_count(line) > 0;

-- Extract domains (returns JSON array)
SELECT oast_extract(line) as domains
FROM read_csv('logs/access.log', header=false, columns={'line': 'VARCHAR'})
WHERE oast_count(line) > 0;

-- Returns: ["c58bduhe008dovpvhvugcfemp9yyyyyyn.oast.pro"]
```

### Extract and Decode in One Call

```sql
-- Table macro: decoded domains as rows with proper columns
SELECT * FROM oast_extract_tbl('Log entry with c58bduhe008dovpvhvugcfemp9yyyyyyn.oast.pro and c5aov2fh0s0006ocs40gcfemp9yyyyyyn.oast.fun');

-- First domain only (common single-domain-per-line case)
SELECT oast_first('Log entry with c58bduhe008dovpvhvugcfemp9yyyyyyn.oast.pro').campaign;
-- Returns: he008

-- JSON output (for compatibility)
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

Use the table macro for the cleanest syntax. Falls back to `oast_struct()` subquery if needed.

```sql
-- Best: table macro with LATERAL join (parses once, clean column access)
SELECT t.*, d.campaign, d.ksort, d.machine_id
FROM sessions t, LATERAL oast_decode_tbl(t.domain) d
WHERE d.valid;

-- Alternative: struct subquery (parses once)
SELECT d.campaign, d.ksort, d.machine_id
FROM (SELECT oast_struct(domain) AS d FROM sessions WHERE oast_validate(domain));

-- Bad: parses three times
SELECT oast_campaign(domain), oast_ksort(domain), oast_machine_id(domain)
FROM sessions WHERE oast_validate(domain);
```

### All fields

Use the table macro for `SELECT *`, or `unnest()` with struct macros.

```sql
-- Table macro (includes proper timestamp column)
SELECT * FROM sessions t, LATERAL oast_decode_tbl(t.domain) d WHERE d.valid;

-- Struct macro + unnest
SELECT unnest(oast_struct(domain)) FROM sessions WHERE oast_validate(domain);

-- Just the commonly used fields
SELECT unnest(oast_summary(domain)) FROM sessions WHERE oast_validate(domain);
```

### Extracting from text

Use `oast_extract_tbl()` for the full pipeline from raw text to decoded rows.

```sql
-- Table macro: each domain becomes a row with typed columns
SELECT t.line, d.campaign, d.timestamp
FROM raw_logs t, LATERAL oast_extract_tbl(t.payload) d;

-- Single domain per line (common case)
SELECT oast_first(payload).campaign
FROM raw_logs WHERE oast_has_oast(payload);

-- Struct list approach (when you need the list itself)
SELECT unnest(oast_extract_structs(payload), recursive := true)
FROM raw_logs WHERE oast_has_oast(payload);
```

## Threat Hunting Workflows

### Filter Valid OAST Hits from Sensor Data

```sql
-- Load sensor session data
CREATE TABLE sensor_sessions AS
SELECT * FROM read_csv('sensor_data.csv');

-- Find all valid OAST callbacks with decoded metadata
SELECT
    t.timestamp as log_ts,
    t.client_ip,
    t.domain,
    d.campaign,
    d.timestamp as oast_ts
FROM sensor_sessions t, LATERAL oast_decode_tbl(t.domain) d
WHERE d.valid
ORDER BY t.timestamp DESC;
```

### Time-Based Analysis

```sql
-- Correlate OAST callbacks by timestamp
SELECT
    date_trunc('hour', d.timestamp) as hour,
    count(*) as callback_count,
    d.campaign
FROM sensor_sessions t, LATERAL oast_decode_tbl(t.domain) d
WHERE d.valid
GROUP BY hour, d.campaign
ORDER BY hour DESC;
```

### K-Sort Deduplication

```sql
-- Deduplicate callbacks using K-sort identifier
SELECT
    d.ksort,
    d.campaign,
    min(d.timestamp) as first_seen,
    max(d.timestamp) as last_seen,
    count(*) as total_hits
FROM sensor_sessions t, LATERAL oast_decode_tbl(t.domain) d
WHERE d.valid
GROUP BY d.ksort, d.campaign;
```

### Extract from Arbitrary Logs

```sql
-- Scan web server logs: extract and decode all OAST domains as rows
SELECT t.line, d.campaign, d.ksort, d.timestamp
FROM read_csv('logs/access.log', header=false, columns={'line': 'VARCHAR'}) t,
     LATERAL oast_extract_tbl(t.line) d;

-- Quick scan: first domain per line only
SELECT line, oast_first(line).campaign as campaign
FROM read_csv('logs/access.log', header=false, columns={'line': 'VARCHAR'})
WHERE oast_has_oast(line);

-- JSON output (for raw domain lists)
SELECT line, oast_extract(line) as found_domains
FROM read_csv('logs/access.log', header=false, columns={'line': 'VARCHAR'})
WHERE oast_has_oast(line);
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

-- Star expansion via unnest
SELECT unnest(oast_struct(domain)) FROM logs;
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
SELECT unnest(oast_summary(domain)) FROM logs WHERE oast_validate(domain);
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
SELECT line FROM read_csv('access.log', header=false, columns={'line':'VARCHAR'})
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
SELECT unnest(oast_extract_structs(payload), recursive := true) FROM raw_logs WHERE oast_has_oast(payload);

-- Access specific fields
SELECT s.campaign FROM (SELECT unnest(oast_extract_structs(payload)) AS s FROM raw_logs);
```

#### `oast_first(text VARCHAR) -> STRUCT`

Extract and decode the first OAST domain found in text. Returns a single STRUCT (not a list). Convenient for the common one-domain-per-line pattern.

- Input: Text to search
- Returns: STRUCT with same fields as `oast_struct()`, or NULL if no OAST domains found
- NULL handling: Returns NULL for NULL input

Example:
```sql
-- Single-field access on first domain
SELECT oast_first(line).campaign
FROM read_csv('access.log', header=false, columns={'line':'VARCHAR'})
WHERE oast_has_oast(line);
```

#### Table Macros

Table macros return result sets and are used in `FROM` clauses. They support `SELECT *` and LATERAL joins naturally.

#### `oast_decode_tbl(domain VARCHAR) -> TABLE`

Decodes an OAST domain into a single-row table with all fields plus a proper `timestamp` column (TIMESTAMP WITH TIME ZONE).

- Input: OAST domain (subdomain or FQDN)
- Returns: Row with fields: `original`, `valid`, `ts`, `machine_id`, `pid`, `counter`, `ksort`, `campaign`, `nonce`, `timestamp`

Example:
```sql
-- Standalone
SELECT * FROM oast_decode_tbl('c58bduhe008dovpvhvugcfemp9yyyyyyn.oast.pro');

-- LATERAL join with existing table
SELECT t.client_ip, d.campaign, d.timestamp
FROM sensor_logs t, LATERAL oast_decode_tbl(t.domain) d
WHERE d.valid;
```

#### `oast_extract_tbl(text VARCHAR) -> TABLE`

Extracts all OAST domains from text, decodes each, and returns one row per domain with all fields plus a proper `timestamp` column.

- Input: Text to search
- Returns: One row per domain found, with fields: `original`, `valid`, `ts`, `machine_id`, `pid`, `counter`, `ksort`, `campaign`, `nonce`, `timestamp`

Example:
```sql
-- Standalone
SELECT * FROM oast_extract_tbl('log with c58bduhe008dovpvhvugcfemp9yyyyyyn.oast.pro embedded');

-- LATERAL join for log scanning
SELECT t.line, d.campaign, d.timestamp
FROM read_csv('access.log', header=false, columns={'line':'VARCHAR'}) t,
     LATERAL oast_extract_tbl(t.line) d;
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
