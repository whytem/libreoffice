# Spreadsheet Engine: Project Status

This document is the single consolidated reference for the `spreadsheet_engine/`
project. It describes what has been built, what works today, what is actively
being worked on, and what the forward roadmap looks like.

It replaces and consolidates the content previously spread across `STATUS.md`,
`NEXT_STEPS.md`, `TOKEN_COMPILER_HOST_MODEL.md`, and `COMPILER_SWITCHOVER.md`.

## What Is This Project?

`spreadsheet_engine/` is an incremental extraction of LibreOffice Calc's core
spreadsheet computation engine into a standalone, independently buildable C++20
module. The goal is to separate pure spreadsheet calculation logic from the
broader LibreOffice application stack so that:

- the engine builds standalone from `spreadsheet_engine/` using CMake, with only
  ICU and libxml2 as external dependencies
- the same engine also builds inside LibreOffice and links into the `sc` (Calc)
  module through a narrow adapter layer
- the engine uses its own API types and does not leak LibreOffice internals
  into its public interface
- the engine can load and evaluate real spreadsheet workbooks (FODS format)
  without any LibreOffice runtime
- Calc and standalone share the same authoritative compiler, token model, and
  function implementations

This is not a rewrite. It is a phased migration of existing Calc engine code
into a self-contained library, validated at every step by differential testing
against Calc.

---

## Current State At A Glance

The project has completed two major programs and is actively working on a third:

| Program | Status | Summary |
|---------|--------|---------|
| Initial extraction (Phases 0-11) | **Complete** | Pure calculation logic, function families, matrix/execution substrate, host runtime, reference/dependency planning, and standalone packaging all extracted |
| Token and compiler host model | **Complete** | Canonical token schema, compile-host interfaces, Calc bridge, shadow compiler, differential validation, first native consumers, and first bridged Calc compile adopters all in place |
| Compiler switchover | **Active** | Standalone FODS replay now defaults to shared compiler path for all six enabled families; convergence hardening and broader lexical parity in progress |

### What the engine owns today

- **Extracted function logic:** math, text, date/time, logical, lookup/query,
  array/reshape, reference, shared-formula, and formula-cell planner layers
  (100+ spreadsheet functions implemented standalone)
- **Host abstractions:** `EvaluationHost` interface for cell reading, reference
  resolution, text coercion, formatting, workbook info, and runtime environment
- **Canonical token model:** engine-owned token schema covering all 18
  compiler-emitted token kinds, with native hashing, equality, shared-formula
  comparison, and diagnostic stringification
- **Compiler host model:** engine-owned compile-host interfaces with resolver
  contracts for names, database ranges, table refs, col/row names, external
  names, and grammar/locale settings
- **Compiler pipeline:** compile request/status contract, Calc-backed shadow
  compiler, standalone workbook-backed compile host, compile-diff harness
- **Standalone FODS runtime:** sparse workbook model, read-only FODS loader,
  ODF formula parser, lazy evaluator with memoization and cycle detection
- **Raw FODS replay:** six function families fully enabled (logical,
  mathematical, text, date_time, spreadsheet, information) across 228
  workbooks
- **Parity infrastructure:** shared TSV datasets, dual-mode validation scripts,
  compile-diff harness, compiler preflight classifier

### What Calc still owns

- `ScDocument` / `ScTable` / `ScColumn` in-memory storage
- The main production formula compiler path via `ScCompiler` (engine compiler
  is adopted in selected flows with legacy fallback)
- `ScTokenArray` as the pervasive token container (engine canonical model is
  used by first native consumers with bridge adapters)
- Listener/broadcaster wiring and dependency graph ownership
- Formula trees, dirty tracking, and recalculation orchestration
- `ScInterpreter` CPU evaluator
- Threaded and OpenCL backend execution
- UI, shell, persistence, import/export, UNO, rendering

---

## Architecture

### Directory Layout

```
spreadsheet_engine/
├── CMakeLists.txt              # Standalone CMake build (C++20)
├── inc/spreadsheetengine/      # Public and internal headers
│   ├── api/                    #   Engine-owned public API types
│   ├── compat/                 #   LibreOffice adapter headers
│   │   ├── formula/            #     Copied formula grammar helpers
│   │   └── libreoffice/        #     Calc-specific integration adapters (19 headers)
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
├── docs/                       # Documentation
├── cmake/                      # CMake source manifests and config templates
└── tools/                      # Maintenance validation scripts
```

