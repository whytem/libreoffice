# Standalone Spreadsheet Engine Package Plan

This plan focuses on a second, parallel goal next to the in-tree LibreOffice
extraction work:

- keep extracting Calc engine logic into `spread_engine_extract/`
- also make `spread_engine_extract/` independently buildable as a genuinely
  standalone spreadsheet-engine package
- test extracted components in both environments as the migration continues

The intent is not to fork the implementation. The same engine sources should be
usable in two modes:

- LibreOffice mode:
  - built by gbuild
  - linked into `sc`
  - validated by existing Calc unit tests
- Standalone mode:
  - built from `spread_engine_extract/` alone
  - no include or link dependency on `sc/`, `formula/`, `sal`, `utl`,
    `comphelper`, or other LibreOffice libraries
  - validated by a standalone test runner and shared parity datasets

## What "Genuinely Standalone" Means

For this plan, "standalone" means:

- a checkout of `spread_engine_extract/` is enough to build the package
- the package has its own build system, tests, and public API
- it does not include headers from `sc/` or `formula/`
- it does not link against LibreOffice runtime libraries
- any third-party dependencies are explicitly declared package dependencies
  rather than implicit LibreOffice transitive dependencies

That is a stricter target than the current `spreadsheetengine` library, which
is extracted but still embedded in the LibreOffice build graph.

## Current Reality

Today we have useful extracted logic, but not a standalone package yet.

## Status

- Phase A: initial scaffold completed
  - a root standalone `CMakeLists.txt` now exists in `spread_engine_extract/`
  - source-group manifests now exist in
    `spread_engine_extract/cmake/SpreadsheetEngineSourceGroups.cmake`
  - a first standalone compatibility shim layer now exists under
    `spread_engine_extract/standalone/include/`
  - a smoke runner now exists at
    `spread_engine_extract/tests/standalone/smoke_main.cxx`
  - the current standalone-ready smoke slice is:
    - `Phase0`
    - `MathBitwise`
    - `MathScalar`
    - `MathTranscendental`
  - Phase A has now been validated with the standalone build entry point:
    - `cmake -S spread_engine_extract -B /tmp/spreadsheetengine-standalone-build`
    - `cmake --build /tmp/spreadsheetengine-standalone-build`
    - `ctest --test-dir /tmp/spreadsheetengine-standalone-build --output-on-failure`
  - the LibreOffice-integrated `make Library_spreadsheetengine` path still
    builds unchanged
- Phase B: completed for the low-coupling math families
  - engine-owned standalone base types now exist under
    `spread_engine_extract/inc/spreadsheetengine/api/`
  - the initial base-type set now includes:
    - `Error`
    - `String`
    - `Date`
    - `Grammar`
    - `Rounding`
  - a first standalone-facing math API now exists in
    `spread_engine_extract/inc/spreadsheetengine/api/Math.hxx`
  - that API now wraps the low-coupling helper families:
    - bitwise
    - scalar
    - transcendental
    - rounding
    - financial
  - initial LibreOffice compatibility adapters now exist under
    `spread_engine_extract/inc/spreadsheetengine/compat/libreoffice/`
  - the standalone build now includes both:
    - `spreadsheetengine_smoke`
    - `spreadsheetengine_tests`
  - the standalone test executables now exercise the new API layer instead of
    including `spreadsheetengine/core/*` headers directly
  - validation completed with:
    - `cmake -S spread_engine_extract -B /tmp/spreadsheetengine-standalone-build`
    - `cmake --build /tmp/spreadsheetengine-standalone-build`
    - `ctest --test-dir /tmp/spreadsheetengine-standalone-build --output-on-failure`
    - `make Library_spreadsheetengine`
    - `./spread_engine_extract/run_spreadsheet_unit_tests.sh CppunitTest_sc_financial_functions_test CppunitTest_sc_mathematical_functions_test CppunitTest_sc_ucalc`

### What is already extracted

- numeric helper families:
  - `MathRounding`
  - `MathScalar`
  - `MathTranscendental`
  - `MathBitwise`
  - `MathFinancial`
