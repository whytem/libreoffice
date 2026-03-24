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
- OpenCL backend rule:
  - standalone feature parity does not require the OpenCL execution backend
  - OpenCL may be preserved as optional config vocabulary and an optional host
    acceleration backend
  - CPU execution remains the required authoritative implementation for
    standalone parity

## Operating Principles

- Keep one core implementation and add thin LibreOffice and standalone adapters
  around it.
- Prefer engine-owned types over leaking `OUString`, `Date`,
  `rtl_math_RoundingMode`, `formula::FormulaGrammar`, `OpCode`, or `Sc*`
  vocabulary into new interfaces.
- Treat acceleration backends such as OpenCL as optional adapters unless they
  implement unique spreadsheet semantics. Config compatibility may still be
  preserved even when the backend itself is deferred.
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
  - the third Phase 7 slice has completed on the symbolic opcode-string seam:
    - `CalcConfig.hxx` and `CalcConfig.cxx` no longer expose
      `formula/opcode.hxx`, `o3tl::sorted_vector`, or `FormulaOpCodeSet`
    - engine-owned symbolic opcode tokenization now lives in
      `core::SymbolicOpCodeList`,
      `symbolicOpCodeListToString()`, and `stringToSymbolicOpCodeList()`
    - `CalcConfig.cxx` is now standalone-ready and has moved into the
      standalone core source manifest
    - Calc now owns the `FormulaCompiler` and `OpCode` mapping step locally in
      `sc/source/core/tool/calcconfig.cxx`
    - standalone config coverage now includes symbolic opcode tokenization and
      formatting checks
    - this slice has been validated successfully in both modes:
      - standalone all six standalone tests passed
      - LibreOffice `CppunitTest_sc_ucalc_formula`
      - LibreOffice `CppunitTest_sc_ucalc_formula2`
      - LibreOffice `CppunitTest_sc_ucalc_range`
      - LibreOffice `CppunitTest_sc_spreadsheet_functions_test`
      - LibreOffice `CppunitTest_sc_ucalc`
  - the OpenCL backend has been explicitly scoped as optional for standalone
    parity:
    - standalone should preserve compatible config behavior where useful
    - standalone does not need to ship the OpenCL execution backend itself in
      order to match spreadsheet functionality
  - the fourth Phase 7 slice has completed on the default symbolic OpenCL
    subset seam:
    - `spreadsheetengine` now owns the default symbolic subset vocabulary for
      the OpenCL-related config surface
    - Calc no longer hardcodes the default subset as raw `OpCode` enums in
      `setOpenCLConfigToDefault()`
    - standalone config coverage now verifies the default symbolic subset
      content as part of `spreadsheetengine_config_tests`
    - this slice has been validated successfully in both modes:
      - standalone all six standalone tests passed
      - LibreOffice `CppunitTest_sc_ucalc_formula`
      - LibreOffice `CppunitTest_sc_ucalc_formula2`
      - LibreOffice `CppunitTest_sc_ucalc_range`
      - LibreOffice `CppunitTest_sc_spreadsheet_functions_test`
      - LibreOffice `CppunitTest_sc_ucalc`
  - the fifth Phase 7 slice has completed on the copied grammar seam:
    - `compat/formula/FormulaGrammar` now uses engine-owned
      `api::FormulaLanguage`, `api::AddressConvention`, and `api::Grammar`
      instead of exposing `::formula::FormulaGrammar`
    - Calc now converts between engine grammar types and legacy
      `::formula::FormulaGrammar` at the boundary in
      `compiler.cxx`, `rangeutl.cxx`, and `tokenstringcontext.cxx`
    - `source/compat/formula/FormulaGrammar.cxx` has moved into the
      standalone-ready source manifest
    - standalone compiler coverage now exercises the extracted grammar helper
      behavior directly
    - this slice has been validated successfully in both modes:
      - standalone all six standalone tests passed
      - LibreOffice `CppunitTest_sc_ucalc_formula`
      - LibreOffice `CppunitTest_sc_ucalc_formula2`
      - LibreOffice `CppunitTest_sc_ucalc_range`
      - LibreOffice `CppunitTest_sc_spreadsheet_functions_test`
      - LibreOffice `CppunitTest_sc_ucalc`
  - the sixth Phase 7 slice has completed on the config opcode vocabulary
    seam:
    - engine-owned config opcode identifiers now live in
      `api::ConfigOpCodeSymbol`
    - `CalcConfig` now owns typed config opcode names and the default OpenCL
      subset as engine-owned data instead of raw string literals
    - Calc now maps the engine-owned config symbols to `OpCode` locally in
      `sc/source/core/tool/calcconfig.cxx`
    - the known config vocabulary no longer requires `FormulaCompiler` for
      parsing or formatting at the Calc boundary
    - `FormulaCompiler` remains only as a compatibility fallback for config
      tokens and opcodes outside the currently engine-owned subset
    - standalone config coverage now exercises the typed config opcode
      vocabulary directly
    - this slice has been validated successfully in both modes:
      - standalone all six standalone tests passed
      - LibreOffice `CppunitTest_sc_ucalc_formula`
      - LibreOffice `CppunitTest_sc_ucalc_formula2`
      - LibreOffice `CppunitTest_sc_ucalc_range`
      - LibreOffice `CppunitTest_sc_spreadsheet_functions_test`
      - LibreOffice `CppunitTest_sc_ucalc`
  - the seventh Phase 7 slice has completed on the schema-compatible config
    token seam:
    - the engine-owned config opcode vocabulary now models both legacy and
      modern statistical token families where Calc historically distinguishes
      them
    - canonical config token formatting now matches the persisted schema
      surface for the OpenCL subset, including operator tokens such as `+`,
      `-`, `*`, `/`, and `^`
    - the default OpenCL subset now uses the historic Calc token set and
      legacy opcode choices again, instead of the temporary placeholder token
      spellings introduced during extraction
    - the Calc formatter now canonicalizes duplicate known config symbols such
      as unary and binary minus to avoid duplicated emitted tokens for the
      engine-owned subset
    - `FormulaCompiler` remains only as a fallback for config tokens outside
      the currently copied lookup table
    - standalone config coverage now verifies canonical config-token output and
      legacy-versus-modern statistical token parsing
    - this slice has been validated successfully in both modes:
      - standalone all six standalone tests passed
      - LibreOffice `CppunitTest_sc_ucalc_formula`
      - LibreOffice `CppunitTest_sc_ucalc_formula2`
      - LibreOffice `CppunitTest_sc_ucalc_range`
      - LibreOffice `CppunitTest_sc_spreadsheet_functions_test`
      - LibreOffice `CppunitTest_sc_ucalc`
  - the eighth Phase 7 slice has completed on the broader English config
    lookup seam:
    - the engine-owned config lookup table now covers the conditional
      aggregate and lookup family around the existing OpenCL subset surface
    - `COUNTIF`, `SUMIF`, `AVERAGEIF`, `COUNTIFS`, `AVERAGEIFS`, `MATCH`,
      `XMATCH`, `LOOKUP`, `HLOOKUP`, and `XLOOKUP` now round-trip through the
      engine-owned config vocabulary without using `FormulaCompiler`
    - Calc still performs only the local `ConfigOpCodeSymbol` to `OpCode`
      translation at the boundary for these tokens
    - the remaining `FormulaCompiler` fallback in `calcconfig.cxx` is now
      limited to a smaller tail of less-common English opcode names outside the
      copied lookup table
    - standalone config coverage now exercises the new lookup-family symbols
    - this slice has been validated successfully in both modes:
      - standalone all six standalone tests passed
      - LibreOffice `CppunitTest_sc_ucalc_formula`
      - LibreOffice `CppunitTest_sc_ucalc_formula2`
      - LibreOffice `CppunitTest_sc_ucalc_range`
      - LibreOffice `CppunitTest_sc_spreadsheet_functions_test`
      - LibreOffice `CppunitTest_sc_ucalc`
  - the ninth Phase 7 slice has completed on the financial config lookup seam:
    - the engine-owned config lookup table now covers a substantial financial
      family in addition to the earlier lookup and conditional-aggregate
      groups
    - `PV`, `SYD`, `DDB`, `DB`, `VDB`, `PDURATION`, `SLN`, `PMT`, `RRI`, `FV`,
      `NPER`, `RATE`, `IPMT`, `PPMT`, `CUMIPMT`, `CUMPRINC`, `EFFECT`,
      `NOMINAL`, and `ISPMT` now round-trip through the engine-owned config
      vocabulary without using `FormulaCompiler`
    - Calc still performs only the local `ConfigOpCodeSymbol` to `OpCode`
      translation at the boundary for these tokens, including the historical
      `VDB` to `ocVBD` compatibility mapping
    - the remaining `FormulaCompiler` fallback in `calcconfig.cxx` is now
      reduced further to the long tail of English opcode names outside the
      currently copied engine-owned tables
    - standalone config coverage now exercises representative financial config
      symbols and list-formatting behavior
    - this slice has been validated successfully in both modes:
      - standalone all six standalone tests passed
      - LibreOffice `CppunitTest_sc_ucalc_formula`
      - LibreOffice `CppunitTest_sc_ucalc_formula2`
      - LibreOffice `CppunitTest_sc_ucalc_range`
      - LibreOffice `CppunitTest_sc_spreadsheet_functions_test`
      - LibreOffice `CppunitTest_sc_ucalc`
  - the tenth Phase 7 slice has completed on the descriptive-statistics and
    regression config lookup seam:
    - the engine-owned config lookup table now covers a larger statistical
      family around descriptive aggregates and linear-regression helpers
    - `SUMSQ`, `AVERAGEA`, `VARA`, `VARP`, `VARPA`, `STDEV`, `STDEVA`,
      `STDEVP`, `STDEVPA`, `GEOMEAN`, `HARMEAN`, `AVEDEV`, `DEVSQ`, `MEDIAN`,
      `KURT`, `SKEW`, `SKEWP`, `ZTEST`, `RSQ`, `STEYX`, `INTERCEPT`, and
      `FORECAST` now round-trip through the engine-owned config vocabulary
      without using `FormulaCompiler`
    - Calc still performs only the local `ConfigOpCodeSymbol` to `OpCode`
      translation at the boundary for these tokens
    - the remaining `FormulaCompiler` fallback in `calcconfig.cxx` is now
      concentrated further into the remaining long tail such as modern dotted
      statistical names, database families, and other less-common English
      opcode names not yet copied into the engine-owned tables
    - standalone config coverage now exercises representative descriptive and
      regression symbols and mixed list-formatting behavior
    - this slice has been validated successfully in both modes:
      - standalone all six standalone tests passed
      - LibreOffice `CppunitTest_sc_ucalc_formula`
      - LibreOffice `CppunitTest_sc_ucalc_formula2`
      - LibreOffice `CppunitTest_sc_ucalc_range`
      - LibreOffice `CppunitTest_sc_spreadsheet_functions_test`
      - LibreOffice `CppunitTest_sc_statistical_functions_test`
      - LibreOffice `CppunitTest_sc_ucalc`
  - the eleventh Phase 7 slice has completed on the database aggregate config
    lookup seam:
    - the engine-owned config lookup table now covers the database aggregate
      family used by Calc’s database-function surface
    - `DSUM`, `DCOUNT`, `DCOUNTA`, `DAVERAGE`, `DGET`, `DMAX`, `DMIN`,
      `DPRODUCT`, `DSTDEV`, `DSTDEVP`, `DVAR`, and `DVARP` now round-trip
      through the engine-owned config vocabulary without using
      `FormulaCompiler`
    - Calc still performs only the local `ConfigOpCodeSymbol` to `OpCode`
      translation at the boundary for these tokens
    - standalone config coverage now exercises representative database tokens
      and mixed config list formatting
    - this slice has been validated successfully in both modes:
      - standalone all six standalone tests passed
      - LibreOffice `CppunitTest_sc_database_functions_test`
      - LibreOffice `CppunitTest_sc_ucalc_formula`
      - LibreOffice `CppunitTest_sc_spreadsheet_functions_test`
      - LibreOffice `CppunitTest_sc_ucalc`
  - the twelfth Phase 7 slice has completed on the modern statistical
    compatibility-name seam:
    - the engine-owned config vocabulary now covers a wide compatibility layer
      for legacy and modern dotted statistical and inference tokens where Calc
      distinguishes different opcodes
    - this includes the `VAR.P` / `VAR.S`, `STDEV.P` / `STDEV.S`, `NORMINV` /
      `NORM.INV`, `LOGNORMDIST` / `LOGNORM.DIST`, `LOGINV` / `LOGNORM.INV`,
      `TDIST` / `T.DIST` / `T.DIST.RT` / `T.DIST.2T`, `FDIST` / `F.DIST` /
      `F.DIST.RT`, `CHIDIST` / `CHISQ.DIST.RT`, `CHIINV` / `CHISQ.INV.RT`,
      `CHISQDIST` / `CHISQ.DIST`, `CHISQINV` / `CHISQ.INV`, `GAMMADIST` /
      `GAMMA.DIST`, `GAMMAINV` / `GAMMA.INV`, `TINV` / `T.INV` / `T.INV.2T`,
      `FINV` / `F.INV` / `F.INV.RT`, and `ZTEST` / `Z.TEST`, plus `TTEST` /
      `T.TEST` and `FTEST` / `F.TEST`
    - Calc still performs only the local `ConfigOpCodeSymbol` to `OpCode`
      translation at the boundary for these tokens
    - the remaining `FormulaCompiler` fallback in `calcconfig.cxx` is now
      mostly limited to a cleanup tail of less-common English opcode names
      outside the currently copied config families
    - standalone config coverage now exercises representative dotted
      compatibility names and mixed config list formatting
    - this slice has been validated successfully in both modes:
      - standalone all six standalone tests passed
      - LibreOffice `CppunitTest_sc_database_functions_test`
      - LibreOffice `CppunitTest_sc_statistical_functions_test`
      - LibreOffice `CppunitTest_sc_ucalc_formula`
      - LibreOffice `CppunitTest_sc_ucalc_formula2`
      - LibreOffice `CppunitTest_sc_ucalc_range`
      - LibreOffice `CppunitTest_sc_spreadsheet_functions_test`
      - LibreOffice `CppunitTest_sc_ucalc`
  - the thirteenth Phase 7 slice has completed on the vectorized long-tail
    config seam:
    - the engine-owned config vocabulary now covers the remaining plain-English
      function names used by Calc’s vectorized/configurable execution surface
    - this includes the cleanup families around scalar math and rounding,
      trigonometric and hyperbolic helpers, combinatorics and distributions,
      logical and bitwise functions, database-adjacent aggregates like
      `COUNTA` / `MINA` / `MAXA`, financial helpers like `NPV` / `IRR` /
      `MIRR`, dynamic-array and reshape functions like `FILTER`, `SORT`,
      `SEQUENCE`, `TEXTSPLIT`, `TOCOL`, `TOROW`, `UNIQUE`, `WRAPCOLS`,
      `WRAPROWS`, and the final uncovered token `TRUNC`
    - a direct diff against the vectorization support switch in
      `sc/source/core/tool/token.cxx` now shows no remaining opcodes outside
      the engine-owned config mapping in `sc/source/core/tool/calcconfig.cxx`
    - `FormulaCompiler` remains only as a compatibility path for non-core
      residual names and arbitrary tokens outside the copied engine-owned
      tables, not for the normal vectorized/configurable Calc function surface
    - standalone config coverage now exercises representative legacy, dotted,
      database, dynamic-array, and scalar cleanup tokens in mixed list
      formatting
    - this slice has been validated successfully in both modes:
      - standalone all six standalone tests passed
      - LibreOffice `CppunitTest_sc_mathematical_functions_test`
      - LibreOffice `CppunitTest_sc_database_functions_test`
      - LibreOffice `CppunitTest_sc_statistical_functions_test`
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

