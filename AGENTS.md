AGENTS.md — Agent guidelines for Harmora

Scope
- This file contains guidance for automated agents and contributors working in the repository root (/home/rbarbier/Documents/Harmora).
- If a more deeply nested AGENTS.md exists, that file takes precedence for files in its subtree.

Quick summary
- Build: `make` (default target), `make .build/program.elf` (compile only), `make flash` (compile + flash via avrdude)
- Inspect size: `make size`
- Clean: `make clean`
- Serial monitor: `make screen` (uses `screen ${PORT} 115200` as defined in Makefile)
- Lint / static analysis (recommended): `cppcheck --enable=all --inconclusive --std=c11 --suppress=missingIncludeSystem src/`
- Formatting: use `clang-format` (suggested style: Google or LLVM). Example: `clang-format -i src/**/*.c src/**/*.h`
- Tests: There are no unit tests in this repo. Recommended test approach: Unity + Ceedling for host-side unit tests. Example Ceedling commands (after installing Ceedling): `ceedling test:all` and `ceedling test:some_test.c` (see Ceedling docs for running a single test file).

Important: repository specifics
- Platform: AVR (ATmega328P). Toolchain used in Makefile: `avr-gcc`, `avr-objcopy`, `avrdude`.
- MCU flags: `-mmcu=atmega328p -DF_CPU=16000000UL -Os` (size optimization) and include paths under `src/` are already configured in `Makefile`.

Build / Flash / Debug commands
- Build & flash (default):
  - `make` (the default `all` target in the Makefile triggers `flash`)
  - Equivalent explicit: `make flash` — will build `.build/program.hex` then call `avrdude` using `PORT`, `BAUD`, and `PROGRAMMER` variables from the Makefile.
- Compile only (no flash):
  - `make .build/program.elf` — compiles sources into the ELF binary in `$(BUILD_DIR)`. This is the canonical way to trigger the compile step without flashing.
  - Alternatively run: `make` then interrupt before flashing, but prefer the explicit ELF target.
- Generate HEX (if needed):
  - `make .build/program.hex` (this will also compile ELF as a dependency)
- Print memory usage:
  - `make size` — runs `avr-size` (configured as `avr-s:ize` in the Makefile) then parses and prints flash/RAM usage.
- Flash manually (if you prefer to run avrdude yourself):
  - `avrdude -p atmega328p -c arduino -P /dev/ttyACM0 -b 115200 -U flash:w:.build/program.hex:i` (adjust `PORT` and `BAUD` as needed)
- Serial console / UART capture:
  - `make screen` — opens `screen $(PORT) 115200` as defined in the Makefile. If `screen` is not available, use `minicom` or `picocom`: `picocom -b 115200 /dev/ttyACM0` or `minicom -D /dev/ttyACM0 -b 115200`.

Linting & Static Analysis
- Static analyzer: `cppcheck` (good for C projects):
  - `cppcheck --enable=all --inconclusive --std=c11 --suppress=missingIncludeSystem src/`
  - Tune suppressions as needed (hardware headers often cause false positives).
- Clang tidy (optional; requires configuration) for more advanced static checks.
- Headers that map to AVR-specific intrinsics may require `-I` paths or `--suppress` rules.

Formatting
- Use `clang-format` to keep a consistent style. Recommended configuration:
  - Indent with 4 spaces
  - Column limit: 80
  - Pointer alignment: left (e.g. `uint8_t *ptr`)
  - Break before binary operators turned off (i.e. keep operators with left operand)
- Example command to format repository C files:
  - `find src -name "*.c" -o -name "*.h" | xargs clang-format -i`
- Save a `.clang-format` file at repo root if you want strict enforcement. If none exists, use a well-known style (Google or LLVM).

Code style guidelines (applies to all C files under src/)
- File layout
  - One top-level module per folder (already used: `src/I2C`, `src/UART`, `src/display`, etc.). Keep implementation `.c` files and headers `.h` together.
  - Each header must use include guards: `#ifndef MODULE_NAME_H` / `#define MODULE_NAME_H` / `#endif`.
  - Public API functions should be declared in headers; keep `static` helper functions in `.c` files.
- Includes
  - Order: system headers (e.g. `<stdint.h>`, `<avr/io.h>`), third-party headers, then local headers using quotes. Example:
    - `#include <stdint.h>`
    - `#include <avr/io.h>`
    - `#include "I2C.h"`
  - Avoid including headers unnecessarily in other headers; prefer forward declarations where possible.
- Naming conventions
  - Files and directories: lowercase with underscores (already used).
  - Functions: `snake_case` for both public and internal functions (e.g. `display_init`, `i2c_read_byte`).
  - Types/structs: `CamelCase` or `snake_case_struct` are acceptable; prefer `snake_case` with `_t` suffix for typedefs (e.g. `display_config_t`). Use `typedef struct { ... } foo_t;` if common across modules.
  - Macros & constants: `ALL_CAPS_SNAKE` for macros and compile-time constants (e.g. `#define DIRTYPAGES_MODE 1`).
  - Global variables: avoid them when possible. If necessary, prefix with module name (e.g. `display_buffer`) and mark `static` when internal. If global and exported, use a `module_name_` prefix.
  - File-scope static variables: start name with `s_` or `module_` to indicate scope (team preference; choose one and keep consistent).
