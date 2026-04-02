# Spreadsheet Engine: Project Status

This document is the single consolidated reference for the `spreadsheet_engine/`
project. It describes what has been built, what works today, what is actively
being worked on, and what the forward roadmap looks like.

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
- the engine can load and evaluate real spreadsheet workbooks 
  without any LibreOffice runtime
- Calc and standalone share the same authoritative compiler, token model, and
  function implementations

This is not a rewrite. It is a phased migration of existing Calc engine code
into a self-contained library, validated at every step by differential testing
against Calc.

---

## Current State At A Glance

The project has now closed out seven major programs. Compiler switchover is no
longer an active extraction stream; it has moved into maintenance mode with the
shared compiler path established as the default standalone replay path, and the
large `FormulaEvaluator`/`ScInterpreter` pure-computation convergence program
is now also in closeout status.

| Program | Status | Summary |
|---------|--------|---------|
| Initial extraction (Phases 0-11) | **Complete** | Pure calculation logic, function families, matrix/execution substrate, host runtime, reference/dependency planning, and standalone packaging all extracted |
| Token and compiler host model | **Complete** | Canonical token schema, compile-host interfaces, Calc bridge, shadow compiler, differential validation, first native consumers, and first bridged Calc compile adopters all in place |
| Compiler switchover | **Complete** | Shared compiler plus compiled-token execution is now the default standalone replay path for all eleven enabled families, with zero hard compiler blockers remaining on the replay corpus |
| Calc-backed workbook facade | **Complete** | Engine-owned workbook facade contract with Calc-backed and in-memory implementations, richer named-range mutation payloads, dedicated standalone/Calc validation lanes, and first live consumption through dependency-shadow runtime auditing |
| Dependency and invalidation extraction | **Complete** | Engine-owned dependency snapshots, reverse dependency indexing, invalidation planning, structural rebuild scopes, workbook-scale Calc shadow corpus, maintenance-lane integration, and an opt-in runtime shadow audit are all in place |
| Recalc orchestration extraction | **Complete** | Engine-owned recalc planning, queue construction, authority pilots, and Calc queue-consumption bridge are complete through the safe structural/named-range pilot surface |
| FormulaEvaluator runtime modularization | **Complete** | The old evaluator monolith has been split across focused runtime and support modules such as `LookupRuntime`, `QueryRuntime`, `TextFunctionRuntime`, `DateTimeParse`, `FinancialRuntime`, `MathAggregate`, `MathFunctionRuntime`, and `ConversionRuntime` |
| Calc pure-computation convergence | **Complete** | Calc now delegates the in-scope pure-computation statistical, aggregate, inverse-distribution, combinatoric, and error-function families to the same shared runtime modules used by standalone, with Calc-aligned algorithms adopted where behavior risk existed |
| Execution backend extraction | **Active** | Phases 0 through 8 are complete: the execution boundary is frozen, shared scalar coercion and operator-shell helpers are engine-owned, the first reference-sensitive slices and bounded lookup traversal moved behind compat bridges, `ROW` / `COLUMN` / `ROWS` / `COLUMNS` / `AREAS` / `SHEET` / `SHEETS` now share bounded reference-shape logic, the bounded `CHOOSE` / `IFERROR` / `IFNA` / `INDIRECT` special-form shell now routes through shared or compat-owned helpers, Calc's bounded `LET` / `SWITCH` shell now routes through dedicated compat helpers, and the generic jump-matrix cursor/finalization shell now routes through compat helpers |

The immediate active frontier is now narrower and more practical:

- keep the fully promoted replay corpus green across all eleven function
  families with zero cached-fallback cells
- continue the execution-backend milestone from the now-frozen recalc
  orchestration boundary, with the next slice targeting the broader
  token-walking and final string-reference compilation shell on top of the
  completed `MATCH` / `XMATCH` / `ADDRESS` /
  `OFFSET` / `INDEX` / `LOOKUP` / `VLOOKUP` / `HLOOKUP` / `XLOOKUP` /
  `ROW` / `COLUMN` / `ROWS` / `COLUMNS` / `AREAS` / `SHEET` / `SHEETS` /
  `CHOOSE` / `IFERROR` / `IFNA` / `INDIRECT` / `LET` / `SWITCH` /
  generic-jump-matrix boundary