### Architectural Layers

**Layer 1: Public API** (`api/`)
Engine-owned types forming the stable public interface: `CellAddress`,
`CellRange`, `CellValue`, `EvaluationHost`, `FormulaResult`, `Grammar`,
`Compiler`, `Math`, `Text`, `Calendar`, `Lookup`, `Query`, `Logic`,
`Rounding`, `Numeral`, `Workday`, `ReferenceData`, `ReferenceUpdate`,
`SharedFormula`, `Error`, `Config`, `Matrix`, `Array`, `Date`, `String`.

**Layer 2: Token and Compiler Model** (`detail/`)
Engine-owned canonical token schema (`TokenModel`), compile-host interfaces
(`CompileHost`), compiler pipeline contract (`CompilerPipeline`), workbook
compile host (`WorkbookCompileHost`), compiler preflight classifier
(`FodsCompilerPreflight`), shared-formula token services
(`SharedFormulaToken`), token stringifier (`TokenStringifier`), and workbook
compiler lowering (`WorkbookCompilerLowering`).

**Layer 3: FODS Runtime** (`detail/`)
Standalone workbook subsystem: `WorkbookModel` (sparse cell storage with named
ranges and imported-sheet metadata), `FodsLoader` (libxml2-backed XML parser),
`OdfFormulaParser` (ODF `of:=` formula tokenizer and AST), `FodsEvaluator`
(lazy formula evaluator with memoization, cycle detection, and compiled-token
execution support).

**Layer 4: Runtime Helpers** (`runtime/`)
Standalone function implementations: `MathScalar`, `MathTranscendental`,
`MathBitwise`, `MathFinancial`, `MathRounding`, `TextCase`, `TextScalar`,
`TextWidth`, `TextServices`, `DateTimeParts`, `DateTimeWeek`,
`DateTimeWorkday`, `NumeralConversion`, `InMemoryHost`, `LibraryProbe`.

**Layer 5: LibreOffice Adapters** (`compat/libreoffice/`)
Nineteen thin adapter headers bridging engine types to Calc internals:
`Host`, `Address`, `Error`, `String`, `Grammar`, `Config`, `Rounding`, `Date`,
`LookupCache`, `SharedFormula`, `ReferenceUpdate`, `FormulaResult`, `Parsing`,
`TextServices`, `LibraryProbe`, `TokenBridge`, `CompileHost`,
`ShadowCompiler`, `CompilerDiff`.

**Layer 6: Shims** (`shims/include/`)
Minimal SAL/RTL replacements for standalone builds: `sal/types.h`,
`rtl/math.hxx`, `sal/log.hxx`, `basegfx/numeric/ftools.hxx`, `kahan.hxx`.

---

## Completed Milestones

### Initial Extraction (Phases 0-11)

All phases are complete. The engine entered routine maintenance mode after
Phase 11 closeout.

| Phase | Name | Key Deliverables |
|-------|------|-----------------|
| 0 | Module skeleton | `Library_spreadsheetengine.mk`, `Module_spreadsheet_engine.mk`, Calc link, validation scripts |
| 1 | FormulaGrammar helpers | Copied formula grammar under `source/compat/formula/` |
| 2 | Calc-config helpers | `CalcConfig.cxx` with pure config logic |
| 3 | MatrixOperators | Matrix operation support in `source/core/` |
| 4 | Compiler char-table | `CompilerSupport.cxx` with engine-owned char flags |
| 5 | Low-coupling families | Rounding, scalar/transcendental/financial/bitwise math, numeral conversion, text, date helpers |
| 6 | Portable helper parity | Engine-owned types for all helpers, service interfaces, shared parity datasets |
| 7 | Compiler and config primitives | Engine-owned grammar enums, config opcode vocabulary, OpenCL subset config, copied formula grammar seam — 13 incremental slices covering the full vectorized/configurable Calc function surface |
| 8 | Matrix and execution substrate | Matrix dimensioning/replication/allocation, jump-matrix cursor/buffering, execution-context scratch/cache |
| 9 | Host runtime and interpreter | `EvaluationHost` interface, `InMemoryHost`, logical/parsing/lookup/array/reference helpers, `INDIRECT`/`ADDRESS` policy — 6 passes |
| 10 | References and dependencies | Formula-result and reference-data carriers, lookup-cache semantics, query-policy evaluation, shared-formula planning, formula-cell state planning, reference-update kernels, recalculation planning — 6 passes |
| 11 | Package hardening | Standalone CMake package with install/export, consumer smoke test, combined maintenance validation, Calc adapter consolidation, compatibility layer cleanup |

