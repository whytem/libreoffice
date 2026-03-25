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

- Phase 8 is complete.
- the engine now owns the core matrix and execution substrate under
  `spread_engine_extract`, including:
  - public matrix value and dimension types in
    `inc/spreadsheetengine/api/Matrix.hxx`
  - matrix geometry and planning helpers in
    `inc/spreadsheetengine/core/MatrixGeometry.hxx`
  - matrix runtime and stored-element helpers in
    `inc/spreadsheetengine/core/MatrixRuntime.hxx`
  - jump-matrix buffering and cursor helpers in
    `inc/spreadsheetengine/core/JumpMatrixRuntime.hxx`
  - execution-context scratch, token-cache, pool, and recent-cache helpers in
    `inc/spreadsheetengine/core/ExecutionContext.hxx`
- Calc now consumes those helpers through thin adapters in:
  - `sc/source/core/tool/scmatrix.cxx`
  - `sc/source/core/tool/jumpmatrix.cxx`
  - `sc/source/core/tool/interpretercontext.cxx`
  - `sc/source/core/tool/interpr4.cxx`
- the extracted ownership covers the host-independent substrate work that Phase
  8 targeted:
  - matrix dimensioning, replication, coordinate validity, and write planning
  - matrix allocation, resize, clone/extend, budget, and stored-element
    classification rules
  - jump-matrix cursor, growth, buffering, and flush planning
  - interpreter-context scratch cleanup, token reuse, pool lifecycle, and
    formatter-side MRU/cache bookkeeping
- standalone validation for the extracted runtime layer now includes both
  `spreadsheetengine_matrix_tests` and `spreadsheetengine_execution_tests`
  alongside the broader standalone suite
- the remaining `scmatrix` and `interpretercontext` work is no longer mainly
  substrate ownership; it is the host-aware behavior that belongs in Phase 9:
  - matrix-aware evaluator and aggregation behavior
  - compare/query and coercion semantics
  - formatting, lookup-cache, and document-bound services
  - wider public-header cleanup across Calc and formula include graphs

Assessment:

- Phase 8 is complete
- the remaining work in `scmatrix` is no longer mainly substrate ownership;
  it is mostly:
  - matrix-aware evaluator and aggregation behavior
  - compare/query and coercion semantics
  - formatting or string-conversion paths that still rely on Calc host
    services
  - public-header cleanup that crosses wider LibreOffice include graphs
- those residual items are better handled as part of the Phase 9 host/runtime
  transition instead of continuing to treat them as pure substrate extraction
- a final audit of `scmatrix`, `jumpmatrix`, and `interpretercontext` did not
  find another remaining host-independent runtime slice large enough to justify
  a separate Phase 8 iteration

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

Implementation approach:

- execute Phase 9 in narrow passes instead of trying to move `ScInterpreter`
  or `ScDocument` logic wholesale
- pass 1: host contract primitives
  - introduce engine-owned host-facing types for:
    - cell address and sheet identity
    - scalar, string, error, and matrix-backed cell value views
    - resolved references and small result wrappers
  - define minimal service interfaces for:
    - reading a cell or range
    - null-date and locale/time services
    - text conversion and formatting hooks that remain host-bound
  - keep this pass behavior-free: it should only establish types and seams
- pass 2: minimal Calc adapter and standalone in-memory host
  - add a Calc adapter layer that implements the host interfaces over
    `ScDocument`, `ScInterpreterContext`, and the already-extracted runtime
    helpers
  - add a tiny standalone workbook/sheet store that implements the same
    interfaces for parity tests
  - use this pass to prove that nontrivial evaluator code can target the host
    abstraction without changing spreadsheet results yet
- pass 3: logical and control-flow families first
  - start with the smallest coupled families that benefit from host access but
    do not yet need full query or dependency machinery
  - preferred first candidates:
    - `IF`, `IFERROR`, `IFNA`, `CHOOSE`
    - reference-light helpers that mostly need value fetching and lazy branch
      control
  - keep branch selection and visible error behavior identical in Calc while
    moving reusable decision logic into the engine
