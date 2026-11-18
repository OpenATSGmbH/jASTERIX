# Open PRs on OpenATSGmbH/jASTERIX

Reminder for two contributor PRs. Status 2026-11-18.

## PR 51: json.hpp include in the public headers

https://github.com/OpenATSGmbH/jASTERIX/pull/51

Problem: the installed public headers include `"json.hpp"`, but the installation does not copy this file. A program that uses the installed library does not compile.

The PR changes the include to `<nlohmann/json.hpp>` in 8 public headers. Do not merge it as it is:

- The bundled header is `lib/json.hpp`. There is no `nlohmann/` directory, so the build fails without a system nlohmann package.
- The files in `src/` still include `"json.hpp"`.
- The public headers use `nlohmann::json` in signatures. A program must use the same nlohmann version as the library (ABI namespace `json_abi_v3_12_0`).

Task, as part of the shared library rework:

1. Move the bundled header to `lib/nlohmann/json.hpp`.
2. Use `<nlohmann/json.hpp>` in all files, also in `src/`.
3. Install the header with the public headers.
4. Close PR 51 with a link to the change. Credit the reporter.

## PR 52: `"data_type": "string"` for `dynamic_bytes`

https://github.com/OpenATSGmbH/jASTERIX/pull/52

The PR adds an option. With it, `dynamic_bytes` writes the bytes as text, not as `{"index", "length"}`. It is for fields with one length octet (which counts itself), followed by ASCII characters.

The PR names "the IFPLID subfield of CAT011". This does not agree with the specification:

- CAT011 edition 1.3 and the drafts 0.10 to 0.14: I011/390 subfield 3 (IFPS_FLIGHT_ID) is a fixed 4-octet binary field. Subfield 2 (Callsign) is a fixed 7-octet ASCII field.
- No standard CAT011 item has a length octet followed by text. Only SP and RE have an explicit length.
- EUROCONTROL lists no REF for CAT011 (categories and statuses, 2025-10-22).

We asked the contributor for the document and data item of the field, and for sample binary CAT011 data.

Task:

1. Wait for the reply. Without a reference, do not add the option.
2. If the field is real, request these changes before the merge:
   - Bounds check `index + length > total_size`, as in `fixed_bytes`.
   - Check for length underflow (length 0 with `additative_factor` -1 gives `SIZE_MAX`).
   - ASCII check with `isASCII()`, as in `fixed_bytes`.
   - Enum or bool, not a string compare in `parseItem`.
   - Unit test, with the sample data if possible.
