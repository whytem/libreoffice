# Spreadsheet Engine Extraction: Status and Overview

This document provides a comprehensive summary of the `spreadsheet_engine/`
module for newcomers seeking to understand the project's current progress,
capabilities, limitations, and future direction.

## What Is This Project?

`spreadsheet_engine/` is an incremental extraction of LibreOffice Calc's core
spreadsheet computation engine into a standalone, independently buildable C++
module. The goal is to separate the pure spreadsheet logic (formula compilation,
evaluation, matrix operations, lookup/query, reference tracking, and
recalculation) from the broader LibreOffice application stack (UI, UNO, import/
export filters, rendering, drawing layer).

The module is **not** a rewrite. It is a careful, phased migration of existing
Calc engine code into a self-contained library that:

- builds standalone from `spreadsheet_engine/` using CMake (C++20, ICU, libxml2)
- also builds inside LibreOffice and links into the `sc` (Calc) module
- uses its own engine-owned API types instead of leaking LibreOffice internals
- provides a lightweight in-memory host for standalone use without a full Calc
  document model

## Current Extraction Status

**The extraction is substantially complete.** Phases 6 through 11 of the
extraction plan have all been finished. The project is now in routine
maintenance mode rather than an active extraction sequence.

### Current State in Practice

The extracted engine now owns a large share of Calc's pure spreadsheet
calculation logic, and LibreOffice Calc has been retargeted to use that
engine-owned code through the `compat/libreoffice/` adapter layer.

Calc still owns the full document-backed runtime:

- the `ScDocument` / `ScFormulaCell` in-memory spreadsheet representation
- the main formula compiler/parser used for ordinary Calc documents
- dependency-graph ownership and listener/broadcaster wiring
- recalculation orchestration and backend execution

The standalone project now also has its own narrow workbook runtime for raw
FODS replay:

- a sparse in-memory workbook model
- a read-only FODS loader
- a small ODF formula parser
- a lazy evaluator with memoization and cycle detection

That standalone runtime is intentionally narrower than Calc. It can evaluate
supported formulas live across cell, range, and named-range references, but it
does not yet implement Calc's full persistent dependency graph or full
document-level recalculation engine. For unsupported formula paths, the FODS
replay flow can still fall back to cached workbook results where needed.

### Completed Extraction Phases

| Phase | Name | Status |
|-------|------|--------|
| 0 | Module skeleton and build integration | Complete |
| 1 | Copied FormulaGrammar helpers | Complete |
| 2 | Pure Calc-config helpers | Complete |
| 3 | MatrixOperators extraction | Complete |
| 4 | Compiler char-table construction | Complete |
| 5 | Low-coupling math and text families | Complete |
| 6 | Portable helper completion and parity | Complete |
| 7 | Formula, compiler, and config primitives | Complete |
| 8 | Matrix and execution substrate ownership | Complete |
| 9 | Host runtime and coupled interpreter families | Complete |
| 10 | References, dependencies, and recalculation | Complete |
| 11 | Calc cleanup and standalone package hardening | Complete |

### Active Work: FODS Workbook Support

Beyond the extraction track, the project is actively building standalone FODS
(Flat OpenDocument Spreadsheet) support. This lets the engine load and evaluate
real Calc function-test workbooks without any LibreOffice runtime. Current
status:

- **Logical family:** fully enabled
- **Mathematical family:** fully enabled (including AGGREGATE, SUBTOTAL)
- **Text family:** fully enabled (CLEAN, UNICHAR, EXACT, array constants)
- **Date/time family:** fully enabled
- **Spreadsheet family:** fully enabled (including live VLOOKUP/HLOOKUP exact
  and sorted lookup behavior)
- **Information family:** fully enabled (including live FORMULA and the small
  MAX/MIN path needed by the corpus)
- **Next frontier:** add-in family

### Active Work: Shared-Compiler Switchover

The next active track is switching standalone replay off the bespoke parser hot
path and onto the shared compiler/token pipeline.

Current state:

- the default standalone replay lane now prefers the shared compiler plus
  compiled-token execution for all six enabled families
