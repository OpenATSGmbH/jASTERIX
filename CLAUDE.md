# jASTERIX - ASTERIX to JSON Converter

jASTERIX is a C++ library that converts EUROCONTROL ASTERIX binary data to JSON, and encodes JSON back to bytes. It is part of the OpenATS COMPASS project, but released as a standalone library for use in other projects. All ASTERIX structure is defined in JSON files, so categories and editions are added without recompilation.

**Read [readme_jasterix.md](readme_jasterix.md) first.** It is the canonical reference for the repository layout, the supported categories and editions, the definition file format, the REF/SPF fallback behavior, the JSON output formats, the library API, and the CLI client. Do not duplicate that content here.

Keep readme_jasterix.md up to date in the same change whenever a new category, edition, REF, or SPF is added, or when the definition format, the decoder or encoder behavior, the output formats, or the API change.

This file holds only what readme_jasterix.md does not cover: build, test, conventions, dependencies, and the non-obvious constraints below.

## Non-obvious constraints

- **Output contract**: the public API returns `nlohmann::json` objects to the caller. This is a hard interface requirement. COMPASS depends on receiving JSON trees, not raw bytes or strings. Do not change it for performance reasons.
- **Primary consumer is COMPASS, not the CLI**: COMPASS links the library and calls `decodeFile()` / `decodeData()` with a callback receiving `std::unique_ptr<nlohmann::json>` chunks for in-memory processing. The CLI client and file output are secondary. Weigh API changes by the COMPASS use case first.
- **`libjasterix` is a static library** (`libjasterix.a`), despite the name. `deploy_jasterix.sh` in the COMPASS repo copies it into the AppImage.
- **Definition JSONs are ASCII only**. No Greek letters, curly quotes, or dashes outside ASCII. Their content renders through pdflatex in the COMPASS ASTERIX Import and Data Item Analysis reports. Note that some existing files still carry `\uXXXX` escapes.
- **Definitions are duplicated**: `definitions/` here and `compass/data/jasterix_definitions/` in the COMPASS repo. Any definition change must be synced into both, including `categories.json`.
- **The three registry keys look alike**: `editions`, `ref_editions`, and `spf_editions` in `categories.json`. Registering an SPF under `ref_editions` silently replaces the REF registration, because duplicate JSON keys resolve to the last one.
- **Threading**: data blocks of one chunk are decoded in parallel through TBB and share one `ASTERIXParser`. Per-parser counters that the decode path writes must be atomic. Use `--single_thread` for deterministic ordering while debugging.
- **Error paths must clamp their hexdumps**: lengths in error messages come from possibly corrupt input. Use `binary2hex_bounded()`, never `binary2hex()` with a declared length. An unclamped dump reads past the end of the memory-mapped file and crashes.
- **The mapping feature is removed** (conversion to a "general format"). If you find `Mapping`, `setCurrentMapping()`, or `*_mapping.json` references anywhere, they are stale.

## Platform and distribution

- **OS**: Linux 64-bit (x86_64) only. No Windows or macOS support.
- **Distribution**: AppImage, built on Debian 10 (Buster) in Docker to maximize glibc compatibility. Built from the COMPASS repo via `docker/build_jasterix.sh` and `docker/deploy_jasterix.sh`.
- **Licensing**: source code is GPL-3.0, the AppImage binary is CC BY 4.0. Free for all use including commercial.

## Build

```bash
cmake -B build -S .
cmake --build build -j$(nproc)
```

Output goes to `build/bin/` (executables) and `build/lib/` (libraries).

`scripts/make.sh` configures (Ninja, install prefix `/usr`), builds and runs `sudo cmake --install build`. `--clean` removes `build/` first, `--test` runs `test_categories` before installing.

**C++ standard**: C++17 (`-std=c++17`). The AppImage is built with GCC 8.3 on Debian 10, so keep to what that compiler supports. Structured bindings, `std::optional`, `std::variant`, `if constexpr`, `std::string_view`, and `std::any` are available.

**Compiler flags** (set in `CMakeLists.txt`, selected by the presence of `/.dockerenv`):
- Docker: `-UNDEBUG -Wall -std=c++17 -fext-numeric-literals`
- Local: `-UNDEBUG -Wall -std=c++17 -fno-omit-frame-pointer`