- complete
- standalone suite now split into six independently reported tests
- compiler-support slice complete and validated
- force-calculation config slice complete and validated
- symbolic opcode-string config slice complete and validated
- default symbolic OpenCL subset config slice complete and validated
- copied grammar seam complete and validated
- typed config opcode vocabulary seam complete and validated
- schema-compatible config token seam complete and validated
- broader English config lookup seam complete and validated
- financial config lookup seam complete and validated
- descriptive-statistics and regression config lookup seam complete and
  validated
- database aggregate config lookup seam complete and validated
- modern statistical compatibility-name seam complete and validated
- vectorized long-tail config cleanup seam complete and validated
- exit criteria satisfied for the copied compiler/config vocabulary needed by
  the extracted Calc engine and standalone parity track
- standalone-facing compiler/config headers no longer depend on `formula/` or
  `sc/inc`; the remaining `formula/` include is confined to the LibreOffice
  adapter header in `compat/libreoffice/`
- any remaining `FormulaCompiler` fallback in `calcconfig.cxx` is now optional
  cleanup for non-core residual names rather than a blocker for the extracted
  engine path

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
- execute the phase in this order instead of attempting a wholesale move:
  - matrix core types and replication/dimension helpers
  - pure `ScMatrix` storage and iteration logic
  - `ScJumpMatrix` runtime state and buffering
  - an `ExecutionContext` split from the execution-local parts of
    `ScInterpreterContext`
  - Calc-only host services such as number formatting, lookup caches, and
    document access remain explicit adapters until Phase 9