- Types
  - Use fixed-width integer types from `<stdint.h>` (`uint8_t`, `int16_t`, etc.).
  - Keep function prototypes explicit about signedness and widths.
  - Avoid implicit integer promotions; cast explicitly where required.
- Function design
  - Keep functions short and single-purpose. Avoid deep nesting; prefer early return for error conditions.
  - Return error codes as `int` or an `enum` with `ERR_OK` / `ERR_*` values; document return semantics in the header.
  - For APIs that alter hardware state, prefer explicit return codes rather than `void` where failure is possible.
- Error handling & logging
  - Check return values from hardware and bus routines (I2C, UART). Do not ignore potential failure (unless a documented and explicit reason).
  - Standard pattern: callers check return value and propagate or log: `ret = i2c_write(...); if (ret != ERR_OK) return ret;`
  - For unrecoverable errors during initialization, prefer reporting via UART and halting (`while(1);`) rather than undefined behavior.
  - Provide small logging macros that can be compiled out for release builds, e.g.:
    - `#ifdef DEBUG
         #define LOG(fmt, ...) UART_printf(fmt, ##__VA_ARGS__)
       #else
         #define LOG(fmt, ...)
       #endif`
- Memory and performance
  - The MCU is constrained (ATmega328P): avoid dynamic allocation (`malloc`) and large stack frames.
  - Use `const` for immutable data placed in flash when possible (`const PROGMEM` for AVR if large lookup tables are needed).
  - Keep buffers small and clearly document maximum sizes.
- Concurrency / ISRs
  - Mark variables shared between ISRs and main code as `volatile`.
  - Keep ISRs short; do minimal work, set flags, and return.
  - Use `cli()`/`sei()` sparingly; prefer atomic operations or disable interrupts only for the shortest critical section.
- Header API discipline
  - Headers should expose only the public API and necessary type definitions.
  - Avoid leaking internal helper macros and static inline implementation details into headers unless intended for inlining.
- Comments & documentation
  - Use short function header comments for public functions: purpose, parameters, return value, side effects.
  - Prefer TODO/TODO-NOTES only for short-lived work; avoid leaving large unfinished sections.
- Formatting specifics
  - Indentation: 4 spaces (no tabs)
  - Maximum line length: 80 characters preferred; 100 characters acceptable for rare cases.
  - Brace style: K&R or Allman is acceptable, pick one; prefer K&R for compactness:
    - if (cond) {
        do_something();
      } else {
        handle_other();
      }

Testing guidance (there are no tests currently)
- Current state: no testing framework or `test/` directory present.
- Recommended approach:
  - Add host-based unit tests using Unity (throw-away hardware dependencies) and Ceedling as the test runner.
  - Keep test code in `test/unit/` and mock hardware-specific drivers (UART, I2C) with small fakes.
- Example Ceedling commands (after installing Ceedling):
  - Run all tests: `ceedling test:all`
  - Run a single test file: `ceedling test:test_filename.c`
  - Run a single test case: use arguments to Unity runner or isolate it in the test file (see Ceedling docs).
- If you prefer CMake + Unity: add a `tests/CMakeLists.txt` and provide `make test` or `ctest -R <testname>` support.

Cursor/Copilot rules
- No Cursor rules found in `.cursor/rules/` or `.cursorrules` in this repository.
- No GitHub Copilot instruction file found at `.github/copilot-instructions.md`.
- If you add either, list them here and ensure agents read them before editing files.

Commit and edit safety for agents
- Respect existing project structure. Do not rename top-level files without explicit instruction.
- Avoid changing Makefile or hardware settings unless requested; if you must modify build scripts, explain the rationale clearly in your change.
- Do not add large new dependencies to the build toolchain (e.g. full Python stacks) unless requested.

When you touch files
- Always run `make .build/program.elf` locally (or the relevant compile command) to ensure the change compiles for the AVR target.
- Run `cppcheck` and `clang-format` on changed files. If formatting modifies files, include those changes in the same edit pass.
- If you add tests, provide instructions to run a single test file and show sample output.

Examples and snippets
- Compile only (explicit):
  - `make .build/program.elf`
- Flash:
  - `make flash`
- Show size:
  - `make size`
- Static analysis example:
  - `cppcheck --enable=all --inconclusive --std=c11 --suppress=missingIncludeSystem src/`

Preferences & conventions summary (for quick agent reference)
- Indent: 4 spaces
- Line length: 80
- Naming: snake_case for functions, ALL_CAPS for macros, `_t` for typedefs
- Types: use `<stdint.h>` fixed-width types
- Error handling: explicit return codes, check and propagate
- Avoid dynamic memory and large stack frames
- Keep ISRs minimal and mark shared state `volatile`

If you are unsure
- Ask a human before rewriting large parts of the build system, changing MCU flags, or adding new external dependencies.
- If a change affects hardware behavior (I2C/UART/display), include testing steps and expected serial output to help manual verification.

Last updated: 2026-01-15