- conversion and text/date helpers:
  - `NumeralConversion`
  - `TextScalar`
  - `TextCase`
  - `TextWidth`
  - `DateTimeParts`
  - `DateTimeWeek`
  - `DateTimeWorkday`
- some copied/shared infrastructure:
  - `compat/formula/FormulaGrammar`
  - `CompilerSupport`
  - `CalcConfig`
  - `MatrixOperators`

### What still prevents standalone packaging

- the build still includes `-I$(SRCDIR)/sc/inc`
- several headers still expose Calc or `formula` types
- several implementations still depend on LibreOffice runtime services
- there is no standalone public API layer
- there is no standalone test harness
- there is no shared parity suite that runs in both build modes

## Guiding Principles

These principles keep the standalone work from drifting away from the main
extraction work.

- Do not create duplicate implementations for LibreOffice mode and standalone
  mode.
- Keep a single core implementation and add thin environment adapters around
  it.
- Prefer introducing engine-owned types over leaking LibreOffice types into the
  public standalone surface.
- Every new extracted slice should answer two questions:
  - how does Calc call it?
  - how can the standalone runner call it?
- Standalone coverage should grow in the same order as the extraction:
  low-coupling helpers first, evaluator/runtime later.

## Target Package Shape

The end state should look roughly like this:

- `spread_engine_extract/`
  - `CMakeLists.txt` or `meson.build`
  - `include/spreadsheetengine/api/...`
  - `include/spreadsheetengine/internal/...`
  - `source/core/...`
  - `source/compat/libreoffice/...`
  - `source/compat/thirdparty/...` only if truly needed
  - `tests/shared_cases/...`
  - `tests/standalone/...`
  - `tools/` for parity runners or generators
- LibreOffice integration:
  - `Library_spreadsheetengine.mk`
  - adapters that convert LibreOffice types and services into engine-owned
    interfaces

The standalone build should compile the engine core plus standalone adapters.
The LibreOffice build should compile the same engine core plus LibreOffice
adapters.

## Dependency Strategy

The most important design move is to separate core logic from environment
dependencies.

### Core layer

The core layer should depend only on:

- C++ standard library
- package-owned engine headers and types
- explicitly chosen third-party libraries that we are willing to ship as part
  of the standalone package surface

### Adapter layer

The adapter layer should isolate environment-specific services such as:

- LibreOffice string/date/locale/runtime wrappers
- ICU or alternate text-conversion services
- current Calc/compiler compatibility shims
- future workbook/document host implementations

### Initial acceptable external dependencies

A practical first standalone build can allow a small dependency set such as:

- ICU for Unicode and transliteration behavior
- a test framework such as Catch2 or doctest

It should not require the LibreOffice runtime stack.

## Component Readiness Matrix

This is the current rough classification of extracted code.

| Component | Standalone readiness | Main blockers | First step |
| --- | --- | --- | --- |
| `MathBitwise`, `MathScalar`, `MathTranscendental`, most of `MathFinancial` | High | `sal`/`rtl` types in some signatures | replace leaked LO types with engine-owned or `std` types |
| `MathRounding` | Medium | `rtl_math_RoundingMode` in public API | define engine-owned rounding enum and adapter |
| `NumeralConversion` | Medium | `OUString`, `rtl::math` in public surface | introduce engine string facade and parser/formatter helpers |
| `DateTimeParts`, `DateTimeWeek`, `DateTimeWorkday` | Medium | `Date`, `tools::Time`, weekday constants | add engine-owned date serial/calendar utilities |
| `TextScalar` | Medium | `OUString`, encoding helpers, ICU calls | move Unicode/string utilities behind engine facade |
| `TextCase` | Low-medium | `CharClass` in public API | define locale/case service interface |
| `TextWidth` | Low | UNO/process factory/transliteration wrapper | move entirely behind service interface; provide standalone ICU implementation |
| `FormulaGrammar` | Low-medium | still uses `formula::FormulaGrammar` enums | copy the enum/bit layout into engine-owned types |
| `CompilerSupport` | Low | depends on `compiler.hxx` and `ScCharFlags` | create engine-owned character classification enums |
| `CalcConfig` | Low | depends on `formula` opcodes and LO enums | define engine-owned opcode/config vocabulary |
| `MatrixOperators` | Low | depends on `sc/inc/matrixoperators.hxx` and Calc-owned types | move matrix op types fully under engine ownership |