Current status:

- first slice completed:
  - engine-owned matrix API/value types now live under
    `spread_engine_extract/inc/spreadsheetengine/api/Matrix.hxx`
  - standalone runtime-substrate coverage now includes
    `spreadsheetengine_matrix_tests`
  - Calc consumes the first extracted matrix helpers through
    `sc/source/core/tool/scmatrix.cxx` for element-count and
    replication/validation behavior without changing the public `ScMatrix`
    interface yet
- second slice completed:
  - engine-owned matrix geometry helpers now live under
    `spread_engine_extract/inc/spreadsheetengine/core/MatrixGeometry.hxx`
  - Calc now consumes extracted allocatability, index-to-coordinate, and
    column-vector placement rules through `sc/source/core/tool/scmatrix.cxx`
  - standalone matrix tests now cover those geometry helpers directly
- third slice completed:
  - the extracted matrix geometry layer now also owns range validation and
    row/column span helpers used by pure matrix write/fill logic
  - Calc consumes those helpers in `sc/source/core/tool/scmatrix.cxx` for
    rectangular fill and vector-write checks without changing matrix storage
    semantics
- fourth slice completed:
  - engine-owned jump-matrix runtime helpers now live under
    `spread_engine_extract/inc/spreadsheetengine/core/JumpMatrixRuntime.hxx`
  - Calc consumes extracted jump-coordinate normalization, cursor advancement,
    result-dimension expansion, and buffered-write threshold checks through
    `sc/source/core/tool/jumpmatrix.cxx`