- keep expanding engine-first adoption inside Calc only where differential
  validation keeps compiler/runtime behavior safe

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
- **Workbook facade:** engine-owned calculation-facing workbook contract with
  Calc-backed and in-memory implementations, formula-cell iteration, named-range
  queries, shared-formula group metadata, mutation event vocabulary (14 kinds)
  with before/after named-range descriptors, first shadow consumers
  (formula-cell enumeration, group summary, named-range inventory, snapshot
  comparison, formula corpus collection), dedicated Calc-backed validation via
  `CppunitTest_sc_ucalc_workbook_facade`, and first live runtime consumption
  through dependency-shadow auditing
- **Dependency and invalidation planning:** engine-owned dependency node/edge
  model, workbook-facade snapshot builder, reverse-dependency index, named-range
  and shared-group normalization, mutation-to-dirty planning, rebuild-scope
  modeling, standalone dependency tests, workbook-scale Calc shadow validation
  via `CppunitTest_sc_ucalc_dependency_shadow`, and opt-in runtime auditing of
  `ScDocument::SetValue()`, `SetString()`, and `SetEmptyCell()`
- **Standalone FODS runtime:** sparse workbook model, read-only FODS loader,
  ODF formula parser, lazy evaluator with memoization and cycle detection
  (`OdfFormulaParser` still serves as the engine's ODF formula frontend for
  standalone lowering, dependency snapshotting, diagnostics, and the retained
  debug/legacy AST path; it is no longer the authority boundary for promoted
  replay-family compilation decisions)
- **Runtime modularization and shared adoption:** extracted runtime families now
  cover lookup/query, text/search, date parsing, financial dispatch,
  aggregate/statistics kernels, conversion/combinatorics, and focused evaluator
  support helpers; those same shared modules now serve both standalone replay
  and selected Calc `ScInterpreter` entry points
- **Raw FODS replay:** all eleven Calc function-workbook families fully enabled
  (`logical`, `mathematical`, `text`, `date_time`, `spreadsheet`,
  `information`, `financial`, `statistical`, `addin`, `array`, `database`)
- **Parity infrastructure:** shared TSV datasets, dual-mode validation scripts,
  compile-diff harness, compiler preflight classifier

### What Calc still owns

- `ScDocument` / `ScTable` / `ScColumn` in-memory storage
- The main production formula compiler path via `ScCompiler` (engine compiler
  is adopted in selected flows with legacy fallback)
- `ScTokenArray` as the pervasive token container (engine canonical model is
  used by first native consumers with bridge adapters)
- Listener/broadcaster wiring and authoritative dependency-graph side effects
- Authoritative dirty-bit setting, formula-tree ownership, and recalculation
  orchestration (the engine now owns shadow dependency snapshots and
  invalidation planning, but Calc still owns production side effects)
- The production `ScInterpreter` evaluator shell for most runtime execution,
  especially scheduling/database/matrix/storage-sensitive behavior and the
  broader token-walking shell beyond the bounded lookup/reference slices now
  extracted; selected pure-computation statistical and aggregate kernels now
  delegate to shared engine runtime modules
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
│   │   └── libreoffice/        #     Calc-specific integration adapters (24 headers)
│   ├── detail/                 #   Internal implementation headers
│   │   └── workbook/           #     Workbook facade contract and implementations
│   └── runtime/                #   Standalone runtime helpers
├── shims/include/              # SAL/RTL type replacements for standalone
├── source/core/                # Implementation files
├── tests/
│   ├── unit/                   #   25 test sources + 3 shared support headers
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

**Layer 3: Workbook Runtime** (`detail/`)
Standalone workbook subsystem: `WorkbookModel` (sparse cell storage with named
ranges and imported-sheet metadata), `FodsLoader` (libxml2-backed XML parser),
`OdfFormulaParser` (ODF `of:=` formula tokenizer and AST), `FormulaEvaluator`
(lazy formula evaluator with memoization, cycle detection, and compiled-token
execution support).