### Token and Compiler Host Model

This milestone created the platform for shared compilation between Calc and the
standalone engine. It was executed in 8 phases, all now complete.

**Core deliverables:**

- **Canonical token schema** (`detail/TokenModel.hxx`): 18 token kinds covering
  the full compiler-emitted repertoire (opcodes, scalars, strings, refs, names,
  db ranges, external refs, table refs, matrices, jumps, errors, whitespace)
- **Compile-host interfaces** (`detail/CompileHost.hxx`): resolver contracts for
  `NameResolver`, `DatabaseRangeResolver`, `TableRefResolver`,
  `ColRowNameResolver`, `ExternalNameResolver`, plus `CompileContext` and
  `CompileHosts` bundle
- **Compiler pipeline** (`detail/CompilerPipeline.hxx`): `FormulaSource`,
  `CompileRequest`, `CompileStatus` contract
- **Calc bridge** (`compat/libreoffice/TokenBridge.hxx`): lossless
  `ScTokenArray` ↔ `CompiledFormula` conversion for the milestone corpus
- **Calc compile-host adapters** (`compat/libreoffice/CompileHost.hxx`):
  `DocumentCompileHost` wrapping `ScDocument` services
- **Shadow compiler** (`compat/libreoffice/ShadowCompiler.hxx`): engine compile
  request executed against Calc's legacy backend for differential comparison
- **Compile-diff harness** (`compat/libreoffice/CompilerDiff.hxx`): canonical
  token stream comparison with diagnostic output

**First native consumers (live in Calc):**

- Shared-formula token comparison via `ScFormulaCell::CompareByTokenArray()`
- Lexical equality via `ScTokenArray::EqualTokens()`
- Lexical hashing via `ScTokenArray::GenHash()`
- Diagnostic token stringification for compile-diff reporting

**First bridged Calc compile adopters (live in Calc with legacy fallback):**

- `ScSimpleFormulaCalculator` formula compilation
- `ScRangeData::CompileRangeData()` named-range compilation
- `ScRefTokenHelper::compileRangeRepresentation()` reference compilation

**Token kind coverage:**

| Token kind | Schema | Bridge | Shadow/diff | Live consumer |
|------------|--------|--------|-------------|---------------|
| PlainOpcode | yes | yes | yes | yes |
| Value | yes | yes | yes | yes |
| String | yes | yes | yes | yes |
| SingleRef | yes | yes | yes | yes |
| DoubleRef | yes | yes | yes | yes |
| RangeName | yes | yes | yes | yes |
| DatabaseRange | yes | yes | yes | yes |
| ExternalName | yes | yes | yes | yes |
| Matrix | yes | yes | yes | yes |
| ColRowName | yes | yes | yes | yes |
| TableRef | yes | yes | yes | yes |
| Error | yes | yes | partial | indirect |
| Whitespace | yes | yes | yes | yes |
| Missing | yes | yes | no | indirect |
| Byte | yes | yes | no | indirect |
| StringName | yes | yes | no | indirect |
| ExternalSingleRef | yes | yes | no | bridge-only |
| ExternalDoubleRef | yes | yes | no | bridge-only |
| Jump | yes | yes | no | bridge-only |

### FODS Workbook Support

All foundation passes complete. Six function families fully enabled for
standalone replay:

| Family | Status | Notable capabilities |
|--------|--------|---------------------|
| Logical | Enabled | AND, OR, NOT, IF, IFERROR, IFNA, IFS scaffolding |
| Mathematical | Enabled | Full AGGREGATE with option filtering, SUBTOTAL, nested skipping |
| Text | Enabled | CLEAN, UNICHAR, EXACT, array constants, `text:s`/`text:tab` markup |
| Date/time | Enabled | DATE, TIME, VALUE, DATEVALUE, TIMEVALUE, DATEDIF, DAYS, DAYS360, EDATE, EOMONTH, EASTERSUNDAY, plus OpenOffice date extensions |
| Spreadsheet | Enabled | VLOOKUP/HLOOKUP with exact and sorted lookup, MATCH, INDEX |
| Information | Enabled | FORMULA, MAX/MIN extrema paths |

**Corpus metrics (frozen baseline):**

- 228 workbooks across 6 families
- 19,930 formula cells
- 18,991 parsed formulas
- ~33.6% cached-fallback rate (formulas using stored values rather than live
  evaluation)
- 17,929 function-call nodes, 22,903 cell-reference nodes

---

## Active Work: Compiler Switchover

The compiler switchover program moves standalone FODS replay from the bespoke
`OdfFormulaParser` onto the shared production compiler pipeline. This is the
bridge between "standalone has its own parser" and "Calc and standalone share
one authoritative compiler."

### Current state

**All six families are promoted.** The default standalone replay lane now
prefers the shared compiler plus compiled-token execution for every
preflight-ready formula. The legacy parser path is retained only as a
`--legacy-only` debug escape hatch.

**Switchover phases completed:**

| Phase | Goal | Status |
|-------|------|--------|
| 0 | Freeze corpus and success metrics | Complete |
| 1 | Standalone workbook compile host | Complete |
| 2 | Native engine compiler for FODS-safe subset | Complete |
| 3 | Token execution adapter in standalone | Complete |
| 4 | Dual-path FODS replay harness | Complete |
| 5 | Family-by-family switchover | Complete (all 6 families) |
| 6 | Calc/standalone compiler convergence hardening | In progress |
| 7 | Switchover completion and retirement | Partially complete |

**Phase 7 progress:** The default replay path is switched. The legacy parser
mode remains as `--legacy-only`. The standard maintenance path keeps the
compiled-default replay lane in the hot path.

**Phase 6 progress (convergence hardening):**

The Calc compile-diff smoke now covers formulas from all six enabled FODS
families. A standalone-vs-Calc artifact smoke is in place for a mixed lexical
subset covering operators, references, range names, array/error literals, and
a representative function set including `SUM`, `DATEVALUE`, `FORMULA`,
`VLOOKUP`, `IFERROR`, `ROUND`, `CEILING`, `FLOOR`, `MATCH`, `SUMIF`,
`ADDRESS`, `OFFSET`, `INDIRECT`, `NETWORKDAYS`, `NETWORKDAYS.INTL`,
`GETPIVOTDATA`, `EUROCONVERT`, and approximately 60 more functions.

Bad-name lexical preservation is verified for compatibility heads like
`COM.MICROSOFT.CONCAT`, `ORG.OPENOFFICE.CONVERT`, `CONVERT`, `DEC2HEX`,
`MROUND`, `MULTINOMIAL`, and `YEARFRAC`.

The standalone workbook compile host now emits real `ExternalName` carriers for
a curated built-in add-in subset (`CONVERT`, `DEC2HEX`, `MROUND`,
`MULTINOMIAL`, `YEARFRAC`, `WORKDAY`, `RANDBETWEEN`, `SERIESSUM`, `QUOTIENT`,
`SQRTPI`) shared with Calc's `DocumentCompileHost`, including alias
normalization for `ORG.OPENOFFICE.CONVERT`.

**Remaining convergence gap:** The main remaining work is broader
function-catalog coverage and full-stream lexical parity. Standalone execution
lowering still carries a separate RPN-oriented execution path alongside the
Calc-shaped lexical lowering used for parity checks. The preflight tail stands
at 51 hard blockers and 41 expected-error formulas out of 19,930 total.

### Preflight and lowering baselines

- Preflight-ready: 19,838 / 19,930 (99.54%)
- Successfully lowered: 19,838 / 19,838 (100% of preflight-ready)
- Hard blockers: 51 (parse failures: 14, missing named references: 37)
- Expected-error formulas: 41 (expected-error parse failures: 40,
  expected-error missing named references: 1)

### Dual-path replay baselines

