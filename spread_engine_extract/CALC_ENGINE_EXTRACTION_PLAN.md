# Spreadsheet Engine Extraction and Standalone Package Plan

This document is now the single source of truth for the remaining work under
`spread_engine_extract/`.

It combines the earlier Calc extraction roadmap and the earlier standalone
package roadmap into one phased plan so that:

- Calc continues to migrate engine logic out of `sc/`
- the same extracted code becomes independently buildable from
  `spread_engine_extract/`
- validation grows in both environments at the same time

## Overall Goal

The end state is a highly focused spreadsheet engine module that:

- owns the spreadsheet-specific compiler, evaluator, matrix, lookup,
  dependency, and recalculation logic needed by Calc
- builds in LibreOffice mode and is linked into `sc`
- also builds in standalone mode from `spread_engine_extract/` alone
- has its own engine-owned public API and does not require `sc/`, `formula/`,
  or LibreOffice runtime libraries when built standalone

This is still an incremental extraction, not a rewrite. The work should
continue in small slices that leave Calc behavior stable and preserve a working
standalone build as the engine grows.

## Boundary Rules

- In scope for `spreadsheetengine`:
  - spreadsheet-specific formula compilation and evaluation
  - copied `formula/` pieces that Calc needs in order to become self-contained
  - matrix and array execution support
  - lookup/query helpers used by formula execution
  - reference/update logic
  - dependency and recalculation algorithms that can be expressed behind host
    interfaces
  - a minimal standalone workbook/runtime host
- Out of scope for `spreadsheetengine`:
  - Calc UI, shell, dialogs, view state
  - import/export filters
  - UNO wrappers and drawing-layer integration
  - non-Calc consumers that still use the original `formula/` module
- Transitional rule:
  - temporary duplication with `formula/` is acceptable
  - Calc should move to the copied implementation under
    `spread_engine_extract/` as soon as practical
  - other LibreOffice applications may continue using the old `formula/`
    module until later cleanup

## Operating Principles

- Keep one core implementation and add thin LibreOffice and standalone adapters
  around it.
- Prefer engine-owned types over leaking `OUString`, `Date`,
  `rtl_math_RoundingMode`, `formula::FormulaGrammar`, `OpCode`, or `Sc*`
  vocabulary into new interfaces.
- Do not move giant classes such as `ScDocument`, `ScTable`, `ScColumn`, or
  `ScFormulaCell` wholesale. Shrink them into hosts by extracting smaller
  services first.
- Every extraction slice should answer two questions:
  - how does Calc call this?
  - how can standalone tests call this?
- Shared parity data should grow alongside extraction so behavior can be
  compared in both modes.

## Current Status

### Completed foundations

- Extraction Phase 0 completed:
  - `Module_spread_engine_extract.mk` and `Library_spreadsheetengine.mk` exist
  - `sc/Library_sc.mk` depends on `spreadsheetengine`
  - the first bridge seam and copied-formula staging area exist
  - `run_spreadsheet_unit_tests.sh` supports smoke and broader engine profiles
- Extraction Phase 1 started:
  - copied `FormulaGrammar` helper logic now lives under
    `source/compat/formula/`
- Extraction Phase 2 started:
  - pure Calc-config helper logic now lives in `source/core/CalcConfig.cxx`
- Extraction Phase 3 started:
  - `MatrixOperators` implementation now lives in `source/core/`
- Extraction Phase 4 started:
  - Calc compiler char-table construction now lives in
    `source/core/CompilerSupport.cxx`
- Extraction Phase 5 is substantially complete for low-coupling families:
  - rounding, scalar math, transcendental math, financial math, bitwise
  - numeral conversion
  - scalar text, case shaping, width conversion
  - date parts, week logic, workday/networkdays logic

### Completed standalone foundations

- Standalone Phase A completed:
  - root `CMakeLists.txt` exists
  - source manifests exist under `cmake/`
  - standalone shim headers exist under `standalone/include/`
  - standalone smoke tests build and run