**Layer 4: Runtime Helpers** (`runtime/`)
Standalone and shared pure-computation helpers: `MathScalar`,
`MathTranscendental`, `MathBitwise`, `MathFinancial`, `MathRounding`,
`MathStatistical`, `MathAggregate`, `MathFunctionRuntime`,
`ConversionRuntime`, `FinancialRuntime`, `QueryRuntime`, `LookupRuntime`,
`TextCase`, `TextScalar`, `TextFunctionRuntime`, `TextRuntimeSupport`,
`TextWidth`, `TextServices`, `DateTimeParse`, `DateTimeParts`,
`DateTimeWeek`, `DateTimeWorkday`, `NumeralConversion`, `InMemoryHost`,
`LibraryProbe`.

**Layer 5: LibreOffice Adapters** (`compat/libreoffice/`)
Twenty-four thin adapter headers bridging engine types to Calc internals:
`Host`, `Address`, `Error`, `String`, `Grammar`, `Config`, `Rounding`, `Date`,
`LookupCache`, `SharedFormula`, `ReferenceUpdate`, `FormulaResult`, `Parsing`,
`TextServices`, `LibraryProbe`, `TokenBridge`, `CompileHost`,
`ShadowCompiler`, `CompilerDiff`, `WorkbookFacade`, `MutationTranslator`,
`DependencyShadow`, `LookupExecution`, `ReferenceExecution`.

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

### FormulaEvaluator Runtime Modularization And Calc Convergence

This program is now closed out. The standalone evaluator is no longer a
single-purpose monolith, and the in-scope pure-computation convergence work
with Calc has been completed on top of the extracted runtime surface.

**Closeout summary:**

- `FormulaEvaluator` has been split across focused helpers such as
  `FormulaEvaluatorAggregate`, `FormulaEvaluatorSpecialForms`, and
  `CompiledFormulaInflation`
- lookup/query, text/search, date parsing, financial dispatch, aggregate math,
  conversion/combinatorics, and other pure-computation helpers now live in
  dedicated runtime modules
- shared runtime modules now serve both standalone replay and selected Calc
  `ScInterpreter` entry points
- Calc now delegates the in-scope pure-computation statistical, aggregate,
  inverse-distribution, combinatoric, and error-function families to shared
  runtime
- behavior-sensitive shared algorithms adopt Calc's original implementation
  where that was necessary to avoid divergence

The remaining execution-backend work is no longer "modularize the evaluator"
or "extract pure-computation kernels." It is the harder next layer:
authoritative production execution ownership, reference-sensitive behavior, and
recalculation orchestration.

### FODS Workbook Support

All foundation passes complete. All eleven Calc function-workbook families with
dedicated `fods` directories are now enabled for standalone replay.

| Calc FODS family | Workbooks | What it covers | Standalone testing-suite status |
|------------------|-----------|----------------|---------------------------------|
| `logical` | 9 | Boolean logic and control-flow formulas such as `AND`, `OR`, `NOT`, `IF`, `IFERROR`, `IFNA`, and `IFS` | **Enabled in default standalone replay** |
| `mathematical` | 79 | Core scalar/transcendental math, rounding, aggregates, and option-sensitive arithmetic cases such as `AGGREGATE` and `SUBTOTAL` | **Enabled in default standalone replay** |
| `text` | 44 | String conversion, width/case, replacement/concatenation, byte-width compatibility, and markup-sensitive text cases | **Enabled in default standalone replay** |
| `date_time` | 32 | Calendar parsing, serial/date arithmetic, week/day logic, and OpenOffice date extensions | **Enabled in default standalone replay** |
| `spreadsheet` | 44 | Lookup/reference and sheet-structure formulas such as `ADDRESS`, `AREAS`, `CHOOSE`, `INDEX`, `MATCH`, `VLOOKUP`, and `HLOOKUP` | **Enabled in default standalone replay** |
| `information` | 20 | Formula/type/error introspection such as `FORMULA`, `INFO`, `CURRENT`, and `IS*` families | **Enabled in default standalone replay** |
| `addin` | 49 | Analysis/add-in and compatibility functions such as Bessel, base conversion, engineering, and other external-name style formulas | **Enabled in default standalone replay** |
| `array` | 13 | Array/matrix and regression-style workbooks such as `FOURIER`, `FREQUENCY`, `GROWTH`, `LOGEST`, and `MDETERM` | **Enabled in default standalone replay** |
| `database` | 12 | Criteria-driven database aggregation formulas such as `DAVERAGE`, `DCOUNT`, `DGET`, and `DMAX` | **Enabled in default standalone replay** |
| `financial` | 51 | Coupon, amortization, depreciation, accrual, and yield-oriented financial formulas | **Enabled in default standalone replay** |
| `statistical` | 147 | Descriptive, inferential, and distribution/regression workbooks such as `AVERAGEIF`, `KahanSum`, and the broader statistics catalog | **Enabled in default standalone replay** |
| `functions/fods` (top-level) | 5 | Mixed top-level regression workbooks and operator/compatibility fixtures such as `reference_operators` and `Functions_Excel_2016` | **Available in Calc corpus; not currently promoted as its own standalone family lane** |