- fifth slice completed:
  - engine-owned matrix runtime helpers now live under
    `spread_engine_extract/inc/spreadsheetengine/core/MatrixRuntime.hxx`
  - Calc now consumes extracted element-budget and allocatable-shape logic in
    `sc/source/core/tool/scmatrix.cxx` for size checks and matrix-limit
    planning
- sixth slice completed:
  - the jump-matrix runtime layer now also owns buffered-write continuation
    rules
  - Calc consumes those helpers in `sc/source/core/tool/jumpmatrix.cxx` for
    buffer flush decisions without changing result semantics
- seventh slice completed:
  - the extracted matrix runtime layer now also owns allocation planning and
    fallback-shape decisions
  - Calc consumes those helpers in `sc/source/core/tool/scmatrix.cxx` for
    constructor and resize fallback planning while preserving the existing
    error behavior
- eighth slice completed:
  - the jump-matrix runtime layer now also owns result-expansion planning
    including fill ranges and cursor adjustment
  - Calc consumes those helpers in `sc/source/core/tool/jumpmatrix.cxx` for
    result-matrix growth without changing formula results
- ninth slice completed:
  - the extracted matrix runtime layer now also owns default memory-budget
    policy and clone/extend planning helpers
  - Calc consumes those helpers in `sc/source/core/tool/scmatrix.cxx` for
    platform-default matrix limits and clone target sizing while preserving
    existing matrix contents and error behavior