- pass 4: locale-aware text and date parsing
  - move the spreadsheet-facing parsing helpers that still depend on Calc host
    services:
    - `DATEVALUE`, `TIMEVALUE`
    - richer text-to-number and locale-aware text coercion paths
    - any remaining width/case/encoding-dependent interpreter helpers that now
      fit the host service interfaces
  - validate these slices through both standalone shared cases and Calc formula
    evaluation
- pass 5: lookup/query helpers
  - extract reusable lookup and criteria-evaluation helpers behind explicit
    host callbacks for:
    - value access
    - comparator/query policy
    - range iteration
  - preferred first candidates:
    - `MATCH`
    - `LOOKUP`
    - `VLOOKUP`/`HLOOKUP` subroutines that can be separated from sheet/document
      orchestration
- pass 6: array-aware and document-coupled evaluator helpers
  - move the remaining interpreter pieces that operate on references, arrays,
    or mixed scalar/matrix evaluation once the host contracts are proven
  - keep anything still tied to dependency graphs or formula-cell ownership out
    of Phase 9 and defer it to Phase 10

First logical slice:

- add engine-owned host contract headers and a no-op Calc adapter seam without
  changing formula behavior
- add a standalone `spreadsheetengine_host_tests` target that exercises the
  in-memory host on basic cell/range fetches
- validate that the contracts compile in both standalone and LibreOffice
  builds before moving any interpreter family onto them

Status:

- completed
- implemented in:
  - `spread_engine_extract/inc/spreadsheetengine/api/Host.hxx`
  - `spread_engine_extract/inc/spreadsheetengine/core/InMemoryHost.hxx`
  - `spread_engine_extract/inc/spreadsheetengine/compat/libreoffice/Host.hxx`
  - `spread_engine_extract/inc/spreadsheetengine/core/HostValueAccess.hxx`
  - `spread_engine_extract/tests/standalone/host_api_tests.cxx`
  - `sc/qa/unit/ucalc_shared_cases.cxx`

Current status:

- prerequisites from earlier phases are in place:
  - standalone and LibreOffice builds already exercise shared extracted code
  - shared TSV parity cases now run in both the standalone suite and the new
    Calc-side `CppunitTest_sc_ucalc_shared_cases` target
  - Phase 8 completed the host-independent matrix/runtime substrate, so Phase 9
    can focus on genuinely host-aware evaluator work
- pass 1 is complete:
  - the engine owns workbook/runtime/cell-read host contracts
  - resolved-reference and matrix-backed value-view primitives are defined
  - host-bound text coercion and number-formatting seams are defined
  - the standalone suite exercises those contracts through the in-memory host
  - Calc exercises them through a thin `ScDocument` adapter seam
- pass 2 is now substantially complete:
  - the standalone in-memory host and Calc adapter both implement the same
    host contracts
  - the Calc adapter can now route parse/format behavior through
    `ScInterpreterContext`, not just raw document formatting
  - engine-owned host helper logic in `HostValueAccess.hxx` now resolves
    scalar vs matrix-backed views, fetches reference elements, and performs
    host-driven number coercion/formatting without depending on Calc classes
  - those helpers are exercised in both the standalone suite and the
    Calc-side shared-case target
- pass 3 is now substantially complete for the low-coupling control-flow layer:
  - engine-owned logical helpers now cover reusable decision logic for:
    - `IF`
    - `IFERROR` / `IFNA`
    - `CHOOSE`
    - `IFS`
  - Calc now consumes that helper layer while still owning stack management,
    token movement, and jump-matrix orchestration
  - the standalone suite exercises the decision policies directly through the
    new `spreadsheetengine_logic_tests` target
  - Calc coverage now includes the logical-function gate in addition to the
    shared-case and smoke gates
- the remaining pass 3 leftovers are mostly adapter-side mechanics:
  - jump-matrix setup and result buffering still live in Calc
  - lazy branch execution and token-stack ownership still live in Calc
