# jASTERIX

## What jASTERIX is

**jASTERIX** is the OpenATS-maintained C++ library that converts EUROCONTROL ASTERIX binary data to JSON - and, for supported categories, encodes JSON back to bytes. It is developed as a standalone GPL-3.0 library in its own repository (`~/workspace/jasterix/`) and is the decoding engine embedded in COMPASS. All ASTERIX structure knowledge lives in JSON definition files, not in C++ code: new categories and editions are added by writing definitions only, with no recompilation.

Two consumers:

- **COMPASS** (primary use case): links `libjasterix` and calls `decodeFile()` / `decodeData()` with callbacks receiving `std::unique_ptr<nlohmann::json>` chunks for in-memory processing. The COMPASS-side import pipeline (JSON-to-DBContent mappings, edition selection per data context) is documented in [readme_asterix.md](/home/sk/workspace/compass/src/task/import/asterix/readme_asterix.md).
- **`jasterix_client`** (secondary): CLI application for standalone decoding to stdout, text file, or zip; distributed as an AppImage.

Build, code conventions, dependencies, and naming rules for the library are covered in the repo's own [CLAUDE.md](/home/sk/workspace/jasterix/CLAUDE.md) - this readme covers the domain side: repo layout, definitions, output formats, and the API.

## Repository layout

| Path | What lives there |
|---|---|
| `include/jasterix/` | Public API headers: `jasterix.h` (main `jASTERIX` class), `category.h`, `edition.h`/`editionbase.h`, `refedition.h`/`ref.h`, `spfedition.h`/`spf.h`, `frameparser.h`, `record.h` (FSPEC/UAP), `itemparserbase.h`, `iteminfo.h` |
| `src/jasterix.cpp` | Main library implementation (decode entry points, chunking, TBB task orchestration) |
| `src/asterix/` | Parsing core: `ASTERIXParser` (data block splitting), `Category`, `Edition`, `Record`, REF/SPF editions |
| `src/frames/` | Framing parsers (`FrameParser`, `FrameParserTask`) for IOSS / IOSS-seq / RFF wrappers |
| `src/items/` | Item parser implementations, one per definition item type: fixed bytes/bits/bitfield, extendable, compound, repetitive, dynamic bytes, optional, skip. Factory dispatch via `ItemParserBase::createItemParser()` |
| `src/write/` | JSON output writing (`JsonWriter`, `JsonFileWriteTask`) |
| `src/client/` | `jasterix_client` CLI (`main.cpp`) |
| `src/test/` | Catch2 tests (`test_cat<NNN>_<edition>.cpp`, `test_encode.cpp`, `test_limits.cpp`, `test_performance.cpp`) plus small per-category binary test recordings |
| `src/utils/` | Files, logger, string conversion, ARTAS MD5 hash checker |
| `definitions/` | The canonical ASTERIX definition files (see below). COMPASS ships a copy in `compass/data/jasterix_definitions/`, kept in sync with this folder |
| `analyze/` | Python analysis scripts (data item statistics, ADS-B quality, mapping checks) |
| repo root | EUROCONTROL spec PDFs for offline reference (Part 1 ed 3.1 and others) |

## Supported categories and editions

From `definitions/categories/` (the `categories.json` registry lists the default edition per category):