- Full corpus: 19,838 eligible, 19,838 matched, 0 execution mismatches
- One cached-fallback-only difference (`let.fods Sheet2.I49` where AST uses
  cached value but compiled path evaluates live)

---

## Build and Validation

### Standalone Build

```bash
cmake -S spreadsheet_engine -B /tmp/se-build
cmake --build /tmp/se-build
ctest --test-dir /tmp/se-build --output-on-failure
```

**Requirements:** C++20 compiler, ICU, libxml2

**Output:** `spreadsheetengine::core` static library

### Installation

```bash
cmake --install /tmp/se-build --prefix /tmp/se-install
```

Downstream projects can consume via:

```cmake
find_package(spreadsheetengine REQUIRED)
target_link_libraries(myapp PRIVATE spreadsheetengine::core)
```

### Validation Commands

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

### Test Suite

24 CTest entries: 23 executables plus 1 installed-package consumer smoke.

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

Calc-side validation targets for the token/compiler work:

- `CppunitTest_sc_ucalc_token_bridge`
- `CppunitTest_sc_ucalc_compile_host`
- `CppunitTest_sc_ucalc_shadow_compiler`
- `CppunitTest_sc_ucalc_compile_diff`

---

## Points of Coupling with LibreOffice

### Intentional Coupling (Adapter Layer)

The 19 adapter headers under `compat/libreoffice/` bridge engine types to Calc
internals. This coupling is by design:

- **`DocumentEvaluationHost`** wraps `ScDocument` for cell/reference access
- **`DocumentCompileHost`** wraps `ScDocument` for compile-time name/db/table/
  external-ref resolution
- **`TokenBridge`** provides lossless `ScTokenArray` ↔ `CompiledFormula`
  conversion
- **`ShadowCompiler`** routes engine compile requests through Calc's legacy
  backend
- **Type converters** for `ScAddress`, `OUString`, `FormulaError`, `Grammar`,
  etc.

### Residual Coupling

- **`FormulaCompiler` fallback:** a small compatibility path in `calcconfig.cxx`
  for non-core residual opcode names
- **Copied formula grammar:** `compat/formula/FormulaGrammar` intentionally
  duplicated from the `formula/` module
- **SAL type shims:** `sal_Int32`, `sal_Unicode`, etc. shimmed rather than
  eliminated from API signatures
- **Legacy compiler fallback:** adopted Calc compile flows retain direct
  `ScCompiler::CompileString()` as fallback for unsupported bridge corners

---

## Limitations

- **Read-only FODS only:** loads workbooks for evaluation, cannot write them
- **No full ODS:** only flat single-file FODS, not zipped ODS packages
- **No rendering:** no layout, styles, page model, or print support
- **No charts, drawing objects, annotations:** out of scope
- **No macros:** no BASIC, Python, or UNO macro execution
- **Standalone evaluation is partial:** live for six families, with
  cached-result fallback for unsupported paths; not a full Calc-equivalent
  workbook runtime
- **No persistent dependency graph in standalone:** evaluation is lazy and
  memoized with cycle detection, but no Calc-style dependency graph or
  document-level recalc orchestration
- **Compiled-token execution is indirect:** currently inflates canonical tokens
  back into executable nodes and reuses the AST evaluator semantics, rather
  than running a direct token interpreter
- **Family coverage still expanding:** add-in and other families remain future
  work for FODS replay
- **Lexical parity is not yet corpus-wide:** exact canonical token-stream parity
  between standalone lowering and Calc's canonical tokens is proven on a
  representative mixed subset but not yet the full corpus

---

## Forward Roadmap

### Near-term: Complete compiler switchover convergence

- Close the remaining Phase 6 lexical parity gaps: broader function-catalog
  coverage, add-in/external-name handling, edge-form normalization
- Reduce the 51 hard preflight blockers (ragged-array constructs, unresolved
  workbook names)
- Formally close Phase 7 once the legacy parser is fully retired from the
  standard execution path

### Medium-term: Expand FODS family coverage

- Enable the add-in family for FODS replay
- Continue expanding standalone live evaluation to reduce the ~33.6%
  cached-fallback rate
- Broaden shared parity TSV datasets

### Long-term: Extract remaining Calc calculation core