- Standalone Phase B completed for the low-coupling math families:
  - engine-owned base API types exist under `inc/spreadsheetengine/api/`
  - a standalone-facing math API exists in `api/Math.hxx`
  - LibreOffice adapters exist under `compat/libreoffice/`
  - standalone tests run against the API layer rather than internal headers
- Standalone suite structure improved:
  - the previous monolithic standalone API test binary has been split into
    area-specific executables
  - standalone CTest now reports:
    - `spreadsheetengine_smoke`
    - `spreadsheetengine_compiler_tests`
    - `spreadsheetengine_math_tests`
    - `spreadsheetengine_calendar_tests`
    - `spreadsheetengine_text_tests`
    - `spreadsheetengine_config_tests`
- Phase 6 is now substantially complete for the portable helper lane:
  - `NumeralConversion` now uses engine-owned string types instead of
    `OUString` in its core interface
  - a standalone-facing numeral API now exists in `api/Numeral.hxx`
  - Calc keeps the existing interpreter behavior through string adapters at the
    boundary
  - the standalone source manifest now treats `NumeralConversion.cxx` as
    standalone-ready
  - a first shared parity seed now exists at
    `tests/shared_cases/numeral_conversion_cases.tsv`
  - `DateTimeWorkday` now uses engine-owned date serial and string types in its
    core interface instead of `OUString` and `tools/date.hxx` day constants
  - a standalone-facing workday API now exists in `api/Workday.hxx`
  - the standalone source manifest now treats `DateTimeWorkday.cxx` as
    standalone-ready
  - a shared parity seed now also exists at
    `tests/shared_cases/workday_cases.tsv`
  - `DateTimeWeek` and `DateTimeParts` now use engine-owned date parts and
    string-view types in their core interfaces instead of `Date` and `OUString`
  - a copied internal Gregorian calendar helper now lives in
    `source/core/DateAlgorithms.hxx` and is shared by the extracted date
    helper families
  - a standalone-facing calendar API now exists in `api/Calendar.hxx`
  - the standalone source manifest now treats `DateTimeWeek.cxx` and
    `DateTimeParts.cxx` as standalone-ready
  - a shared parity seed now also exists at
    `tests/shared_cases/calendar_cases.tsv`
  - `TextScalar`, `TextCase`, and `TextWidth` now use engine-owned string
    types plus extracted service interfaces instead of direct LibreOffice
    runtime dependencies
  - LibreOffice-backed text adapters now live under
    `compat/libreoffice/TextServices.hxx`
  - a standalone-facing text API now exists in `api/Text.hxx`
  - the standalone source manifest now treats `TextScalar.cxx`,
    `TextCase.cxx`, and `TextWidth.cxx` as standalone-ready
  - a shared parity seed now also exists at `tests/shared_cases/text_cases.tsv`
  - the Phase 6 validation lane has been exercised successfully in both modes:
    - standalone `spreadsheetengine_smoke`
    - standalone `spreadsheetengine_math_tests`
    - standalone `spreadsheetengine_calendar_tests`
    - standalone `spreadsheetengine_text_tests`
    - LibreOffice `CppunitTest_sc_text_functions_test`
    - LibreOffice `CppunitTest_sc_datetime_functions_test`
    - LibreOffice `CppunitTest_sc_spreadsheet_functions_test`
    - LibreOffice `CppunitTest_sc_ucalc`