- the remaining preflight tail is small:
  - `51` hard blockers
  - `41` expected-error formulas
- the replay harness has:
  - `--compiled-diff` for AST-vs-compiled comparison
  - `--legacy-only` as an explicit stabilization/debug escape hatch
- the maintenance runner now has `--compiler-diff`, which adds a fast logical
  family compiled-diff smoke without changing the default Calc profile
- Calc-side compile-diff smoke now covers formulas sourced from all six enabled
  FODS families

The main remaining convergence gap is exact canonical token-stream parity
between standalone lowering and Calc-imported canonical tokens outside the new
mixed lexical smoke subset. Ordinary-formula XML formula-source preservation is
now aligned, and focused Calc smoke now covers exact lexical parity for
operators, references, range names, a wider representative function subset
(`SUM`, `DATEVALUE`, `FORMULA`, `VLOOKUP`, `IFERROR`, `IFNA`, `FALSE`, `PI`,
`AND`, `ISERROR`, `UPPER`, `LOWER`, `LEN`, `ROUND`, `CEILING`, `FLOOR`,
`GCD`, `LCM`, `DEGREES`, `ATANH`, `DATE`, `TIME`, `DATEDIF`, `MATCH`,
`SUMIF`, `ADDRESS`, `CHAR`, `CODE`, `JIS`, `ASC`, `COLUMNS`, `AREAS`,
`REPLACE`, `REPLACEB`, `RIGHT`, `MID`, `TEXT`, `CONCATENATE`, `DECIMAL`,
`MMULT`, `T`, `N`, `TODAY`, `WEEKNUM`, `WEEKDAY`, `ROUNDDOWN`, `ROUNDUP`,
`OFFSET`, `INDIRECT`, `HYPERLINK`, `LENB`, `FINDB`, `SEARCHB`, `SEARCH`,
`XOR`, `ACOT`, `ISBLANK`, `ISEVEN`, `ISODD`, `LOG`, `DAYS360`, `LEFT`,
`BASE`, `NETWORKDAYS`, `NETWORKDAYS.INTL`, `GETPIVOTDATA`, `EUROCONVERT`,
`MAX`, `MOD`), plus exact lexical bad-name parity for preserved function heads
like `COM.MICROSOFT.CONCAT`, `ORG.OPENOFFICE.CONVERT`, `CONVERT`, `DEC2HEX`,
`MROUND`, `MULTINOMIAL`, and `YEARFRAC`.
On the standalone compiled-token path, workbook lowering also now emits real
`ExternalName` carriers for a broader curated add-in subset (`CONVERT`,
`DEC2HEX`, `MROUND`, `MULTINOMIAL`, `YEARFRAC`, `WORKDAY`, `RANDBETWEEN`,
`SERIESSUM`, `QUOTIENT`, `SQRTPI`) instead of flattening them to the generic
`StringName` call carrier, and compiled-token evaluation preserves safe
cached-result fallback for those unsupported add-in bodies.
That built-in add-in catalog is now shared between the standalone workbook
compile host and Calc’s `DocumentCompileHost`, including alias normalization
for `ORG.OPENOFFICE.CONVERT`, so both hosts resolve the same canonical
`ExternalName` payloads and built-in catalog ID for that subset.
Lexical jump tokens imported from Calc are also normalized now so parity checks
ignore the undefined trailing payload bytes that Calc’s lexical `ocIf*` jump
construction leaves uninitialized. The remaining gap is broader function-catalog
and full-stream lexical parity, especially around richer add-in/external-name
catalog coverage, because standalone execution lowering still carries an
RPN-oriented function-call path alongside the Calc-shaped lexical lowering used
for parity checks.

## Architecture Overview

### Directory Layout

