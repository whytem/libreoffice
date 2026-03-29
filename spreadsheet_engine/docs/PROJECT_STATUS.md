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

The project has now closed out five major programs. Compiler switchover is no
longer an active extraction stream; it has moved into maintenance mode with the
shared compiler path established as the default standalone replay path.

| Program | Status | Summary |
|---------|--------|---------|
| Initial extraction (Phases 0-11) | **Complete** | Pure calculation logic, function families, matrix/execution substrate, host runtime, reference/dependency planning, and standalone packaging all extracted |
| Token and compiler host model | **Complete** | Canonical token schema, compile-host interfaces, Calc bridge, shadow compiler, differential validation, first native consumers, and first bridged Calc compile adopters all in place |
| Compiler switchover | **Complete** | Shared compiler plus compiled-token execution is now the default standalone replay path for all eight enabled families, with zero hard compiler blockers remaining on the original switchover corpus |
| Calc-backed workbook facade | **Complete** | Engine-owned workbook facade contract with Calc-backed and in-memory implementations, richer named-range mutation payloads, dedicated standalone/Calc validation lanes, and first live consumption through dependency-shadow runtime auditing |
| Dependency and invalidation extraction | **Complete** | Engine-owned dependency snapshots, reverse dependency indexing, invalidation planning, structural rebuild scopes, workbook-scale Calc shadow corpus, maintenance-lane integration, and an opt-in runtime shadow audit are all in place |

The immediate active frontier is now narrower and more practical:

- broaden standalone replay beyond the eight promoted families, starting with the
  add-in family
- reduce cached-fallback usage on the promoted corpus by turning more formulas
  into live standalone execution
- use the completed workbook facade plus dependency/invalidation planner as the
  substrate for recalculation-orchestration extraction

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
- **Raw FODS replay:** eight function families fully enabled (logical,
  mathematical, text, date_time, spreadsheet, information, financial,
  statistical) across 426 workbooks
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
│   │   └── libreoffice/        #     Calc-specific integration adapters (22 headers)
│   ├── detail/                 #   Internal implementation headers
│   │   └── workbook/           #     Workbook facade contract and implementations
│   └── runtime/                #   Standalone runtime helpers
├── shims/include/              # SAL/RTL type replacements for standalone
├── source/core/                # Implementation files
├── tests/
│   ├── unit/                   #   25 test files (19 API/compiler + 4 FODS + 1 facade + 1 dependency)
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
Twenty-two thin adapter headers bridging engine types to Calc internals:
`Host`, `Address`, `Error`, `String`, `Grammar`, `Config`, `Rounding`, `Date`,
`LookupCache`, `SharedFormula`, `ReferenceUpdate`, `FormulaResult`, `Parsing`,
`TextServices`, `LibraryProbe`, `TokenBridge`, `CompileHost`,
`ShadowCompiler`, `CompilerDiff`, `WorkbookFacade`, `MutationTranslator`,
`DependencyShadow`.

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

All foundation passes complete. Eight function families are fully enabled for
standalone replay today, but the Calc FODS corpus available to the project is
broader than the currently promoted replay lane.

| Calc FODS family | Workbooks | What it covers | Standalone testing-suite status |
|------------------|-----------|----------------|---------------------------------|
| `logical` | 9 | Boolean logic and control-flow formulas such as `AND`, `OR`, `NOT`, `IF`, `IFERROR`, `IFNA`, and `IFS` | **Enabled in default standalone replay** |
| `mathematical` | 79 | Core scalar/transcendental math, rounding, aggregates, and option-sensitive arithmetic cases such as `AGGREGATE` and `SUBTOTAL` | **Enabled in default standalone replay** |
| `text` | 44 | String conversion, width/case, replacement/concatenation, byte-width compatibility, and markup-sensitive text cases | **Enabled in default standalone replay** |
| `date_time` | 32 | Calendar parsing, serial/date arithmetic, week/day logic, and OpenOffice date extensions | **Enabled in default standalone replay** |
| `spreadsheet` | 44 | Lookup/reference and sheet-structure formulas such as `ADDRESS`, `AREAS`, `CHOOSE`, `INDEX`, `MATCH`, `VLOOKUP`, and `HLOOKUP` | **Enabled in default standalone replay** |
| `information` | 20 | Formula/type/error introspection such as `FORMULA`, `INFO`, `CURRENT`, and `IS*` families | **Enabled in default standalone replay** |
| `addin` | 49 | Analysis/add-in and compatibility functions such as Bessel, base conversion, engineering, and other external-name style formulas | **Available in Calc corpus; next standalone replay frontier** |
| `array` | 13 | Array/matrix and regression-style workbooks such as `FOURIER`, `FREQUENCY`, `GROWTH`, `LOGEST`, and `MDETERM` | **Available in Calc corpus; not yet promoted in standalone replay** |
| `database` | 12 | Criteria-driven database aggregation formulas such as `DAVERAGE`, `DCOUNT`, `DGET`, and `DMAX` | **Available in Calc corpus; not yet promoted in standalone replay** |
| `financial` | 51 | Coupon, amortization, depreciation, accrual, and yield-oriented financial formulas | **Enabled in default standalone replay** |
| `statistical` | 147 | Descriptive, inferential, and distribution/regression workbooks such as `AVERAGEIF`, `KahanSum`, and the broader statistics catalog | **Enabled in default standalone replay** |
| `functions/fods` (top-level) | 5 | Mixed top-level regression workbooks and operator/compatibility fixtures such as `reference_operators` and `Functions_Excel_2016` | **Available in Calc corpus; not currently promoted as its own standalone family lane** |