- pass 4 is now substantially complete for the first spreadsheet-facing parsing slice:
  - the host contract now distinguishes parsed number/date/time/datetime results
    and supports the `LAX_TIME` parse mode needed by `TIMEVALUE`
  - engine-owned parsing helpers now cover spreadsheet-facing `VALUE`,
    `DATEVALUE`, and `TIMEVALUE` behavior over the host interface
  - Calc now consumes those helpers while keeping wider interpreter and
    workbook orchestration local
  - parity coverage now includes a shared TSV dataset for locale-aware parsing
    that runs in both the standalone suite and `CppunitTest_sc_ucalc_shared_cases`
  - the main remaining pass 4 leftovers are broader `ConvertStringToValue()`
    and formula-wide coercion paths that touch more than the direct
    spreadsheet-facing parsing functions
- pass 5 is now substantially complete for the low-coupling lookup/query layer:
  - engine-owned lookup helpers now cover:
    - `MATCH` / `XMATCH` mode normalization
    - reusable search-policy planning for equality, approximate, wildcard, and
      regex lookup modes
    - vector-shape and result-shape validation for lookup-oriented matrix paths
    - index/result planning shared by `LOOKUP`, `VLOOKUP`/`HLOOKUP`, and
      `XLOOKUP`
  - Calc now consumes that lookup planner layer in:
    - `ScMatch()`
    - `ScXMatch()`
    - `ScLookup()`
    - `CalculateLookup()`
    - `ScXLookup()`
    - `SearchVectorForValue()`
  - the standalone suite now exercises the extracted lookup policies directly
    through `spreadsheetengine_lookup_tests`
  - Calc validation now includes the lookup-heavy gates in
    `CppunitTest_sc_ucalc_formula2` and the sorted-range `MATCH` coverage in
    `CppunitTest_sc_ucalc_sort`
- the main remaining pass 5 leftovers are the genuinely host-heavy pieces:
  - comparator and query policy that still depends on Calc iterators and search
    services
  - lookup-cache ownership and reuse
  - broader criteria/query evaluation that is closer to Phase 10 dependency and
    cache work
- pass 6 is now substantially complete for the low-coupling array-aware evaluator layer:
  - engine-owned array helpers now cover reusable planning for:
    - `TAKE` / `DROP`
    - `CHOOSECOLS` / `CHOOSEROWS`
    - `EXPAND`
    - `TOCOL` / `TOROW`
    - `WRAPCOLS` / `WRAPROWS`
    - `HSTACK` / `VSTACK` dimension accumulation
  - Calc now consumes that helper layer in the dynamic-array reshaping and
    selection functions in `interpr1.cxx`, while still owning cell copying,
    token-stack orchestration, and matrix/reference materialization
  - standalone coverage now includes `spreadsheetengine_array_tests`
  - Calc-side coverage now includes direct dynamic-array formula cases in
    `CppunitTest_sc_ucalc_shared_cases`, in addition to the broader `ucalc`
    and smoke gates
- a narrow document-coupled follow-up slice is now also in place:
  - engine-owned reference helpers in `Reference.hxx` now cover reusable
    planning for:
    - `OFFSET` reference-window relocation and resize behavior
    - `INDEX` area normalization for ref-list inputs
    - `INDEX` matrix-shape selection planning
    - `INDEX` reference-shape selection planning
  - Calc now consumes that reference planner layer in `ScOffset()` and
    `ScIndex()` while still owning stack movement, token/reference materialization,
    and external/name-based orchestration
  - standalone coverage now includes `spreadsheetengine_reference_tests`
  - Calc validation for the reference-producing slice now includes
    `CppunitTest_sc_ucalc_formula2` in addition to shared-case and smoke gates