```
spreadsheet_engine/
├── CMakeLists.txt              # Standalone CMake build (C++20)
├── inc/spreadsheetengine/      # Public and internal headers
│   ├── api/                    #   Engine-owned public API types
│   ├── compat/                 #   LibreOffice adapter headers
│   │   ├── formula/            #     Copied formula grammar helpers
│   │   └── libreoffice/        #     Calc-specific integration adapters
│   ├── detail/                 #   Internal implementation headers
│   └── runtime/                #   Standalone runtime helpers
├── shims/include/              # SAL/RTL type replacements for standalone
├── source/core/                # Implementation files
├── tests/
│   ├── unit/                   #   23 test files (19 API/compiler + 4 FODS)
│   ├── consumer/               #   Installed-package consumer smoke test
│   ├── data/fods/              #   FODS test fixture workbooks
│   └── parity/                 #   Shared parity TSV datasets
├── integration/libreoffice/    # LibreOffice build integration scripts
├── docs/                       # Architecture and extraction documentation
├── cmake/                      # CMake source manifests and config templates
└── tools/                      # Maintenance validation scripts
```

### Key Architectural Layers

#### 1. Public API (`inc/spreadsheetengine/api/`)

Engine-owned types that form the stable public interface. These replace
LibreOffice types entirely in the standalone build:

- **Cell and range types:** `CellAddress`, `CellRange`, `CellValue`,
  `ResolvedReference`
- **Host interface:** `EvaluationHost` (abstract base combining cell reading,
  reference resolution, text coercion, formatting, workbook info, and runtime
  environment)
- **Formula types:** `FormulaResult`, `Grammar`, `Compiler` config vocabulary
- **Function APIs:** `Math`, `Text`, `Calendar`, `Lookup`, `Query`, `Logic`,
  `Rounding`, `Numeral`, `Workday`
- **Reference/update:** `ReferenceData`, `ReferenceUpdate`, `SharedFormula`
- **Support types:** `Error`, `Config`, `Matrix`, `Array`, `Date`, `String`

#### 2. Runtime Helpers (`inc/spreadsheetengine/runtime/`)

Standalone implementations that work without any LibreOffice dependency:

- `InMemoryHost` — lightweight `EvaluationHost` implementation with in-memory
  cell storage for standalone scenarios
- Math functions: `MathScalar`, `MathTranscendental`, `MathBitwise`,
  `MathFinancial`, `MathRounding`
- Text functions: `TextCase`, `TextScalar`, `TextWidth`, `TextServices`
- Date functions: `DateTimeParts`, `DateTimeWeek`, `DateTimeWorkday`
- Conversion: `NumeralConversion`
- Detection: `LibraryProbe`

#### 3. FODS Support (`detail/FodsLoader`, `detail/FodsEvaluator`)

A self-contained subsystem for loading and evaluating Flat ODS workbooks:

- **WorkbookModel** — internal sparse workbook/sheet/cell structure with named
  ranges and imported-sheet metadata
- **FodsLoader** — XML parser (libxml2) that reads FODS files into the workbook
  model, handling repeated rows/cells, cached values, named ranges, and
  `table:table-source` imports
- **OdfFormulaParser** — tokenizer and AST for ODF `of:=` formula syntax
- **FodsEvaluator** — lazy formula evaluator with memoization and cycle
  detection, supporting scalar arithmetic, comparisons, range references,
  named ranges, and a growing set of spreadsheet functions, while still using
  cached workbook values as a fallback for unsupported paths

#### 4. LibreOffice Compatibility (`compat/libreoffice/`)

Thin adapter headers that bridge between engine-owned types and LibreOffice/Calc
internals. These are only used when building inside LibreOffice:

- `Host.hxx` — `DocumentEvaluationHost` wrapping `ScDocument`
- `Address.hxx` — `ScAddress`/`ScRange` conversion
- `Error.hxx` — `FormulaError` to `api::Error` mapping
- `String.hxx` — `OUString` to `api::String` conversion
- `Grammar.hxx`, `Config.hxx`, `Rounding.hxx`, `Date.hxx`, etc.
- `LookupCache.hxx`, `SharedFormula.hxx`, `ReferenceUpdate.hxx`
- `FormulaResult.hxx`, `Parsing.hxx`, `TextServices.hxx`

#### 5. Shims (`shims/include/`)

Minimal standalone replacements for LibreOffice SAL/RTL types:

- `sal/types.h` — `sal_Int32`, `sal_Unicode`, `sal_uInt8`, etc.
- `rtl/math.hxx` — floating-point math and string parsing
- `sal/log.hxx` — logging stub
- `basegfx/numeric/ftools.hxx` — graphics geometry stub
- `kahan.hxx` — Kahan summation algorithm

## Extracted Functionality

### Fully Standalone (No LibreOffice Required)

**Mathematical functions (48+):**
ABS, INT, SQRT, POWER, EXP, LN, LOG, LOG10, MOD, SIN, COS, TAN, ASIN, ACOS,
ATAN, ATAN2, SINH, COSH, TANH, CSCH, SECH, COTH, ROUND, ROUNDDOWN, ROUNDUP,
CEILING, FLOOR, TRUNC, BITAND, BITOR, BITXOR, BITLSHIFT, BITRSHIFT, PV, FV,
PMT, NPER, RATE, YIELD, PRICE, DURATION, ACCRINT, XIRR, XNPV, IRR, MIRR,
PPMT, IPMT, EFFECT, NOMINAL

**Text functions (20+):**
UPPER, LOWER, PROPER, CLEAN, LEN, CONCATENATE, EXACT, UNICHAR, CODE, VALUE,
NUMBERVALUE

**Date/time functions (25+):**
YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, DATE, TIME, DATEVALUE, TIMEVALUE,
DATEDIF, DAYS, DAYS360, WEEKDAY, WEEKNUM, ISOWEEKNUM, WORKDAY, NETWORKDAYS,
NETWORKDAYS.INTL, EDATE, EOMONTH, EASTERSUNDAY, DAYSINMONTH, DAYSINYEAR,
ISLEAPYEAR

**Logic functions:** IF, IFERROR, IFNA, CHOOSE, IFS

**Lookup/reference planning:**
MATCH/XMATCH mode normalization, VLOOKUP/HLOOKUP/XLOOKUP/LOOKUP result
planning, INDEX area/matrix/reference planning, OFFSET reference-window
planning, INDIRECT/ADDRESS policy resolution

**Array/reshape planning:**
TAKE, DROP, CHOOSECOLS, CHOOSEROWS, EXPAND, TOCOL, TOROW, WRAPCOLS, WRAPROWS,
HSTACK/VSTACK dimension accumulation

**Query helpers:** SUMIF, COUNTIF, AVERAGEIF operator and criteria policy

**Numeral conversion:** BASE, DECIMAL, ROMAN

**Compiler and config:** formula grammar helpers, compiler char tables, config
opcode vocabulary, OpenCL subset config, force-calculation parsing

**Matrix and execution substrate:** matrix dimensioning, replication, allocation,
jump-matrix cursor/buffering, execution-context scratch and token cache

**Reference and dependency:** formula-result carriers, reference-data types,
lookup-cache semantics, query-policy evaluation, shared-formula planning,
formula-cell state planning, reference-update kernels, recalculation planning

**FODS workbook support:** XML loading, ODF formula parsing, lazy evaluation
with memoization and cycle detection, named ranges, external sheet imports

### Host-Dependent (Requires EvaluationHost Implementation)

These features work in standalone mode through `InMemoryHost` but need a host
implementation for full behavior:

- Number parsing with locale awareness
- Number formatting
- Full text collation and transliteration
- Wildcard/regexp search execution in queries

### Remaining in LibreOffice Calc

The following are explicitly out of scope for the engine and remain in Calc:

- UI, shell, dialogs, view state
- Import/export filters (except read-only FODS)
- UNO wrappers and service registration
- Drawing layer, charts, annotations
- Pivot tables, autofilters, conditional formatting
- Document persistence and undo/redo
- Broadcaster/listener registration and document graph ownership
- Thread-pool scheduling and OpenCL backend execution
- Macro support

## Build System

### Standalone Build (Primary)

```bash
cmake -S spreadsheet_engine -B /tmp/se-build
cmake --build /tmp/se-build
ctest --test-dir /tmp/se-build --output-on-failure
```