| CAT | Editions | REFs | SPFs | Description |
|---|---|---|---|---|
| 001 | 1.1 | | | Monoradar target reports (legacy) |
| 002 | 1.0 | | | Monoradar service messages (legacy) |
| 004 | 1.4 | | | Safety net messages |
| 010 | 0.24 (Sensis), 0.31 | | | Surface movement data |
| 019 | 1.2, 1.3 | | | Multilateration system status |
| 020 | 1.5, 1.8 | 1.3 | | Multilateration target reports |
| 021 | 0.26, 2.1, 2.4 | 1.5 | Aireon (validation, not default) | ADS-B target reports |
| 023 | 1.2 | | | CNS/ATM ground station status |
| 025 | 1.1, 1.5 | | | CNS/ATM ground system status reports |
| 030 | 7.0 | | | ARTAS track messages |
| 034 | 1.26 | | | Monoradar service messages |
| 048 | 1.15, 1.23 | 1.9 | | Monoradar target reports |
| 062 | 1.12, 1.16, 1.18, 1.21 | 1.2, 1.4 | ARTAS TRIs | System track messages (SDPS/ARTAS) |
| 063 | 1.0, 1.1 | | | Sensor status messages |
| 065 | 1.2, 1.3 | | | SDPS service status |
| 247 | 1.2 | | | Version number exchange |
| 252 | 7.0 | | | ARTAS track updates |

COMPASS imports all of these except the ARTAS-internal 030/252.

## ASTERIX binary format essentials

The spec reference is `eurocontrol-specification-asterix-part1-ed-3-1.pdf` in the repo root.

```
[Framing]                      Optional recording/network encapsulation (IOSS, IOSS-seq, RFF)
  +- Data Block                CAT (1 byte) + LEN (2 bytes) + Data Records
       +- Data Record          FSPEC + Data Fields (one per FSPEC-selected item)
            +- Data Item       Structure per item type (see below)
                 +- Subitem    Fixed or variable octets
                      +- Element   Individual bit fields within a subitem
```

- **UAP (User Application Profile)**: ordered table mapping FRN positions to data item numbers; one per category/edition (some categories have conditional UAPs selected by a data item value).
- **FSPEC**: variable-length bitmask at the start of each record; each bit selects a UAP entry, the FX (LSB) bit chains additional FSPEC octets. There is no fixed record layout.
- **Data item types** (spec section 5.2.5): fixed length, extended length (FX-chained), explicit length (leading LEN octet), repetitive (leading REP count octet), and compound (a mini-FSPEC presence bitmask selecting data subitems, each of which can itself be any of the other types).

## Definition file format

All decoding rules live in `definitions/`:

- `data_block_definition.json` - the CAT/LEN/content envelope
- `framings/{ioss,ioss_seq,rff}.json` - recording wrappers (per-frame recording time, sequence numbers, board metadata, RFF file header)
- `categories/categories.json` - registry of supported editions, REFs, SPFs, and the default per category
- `categories/<NNN>/cat<NNN>_<edition>.json` - per-edition UAP plus per-item layout
- `categories/<NNN>/cat<NNN>_ref_<edition>.json` / `cat<NNN>_spf_*.json` - REF and SPF definitions (e.g. ARTAS TRIs in 062)

A category definition is a `record` with a `field_specification` (FSPEC), a `uap` array, and an `items` array; each item has `data_fields` describing the bit/byte layout:

```json
{
    "name": "cat048_1.23_record",
    "type": "record",
    "field_specification": { "name": "FSPEC", "type": "extendable_bits", ... },
    "uap": ["010", "140", "020", "040", "070", "090", "130", ...],
    "items": [
        { "number": "010", "name": "Data Source Identifier", "type": "item",
          "data_fields": [
              { "name": "SAC", "type": "fixed_bytes", "length": 1, "data_type": "uint" },
              { "name": "SIC", "type": "fixed_bytes", "length": 1, "data_type": "uint" }
          ]
        }
    ]
}
```

**Field types**: `fixed_bytes`, `fixed_bits`, `fixed_bitfield`, `extendable_bits`, `extendable`, `compound`, `repetitive`, `dynamic_bytes`, `optional`, `skip_bytes`. Scaled quantities carry an `lsb` factor (e.g. 360/2^16 degrees for azimuth).

**ASCII only**: definition JSONs (here and in the COMPASS copy) must use plain ASCII - no Greek letters, curly quotes, or en/em-dashes. Their content renders through pdflatex in the COMPASS ASTERIX Import / Data Item Analysis reports.