- a final closeout cleanup slice is now also in place:
  - engine-owned string-reference helpers in `StringReference.hxx` now cover:
    - `INDIRECT` address-syntax policy resolution
    - `ADDRESS` string-reference convention selection
  - Calc now consumes that policy layer in `ScIndirect()` and
    `ScAddressFunc()` while still owning actual name resolution, compilation,
    and external/table-reference materialization
  - standalone coverage now extends the compiler-policy lane to exercise the
    new string-reference decisions directly
- Phase 9 is now complete:
  - the host/runtime model needed by the extracted interpreter families is in
    place and exercised in both standalone and Calc-integrated lanes
  - the remaining related work is no longer Phase 9-shaped:
    - compiler/name-resolution-heavy `INDIRECT` internals
    - ref-list/reference-generator ownership details
    - broader reference/dependency/recalculation behavior
  - those remaining seams belong with Phase 10's reference and dependency work,
    not with additional Phase 9 closeout

Validation:

- standalone:
  - host-runtime tests
  - evaluator parity cases against the in-memory host
  - shared-case TSV runner where the rows are spreadsheet-facing
  - `spreadsheetengine_lookup_tests`
  - `spreadsheetengine_array_tests`
  - `spreadsheetengine_reference_tests`
  - compiler-policy coverage for `INDIRECT` / `ADDRESS` string-reference rules
- LibreOffice:
  - `CppunitTest_sc_ucalc_shared_cases`
  - `CppunitTest_sc_logical_functions_test`
  - `CppunitTest_sc_text_functions_test`
  - `CppunitTest_sc_spreadsheet_functions_test`
  - `CppunitTest_sc_ucalc`
  - `CppunitTest_sc_ucalc_formula2`
  - `CppunitTest_sc_ucalc_sort`
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

Implementation approach:

- execute Phase 10 in passes that keep ownership changes low-risk and easy to
  validate, rather than trying to move `ScFormulaCell` or dependency listeners
  wholesale
- pass 1: pure reference and result carriers
  - move the lowest-coupling data containers first:
    - `ScFormulaResult`
    - `sc::FormulaResultValue`
    - `ScSingleRefData`
    - `ScComplexRefData`
  - copy these into engine-owned headers and implementations under
    `spread_engine_extract`, then turn the Calc-facing versions into thin
    compatibility wrappers or consumers of the relocated code
  - do not move broadcaster/listener integration or document lookups in this
    pass; keep anything that still needs `ScDocument` as an adapter seam
- pass 2: lookup cache vocabulary and cache-entry ownership
  - split `lookupcache` into:
    - engine-owned query key / criteria / cached-result records
    - Calc-owned invalidation, document listeners, and cache-map attachment
  - keep `ScLookupCache::Notify()` and document registration in Calc until the
    dependency layer is ready
  - extract only the cache semantics and query-key behavior needed to make the
    cache testable outside Calc
- pass 3: query evaluator pure policy
  - carve out the policy-heavy but host-neutral parts of `queryevaluator`:
    - operator classification
    - whole-cell vs partial-match policy
    - query-by-value vs query-by-string classification
    - sorted-item/cache helper state that does not require table iterators
  - keep table iteration, string-pool ownership, transliteration setup, and
    number-format lookups behind host adapters for now
- pass 4: shared-formula grouping helpers
  - move the structural shared-formula operations in `sharedformula.cxx` behind
    engine-owned services:
    - group / split / join planning
    - group metadata and range-bound calculations
  - keep cell-store mutation, listener teardown/restart, and document-specific
    block traversal in Calc adapters until the `ScFormulaCell` seam is thinner
- pass 5: `ScFormulaCell` result/dependency helpers
  - relocate reusable non-UI orchestration out of `ScFormulaCell`:
    - result-token handling
    - matrix/result-state transitions
    - portions of dirty / compile / recalc state that do not directly touch
      broadcasters or persistence
  - keep document insertion/removal, shell-facing behavior, and persistence
    hooks in Calc
- pass 6: reference update and recalculation algorithms
  - extract the algorithms that update references, dirty dependents, and manage
    recalc/group evaluation once the lower-level value/reference types are
    already engine-owned
  - this is where listener/query/cache integration and the remaining
    `ScFormulaCell` ownership changes should converge