- tenth slice completed:
  - the jump-matrix runtime layer now also owns buffer-window lifecycle
    helpers used when opening and continuing buffered result writes
  - Calc consumes those helpers in `sc/source/core/tool/jumpmatrix.cxx` for
    buffered result accumulation without changing runtime semantics
- eleventh slice completed:
  - the extracted jump-matrix runtime layer now also owns linear jump-entry
    indexing and buffered-write progression helpers
  - Calc consumes those helpers in `sc/source/core/tool/jumpmatrix.cxx` for
    jump entry access and buffer-count progression without changing results
- twelfth slice completed:
  - the extracted jump-matrix runtime layer now also owns buffered-write flush
    decision helpers
  - Calc consumes those helpers in `sc/source/core/tool/jumpmatrix.cxx` to
    decide when buffered result ranges must be flushed without changing result
    ordering or values
- thirteenth slice completed:
  - the extracted matrix runtime layer now also owns resize-budget accounting
    and resize planning helpers
  - Calc consumes those helpers in `sc/source/core/tool/scmatrix.cxx` for
    resize allocation planning while preserving existing fallback and error
    behavior
- fourteenth slice completed:
  - the extracted matrix runtime layer now also owns copy-destination
    compatibility checks for matrix storage copies
  - Calc consumes those helpers in `sc/source/core/tool/scmatrix.cxx` for
    `MatCopy` destination-size validation without changing matrix contents