- Phase 7 started:
  - `CompilerSupport` no longer exposes `compiler.hxx`, `ScCharFlags`, or
    `formula::FormulaGrammar::AddressConvention`
  - engine-owned compiler char flags now live in `api/Compiler.hxx`
  - `CompilerSupport` now uses engine-owned `api::AddressConvention`
  - Calc consumes that slice through a thin conversion shim in
    `sc/source/core/tool/compiler.cxx`
  - the standalone source manifest now treats `CompilerSupport.cxx` as
    standalone-ready
  - the first compiler-support standalone assertions now run in
    `tests/standalone/compiler_api_tests.cxx`
  - the first Phase 7 slice has been validated successfully in both modes:
    - standalone `spreadsheetengine_smoke`
    - standalone `spreadsheetengine_compiler_tests`
    - LibreOffice `CppunitTest_sc_ucalc_formula`
    - LibreOffice `CppunitTest_sc_ucalc_formula2`
    - LibreOffice `CppunitTest_sc_ucalc_range`
    - LibreOffice `CppunitTest_sc_spreadsheet_functions_test`
  - the second Phase 7 slice has started on the config side:
    - force-calculation parsing now lives in a standalone-safe config seam
      under `core/ForceCalculation.hxx` and `source/core/ForceCalculation.cxx`
    - engine-owned config vocabulary now includes `api::ForceCalculationMode`
    - `CalcConfig` retains the opcode-set helpers while delegating the
      environment parsing path to the new seam
    - standalone coverage now includes `spreadsheetengine_config_tests`
    - the combined compiler/config slice has been validated successfully in
      both modes:
      - standalone all six standalone tests passed
      - LibreOffice `CppunitTest_sc_ucalc_formula`
      - LibreOffice `CppunitTest_sc_ucalc_formula2`
      - LibreOffice `CppunitTest_sc_ucalc_range`
      - LibreOffice `CppunitTest_sc_spreadsheet_functions_test`
      - LibreOffice `CppunitTest_sc_ucalc`

### Current validation lanes

- LibreOffice fast gate:
  - `./spread_engine_extract/run_spreadsheet_unit_tests.sh`
- LibreOffice targeted broader gates:
  - `CppunitTest_sc_ucalc*`
  - function-family tests
  - targeted matrix/compiler/runtime tests by slice
- Standalone gate:
  - `cmake -S spread_engine_extract -B /tmp/spreadsheetengine-standalone-build`
  - `cmake --build /tmp/spreadsheetengine-standalone-build`
  - `ctest --test-dir /tmp/spreadsheetengine-standalone-build --output-on-failure`

## Unified Workstreams

From this point forward, every phase should advance all three workstreams
together:

1. Calc extraction
   - move logic out of `sc/` or copied `formula/` seams into
     `spread_engine_extract/`
2. Standalone hardening
   - remove LibreOffice and Calc type leakage from the newly extracted slice
3. Dual validation
   - keep Calc tests green
   - add or expand standalone tests and shared parity cases for the slice

## Mapping From The Old Plans

- Old extraction Phase 0 through Phase 4 are completed or in progress and are
  now treated as foundations already laid.
- Old extraction Phase 5 remains active, but only for the more coupled
  interpreter families.
- Old standalone Phase A and Phase B are completed and now treated as the base
  of the standalone track.
- The remaining work below combines:
  - the rest of old extraction Phase 5
  - the unfinished parts of old extraction Phases 1 through 4
  - old standalone Phases C through F

## Unified Remaining Roadmap

### Phase 6: Portable Helper Completion And Parity Lane

Objective:

- finish making the already extracted low-coupling helper families portable,
  directly testable, and parity-checked in both build modes

Work:

- move the remaining helper families off leaked LibreOffice types:
  - `NumeralConversion`
  - `DateTimeParts`
  - `DateTimeWeek`
  - `DateTimeWorkday`
  - `TextScalar`
  - `TextCase`
  - `TextWidth`
- introduce service interfaces for environment-specific behavior:
  - case mapping
  - width conversion/transliteration
  - locale-aware number parsing where needed
- keep LibreOffice-backed adapters in `compat/libreoffice/`
- add standalone implementations where the behavior can already be supported
- create shared parity datasets for helper families that now run in both modes

Validation:

- standalone:
  - `spreadsheetengine_smoke`
  - `spreadsheetengine_math_tests`
  - `spreadsheetengine_calendar_tests`
  - `spreadsheetengine_text_tests`
  - dedicated tests for migrated text/date/numeral helpers