**Corpus metrics (frozen baseline):**

- 500 workbooks across the 11 promoted families
- 50,661 formula cells
- 50,652 parsed formulas
- 0 cached-fallback cells on the default promoted replay policy
  (`0%` of formula cells)
- cached-fallback family split: none
- cached-fallback top categories: none
- 57,239 function-call nodes
- 72,217 cell-reference nodes, 6,219 range-reference nodes, 398 named-reference nodes
- 446 array-constant nodes

The replay summary now measures fallback through the same compiled replay path
used by the promoted families and emits per-family plus top-category fallback
breakdowns for maintenance and regression tracking.

---

## Completed Program: Compiler Switchover

The compiler switchover program moves standalone FODS replay from the bespoke
`OdfFormulaParser` onto the shared production compiler pipeline. This is the
bridge between "standalone has its own parser" and "Calc and standalone share
one authoritative compiler."

### Closeout state

**All eleven families are promoted.** The default standalone replay lane now
prefers the shared compiler plus compiled-token execution for every promoted
formula that is eligible for shared compilation. The legacy parser path remains
available only as a `--legacy-only` debug escape hatch and is no longer part of
the standard maintenance path.

**Switchover phases completed:**

| Phase | Goal | Status |
|-------|------|--------|
| 0 | Freeze corpus and success metrics | Complete |
| 1 | Standalone workbook compile host | Complete |
| 2 | Native engine compiler for FODS-safe subset | Complete |
| 3 | Token execution adapter in standalone | Complete |
| 4 | Dual-path FODS replay harness | Complete |
| 5 | Family-by-family switchover | Complete (all 11 promoted families) |
| 6 | Calc/standalone compiler convergence hardening | Complete |
| 7 | Switchover completion and retirement | Complete |

**Closeout summary:**

Standalone raw replay and compiled-token replay now cover all eleven promoted
FODS families (`logical`, `mathematical`, `text`, `date_time`, `spreadsheet`,
`information`, `financial`, `statistical`, `addin`, `array`, `database`). Calc compile-diff remains a
representative cross-family maintenance lane rather than a corpus-wide mirror.
That smoke covers operators, references, range names, array/error literals,
and a representative function set including `SUM`, `DATEVALUE`, `FORMULA`,
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

The original switchover corpus now has **zero hard compiler blockers**. The
only remaining skips on that baseline are **6 intentionally invalid
expected-error formulas**, all of which already cache the expected workbook
error result. Later promotion of the `financial` and `statistical` families
kept the shared compiler path green through direct family replay and compiled
replay validation, and the default standalone replay lane now passes
`500 / 500` promoted workbooks.

Exact lexical token-stream parity is still maintained on a representative mixed
subset rather than asserted on the full corpus, and the standalone execution
path still lowers into a separate RPN-oriented carrier before inflating back to
nodes. Those are now maintenance/hardening concerns, not open milestone
blockers for the switchover program.

### Preflight and lowering baselines

These figures are the frozen closeout baseline for the original six-family
compiler-switchover corpus:

- Preflight-ready: `19,924 / 19,930` (`99.97%`)
- Successfully lowered: `19,924 / 19,924` (`100%` of eligible formulas)
- Hard blockers: `0`
- Expected-error formulas skipped by preflight: `6`
  - all six are expected-error parse failures from intentionally invalid inputs

### Dual-path replay baselines

- Full corpus: `19,924` eligible, `19,924` matched, `0` execution mismatches
- Cached-fallback-only differences: `0`

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