**Requirements:** C++20 compiler, ICU (Unicode support), libxml2 (FODS parsing)

**Output:** `spreadsheetengine::core` static library with 49 public API headers
and 11 runtime headers.

### Installation

```bash
cmake --install /tmp/se-build --prefix /tmp/se-install
```

The installed package provides a proper CMake package config so downstream
projects can use:

```cmake
find_package(spreadsheetengine REQUIRED)
target_link_libraries(myapp PRIVATE spreadsheetengine::core)
```

### LibreOffice Build (Integration)

The module also builds within LibreOffice's mk-based build system via
`Library_spreadsheetengine.mk` and `Module_spreadsheet_engine.mk`. Calc links
against it and consumes engine logic through the `compat/libreoffice/` adapter
layer.

### Validation

```bash
# Combined standalone + Calc gate (default smoke)
./spreadsheet_engine/tools/run_maintenance_validation.sh

# Broader stable Calc subset
./spreadsheet_engine/tools/run_maintenance_validation.sh --milestone

# Full non-rendering Calc engine suite
./spreadsheet_engine/tools/run_maintenance_validation.sh --engine

# Standalone compiled-vs-legacy diff smoke
./spreadsheet_engine/tools/run_maintenance_validation.sh --standalone-only --compiler-diff
```

## Test Suite

The standalone test suite currently exposes 24 CTest entries: 23 unit/smoke
executables plus 1 installed-package consumer smoke check. The executable-based
suite covers all major subsystems:

| Test Target | Coverage |
|-------------|----------|
| `spreadsheetengine_smoke` | End-to-end smoke (host, parsing, IF, INDEX, lookup, formatting) |
| `spreadsheetengine_math_tests` | Bitwise, financial, rounding, scalar, transcendental math |
| `spreadsheetengine_calendar_tests` | Date parts, week logic, workday/networkdays |
| `spreadsheetengine_text_tests` | Case, scalar text, width |
| `spreadsheetengine_compiler_tests` | Compiler char tables, grammar helpers |
| `spreadsheetengine_token_compiler_host_tests` | Canonical token schema, compile-host contracts, workbook compile host |
| `spreadsheetengine_config_tests` | Config opcode vocabulary, OpenCL subset |
| `spreadsheetengine_execution_tests` | Execution context scratch and cache |
| `spreadsheetengine_matrix_tests` | Matrix geometry, allocation, dimension |
| `spreadsheetengine_host_tests` | EvaluationHost interface, InMemoryHost |
| `spreadsheetengine_logic_tests` | IF, IFERROR, CHOOSE decision policies |
| `spreadsheetengine_parsing_tests` | VALUE, DATEVALUE, TIMEVALUE parsing |
| `spreadsheetengine_lookup_tests` | MATCH, VLOOKUP, XLOOKUP planning |
| `spreadsheetengine_array_tests` | TAKE, DROP, EXPAND, reshape planning |
| `spreadsheetengine_reference_tests` | OFFSET, INDEX reference planning |
| `spreadsheetengine_carrier_tests` | FormulaResult, reference data carriers |
| `spreadsheetengine_query_tests` | Query policy, criteria classification |
| `spreadsheetengine_sharedformula_tests` | Shared-formula grouping and planning |
| `spreadsheetengine_formulacell_tests` | Formula cell state transitions |
| `spreadsheetengine_fods_tests` | FODS XML loading |
| `spreadsheetengine_fods_parser_tests` | ODF formula AST parsing |
| `spreadsheetengine_fods_evaluator_tests` | FODS formula evaluation |
| `spreadsheetengine_fods_replay_tests` | Raw FODS workbook replay harness |
| `spreadsheetengine_installed_package_smoke` | Installed-package downstream-consumer smoke |

**Shared parity datasets** (TSV files under `tests/parity/`) allow the same
test cases to run in both the standalone suite and LibreOffice's
`CppunitTest_sc_ucalc_shared_cases` target, ensuring behavioral equivalence.

## Points of Coupling with LibreOffice

### Intentional Coupling (Adapter Layer)