- LibreOffice:
  - `CppunitTest_sc_text_functions_test`
  - `CppunitTest_sc_datetime_functions_test`
  - `CppunitTest_sc_spreadsheet_functions_test`
  - `CppunitTest_sc_ucalc`

Exit criteria:

- helper families above compile in standalone mode without `sc/` headers
- helper APIs use engine-owned types
- parity datasets exist for the migrated behavior

Status:

- substantially complete
- the remaining richer locale-aware parsing and document-aware text behavior
  belongs with the Phase 9 host-runtime and coupled-interpreter work, not with
  this low-coupling helper lane

### Phase 7: Own Formula, Compiler, And Config Primitives

Objective:

- remove direct `formula/` and Calc-header dependencies from the early
  compiler/config stack

Work:

- expand the copied `formula` compatibility layer under `source/compat/formula/`
- introduce engine-owned grammar enums and bit layout
- introduce engine-owned opcode/config vocabulary needed by extracted helpers
- remove `CompilerSupport` dependence on `compiler.hxx` and `ScCharFlags`
- remove `CalcConfig` dependence on `formula` opcode headers and Calc enums
- switch additional Calc compiler/config call sites to the copied engine-owned
  primitives

Execution approach:

- slice 1:
  - isolate pure compiler char-table data behind engine-owned conventions and
    bit flags
  - keep Calc on a one-file conversion shim
  - make the slice standalone-buildable immediately
- slice 2:
  - extract Calc-config force-calculation parsing from formula/opcode
    dependencies where possible
  - introduce engine-owned opcode identifiers or symbolic config vocabulary for
    OpenCL/threading subsets
- slice 3:
  - widen the copied grammar/config layer just enough to let compiler/config
    helpers stop depending on `formula/` headers
  - retarget additional Calc compiler/config call sites once the vocabulary is
    owned

Validation:

- standalone:
  - compiler/config unit tests
  - parity tests for grammar/config behavior where feasible
- LibreOffice:
  - `CppunitTest_sc_ucalc_formula`
  - `CppunitTest_sc_ucalc_formula2`
  - `CppunitTest_sc_ucalc_range`
  - `CppunitTest_sc_spreadsheet_functions_test`

Exit criteria:

- no standalone-exposed compiler/config header includes `formula/` or `sc/inc`
- Calc compiler seams use engine-owned or copied primitives at the boundary

Status:

- started
- standalone suite now split into six independently reported tests
- compiler-support slice complete and validated
- force-calculation config slice complete and validated
- next logical slice is the Calc-config/opcode symbolic-vocabulary seam

### Phase 8: Matrix And Execution Substrate Ownership

Objective:

- turn the runtime substrate into engine-owned code instead of Calc-owned code
  with extracted leaf helpers

Work:

- build on the existing `MatrixOperators` move
- extract and own the remaining matrix/runtime substrate:
  - `scmatrix`
  - `jumpmatrix`
  - `interpretercontext`
- introduce engine-owned matrix value, accumulator, and jump/reference support
  types
- keep Calc-side adapters thin and mechanical

Validation:

- standalone:
  - matrix and runtime-substrate tests
- LibreOffice:
  - `CppunitTest_sc_array_functions_test`
  - `CppunitTest_sc_mathematical_functions_test`
  - `CppunitTest_sc_spreadsheet_functions_test`
  - `CppunitTest_sc_ucalc`

Exit criteria:

- matrix and execution-local runtime support live primarily in
  `spread_engine_extract/`
- Calc consumes that substrate through stable adapter seams

### Phase 9: Minimal Host Runtime And Coupled Interpreter Families

Objective:

- give the engine a small host model so the remaining interpreter logic can be
  extracted without depending directly on Calc document classes

Work:

- define host interfaces for:
  - workbook and sheet access
  - cell value and reference resolution
  - lookup/query hooks
  - locale/time services needed by interpreter families
- implement a tiny in-memory standalone workbook host
- continue extracting the remaining interpreter families in this order:
  - logical and control-flow helpers
  - richer text and locale-aware value/date parsing
  - lookup/query helpers
  - array-aware and matrix-aware helpers
  - document-coupled evaluator helpers that can now target host interfaces