26 CTest entries: 25 executables plus 1 installed-package consumer smoke.

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
| `spreadsheetengine_workbook_facade_tests` | Workbook facade types, contract, consumers |
| `spreadsheetengine_dependency_invalidation_tests` | Dependency snapshots, reverse edges, invalidation planning |
| `spreadsheetengine_recalc_planner_tests` | Recalc seed/queue construction and group-policy planning |
| `spreadsheetengine_installed_package_smoke` | Installed-package downstream-consumer smoke |

Calc-side validation targets for the currently completed facade/compiler/
dependency/runtime-convergence work:

- `CppunitTest_sc_ucalc_token_bridge`
- `CppunitTest_sc_ucalc_compile_host`
- `CppunitTest_sc_ucalc_shadow_compiler`
- `CppunitTest_sc_ucalc_compile_diff`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_dependency_shadow` (dependency + recalc shadow comparison)
- `CppunitTest_sc_ucalc_formula2`

---

## Points of Coupling with LibreOffice

### Intentional Coupling (Adapter Layer)

The 22 adapter headers under `compat/libreoffice/` bridge engine types to Calc
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
- **Standalone evaluation is broader but still not full Calc:** the eleven
  promoted FODS families now replay with zero cached-fallback cells, but the
  engine is still not a full Calc-equivalent workbook runtime
- **No persistent dependency graph in standalone:** evaluation is lazy and
  memoized with cycle detection, but no Calc-style dependency graph or
  document-level recalc orchestration
- **Compiled-token execution is indirect:** currently inflates canonical tokens
  back into executable nodes and reuses the AST evaluator semantics, rather
  than running a direct token interpreter
- **Family coverage still expanding beyond function families:** the eleven
  dedicated Calc function-workbook families are now promoted, but the top-level
  mixed regression workbooks under `functions/fods` remain outside the default
  replay corpus
- **Lexical parity is representative, not corpus-wide:** exact canonical
  token-stream parity between standalone lowering and Calc's canonical tokens
  is proven on a representative mixed subset, while corpus-wide value parity is
  already green on every eligible promoted formula

---

## Forward Roadmap

### Near-term: Hold The Zero-Fallback Baseline And Advance Execution Extraction

- Keep the zero-cached-fallback replay result green across the full eleven-family
  promoted corpus (`500` workbooks / `50,661` formula cells)
- Keep the compiler-switchover maintenance lanes green:
  - representative Calc lexical parity smoke
  - compiled replay diff smoke
  - the `--legacy-only` escape hatch until we make an explicit long-term
    keep/remove decision
- Keep the one-shot compiled `--summary` result authoritative as the published
  replay baseline
- Keep the now-completed recalc-orchestration boundary stable while the
  execution backend moves behind it
- Use the completed Phase 0-4 helper inventory, shared scalar-coercion layer,
  operator-shell dispatch helpers, first compat-bridge reference slice, and
  bounded lookup traversal bridge as the base for the next execution slice in
  [EXECUTION_BACKEND_EXTRACTION.md](architecture/EXECUTION_BACKEND_EXTRACTION.md)

### Active Milestone: Execution Backend Extraction

The recalc-orchestration milestone is now complete. The next extraction
program starts from that stable boundary and targets the remaining
Calc-owned execution shell: token walking, coercion, and
reference-sensitive execution behavior.

The completed recalc-orchestration milestone landed:

- Phase 0 froze the zero-fallback replay baseline and the validation lanes
- Phase 1 landed engine-owned recalc seeds, queue entries, plan results, and
  group-policy/structural-rebuild metadata on top of the invalidation planner
- Phase 2 landed a Calc shadow adapter that compares engine queue membership,
  ordering, and group handling against Calc's live formula-tree state after
  representative non-structural edits
- Phase 3 landed an opt-in authoritative dirty-plan pilot with rollback on
  verification failure
- Phase 4 landed engine-owned queue application for the same safe non-structural
  mutation families, validated with ordering-sensitive Calc differential tests
- Phase 5 widened that pilot to whole-row / whole-column structural mutations
  plus named-range mutation flows, with rebuilt post-mutation queue planning
  for snapshot-rebuild cases
- Phase 6 isolated Calc queue consumption behind a dedicated compat bridge and
  opened the next execution-backend milestone from that stable boundary

The active implementation frontier is now the **post-Phase-8 broader
token-walking and final string-reference-compilation shell** in the
execution-backend milestone.

The recommended active sequence is:

1. Keep the completed Phase 0-8 helper and compat-bridge layers stable and
   green.
2. Extract the next bounded token-walking shell slice without reopening general
   opcode-dispatch or storage concerns.
3. Keep token walking, stack shell, and queue authority stable while later
   bounded execution families migrate.

### Medium-term: Extract The Execution Backend On Top Of The Settled Scheduler Boundary

- Keep the completed engine-owned dirty-plan and queue boundary green while
  execution slices move behind it
- Extract reference-sensitive evaluator helpers where Calc and standalone still
  duplicate logic, building on the now-shared coercion and operator-shell layers
- Use explicit compat bridges where Calc still needs host-owned services during
  the execution-backend transition
- Use the completed shared-runtime convergence work as a prerequisite, not as a
  competing roadmap stream

### Long-term: Shift More Execution Authority Out Of Calc

The major extraction prerequisites are now in place: initial function/runtime
extraction, token/compiler-host modeling, standalone compiler switchover,
FormulaEvaluator runtime modularization, Calc pure-computation convergence,
Calc-backed workbook facade, and dependency/invalidation planning. The
remaining long-horizon work is now concentrated in three areas plus one later
authority decision:

| Area | Current state | Next step |
|------|---------------|-----------|
| Compiler authority inside Calc | Standalone switchover is complete; Calc still uses `ScCompiler` for most production paths | Keep engine-first compile adoption expanding only where the bridge/diff lanes make it safe |
| Recalculation orchestration | Engine-owned dirty planning and queue/scheduling authority are complete for the current pilot surface | Keep the authority lane stable while execution-backend extraction builds on top |
| Execution backend | `ScInterpreter` and execution backends are still Calc-owned, although the pure-computation runtime kernels are now substantially shared | Incrementally extract the remaining evaluator shell, coercion, and reference-sensitive execution logic behind strong differential validation |
| Workbook/storage authority | Calc-backed facade exists and is validated | Defer any authority shift until scheduler and execution layers are stable |

**Recommended sequencing:**

1. **Freeze and maintain the promoted replay baseline**
   - Keep the `0 / 50,661` cached-fallback result green
   - Keep compiled replay and lexical parity maintenance lanes green
2. **Incrementally extract the CPU execution backend**
   - Move token walking, coercion, and evaluation mechanics out of Calc in
     small validated slices
   - Continue broadening live function coverage as part of that extraction
3. **Keep threading and OpenCL as backend adapters**
   - Let Calc continue owning resource/runtime concerns while the engine owns
     more calculation semantics
4. **Decide authority shift timing last**
   - Whether `ScDocument` remains a Calc-backed host or the engine workbook
     becomes more authoritative should only be decided after scheduler and
     execution layers stabilize

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
- [docs/architecture/README.md](architecture/README.md) — pointer for active
  living architecture notes vs archived milestone plans
- [RECALC_ORCHESTRATION_EXTRACTION.md](architecture/RECALC_ORCHESTRATION_EXTRACTION.md) —
  active implementation-ready milestone for recalc/scheduler extraction
- [BASIC_FODS_SUPPORT.md](archive/BASIC_FODS_SUPPORT.md) — archived FODS loader/
  evaluator milestone plan
- [TOKEN_COMPILER_HOST_MODEL.md](archive/TOKEN_COMPILER_HOST_MODEL.md) —
  archived token model and compiler-host milestone plan
- [COMPILER_SWITCHOVER.md](archive/COMPILER_SWITCHOVER.md) — archived compiler
  switchover milestone plan
- [CALC_BACKED_WORKBOOK_FACADE.md](archive/CALC_BACKED_WORKBOOK_FACADE.md) —
  archived workbook-facade milestone plan
- [DEPENDENCY_INVALIDATION_EXTRACTION.md](archive/DEPENDENCY_INVALIDATION_EXTRACTION.md) —
  archived dependency/invalidation extraction plan and closeout record
- [FORMULA_EVALUATOR_SCINTERPRETER_CONVERGENCE.md](archive/FORMULA_EVALUATOR_SCINTERPRETER_CONVERGENCE.md) —
  archived FormulaEvaluator/ScInterpreter convergence closeout record
- [CALC_ENGINE_AUDIT.md](extraction-history/CALC_ENGINE_AUDIT.md) — audit of
  original Calc engine source files