- fifteenth slice completed:
  - the extracted jump-matrix runtime layer now also owns buffered result-write
    planning helpers that combine threshold gating with same-type window
    progression
  - Calc consumes those helpers in `sc/source/core/tool/jumpmatrix.cxx` for
    result buffering decisions without changing buffered write ordering or
    values
- sixteenth slice completed:
  - the extracted matrix runtime layer now also owns matrix budget lifecycle
    bookkeeping for construction and destruction
  - Calc consumes those helpers in `sc/source/core/tool/scmatrix.cxx` for
    `ScMatrixImpl` allocation-budget updates while preserving existing matrix
    size limits and fallback behavior
- seventeenth slice completed:
  - the extracted matrix geometry layer now also owns column-major index
    flattening helpers, including offset-aware indexing
  - Calc consumes those helpers in `sc/source/core/tool/jumpmatrix.cxx` for
    jump-entry indexing and in `sc/source/core/tool/scmatrix.cxx` for
    `MatConcat` result-buffer indexing
- eighteenth slice completed:
  - the extracted matrix geometry layer now also owns range-write planning for
    pure matrix fill operations
  - Calc consumes those helpers in `sc/source/core/tool/scmatrix.cxx` for
    `FillDouble` range validation and span sizing
- nineteenth slice completed:
  - the extracted matrix geometry layer now also owns column-vector write
    planning for pure matrix storage writes
  - Calc consumes those helpers in `sc/source/core/tool/scmatrix.cxx` for the
    vectorized double, string, empty-result, and empty-path write paths
- twentieth slice completed:
  - the extracted execution-context layer now also owns generic token-cache
    reset, scratch cleanup, recent-cache reset, and doc-bound-state clearing
    helpers
  - Calc consumes those helpers in
    `sc/source/core/tool/interpretercontext.cxx` for the first small
    `ScInterpreterContext` split without changing document, formatter, or
    lookup-cache semantics

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