The `compat/libreoffice/` headers provide the bridge between engine types and
Calc internals. This coupling is by design — it is how Calc consumes the
engine:

- **ScDocument access:** `DocumentEvaluationHost` wraps `ScDocument` for cell
  reading and reference resolution
- **Type conversion:** `ScAddress` ↔ `CellAddress`, `OUString` ↔ `api::String`,
  `FormulaError` ↔ `api::Error`
- **Config bridge:** engine-owned config opcode symbols mapped to Calc's
  `OpCode` enum at the boundary
- **Grammar bridge:** engine-owned `api::Grammar` mapped to
  `formula::FormulaGrammar` at the boundary

### Residual Coupling

- **FormulaCompiler fallback:** a small compatibility path in
  `calcconfig.cxx` still falls back to `FormulaCompiler` for non-core
  residual opcode names outside the engine-owned config lookup table
- **Copied formula helpers:** `compat/formula/FormulaGrammar` remains
  intentionally duplicated from the `formula/` module
- **SAL type shims:** the standalone build shims `sal_Int32`, `sal_Unicode`,
  etc. rather than removing them from API signatures entirely

### What Would Full Decoupling Require?

To make the engine completely independent of LibreOffice types:

1. Replace remaining `sal_*` types in API signatures with standard C++ types
2. Remove the `compat/` layer entirely (breaking the Calc integration)
3. Potentially replace the formula grammar helpers with a fully independent
   implementation

This is not currently a goal — the dual-mode build is the intended architecture.

## Limitations

- **Read-only FODS only:** the FODS subsystem loads workbooks for evaluation but
  cannot write or modify them
- **No full ODS support:** only the flat (single-file) FODS variant is
  supported, not the zipped ODS package format
- **No rendering:** no layout, styles, page model, or print support
- **No charts or drawing objects:** entirely out of scope
- **No macros:** no BASIC, Python, or UNO macro execution
- **Evaluation host required:** standalone formula evaluation needs an
  `EvaluationHost` implementation; the built-in `InMemoryHost` covers
  basic scenarios but not full locale-aware formatting or collation
- **FODS workbook execution is still partial overall:** the replay subsystem is
  live and green for the currently enabled families, but it is not yet a full
  Calc-equivalent workbook runtime
- **No full dependency graph in standalone FODS mode:** workbook evaluation is
  lazy and memoized with cycle detection, but there is no persistent
  Calc-style dependency graph or full document recalc orchestration
- **Family coverage is still expanding:** raw FODS replay is currently enabled
  for logical, mathematical, text, date_time, spreadsheet, and information;
  other families such as add-in remain future work

## Roadmap / Future Directions

The extraction track (Phases 6-11) is complete. The project is now in
maintenance mode. Practical next steps include:

1. **Continue FODS family expansion** — extend raw replay beyond the currently
   enabled logical, mathematical, text, date_time, spreadsheet, and
   information families
2. **Deepen standalone workbook execution** — gradually replace cached-result
   fallback paths with more live evaluator coverage
3. **Package polish** — improve downstream consumption ergonomics, add more
   consumer examples
4. **Broaden parity coverage** — expand shared TSV parity datasets to cover
   more function families
5. **Optional runtime enrichment** — extend `InMemoryHost` with
   richer locale support if standalone use cases demand it

## Key Documentation

- [README.md](../README.md) — build, test, and install instructions
- [CALC_ENGINE_EXTRACTION_PLAN.md](extraction-history/CALC_ENGINE_EXTRACTION_PLAN.md) —
  the master extraction roadmap with detailed per-phase status
- [BASIC_FODS_SUPPORT.md](architecture/BASIC_FODS_SUPPORT.md) — FODS loader/
  evaluator architecture and implementation checklist
- [CALC_ENGINE_AUDIT.md](extraction-history/CALC_ENGINE_AUDIT.md) — audit of
  original Calc engine source files
- [STANDALONE_PACKAGE_PLAN.md](extraction-history/STANDALONE_PACKAGE_PLAN.md) —
  standalone packaging plan (now merged into the main extraction plan)