## Dual-Build Strategy

We should treat the standalone package as a second build target for the same
engine source tree.

### Source ownership

Create an explicit source-manifest convention:

- engine core sources
- LibreOffice adapter sources
- standalone adapter sources

The same core source list should feed both gbuild and the standalone build to
avoid silent drift.

### Build systems

Keep both build entry points:

- LibreOffice:
  - existing `Library_spreadsheetengine.mk`
- Standalone:
  - new `CMakeLists.txt` in `spread_engine_extract/`

The standalone build should initially produce:

- `libspreadsheetengine_core`
- `spreadsheetengine_smoke`
- `spreadsheetengine_tests`

## Testing Strategy

We need three complementary test lanes.

### Lane 1: LibreOffice integration

Continue using the existing Calc test gate:

```bash
./spread_engine_extract/run_spreadsheet_unit_tests.sh
```

This remains the safety net for extraction regressions.

### Lane 2: Standalone unit tests

Add direct tests for extracted helpers in standalone mode:

- math helpers
- date/time helpers
- numeral conversion
- text helpers that no longer require LibreOffice services

These tests should target engine APIs directly and run without LibreOffice.

### Lane 3: Shared parity tests

Add shared datasets that can be executed in both environments:

- same inputs
- same expected outputs
- same edge cases

These should live under `spread_engine_extract/tests/shared_cases/` and be
consumed by:

- a standalone test runner
- a LibreOffice-side adapter test runner or CppUnit bridge

The parity suite is what lets us validate extracted behavior in parallel while
the migration is still incomplete.

## Required Architectural Changes

### 1. Introduce an engine-owned public API layer

Today many extracted headers still expose LibreOffice types directly. That is
the first major issue to solve.

Add a new standalone-facing layer such as:

- `include/spreadsheetengine/api/String.hxx`
- `include/spreadsheetengine/api/Error.hxx`
- `include/spreadsheetengine/api/Date.hxx`
- `include/spreadsheetengine/api/Grammar.hxx`
- `include/spreadsheetengine/api/Rounding.hxx`

The goal is for the standalone public API to stop exposing:

- `OUString`
- `Date`
- `rtl_math_RoundingMode`
- `formula::FormulaGrammar::*`
- `OpCode`
- `ScCharFlags`

LibreOffice adapters can convert to and from those engine-owned types.

### 2. Split core logic from LibreOffice services

Move environment-specific behavior behind interfaces.

Initial candidates:

- `CaseMappingService`
  - used by `TextCase`
- `WidthConversionService`
  - used by `TextWidth`
- `ClockService`
  - used later when we extract volatile date/time functions
- `LocaleNumberParsingService`
  - used later for `DATEVALUE`, `TIMEVALUE`, and locale-aware text/value logic

In LibreOffice mode these interfaces are backed by LO services.
In standalone mode they are backed by ICU or package-local implementations.

### 3. Copy and own remaining `formula` primitives

For standalone packaging, `spread_engine_extract` must not expose or require
`formula/` headers.

That means we eventually need engine-owned versions of:

- grammar enums and bit layout
- opcodes required by extracted config/compiler helpers
- copied token/compiler helpers that Calc still needs

Calc can keep adapter shims during the transition, but the standalone package
must stop depending on the original `formula` module.

### 4. Copy and own remaining Calc primitive types

Several currently extracted helpers still lean on Calc-owned vocabulary.

Priority replacements:

- engine-owned char classification enums for compiler work
- engine-owned matrix op/value accumulator types
- engine-owned error/status enums where helper functions still assume Calc
  routing

### 5. Introduce a minimal standalone host runtime