### Adding a new edition, REF, or SPF

1. Create the definition file in `categories/<NNN>/`: `cat<NNN>_<edition>.json` for an edition, `cat<NNN>_ref_<edition>.json` for a REF, `cat<NNN>_spf_<name>.json` for an SPF. SPF definitions have type `SimpleSpecialPurposeField` (plain item list, parsed in order) or `ComplexSpecialPurposeField` (own FSPEC + `items_indicator` UAP, structured like an REF).
2. Register it in `categories/categories.json` under the category's `editions` / `ref_editions` / `spf_editions` object. The key is the edition name, the value an object with `document` (source spec title), `date`, and `file` (path relative to `categories/`). Beware: the three registry keys look alike - registering an SPF under `ref_editions` silently replaces the existing REF registration (duplicate JSON keys, last one wins).
3. Set or keep the category's `default_edition` / `default_ref_edition` / `default_spf_edition`. An empty `default_spf_edition` means the SPF content is not decoded (kept as a hex string) unless a consumer selects an SPF edition via `Category::setCurrentSPFEdition()` - which COMPASS does per data context, but `jasterix_client` has no CLI option for, so only the default applies there.
4. Add a `test_cat<NNN>_<edition>.cpp` with a small binary sample in `src/test/` and register it in `src/test/CMakeLists.txt`.
5. Sync the changed definition files AND `categories.json` into `compass/data/jasterix_definitions/`.

### REF/SPF length-mismatch fallback

REF and SPF fields carry a leading 1-byte length indicator, which is authoritative for framing. When a REF/SPF definition is selected but the field content does not match it (the definition reads more or fewer bytes than announced, e.g. a foreign SPF from other equipment mixed into the stream), the field is NOT treated as a decode error: the partial decode is discarded, the content is kept as a raw hex string (same representation as when no REF/SPF edition is selected), a `"ref_error": true` / `"spf_error": true` flag is added to the record, a warning is logged, and parsing resumes after the announced length. The rest of the record and data block decode normally. Only an announced length that overruns the data block remains a hard decode error (stream desync).

Affected records are counted: `jASTERIX::numREFErrors()` / `numSPFErrors()` after decoding, and `num_ref_errors` / `num_spf_errors` keys in the `analyzeFile()`/`analyzeData()` result (next to `num_errors`, which stays 0 for these records). In flat mode the field's leaf columns are null for such records; the hex string and flag are not part of the columnar output.

### Skipped categories in the analysis result

Data blocks of categories that were not decoded are counted per category and reported in the `analyzeFile()` / `analyzeData()` result (and in `--analyze`) under the `skipped_categories` key. `analyzePCAPFile()` returns one sub-result per network stream, each carrying its own `skipped_categories`.

```json
"skipped_categories": {
    "23": { "data_blocks": 412, "bytes": 8240, "reason": "no specification" },
    "34": { "data_blocks": 1150, "bytes": 27600, "reason": "decoding disabled" }
}
```

`reason` is `no specification` when no definition exists for the category, and `decoding disabled` when a definition exists but decoding was switched off (`decodeCategory()` / `--only_cats`). The key is absent when nothing was skipped. This is what COMPASS uses to report unknown categories found in a recording during import probing.

## JSON output formats

Two output formats: **structured** (default) and **flat** (`--flat` / `do_flat`).

**Structured** mirrors the data block / record hierarchy; item numbers are keys:

```json
{ "data_blocks": [ { "category": 48, "content": { "records": [
    { "010": { "SAC": 0, "SIC": 1 },
      "040": { "RHO": 73.92, "THETA": 89.67 },
      "140": { "Time-of-Day": 33499.84 } } ] } } ] }
```

**Flat** is columnar, keyed by category number: each leaf field becomes a top-level array with one entry per record; fields not selected by FSPEC are `null`:

```json
{ "48": { "010.SAC": [0, 0, 5], "040.RHO": [73.92, null, 55.10] } }
```