**Build type** is hardcoded to `RelWithDebInfo` at the top of `CMakeLists.txt`. The Docker build overrides it with `-DCMAKE_BUILD_TYPE=Release`.

**Optional dependencies** are toggled in `CMakeLists.txt`: `USE_LOG4CPP` and `USE_OPENSSL`, both true by default. `global.h` is generated from `global.h.in` and carries these flags plus `PACKAGE_VERSION`.

**Targets**: `jasterix` (library), `jasterix_client` (CLI), `test_categories`, `test_limits`, `test_performance`.

## Testing

Framework: Catch2 (header-only, in `lib/catch.hpp`).

```bash
./build/bin/test_categories --definition_path definitions/ --data_path src/test/
```

Test executables take `--definition_path` and `--data_path` as custom Catch2 options through clara.

Tests live in `src/test/`, named `test_cat<NNN>_<edition>.cpp` for category tests, plus behavior tests such as `test_encode.cpp`, `test_spf_fallback.cpp`, and `test_bounds.cpp`. `test_jasterix.h` holds the shared helpers. Each test decodes a small binary sample stored next to the source and asserts field values.

When adding a category or edition, add a `test_cat<NNN>_<edition>.cpp` with a sample and register it in `src/test/CMakeLists.txt`.

`test_limits` and `test_performance` need a large IOSS recording passed through `--filename`, which is not shipped with the repository. They take no `--data_path`. CTest registers them only when the capture is given at configure time, with `-DTEST_FILENAME=/path/to/file.ff`, and skips them otherwise.

## Code conventions

### Naming
- **Files**: lowercase with underscores: `category.cpp`, `frameparser.h`, `string_conv.h`
- **Headers**: `.h` (not `.hpp`), use `#pragma once`
- **Classes**: PascalCase: `Category`, `Edition`, `FrameParser`, `ItemParserBase`
- **Functions**: camelCase: `hasEdition()`, `setCurrentEdition()`, `decodeFile()`, `parseItem()`
- **Getters**: no `get` prefix, just the property name: `name()`, `number()`, `comment()`
- **Setters**: `set` prefix: `setCurrentEdition()`, `setDebug()`
- **Query methods**: `has` prefix: `hasEdition()`, `hasCurrentREFEdition()`, `hasCategory()`
- **Member variables**: snake_case with trailing underscore: `number_`, `current_edition_`
- **Local variables**: snake_case without trailing underscore
- **Namespace**: `jASTERIX`
- **Language**: American English, in code, comments, and documentation
- **No em-dashes** anywhere. Use an ASCII hyphen.

### Patterns
- **Factory**: `ItemParserBase::createItemParser()` dispatches to the concrete item parser subclass based on the JSON `type` field
- **Smart pointers**: `std::shared_ptr` for editions, `std::unique_ptr` for item parsers
- **Callback-based streaming**: `std::function` callbacks for frames, data blocks, and records, so large files are never fully buffered
- **using declarations**: `using namespace std;` and `using namespace nlohmann;` at the top of `.cpp` files

### Include order
1. Project includes: `#include <jasterix/...>`
2. Third-party headers: `#include "json.hpp"`
3. Standard library: `#include <string>`, `#include <map>`, `#include <memory>`

### Logging
Use the LOG4CPP stream macros from `src/utils/logger.h`. They prepend the function name and work like output streams.

- `logerr` - errors (always printed)
- `logwrn` - warnings
- `loginf` - informational messages
- `logdbg` - debug (compiled in, filtered by runtime log level)

```cpp
loginf << "decoded " << count << " records" << logendl;
```

Do not use `std::cout`, `std::cerr`, or `printf` for application logging. Write parameter values without an equals sign, as in `"records " << count`.

### License header
Every source file carries the GPL-3.0 header referencing jASTERIX. Copy it from any existing `.h` or `.cpp` file.

## Key dependencies

- **Boost** >= 1.73.0: program_options, filesystem, iostreams, regex, system, stacktrace_backtrace
- **Intel TBB**: multi-threaded frame and data block processing
- **LibArchive**: zip output and zip input for flat re-encoding
- **libpcap**: PCAP capture reading
- **LOG4CPP**: logging (optional)
- **OpenSSL**: ARTAS MD5 hashes (optional)
- **nlohmann/json**: JSON, header-only in `lib/`
- **Catch2**: unit testing, header-only in `lib/`