The feasibility assessment for the remaining major extraction work is complete
(originally documented in NEXT_STEPS.md). The four remaining areas and their
recommended sequencing:

| Area | Difficulty | Notes |
|------|-----------|-------|
| Formula compiler/parser authority | Moderate-high | Token model and compile-host contracts already exist; need native engine lowering instead of Calc shadow backend |
| Dependency graph and invalidation | High | Currently distributed across `ScDocument`, broadcasters, formula trees, dirty flags; requires separating planning from side effects |
| Recalculation orchestration | High | Planner seams exist from Phase 10; actual scheduling still flows through Calc document mutation |
| Execution backend | Very high | `ScInterpreter` deeply coupled to `ScDocument`, `ScFormulaCell`, `ScTokenArray`; must be staged incrementally |

**Recommended sequencing:**

1. **Make the engine compiler authoritative** (extend current switchover work)
   - Native engine lowering that no longer depends on `ScCompiler`
   - Calc becomes a pure host adapter for compiler lookups
2. **Introduce an engine workbook facade backed by Calc**
   - Engine-owned workbook interface that abstracts over `ScDocument`
   - Calc-backed and standalone adapters
3. **Extract dependency analysis and invalidation planning**
   - Separate dependency relationships from listener/broadcaster side effects
   - Engine-owned dirty-set and invalidation planner
   - Calc continues executing side effects
4. **Extract recalculation scheduler**
   - Engine-owned recalc queue and scheduling policy
   - Calc uses engine planner output to drive execution
5. **Incrementally extract the CPU execution backend**
   - Move token walking and operand coercion
   - Grow function-family coverage against Calc differential validation
6. **Keep threading and OpenCL as backend adapters**
   - Engine batch-execution contract
   - Calc-side resource management
7. **Decide authority shift timing**
   - Whether `ScDocument` adapts to engine workbook, or engine workbook becomes
     authoritative, should be decided only after compiler/dependency/scheduler
     layers are stable

**Key architectural principle:** use a sidecar and adapter model first, then
converge on shared workbook authority later. Do not attempt to replace
`ScDocument` storage before the algorithm layers are stable.

### Validation strategy for future extraction

Every stage should be driven by differential validation:

- **Compiler:** shadow-mode comparison of token streams, payloads, reference
  lowering, and re-serialized formula text
- **Dependency:** shadow-build dependency edges and compare dirty sets after
  representative mutations (edit, insert/delete row/column, move sheet, rename)
- **Scheduler:** compare recalc queue ordering, group fallback decisions, and
  exhaustion behavior
- **Execution:** compare scalar results, types, errors, and formatting between
  Calc and engine evaluator
- **Performance:** record compile, mutation, and recalc timings from the start
  to catch regressions before they compound

### Risks for the forward roadmap

- Extracting storage too early and destabilizing `ScDocument`
- Trying to move `ScInterpreter` wholesale instead of incrementally
- Conflating dependency planning with listener side effects
- Prematurely baking Calc-specific concepts into the engine API
- Forcing OpenCL/threading concerns into the engine core too early

### Explicit non-goals

The engine should not absorb:

- Styling, rendering, or page model
- UNO object model
- Chart or draw integration
- UI state or dialogs
- Macro or VBA execution
- Import/export fidelity beyond what is needed for calculation behavior

---

## Key Documentation

- [README.md](../README.md) — build, test, and install instructions
- [CALC_ENGINE_EXTRACTION_PLAN.md](extraction-history/CALC_ENGINE_EXTRACTION_PLAN.md) —
  master extraction roadmap with detailed per-phase status (Phases 0-11)
- [BASIC_FODS_SUPPORT.md](architecture/BASIC_FODS_SUPPORT.md) — FODS loader/
  evaluator architecture and implementation checklist
- [TOKEN_COMPILER_HOST_MODEL.md](architecture/TOKEN_COMPILER_HOST_MODEL.md) —
  token model and compiler-host milestone plan with execution tracker
- [COMPILER_SWITCHOVER.md](architecture/COMPILER_SWITCHOVER.md) — compiler
  switchover plan with phased execution checklist
- [CALC_ENGINE_AUDIT.md](extraction-history/CALC_ENGINE_AUDIT.md) — audit of
  original Calc engine source files