What not to do early:

- do not start Phase 10 by moving all of `formulacell.cxx`
- do not pull document broadcaster ownership or listener registration into the
  engine before the carrier and cache types are already stable
- do not couple the first slice to OpenCL, UI, persistence, or import/export
  behaviors

First logical slice:

- extract `ScFormulaResult` and `ScSingleRefData` / `ScComplexRefData` first
- wire Calc to consume the relocated logic through compatibility headers
- add standalone unit coverage for:
  - formula-result type transitions
  - reference relative/absolute conversion behavior
  - reference ordering / validity rules
- validate against Calc tests that are sensitive to formula results and
  reference motion before touching caches or `ScFormulaCell`

Why this order:

- `formularesult` and `refdata` are already identified in the audit as central
  engine primitives, but they are much less coupled than caches, listeners, or
  formula-cell orchestration
- getting those two types engine-owned first reduces the amount of Calc
  vocabulary leaked into later Phase 10 slices
- it also gives the later cache/query/shared-formula work stable engine-native
  building blocks instead of having to extract everything at once

Recommended next implementation slice:

- start with Phase 10 pass 1 by relocating `formularesult` and `refdata`
- defer `lookupcache`, `queryevaluator`, and `sharedformula` until those core
  result/reference primitives are in place

Validation:

- standalone:
  - reference/update/dependency tests against the minimal host
  - parity cases for caches and lookup behavior
- LibreOffice:
  - `CppunitTest_sc_ucalc_shared_cases`
  - `CppunitTest_sc_ucalc_sharedformula`
  - `CppunitTest_sc_cache_test`
  - `CppunitTest_sc_parallelism`
  - `CppunitTest_sc_ucalc`
  - `CppunitTest_sc_ucalc_copypaste`
  - `CppunitTest_sc_ucalc_sort`
  - `CppunitTest_sc_ucalc_formula2`

Current status:

- pass 1 is now substantially complete:
  - the engine owns standalone-facing carrier headers for formula results and
    references in:
    - `spread_engine_extract/inc/spreadsheetengine/api/FormulaResult.hxx`
    - `spread_engine_extract/inc/spreadsheetengine/api/ReferenceData.hxx`
  - Calc now consumes engine-owned formula-result carrier logic for:
    - carrier-type classification
    - value-vs-string detection
    - error-or-double result construction
  - Calc reference carriers now expose explicit conversion seams to and from
    engine-owned reference types through `ScSingleRefData` and
    `ScComplexRefData`, while the exact Calc semantics for range ordering and
    reference normalization remain local adapters for now
  - standalone coverage now includes direct carrier tests for:
    - formula-result state transitions
    - reference ordering and normalization helpers
    - range extension and entire-row/column checks
  - the Calc build graph now consistently propagates
    `spread_engine_extract/inc` into the targets that include `sc/inc/refdata.hxx`
- validation is green for the carrier-sensitive gates:
  - standalone: `ctest` passes `15/15`
  - LibreOffice:
    - `CppunitTest_sc_ucalc`
    - `CppunitTest_sc_ucalc_formula2`
    - `CppunitTest_sc_ucalc_sharedformula`
    - `CppunitTest_sc_ucalc_shared_cases`
    - `CppunitTest_sc_ucalc_copypaste`
    - `CppunitTest_sc_ucalc_sort`
- the remaining planned validation targets are currently environment- or
  infrastructure-limited in this Ubuntu 24.04 WSL setup rather than exposing a
  clear pass-1 carrier regression:
  - `CppunitTest_sc_parallelism` is failing during test document load before it
    reaches the relocated carrier logic
- after installing baseline and CJK Noto fonts, `CppunitTest_sc_cache_test`
  now passes in this environment