Validation:

- standalone:
  - host-runtime tests
  - evaluator parity cases against the in-memory host
- LibreOffice:
  - `CppunitTest_sc_logical_functions_test`
  - `CppunitTest_sc_text_functions_test`
  - `CppunitTest_sc_spreadsheet_functions_test`
  - `CppunitTest_sc_ucalc`
  - targeted function-family tests for each extracted slice

Exit criteria:

- nontrivial evaluator slices can execute against both Calc and the standalone
  host
- remaining interpreter extractions depend on host interfaces, not raw
  `ScDocument` access

### Phase 10: References, Dependencies, And Recalculation

Objective:

- extract the graph, reference, and recalculation logic that still makes Calc
  the real owner of engine execution

Work:

- extract pure value and cache types that are still pending or only partly done:
  - `formularesult`
  - `refdata`
  - `lookupcache`
  - pure pieces of `queryevaluator`
- move reference update and dependency algorithms behind engine-owned services
- begin shrinking the `ScFormulaCell` orchestration role by relocating reusable
  evaluator/dependency logic into the engine
- extract shared-formula and group-evaluation helpers as engine services where
  feasible

Validation:

- standalone:
  - reference/update/dependency tests against the minimal host
  - parity cases for caches and lookup behavior
- LibreOffice:
  - `CppunitTest_sc_ucalc_sharedformula`
  - `CppunitTest_sc_cache_test`
  - `CppunitTest_sc_parallelism`
  - `CppunitTest_sc_ucalc`
  - `CppunitTest_sc_ucalc_copypaste`
  - `CppunitTest_sc_ucalc_sort`

Exit criteria:

- dependency and reference-update logic primarily live in the engine
- Calc owns document persistence and shell behavior, but not the core recalc
  algorithms

### Phase 11: Calc Cleanup And Standalone Package Hardening

Objective:

- finish the split so Calc is clearly a host of the engine and the standalone
  package is independently consumable

Work:

- reduce remaining Calc-side wrappers to host and adapter code
- collapse transitional compatibility layers that are no longer needed
- formalize the standalone package surface:
  - public headers
  - documented build entry points
  - standalone test commands
  - explicit external dependencies
- decide whether copied `formula/` code stays duplicated or can be cleaned up
  after Calc is fully isolated

Validation:

- standalone:
  - clean standalone build from `spread_engine_extract/` alone
  - full standalone test suite and parity suite
- LibreOffice:
  - full spreadsheet engine gate
  - milestone builds of `spreadsheetengine` and `sc`

Exit criteria:

- `spread_engine_extract/` builds as a genuine standalone package
- Calc links the engine as a host consumer instead of owning the engine logic
- the dual-build and dual-test model is routine rather than exceptional

## Per-Iteration Checklist

Every implementation slice should do all of the following:

1. Keep or improve the Calc extraction seam.
2. Decide whether the changed code belongs in:
   - engine core
   - LibreOffice adapter
   - standalone adapter
3. Add or update engine-owned API types instead of leaking new LibreOffice
   types.
4. Add standalone tests if the slice is standalone-capable.
5. Add shared parity cases if the same behavior can be exercised in both modes.
6. Run the relevant LibreOffice and standalone gates before moving to the next
   slice.

## Recommended Near-Term Sequence

The most practical next path is:

1. Phase 6:
   - closed enough to treat as a maintained validation lane rather than the
     main implementation focus
2. Phase 7:
   - own the remaining formula/compiler/config primitives that still leak
     `formula/` and Calc vocabulary
3. Phase 8:
   - continue the matrix/runtime substrate extraction on top of the existing
     `MatrixOperators` move
4. Phase 9:
   - introduce the minimal host runtime and resume the more coupled
     interpreter-family extraction

That order keeps both tracks moving in the same direction: each extraction
slice becomes easier to test in standalone mode, and each standalone milestone
reduces the risk of the next Calc extraction slice.