In flat mode two CAT001 corrections are applied during decoding, since the record ordering needed for them is lost in columnar output: SAC/SIC from the first record of a data block is propagated to subsequent records that omit I001/010, and a full `140.Time-of-Day` column is reconstructed from the truncated Time of Day (I001/141) using the last CAT002 Time of Day (I002/030) of the same SAC/SIC as reference. The reconstruction picks the time consistent with the truncated value that is closest to the reference on the circular 24 h clock, handling the 512 s wrap and the midnight reset per CAT001 Part 2a section 5.2.15 Notes 1 and 2; offsets of 256 s or more from the reference are ambiguous and yield `null`.

Two optional side columns are added per category when their global flag is set: `artas_md5` (`add_artas_md5_hash`) and `record_data` (`add_record_data`, the record's original bytes as lowercase hex). They sit next to the leaf columns, one entry per record. In structured mode the same two values are written as record keys instead. Covered by `test_artas_md5.cpp` and `test_record_data.cpp`.

**Recording time.** Recordings and captures carry the time at which a data block was recorded, separate from the ASTERIX Time of Day items. It is reported with the same keys for every source, each source delivering what it knows:

| key | meaning | IOSS / IOSS-seq | RFF | PCAP |
|---|---|---|---|---|
| `recording_time` | seconds since midnight (double) | frame item, 10 ms resolution | header start time plus `frame_relative_time_ms`, wrapped at midnight | packet capture time, UTC |
| `recording_day` | day counter, 0 at recording start (uint) | frame item | midnight wraps counted | - |
| `recording_date` | UTC date as `YYYYMMDD` (uint) | - | header start date plus `recording_day` | from the capture time |

In structured mode the keys are written into every data block object (`frames[].content.data_blocks[]` when framed, `data_blocks[]` for PCAP), next to `category` and `content`. Framed output also keeps them in the frame object. In flat mode they are side columns per category, like `artas_md5`, one entry per accepted record, and they only exist when the source provides the key. PCAP data blocks additionally keep `pcap_time` / `pcap_time_epoch`.

IOSS names the frame items `recording_time` and `recording_day` directly, so a framing definition provides the keys by naming its items that way. RFF has no absolute time per frame, so `rff.json` declares a computed time: `"recording_time": {"start_item": "start_datetime", "offset_item": "frame_relative_time_ms", "offset_lsb": 0.001}`. The 128 byte RFF file header holds the start and stop wall time as text in one of three layouts (` MM/DD/YY HH:MM:SS`, `DD/MM/YY HH:MM:SS`, `YYYY-MM-DD HH:MM:SS`, seconds resolution), parsed into `recording_start_time` (seconds since midnight) and `recording_start_date` in the header object. Without a readable start time the frames get `recording_time` / `recording_day` relative to the recording start, no `recording_date`, and a warning is logged. The RFF frame header fields (relative time in ms, length) are little-endian. Covered by `test_recording_time_ioss.cpp`, `test_recording_time_rff.cpp` and `test_recording_time_pcap.cpp`. Flat re-encoding skips all side columns.

Raw/netto recordings and the `decodeData` buffer entry point (COMPASS network and PCAP import) have no recording time source, so the keys are absent there.

All columns of a category hold exactly one entry per **accepted** record. A record refused after parsing (it overran its data block, or an item parser threw) has already written leaf values at the current record index, so those cells are dropped again and the record is counted in `num_errors` instead of `num_records`. The alignment invariant is checked in `test_flat_decode_data.cpp`, which also covers the `decodeData` entry point used by the PCAP and network readers.

Repetitive items are represented differently per format. Structured mode nests them as an array of objects plus a `"REP"` count key (`"SPF": { "REP": 2, "Target Report Identifiers": [ { "TRI": "76427f0a" }, { "TRI": "10c4d792" } ] }`). Flat mode flattens down to the leaf (struct-of-arrays): one column per leaf path, each per-record cell an array of scalars aligned by repetition index (`"SPF.Target Report Identifiers.TRI": [["76427f0a", "10c4d792"], null, ...]`), plus a `<prefix>.REP` column mirroring the structured REP location (`"SPF.REP"`). Multi-field repetitions produce one such column per field, aligned by index. Extendable items keep their whole array-of-objects in a single column keyed by the item path. Flat-to-nested reconstruction stays lossless: repetitive leaf cells (arrays of scalars) are zipped back into the array-of-objects form by repetition index.

## Library API

Main entry point is the `jASTERIX` class in `include/jasterix/jasterix.h`:

```cpp
jASTERIX::jASTERIX jasterix(definition_path, print, debug, debug_exclude_framing);

jasterix.decodeFile(filename, framing, callback);      // file with framing ("" = raw/netto)
jasterix.decodeData(data, size, callback);             // raw buffer
jasterix.decodePCAPFile(filename, callback);           // libpcap capture (payload of all streams, raw/netto)

jasterix.hasCategory(cat);
jasterix.decodeCategory(cat, enable);                  // enable/disable per category
jasterix.category(cat);                                // Category object: editions, REFs, SPFs, current selection
jasterix.stopDecoding();                               // abort from another thread
```

Callbacks receive `std::unique_ptr<nlohmann::json>` chunks; decoding is chunked and TBB-parallelized so large files never load fully into memory.

### Encoding

jASTERIX can also encode, not just decode:

```cpp
std::vector<char> encodeRecord(unsigned int category, const nlohmann::json& record_json, bool debug = false);
std::vector<char> encodeDataBlock(unsigned int category, const std::vector<nlohmann::json>& records, bool debug = false);
```

Both take **structured (nested) JSON** as produced by decoding and return a complete data block (CAT + LEN + records). Encoding is byte-exact for round trips (verified in `test_encode.cpp`, including CAT062 1.21). The CLI exposes encoding for **flat** input via `--encode_flat` / `--encode_flat_zip` (the client reconstructs the nested form from the flat columns - lossless, see above - then encodes); structured-JSON encoding is library-only.

## CLI client

```
jasterix_client --definition_path definitions/ --filename <file> [options]
```

`--definition_path` is mandatory; `--filename` is required for anything that reads data. Options (from `src/client/main.cpp`, boost::program_options):

**Input**

| Option | Meaning |
|---|---|
| `--filename <path>` | Input file to decode |
| `--definition_path <path>` | Path to the definition files (mandatory) |
| `--framing <name>` | Input framing as named in `definitions/framings/` (`ioss`, `ioss_seq`, `rff`). Default is raw/netto (no framing) |
| `--pcap` | Input file is a PCAP capture (libpcap); the ASTERIX payload of all network streams is extracted in capture order and decoded as raw/netto (no framing, do not combine with `--framing`) |

**Decoding scope and behavior**

| Option | Meaning |
|---|---|
| `--only_cats <list>` | Restrict decoded categories, e.g. `20,21,48`. Others are skipped |
| `--editions <list>` | Select non-default editions per category, e.g. `21:0.26,48:1.15`. Without this, the defaults from `categories.json` apply |
| `--ref_edition <list>` | Select non-default REF editions per category, same syntax, e.g. `21:1.5,62:1.4` |
| `--spf_edition <list>` | Select non-default SPF editions per category, same syntax, e.g. `21:Aireon,62:ARTAS` |
| `--frame_limit <n>` | Max frames to process (with framing). Default -1 (unlimited) |
| `--frame_chunk_size <n>` | Frames per processing chunk. Default 1000, -1 disables chunking |
| `--data_block_limit <n>` | Max data blocks to process (without framing). Default -1 (unlimited) |
| `--data_block_chunk_size <n>` | Data blocks per processing chunk. Default 1000, -1 disables chunking |
| `--record_limit <n>` | Max records to process, decode and analyze alike. Processing stops at the end of the chunk in which the limit is reached, so slightly more records than `n` are delivered. Tighten with the chunk sizes. Default -1 (unlimited) |
| `--single_thread` | Disable TBB multi-threading (deterministic ordering, easier debugging) |

**Output**

| Option | Meaning |
|---|---|
| `--print` | Print the decoded JSON to stdout |
| `--print_indent <n>` | Indentation for printed JSON, -1 for compact single-line output |
| `--flat` | Output in flat/columnar format (category -> leaf name -> array) instead of the structured record hierarchy |
| `--write_type <text\|zip>` | Write decoded JSON to a file; `text` writes one JSON chunk per line, `zip` writes a zip archive with one member per chunk. Needs `--write_filename` |
| `--write_filename <path>` | Output file for `--write_type`, e.g. `out.json` or `out.zip` |
| `--data_write_size <n>` | Number of chunks per file write. Default 1, -1 disables |
| `--add_record_data` | Include each record's original bytes as hex in the output (useful for definition debugging and byte-exact comparison) |

**Analysis and diagnostics**

| Option | Meaning |
|---|---|
| `--analyze` | Analyze data sources and contents (per-SAC/SIC, per-category, per-item statistics) instead of full JSON output; this is what COMPASS uses for its import probe |
| `--analyze_csv` | Same analysis, printed as CSV |
| `--analyze_record_limit <n>` | Same as `--record_limit`, analyze only. Kept for compatibility. Default 0 (no limit) |
| `--print_cat_info` | Print the supported categories, editions, REFs, and SPFs, then exit |
| `--log_perf` | Print performance statistics (records/s, MB/s) after processing |
| `--debug` | Verbose decoding debug output (only sensible for small files) |
| `--debug_include_framing` | Include framing layer in the debug output (requires `--debug`) |

**ARTAS MD5** (only in builds with OpenSSL)

| Option | Meaning |
|---|---|
| `--add_artas_md5` | Add ARTAS MD5 hashes to the output |
| `--check_artas_md5 <cats>` | Compute and verify ARTAS MD5 hashes for the given categories, e.g. `1,20,21,48`. Implies record data; cannot be combined with `--write_type` |

**Flat re-encoding** (JSON back to binary ASTERIX)

| Option | Meaning |
|---|---|
| `--encode_flat <path>` | Encode flat columnar JSON from a text file (one flat chunk object per line, as written by `--flat --write_type text`) back to raw/netto ASTERIX. Needs `--encode_filename` |
| `--encode_flat_zip <path>` | Same, from a zip archive (members = chunks, as written by `--flat --write_type zip`) |
| `--encode_filename <path>` | Output binary file for the re-encoded raw/netto ASTERIX |
| `--encode_cat <n>` | Restrict encoding to a single category. Default 0 encodes all categories present. Editions are taken from `--editions`, defaults otherwise |

### Example commands

Structured mode (default):

```bash
# Print an IOSS-framed recording as indented structured JSON
jasterix_client --definition_path definitions/ --filename recording.ff --framing ioss \
    --print --print_indent 2

# Decode a raw/netto recording, only CAT021 + CAT048, with specific editions, into a zip
jasterix_client --definition_path definitions/ --filename recording.rec \
    --only_cats 21,48 --editions 21:2.1,48:1.15 \
    --write_type zip --write_filename out.zip

# Decode the ASTERIX payload of a PCAP capture, print compact JSON
jasterix_client --definition_path definitions/ --filename capture.pcap --pcap \
    --print --print_indent -1

# Analyze a recording (data sources, categories, item statistics), no JSON output
jasterix_client --definition_path definitions/ --filename recording.ff --framing ioss --analyze

# Verify ARTAS MD5 hashes for CAT048 tracks
jasterix_client --definition_path definitions/ --filename recording.rec --check_artas_md5 48
```

Flat mode:

```bash
# Print a recording as flat columnar JSON
jasterix_client --definition_path definitions/ --filename recording.rec --flat \
    --print --print_indent 2

# Write flat output to a text file (one flat chunk object per line)
jasterix_client --definition_path definitions/ --filename recording.rec --flat \
    --write_type text --write_filename flat.json

# Round trip: decode to flat zip, then re-encode to binary ASTERIX (byte-exact)
jasterix_client --definition_path definitions/ --filename recording.rec --flat \
    --write_type zip --write_filename flat.zip
jasterix_client --definition_path definitions/ \
    --encode_flat_zip flat.zip --encode_filename reencoded.rec

# Re-encode only CAT062 from a flat text file, using a non-default edition
jasterix_client --definition_path definitions/ \
    --encode_flat flat.json --encode_cat 62 --editions 62:1.21 \
    --encode_filename cat062.rec
```

Note that framed input decodes fine, but re-encoding always produces raw/netto output - framings are not re-generated.

## Testing

```bash
./build/bin/test_categories --definition_path definitions/ --data_path src/test/
```

One `test_cat<NNN>_<edition>.cpp` per category/edition decoding a small binary sample and asserting field values; `test_encode.cpp` for round-trip encoding; `test_limits.cpp` for edge cases; `test_performance.cpp` for benchmarks. Register new tests in `src/test/CMakeLists.txt`. Tests that need framed or PCAP input build the file in memory with the helpers in `test_synthetic_files.h`.

**Instance reuse.** One `jASTERIX` instance can serve any sequence of decode and analyze calls, structured and flat mixed. Every call starts with a clean chunk pipeline (done flags, leftover chunks of a stopped producer task) and, for structured calls, with the parser tree taken out of columnar mode. The analysis result counters (`num_frames`, `num_records`, `num_errors`, `num_ref_errors`, `num_spf_errors`) describe that call only, `numRecords()` and friends stay cumulative over the instance lifetime. Covered by `test_consecutive_calls.cpp`, which also analyzes a PCAP with two network streams, the case that first showed the problem: the second stream got no records without a record limit, and stale data blocks of the first stream with one.

**Release check** (`test/release_check.py`): checks a packaged AppImage together with a definitions zip, the pairing that ships. It decodes every checked-in sample in `src/test/`, and probes a set of recordings from a data folder outside the repository. Per file it runs `--analyze`, a structured decode and a flat decode with the same `--record_limit`, reduces all three to the same statistics (records and data sources per category, count/min/max per item path) and compares them to each other, and the analysis to a stored result (`--store` writes it). Recording settings (framing or PCAP, editions) and the stored results live next to the data as `jasterix_release_check_recordings.json` / `jasterix_release_check_expected.json`. For PCAP the analysis runs per network stream, so with a record limit it is compared to the stored result only. `python3 test/release_check.py --help` lists the options.

## Relationship to COMPASS

- COMPASS ships a copy of the definitions in `compass/data/jasterix_definitions/` - **keep it in sync** with `~/workspace/jasterix/definitions/` whenever definitions change.
- `ASTERIXImportTask` constructs one `jASTERIX` instance against that folder and applies the active data context's per-category edition/REF/SPF selection (`configurejASTERIX()`).
- The decoded JSON is mapped to DBContent variables via the `ASTERIXJSONParser` mappings in `conf/default/task_import_asterix_cat<NNN>.json`.
- The ASTERIX format itself (categories, layered information hierarchy, position accuracy and altitude variants, framings/REF/SPF semantics) and the full import pipeline are documented in [readme_asterix.md](/home/sk/workspace/compass/src/task/import/asterix/readme_asterix.md).

## Reference documents

EUROCONTROL spec PDFs live at the jASTERIX repo root (Part 1 ed 3.1) and, organized per category with historical editions, under `~/Nextcloud/documents/asterix/` - see the reference table in readme_asterix.md for the exact files backing each implemented edition.