- the main pass-1 remainder is not another broad carrier move:
  - exact `refdata` behavioral ownership can advance later together with the
    dependency/reference-update work in subsequent Phase 10 passes
  - the next meaningful implementation step is Phase 10 pass 2
    (`lookupcache` semantics and cache-entry ownership), not more standalone
    carrier scaffolding
- pass 2 is now substantially complete for the low-coupling lookup-cache seam:
  - engine-owned cache semantics now cover:
    - query operations
    - query criteria
    - query keys
    - cached lookup-result records
    - cache-hit classification semantics
    - cached-row search by criteria
  - `ScLookupCache` now stores engine-owned cache entries instead of a Calc-local
    criteria/result pair
  - Calc still owns the parts that are intentionally host-bound for later
    phases:
    - `ScLookupCache` lifetime
    - document listener wiring
    - invalidation through `Notify()`
    - document cache-map ownership
  - validation is green for the cache-sensitive lanes:
    - standalone: `ctest` passes `15/15`
    - LibreOffice:
      - `CppunitTest_sc_cache_test`
      - `CppunitTest_sc_ucalc_sort`
      - `CppunitTest_sc_ucalc_formula2`
      - `CppunitTest_sc_ucalc`
      - `CppunitTest_sc_ucalc_shared_cases`
  - the remaining cache work is no longer another low-risk vocabulary slice:
    - broader query-policy extraction fits better with Phase 10 pass 3
    - listener and invalidation ownership stays Calc-side until the later
      dependency/recalc passes
- pass 3 is now started with the first pure query-policy slice:
  - engine-owned query policy now covers:
    - operator classification for text and partial-text matches
    - ends-with operator detection
    - whole-cell vs partial-match policy
    - wildcard / regexp gate checks
    - query-by-value vs query-by-string classification
    - assigned-empty-string rejection used by function-style query callers
    - multi-item equality fast-path thresholds
    - sorted-cache collection and probe helpers for numeric and string identity
      caches
    - fast string-equality path selection
    - exact shared-string equality path selection
    - operator-driven wildcard and substring match evaluation rules
    - wildcard / regexp search-direction and start/end planning
    - post-search result routing for wildcard / regexp vs test-equality paths
    - branch-selection policy for pattern prepass, post-pattern comparison, and
      text-match vs ordered-compare paths
    - substring search-start planning for contains vs ends-with checks
    - ordered string-compare result policy for the `<`, `>`, `<=`, and `>=`
      family
    - range-lookup comparison policy, including the formula-error exclusion
      used when deciding whether a cell can match a numeric query
    - string-identity multi-item fast-path gating and operand eligibility
      rules, including when `ByValue` operands are treated as string
      candidates
    - top-level comparison-route classification for value, string,
      range-lookup, text-color, and background-color items
    - evaluator short-circuit and result-aggregation policy for per-item
      matches and AND/OR-connected query entries
  - Calc now adapts `ScQueryOp`, `ScQueryEntry::QueryType`, search type, and
    cell-class facts into that engine-owned policy layer from
    `queryevaluator.cxx`
  - Calc also consumes the engine-owned multi-item cache helpers from
    `queryevaluator.cxx` while still owning the actual `ScQueryEntry` item
    containers and evaluator-local cache vectors
  - Calc also still owns the heavyweight parts of string comparison:
    - transliteration
    - collator setup
    - string materialization from cells
    - wildcard / regexp search execution
  - Calc still owns the broader query-evaluation orchestration around those
    helpers:
    - table iteration
    - evaluator-local cache vectors and query items
    - number-format-sensitive comparison
  - table iteration, string-pool ownership, transliteration, collator setup,
    and number-format-sensitive comparison remain in Calc for now
  - validation is green for the first pass-3 slice:
    - standalone: `ctest` passes `16/16`
    - LibreOffice:
      - `CppunitTest_sc_ucalc_sort`
      - `CppunitTest_sc_ucalc`
      - `CppunitTest_sc_ucalc_copypaste`
      - `CppunitTest_sc_ucalc_formula2`
      - `CppunitTest_sc_spreadsheet_functions_test`

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