**Corpus metrics (frozen baseline):**

- 426 workbooks across 8 families
- 40,068 formula cells
- 40,059 parsed formulas
- 15,891 cached-fallback cells on the default promoted replay policy
  (`39.66%` of formula cells)
- cached-fallback family split:
  `statistical:8381`, `mathematical:2029`, `spreadsheet:1925`,
  `financial:1763`, `text:1065`, `date_time:479`, `information:138`,
  `logical:111`
- cached-fallback top categories:
  `binary_op:eq:fn:ROUND:4968`, `LOOKUP:815`, `IF:724`,
  `binary_op:eq:fn:ROUNDSIG:399`, `POISSON:329`, `POISSON.DIST:329`,
  `ROUND:219`, `BINOMDIST:207`, `CONVERT:166`, `BINOM.DIST.RANGE:164`
- 43,165 function-call nodes
- 55,552 cell-reference nodes, 5,219 range-reference nodes, 340 named-reference nodes
- 394 array-constant nodes

The replay summary now measures fallback through the same compiled replay path
used by the promoted families and emits per-family plus top-category fallback
breakdowns for reduction work.

---

## Completed Program: Compiler Switchover

The compiler switchover program moves standalone FODS replay from the bespoke
`OdfFormulaParser` onto the shared production compiler pipeline. This is the
bridge between "standalone has its own parser" and "Calc and standalone share
one authoritative compiler."

### Closeout state

**All eight families are promoted.** The default standalone replay lane now
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
| 5 | Family-by-family switchover | Complete (all 8 promoted families) |
| 6 | Calc/standalone compiler convergence hardening | Complete |
| 7 | Switchover completion and retirement | Complete |

**Closeout summary:**

Standalone raw replay and compiled-token replay now cover all eight promoted
FODS families (`logical`, `mathematical`, `text`, `date_time`, `spreadsheet`,
`information`, `financial`, `statistical`). Calc compile-diff remains a
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
`426 / 426` promoted workbooks.

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
| `spreadsheetengine_installed_package_smoke` | Installed-package downstream-consumer smoke |

Calc-side validation targets for the currently completed facade/compiler/
dependency work:

- `CppunitTest_sc_ucalc_token_bridge`
- `CppunitTest_sc_ucalc_compile_host`
- `CppunitTest_sc_ucalc_shadow_compiler`
- `CppunitTest_sc_ucalc_compile_diff`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_dependency_shadow`

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
- **Standalone evaluation is partial:** live across eight promoted families, with
  cached-result fallback for unsupported paths; not a full Calc-equivalent
  workbook runtime
- **No persistent dependency graph in standalone:** evaluation is lazy and
  memoized with cycle detection, but no Calc-style dependency graph or
  document-level recalc orchestration
- **Compiled-token execution is indirect:** currently inflates canonical tokens
  back into executable nodes and reuses the AST evaluator semantics, rather
  than running a direct token interpreter
- **Family coverage still expanding:** add-in, array, database, and top-level
  mixed regression families remain future work for default FODS replay
- **Lexical parity is representative, not corpus-wide:** exact canonical
  token-stream parity between standalone lowering and Calc's canonical tokens
  is proven on a representative mixed subset, while corpus-wide value parity is
  already green on every eligible promoted formula

---

## Forward Roadmap

### Near-term: Broaden Standalone Replay And Reduce Fallback

- Enable the add-in family for FODS replay
- Continue expanding standalone live evaluation to reduce the cached-fallback
  footprint on the promoted eight-family corpus
- Keep the compiler-switchover maintenance lanes green:
  - representative Calc lexical parity smoke
  - compiled replay diff smoke
  - `--legacy-only` escape hatch until we make an explicit long-term
    keep/remove decision
- Broaden the shared built-in external/add-in catalog and standalone evaluator
  coverage only where it advances replay-family enablement or removes
  high-volume fallback paths

### Active Fallback Reduction Task List

1. **Refine fallback diagnostics**
   - Initial child-head splitting is now in place for `binary_op` and
     `unary_op` wrappers
   - Next, keep drilling the dominant wrapper buckets down far enough that
     they map cleanly to missing semantics, starting with
     `binary_op:eq:fn:ROUND` and `binary_op:eq:fn:ROUNDSIG`
   - Keep the replay `--summary` and `--compiled-diff` outputs aligned with the
     promoted compiled replay path
2. **Burn down the highest-volume statistical buckets**
   - Prioritize `POISSON`, `POISSON.DIST`, `BINOMDIST`,
     `BINOM.DIST.RANGE`, and adjacent distribution/statistics helpers
   - Re-freeze the family split after each cluster lands so the statistical
     bucket trend is visible
3. **Burn down the high-volume promoted-corpus wrapper buckets**
   - Reduce `LOOKUP`, `IF`, and `ROUND` fallback-heavy cases where live
     execution still exits early to cached workbook results
   - Use the new category breakdown to separate “supported function, unsupported
     shape” from “function not yet implemented” paths
4. **Clean up text/financial compatibility tails**
   - Target `CONVERT`, `CHAR`, and the remaining financial-family scalar
     helpers that are large enough to materially move the promoted-corpus rate
5. **Re-baseline before the next family promotion**
   - Record the new compiled-path fallback rate
   - Confirm raw replay and compiled-diff stay green
   - Only then move on to the next promotion candidate, starting with `addin`

### Promotion Gate For The Next Families

Before promoting any additional Calc FODS family into the default standalone
replay corpus:

1. The family must have **zero hard preflight blockers**.
2. Full raw standalone replay for the family must be **green**.
3. Family compiled-diff must be **green**, with no execution mismatches.
4. The replay summary must capture the family's **compiled-path fallback**
   counts and top categories.
5. The family must either:
   - land below the current compiled-path fallback threshold agreed for
     promotion, or
   - carry an explicit reviewed waiver list for the remaining cached paths.
6. The broader promoted corpus must remain green after the family is added to
   the default replay lane.

### Medium-term: Extract Recalculation Orchestration On Top Of The Planner

- Promote the engine invalidation planner from shadow auditing toward
  authoritative dirty-set ownership for selected safe mutation families
- Extract recalc queue/scheduling policy on top of the completed workbook
  facade and dependency planner
- Expand Calc runtime consumers beyond the current opt-in dependency-shadow
  auditing hooks
- Tighten structural-mutation and named-range mutation parity where scheduler
  extraction exposes gaps

### Long-term: Shift More Execution Authority Out Of Calc

The major extraction prerequisites are now in place: initial function/runtime
extraction, token/compiler-host modeling, standalone compiler switchover,
Calc-backed workbook facade, and dependency/invalidation planning. The
remaining long-horizon work is now concentrated in three areas plus one later
authority decision:

| Area | Current state | Next step |
|------|---------------|-----------|
| Compiler authority inside Calc | Standalone switchover is complete; Calc still uses `ScCompiler` for most production paths | Keep engine-first compile adoption expanding only where the bridge/diff lanes make it safe |
| Recalculation orchestration | Workbook facade plus dependency/invalidation planner are complete in shadow mode | Move dirty-set ownership and recalc scheduling onto engine-owned planner outputs |
| Execution backend | `ScInterpreter` and execution backends are still Calc-owned | Incrementally extract CPU execution logic behind strong differential validation |
| Workbook/storage authority | Calc-backed facade exists and is validated | Defer any authority shift until scheduler and execution layers are stable |

**Recommended sequencing:**

1. **Broaden standalone replay coverage**
   - Enable the add-in family
   - Reduce cached-fallback-heavy paths on the promoted corpus
   - Keep compiled replay and lexical parity maintenance lanes green
2. **Extract recalculation orchestration**
   - Promote the current planner from shadow auditing to authoritative dirty
     planning for selected safe edits
   - Move recalc queue/scheduling policy onto engine-owned planner outputs
3. **Incrementally extract the CPU execution backend**
   - Move token walking, coercion, and evaluation mechanics out of Calc in
     small validated slices
   - Continue broadening live function coverage as part of that extraction
4. **Keep threading and OpenCL as backend adapters**
   - Let Calc continue owning resource/runtime concerns while the engine owns
     more calculation semantics
5. **Decide authority shift timing last**
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
- [BASIC_FODS_SUPPORT.md](architecture/BASIC_FODS_SUPPORT.md) — FODS loader/
  evaluator architecture and implementation checklist
- [TOKEN_COMPILER_HOST_MODEL.md](architecture/TOKEN_COMPILER_HOST_MODEL.md) —
  token model and compiler-host milestone plan with execution tracker
- [COMPILER_SWITCHOVER.md](architecture/COMPILER_SWITCHOVER.md) — compiler
  switchover plan with phased execution checklist
- [CALC_BACKED_WORKBOOK_FACADE.md](architecture/CALC_BACKED_WORKBOOK_FACADE.md) —
  workbook facade milestone plan with phased execution checklist
- [DEPENDENCY_INVALIDATION_EXTRACTION.md](architecture/DEPENDENCY_INVALIDATION_EXTRACTION.md) —
  dependency snapshot and invalidation-planner milestone plan with closeout
  status
- [CALC_ENGINE_AUDIT.md](extraction-history/CALC_ENGINE_AUDIT.md) — audit of
  original Calc engine source files