The current extracted helpers are mostly leaf algorithms. To go beyond that we
need a small host runtime for standalone evaluation.

This runtime should be deliberately minimal:

- workbook abstraction
- sheet abstraction
- cell value abstraction
- reference resolution hooks
- lookup/query hooks

This allows future interpreter slices to run against:

- Calc's document model in LibreOffice mode
- a small in-memory host in standalone mode

## Phased Roadmap

### Phase A: Standalone skeleton and manifest

Objective:

- create the package structure and source manifests without changing behavior

Work:

- add standalone build files under `spread_engine_extract/`
- add source-group manifests for core and adapters
- add a tiny smoke binary that links `Phase0` plus one already-extracted helper

Validation:

- standalone smoke binary builds and runs
- LibreOffice build still works unchanged

### Phase B: Standalone API base types

Objective:

- stop leaking LibreOffice types through the standalone surface

Work:

- define engine-owned string, error, date, grammar, and rounding types
- add conversion adapters for LibreOffice mode
- update the easiest extracted helpers to publish standalone-safe APIs

Suggested first targets:

- `MathBitwise`
- `MathScalar`
- `MathTranscendental`
- `MathFinancial`

Validation:

- standalone tests for those helper families
- existing Calc tests remain green

### Phase C: Portable helper library milestone

Objective:

- get the low-coupling helper families fully buildable in standalone mode

Work:

- migrate `NumeralConversion`
- migrate `DateTimeParts`, `DateTimeWeek`, `DateTimeWorkday`
- migrate `TextScalar`
- move `TextCase` and `TextWidth` behind service interfaces

Validation:

- standalone helper-suite passes
- shared parity datasets run in both standalone and Calc mode

### Phase D: Own compiler/config primitives

Objective:

- remove direct `formula/` and Calc-header dependencies from early compiler
  helpers

Work:

- engine-own grammar enums and flags
- engine-own opcode/config vocabulary
- replace `CompilerSupport` dependence on `compiler.hxx`
- replace `CalcConfig` dependence on `formula/opcode.hxx`

Validation:

- standalone compiler/config tests pass
- Calc compiler tests still pass:
  - `CppunitTest_sc_ucalc_formula`
  - `CppunitTest_sc_ucalc_formula2`

### Phase E: Matrix/runtime substrate for standalone

Objective:

- make the array/matrix support layer engine-owned

Work:

- pull `MatrixOperators` off Calc headers
- continue with `ScMatrix`/jump-matrix style abstractions behind engine-owned
  types or host interfaces
- define standalone accumulator and matrix value types

Validation:

- standalone matrix tests
- Calc function-family tests and `ucalc` matrix-heavy cases

### Phase F: Minimal evaluator host

Objective:

- stand up a small standalone runtime that can execute nontrivial extracted
  evaluator logic

Work:

- define host interfaces for cell access, references, and lookups
- implement a tiny in-memory standalone workbook host
- begin moving more interpreter families through those interfaces

Validation:

- shared evaluator parity cases pass in both modes
- Calc integration tests remain the primary regression gate

## Per-Iteration Checklist

Every iteration that moves a new slice should do all of the following:

1. Keep or improve the LibreOffice extraction seam.
2. Decide whether the new code belongs in:
   - engine core
   - LibreOffice adapter
   - standalone adapter
3. Add standalone tests for the slice if it is core-capable.
4. Add or expand shared parity datasets when behavior can be exercised in both
   modes.
5. Run the LibreOffice gate plus the standalone gate.

## Recommended Next Steps

The most practical immediate sequence is:

1. Add `STANDALONE_PACKAGE_PLAN.md` and keep it synced with the main extraction
   plan.
2. Add a standalone build skeleton and smoke test target.
3. Introduce engine-owned rounding and error enums.
4. Make `MathBitwise`, `MathScalar`, and `MathTranscendental` compile in
   standalone mode first.
5. Add a shared parity test format and run those cases in both environments.

That gets us an actual second test lane quickly, without waiting for the full
engine to be standalone-ready.
