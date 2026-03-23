# Calc Engine Extraction Plan

This plan assumes the current Calc calculation engine is spread across the
files listed in `spread_engine_extract/CALC_ENGINE_AUDIT.md`, and that those
files mix true spreadsheet-engine logic with document-model concerns, UI-adjacent
concerns, filter support, and some functionality shared elsewhere in
LibreOffice.

The goal is not a big-bang rewrite. The goal is to progressively concentrate
the spreadsheet-specific calculation logic into a new module under
`spread_engine_extract/`, while leaving Calc's document shell, UI, and
import/export logic in place. For logic that is currently shared through
`formula/`, the plan now explicitly allows that code to be copied into
`spread_engine_extract/` so Calc can ultimately depend on its own local engine
stack. Other LibreOffice applications can continue using the existing
`formula/` module during this migration. Each iteration should be small enough
to validate with the existing spreadsheet unit tests.

## Target End State

At the end of the extraction, the build should look roughly like this:

- `spread_engine_extract/`
  - `Module_spread_engine_extract.mk`
  - `Library_spreadsheetengine.mk`
  - `inc/spreadsheetengine/...`
  - `source/core/...`
  - `source/bridge/calc/...`
  - `tests/` or documentation/helpers only if needed
- `sc/Library_sc.mk`
  - depends on `spreadsheetengine`
  - retains Calc document model, persistence, UI, filter, UNO, drawing, and
    host-side glue
- Calc's engine path
  - uses the compiler/token/grammar/runtime code copied into
    `spread_engine_extract/`
- `formula/`
  - remains in place for non-Calc LibreOffice consumers
  - is allowed to temporarily overlap in responsibility with
    `spreadsheetengine`

The new module should own the spreadsheet-engine logic that is specific to Calc
formula compilation, evaluation, references, matrices, dependency maintenance,
and recalculation policies, including the portions of the current `formula/`
implementation that Calc needs in order to become self-contained. The existing
Calc files should become host/bridge code that delegates to the relocated
capability.

## Boundary Rules

These rules are what keep the new module focused.

- In scope for `spreadsheetengine`:
  - formula evaluation runtime
  - matrix/array execution substrate
  - spreadsheet-specific compile helpers
  - copied compiler/token/grammar infrastructure currently implemented in
    `formula/` when that logic is part of Calc's engine path
  - reference representation and update algorithms
  - lookup/query helpers used by formula execution
  - dependency/recalc algorithms that can be expressed behind host interfaces
  - shared-formula and formula-group logic once decoupled from full document
    ownership
- Out of scope for `spreadsheetengine`:
  - Calc UI, shell, dialogs, view state
  - import/export filters
  - UNO wrappers
  - drawing layer integration
  - pivot table UI and report layout code
  - `formula/` pieces that are only relevant to non-Calc consumers
- Transitional rule:
  - the new module may initially depend on Calc adapter interfaces implemented
    in `sc`, but it should not depend on `sc/source/ui/*`, `sc/source/filter/*`,
    or other non-engine subsystems
  - when a currently shared `formula/` component blocks Calc isolation,
    duplication into `spread_engine_extract/` is preferred over preserving the
    shared dependency

## Duplication Policy

Temporary duplication between `formula/` and `spread_engine_extract/` is
acceptable and intentional.

- If a `formula/` implementation is part of the Calc engine path, it may be
  copied into `spread_engine_extract/` and adapted there.
- Calc should progressively switch to the copied implementation in
  `spread_engine_extract/`.
- Other LibreOffice applications may continue linking to the original
  `formula/` code until a later cleanup or unification decision.
- We should not delay extraction solely to avoid duplication. Isolation of the
  spreadsheet engine is the primary objective.

## Extraction Strategy

The safest pattern is:

1. Create a seam.
2. Move a leaf algorithm or helper behind that seam.
3. Make the old Calc file delegate to the new implementation.
4. Run the spreadsheet test gate.
5. Repeat.

That means we should avoid moving giant classes like `ScDocument`,
`ScTable`, `ScColumn`, or `ScFormulaCell` in one step. Instead, we should
extract services and value types first, then shrink the old classes into hosts.
For shared `formula/` logic, "extract" often means "copy into
`spread_engine_extract/` first, then retarget Calc to the copy."

## Proposed Module Shape

Use a structure that mirrors the engine responsibilities instead of the current
Calc source layout:

- `spread_engine_extract/inc/spreadsheetengine/core/`
  - public engine-facing types and interfaces
- `spread_engine_extract/inc/spreadsheetengine/bridge/`
  - Calc adapter interfaces used during the transition
- `spread_engine_extract/source/core/model/`
  - engine-owned value objects and lightweight runtime state
- `spread_engine_extract/source/core/compiler/`
  - Calc-specific compile helpers and duplicated formula compiler/token support
- `spread_engine_extract/source/core/eval/`
  - interpreter services, runtime context, result handling, matrices
- `spread_engine_extract/source/core/deps/`
  - dependency graph, reference updates, recursion helpers
- `spread_engine_extract/source/core/query/`
  - lookup and query helpers
- `spread_engine_extract/source/bridge/calc/`
  - temporary compatibility shims used by `sc`
- `spread_engine_extract/source/compat/formula/`
  - copied code derived from the current `formula/` module for Calc-only use

## Build Introduction

The first build-system step should be intentionally boring:

1. Add `spread_engine_extract/Module_spread_engine_extract.mk`.
2. Add `spread_engine_extract/Library_spreadsheetengine.mk`.
3. Register the new moduledir in `RepositoryModule_build.mk`.
4. Add `spreadsheetengine` as a dependency of `sc/Library_sc.mk`.
5. Reserve subdirectories and include paths for copied `formula/` code inside
   `spread_engine_extract/`.
6. Start with an empty library plus one trivial shim/header so there is no
   behavior change.

This gives a stable place to move code into before any logic is extracted.

## Iteration Template

Every extraction iteration should follow the same checklist:

1. Pick one small responsibility slice.
2. Add or refine the engine-facing interface in `spread_engine_extract/inc/`.
3. Move or copy the implementation into `spread_engine_extract/source/`.
4. Replace the old Calc implementation with delegation, wrapper types, or thin
   forwarding helpers.
5. Run the spreadsheet unit-test gate.
6. If behavior changed, stop and fix before extracting the next slice.

## Validation Gate

The current helper script is a good fast gate:

```bash
./spread_engine_extract/run_spreadsheet_unit_tests.sh
```

Before starting the extraction work, expand that script so it can optionally run
the broader non-rendering Calc engine suite, not just the current smoke subset.
Use two validation levels:

- Per iteration:
  - run the default fast gate
- Per milestone branch or before merge:
  - run the broader engine-focused Calc suite, including additional
    `CppunitTest_sc_ucalc*` targets and relevant function test targets

Recommended area-to-test mapping:

- compiler, tokens, references:
  - `CppunitTest_sc_ucalc_formula`
  - `CppunitTest_sc_ucalc_formula2`
  - `CppunitTest_sc_ucalc_range`
  - `CppunitTest_sc_ucalc_sharedformula`
- interpreter, lookup, matrix logic:
  - `CppunitTest_sc_ucalc`
  - `CppunitTest_sc_spreadsheet_functions_test`
  - function-family tests like `CppunitTest_sc_array_functions_test`,
    `CppunitTest_sc_logical_functions_test`, `CppunitTest_sc_text_functions_test`
- document/recalc integration:
  - `CppunitTest_sc_ucalc`
  - `CppunitTest_sc_ucalc_copypaste`
  - `CppunitTest_sc_ucalc_sort`
  - `CppunitTest_sc_cache_test`
  - `CppunitTest_sc_parallelism`

## Status

- Phase 0: completed
  - `spread_engine_extract/Module_spread_engine_extract.mk` and
    `spread_engine_extract/Library_spreadsheetengine.mk` now exist and build a
    real `spreadsheetengine` library
  - the new module is registered in both repository module graphs and in
    `Repository.mk`
  - `sc/Library_sc.mk` now depends on `spreadsheetengine`
  - a first bridge/header seam exists through
    `spread_engine_extract/inc/spreadsheetengine/core/Phase0.hxx`,
    `spread_engine_extract/inc/spreadsheetengine/bridge/CalcPhase0Bridge.hxx`,
    and a harmless include touchpoint in `sc/source/core/tool/calcconfig.cxx`
  - the copied-formula staging area exists at
    `spread_engine_extract/source/compat/formula/`
  - `run_spreadsheet_unit_tests.sh` now supports both a smoke profile and a
    broader engine profile
  - validation completed with `make Library_spreadsheetengine` and the smoke
    spreadsheet gate passing
- Phase 1: first slice in progress
  - a Calc-owned copy of the `FormulaGrammar` helper logic now lives under
    `spread_engine_extract/source/compat/formula/`
  - core Calc call sites in the compiler, token-string context, and range
    utility code can now use the copied grammar helper implementation while
    keeping the existing `formula::FormulaGrammar` enum types in place
  - the next slice should extend the copied compatibility layer to additional
    `formula/source/core/api/*` helpers and reduce remaining direct helper use
    from the old `formula` module
- Phase 2: first slice in progress
  - pure Calc-config helper logic for forced-calculation environment parsing
    and OpenCL opcode-set string conversion now lives in
    `spread_engine_extract/source/core/CalcConfig.cxx`
  - `sc/source/core/tool/calcconfig.cxx` now delegates those helpers to the
    new module while keeping `ScCalcConfig` itself in Calc
- Phase 3: first slice in progress
  - the `matrixoperators` implementation now lives in
    `spread_engine_extract/source/core/MatrixOperators.cxx`
  - `sc` no longer builds its own `matrixoperators` object and instead consumes
    the relocated `sc::op` runtime symbols from `spreadsheetengine`
  - this keeps the public Calc headers stable while starting to move the
    matrix/runtime substrate out of the `sc` library
  - the `sc::op` symbols are now explicitly imported/exported so clean relinks
    of `libsclo.so` and downstream test targets continue to succeed against the
    extracted implementation
- Phase 4: first slice in progress
  - Calc-specific address-convention character-table construction now lives in
    `spread_engine_extract/source/core/CompilerSupport.cxx`
  - `sc/source/core/tool/compiler.cxx` now delegates `mrCharTable`
    initialization to `spreadsheetengine::core::compiler::getCharTable()`
  - this is a narrow compiler extraction that moves tokenization policy for
    Calc formula conventions without yet moving the larger compiler facade
  - validation completed with
    `CppunitTest_sc_ucalc_formula`,
    `CppunitTest_sc_ucalc_formula2`,
    `CppunitTest_sc_spreadsheet_functions_test`, and
    `CppunitTest_sc_ucalc_range`
- Phase 5: substantially complete for low-coupling interpreter helper families
  - the numeric-policy part of Calc's rounding family now lives in
    `spread_engine_extract/source/core/MathRounding.cxx`
  - `sc/source/core/tool/interpr2.cxx` keeps argument decoding and error
    handling, but delegates `ROUND*`, `CEILING*`, `FLOOR*`, `EVEN`, and `ODD`
    calculations to `spreadsheetengine::core::math`
  - this gives Phase 5 its first interpreter-family seam without moving
    `ScInterpreter` state management out of Calc
  - a second scalar-math slice now lives in
    `spread_engine_extract/source/core/MathScalar.cxx`
  - `sc/source/core/tool/interpr2.cxx` now also delegates `SIGN`, `ABS`,
    `INT`, `ATAN2`, `LOG`, `LN`, `LOG10`, and `MOD` numeric policy to the
    extracted helper while keeping Calc-side parameter validation and error
    pushes stable
  - a third financial-helper slice now lives in
    `spread_engine_extract/source/core/MathFinancial.cxx`
  - `sc/source/core/tool/interpr2.cxx` now delegates the time-value-of-money
    helper family behind `PV`, `PMT`, `FV`, `IPMT`, `PPMT`, `CUMIPMT`, and
    `CUMPRINC` to the extracted financial helper while still keeping
    argument-count checks, defaults, and Calc-visible error behavior in place
  - a fourth depreciation-helper slice now also lives in
    `spread_engine_extract/source/core/MathFinancial.cxx`
  - `sc/source/core/tool/interpr2.cxx` now delegates `SYD`, `SLN`, `DDB`,
    `DB`, and `VDB` numeric policy, including the intermediate declining-balance
    helper flow, to the extracted financial helper while preserving Calc-side
    validation and error handling
  - a fifth financial-rate slice now also lives in
    `spread_engine_extract/source/core/MathFinancial.cxx`
  - `sc/source/core/tool/interpr2.cxx` now delegates `PDURATION`, `RRI`,
    `NPER`, `EFFECT`, and `NOMINAL` numeric policy to the extracted helper,
    while leaving argument-count checks and invalid-input handling in Calc
  - a sixth financial-solver slice now also lives in
    `spread_engine_extract/source/core/MathFinancial.cxx`
  - `sc/source/core/tool/interpr2.cxx` now delegates `ISPMT` and the `RATE`
    solving path, including the Newton iteration and alternate-guess fallback,
    to the extracted financial helper while keeping Calc-side argument
    validation and `NoConvergence` error behavior stable
  - a seventh transcendental slice now lives in
    `spread_engine_extract/source/core/MathTranscendental.cxx`
  - `sc/source/core/tool/interpr1.cxx` now delegates `PI`, degree/radian
    conversion, trigonometric and hyperbolic-trigonometric helpers, `EXP`,
    and `SQRT` numeric policy to the extracted helper while keeping
    Calc-visible invalid-domain handling in place
  - an eighth bitwise slice now lives in
    `spread_engine_extract/source/core/MathBitwise.cxx`
  - `sc/source/core/tool/interpr1.cxx` now delegates `BITAND`, `BITOR`,
    `BITXOR`, `BITLSHIFT`, and `BITRSHIFT` to the extracted helper while
    preserving existing Calc argument-count checks and `IllegalArgument`
    behavior
  - a ninth numeral-conversion slice now lives in
    `spread_engine_extract/source/core/NumeralConversion.cxx`
  - `sc/source/core/tool/interpr2.cxx` now delegates `BASE`, `DECIMAL`,
    `ROMAN`, and `ARABIC` to the extracted helper while keeping Calc-side
    stack decoding and user-visible error routing stable
  - a tenth text-scalar slice now lives in
    `spread_engine_extract/source/core/TextScalar.cxx`
  - `sc/source/core/tool/interpr1.cxx` now delegates `TRIM`, `LEN`,
    `NUMBERVALUE`, `CLEAN`, `CODE`, `CHAR`, `UNICODE`, and `UNICHAR`
    scalar text/codepoint policy to the extracted helper while keeping
    Calc-side stack handling and array/ref behavior in place
  - an eleventh text-case slice now lives in
    `spread_engine_extract/source/core/TextCase.cxx`
  - `sc/source/core/tool/interpr1.cxx` now delegates `UPPER`, `LOWER`,
    and `PROPER` case-shaping logic to the extracted helper while still
    passing the existing locale-aware `CharClass` from Calc
  - a twelfth text-width slice now lives in
    `spread_engine_extract/source/core/TextWidth.cxx`
  - `sc/source/core/tool/interpr1.cxx` now delegates `ASC` and `JIS`
    half-width/full-width conversion logic to the extracted helper
  - a thirteenth date-parts slice now lives in
    `spread_engine_extract/source/core/DateTimeParts.cxx`
  - `sc/source/core/tool/interpr2.cxx` now delegates `YEAR`, `MONTH`, `DAY`,
    `HOUR`, `MINUTE`, `SECOND`, `DATE`, `TIME`, `DAYS`, `DAYS360`,
    `DATEDIF`, and `EASTERSUNDAY` calendar/date-difference logic to the
    extracted helper while keeping Calc-side stack decoding and visible error
    routing stable
  - a fourteenth date-week slice now lives in
    `spread_engine_extract/source/core/DateTimeWeek.cxx`
  - `sc/source/core/tool/interpr2.cxx` now delegates `WEEKDAY`, `WEEKNUM`,
    `ISOWEEKNUM`, and the legacy OOo week-number helper to the extracted
    helper while preserving Calc's existing argument-count handling and
    invalid-flag behavior
  - a fifteenth workday slice now lives in
    `spread_engine_extract/source/core/DateTimeWorkday.cxx`
  - `sc/source/core/tool/interpr2.cxx` now delegates weekend-mask mapping plus
    the pure `NETWORKDAYS` and `WORKDAY` counting/shift algorithms to the
    extracted helper while leaving holiday-array parsing and stack interaction
    in Calc
  - per-slice validation completed with
    `CppunitTest_sc_financial_functions_test`,
    `CppunitTest_sc_datetime_functions_test`,
    `CppunitTest_sc_mathematical_functions_test`,
    `CppunitTest_sc_text_functions_test`,
    `CppunitTest_sc_spreadsheet_functions_test`, and
    `CppunitTest_sc_ucalc`
  - a broader Phase 5 milestone validation also completed with
    `CppunitTest_sc_ucalc`,
    `CppunitTest_sc_mathematical_functions_test`,
    `CppunitTest_sc_financial_functions_test`,
    `CppunitTest_sc_text_functions_test`,
    `CppunitTest_sc_spreadsheet_functions_test`, and
    `CppunitTest_sc_ucalc_formula2`
  - Phase 5 is now substantially complete for pure numeric, simple text, and
    core calendar/workday helper families; the remaining interpreter work is
    concentrated in logical/control-flow, reference-aware text/value behavior,
    locale-aware date/time parsing, lookup/query, and array-aware or
    document-coupled evaluation paths

## Phased Roadmap

### Phase 0: Prepare the Seams

Objective: introduce the new module without moving real behavior yet.

Small iterations:

1. Create the new module/library and wire it into the build.
2. Add namespace and include conventions, for example `spreadsheetengine::`.
3. Reserve the subtree and naming convention for copied `formula/` sources used
   only by Calc.
4. Add an engine bridge header used by one existing Calc file.
5. Expand the test runner so the extraction work has a consistent validation
   entry point.

Validation:

- full build of the new library plus existing Calc
- spreadsheet unit-test gate unchanged

Completion notes:

- Completed by introducing the new build module/library, linking `sc` against
  it, reserving the Calc-owned copied-formula subtree, adding the first
  bridge/header seam, and extending the spreadsheet test helper with smoke and
  engine profiles.

### Phase 1: Copy Shared Formula Infrastructure Needed By Calc

Best early candidates are the `formula/` pieces that are central to Calc's
compiler/runtime path and small enough to copy without forcing a large semantic
rewrite.

Likely candidates from the audit:

- `formula/source/core/api/FormulaCompiler.cxx`
- `formula/source/core/api/grammar.cxx`
- `formula/source/core/api/token.cxx`
- `formula/source/core/api/vectortoken.cxx`

Approach:

- copy the needed implementation into `spread_engine_extract/source/compat/formula/`
- put the copied code behind Calc-facing wrappers or a dedicated namespace to
  avoid accidental collisions with the original `formula` library
- switch Calc call sites to the copied implementation in small slices instead of
  swapping everything at once
- leave non-Calc callers on the original `formula` library

Why this early:

- it makes the self-contained-engine goal explicit from the beginning
- it prevents the rest of the extraction from re-entangling Calc with
  supposedly shared formula infrastructure
- it lets later compiler/interpreter work happen inside the new module

Validation:

- `CppunitTest_sc_ucalc_formula`
- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_spreadsheet_functions_test`

### Phase 2: Extract Pure Value Types and Stateless Helpers

Best first candidates are files whose logic is useful to the engine but does
not require ownership of the full Calc document model.

Likely candidates from the audit:

- `sc/source/core/tool/formularesult.cxx`
- `sc/source/core/tool/refdata.cxx`
- `sc/source/core/tool/lookupcache.cxx`
- `sc/source/core/tool/calcconfig.cxx`
- selected pure helpers from `queryevaluator.cxx`

Approach:

- move the implementation into `spreadsheetengine`
- keep the existing Calc types or names as wrappers/type aliases where possible
- do not change callers broadly in the same iteration

Why this first:

- these are lower-risk seams
- they reduce future include pressure
- they establish the new module's basic runtime vocabulary

Validation:

- default spreadsheet gate after each file family
- add compiler/reference tests when touching `refdata`

### Phase 3: Extract Matrix and Execution Substrate

Files to target:

- `sc/source/core/tool/scmatrix.cxx`
- `sc/source/core/tool/jumpmatrix.cxx`
- `sc/source/core/tool/matrixoperators.cxx`
- `sc/source/core/tool/interpretercontext.cxx`

Approach:

- move matrix storage and operations first
- keep `ScInterpreter` unchanged initially except for calling relocated matrix
  helpers
- then move `ScInterpreterContext` and related execution-local caches

This phase matters because it carves out the runtime substrate before touching
the large `interpr*.cxx` family.

Validation:

- `CppunitTest_sc_ucalc`
- `CppunitTest_sc_spreadsheet_functions_test`
- array/function-family tests when available

### Phase 4: Extract Calc-Specific Compiler Logic

Files to target:

- spreadsheet-specific parts of `sc/source/core/tool/compiler.cxx`
- Calc token extensions in `sc/source/core/tool/token.cxx`
- possibly `sc/source/core/tool/formulaparserpool.cxx` later, but not first

Approach:

- create `spreadsheetengine::CompilerFacade` on top of the copied
  `spread_engine_extract` formula infrastructure
- extract only the Calc-specific tokenization, syntax decisions, and
  spreadsheet semantics
- leave foreign-format parser pool integration in Calc until the end
- where necessary, keep temporary adapters so existing Calc code can continue to
  use familiar types while the implementation moves underneath

Validation:

- `CppunitTest_sc_ucalc_formula`
- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_spreadsheet_functions_test`

### Phase 5: Extract Interpreter Logic by Function Family

Files to target:

- `sc/source/core/tool/interpr1.cxx` through `interpr8.cxx`

Do not move these as giant files. Split them into coherent function families.

Suggested extraction order:

1. arithmetic and scalar helpers
2. logical and control-flow helpers
3. text and date/time helpers
4. lookup/query helpers
5. array-aware and matrix-aware helpers

Approach:

- introduce engine-side helper classes or namespaces for each family
- keep `ScInterpreter` as the stable facade used by existing Calc code
- each iteration should relocate only one family or one helper cluster
- avoid renaming public Calc types while behavior is still moving
- arithmetic/scalar/transcendental/financial/bitwise/simple-text helper slices
  are now mostly extracted; prioritize the remaining document-aware families
  next

Validation:

- function-family tests matching the extracted helpers
- `CppunitTest_sc_spreadsheet_functions_test`
- `CppunitTest_sc_ucalc`

### Phase 6: Extract Reference Update and Dependency Algorithms

Files to target:

- `sc/source/core/tool/refupdat.cxx`
- `sc/source/core/tool/recursionhelper.cxx`
- `sc/source/core/data/bcaslot.cxx`
- `sc/source/core/data/broadcast.cxx`
- `sc/source/core/data/listenercontext.cxx`

This is where the design needs a real anti-corruption layer between the engine
and Calc's document model.

Introduce host interfaces such as:

- workbook/sheet/cell lookup
- named-range resolution
- listener registration and invalidation callbacks
- formula dirtying and recalc scheduling hooks

Approach:

- define minimal bridge interfaces in `spread_engine_extract/inc/.../bridge`
- implement them in Calc first
- then move algorithms behind those interfaces one subsystem at a time

Validation:

- `CppunitTest_sc_ucalc_formula`
- `CppunitTest_sc_ucalc_sharedformula`
- `CppunitTest_sc_ucalc_range`
- `CppunitTest_sc_ucalc`

### Phase 7: Shrink `ScFormulaCell` Into a Host Object

Primary file:

- `sc/source/core/data/formulacell.cxx`

Desired split:

- keep in Calc:
  - cell object identity
  - persistence hooks
  - listener ownership and document wiring
  - bridge implementation
- move to `spreadsheetengine`:
  - compiled program ownership if possible
  - evaluation orchestration
  - cached runtime/result transitions that are engine-specific
  - shared-formula execution helpers

Approach:

- first extract small evaluation helper methods called by `ScFormulaCell`
- then move more of the execute/recalculate path behind engine services
- leave document-owned lifecycle and storage in Calc

Validation:

- `CppunitTest_sc_ucalc`
- `CppunitTest_sc_ucalc_formula`
- `CppunitTest_sc_ucalc_sharedformula`

### Phase 8: Move Formula Grouping and Parallel Execution

Files to target:

- `sc/source/core/tool/formulagroup.cxx`
- `sc/source/core/tool/sharedformula.cxx`
- `sc/source/core/opencl/formulagroupcl.cxx` last

Approach:

- first move pure grouping policy and shared-formula algorithms
- keep backend selection and OpenCL-specific hooks behind bridges
- move OpenCL support only after the scalar engine path is stable

Validation:

- `CppunitTest_sc_ucalc_sharedformula`
- `CppunitTest_sc_ucalc_parallelism`
- `CppunitTest_sc_parallelism`

### Phase 9: Re-evaluate What Must Stay in Calc

At this point, revisit the larger owners:

- `ScDocument`
- `ScTable`
- `ScColumn`
- `simpleformulacalc.cxx`
- `queryiter.cxx`

The likely end state is not that these all move. The likely end state is:

- `ScDocument`, `ScTable`, and `ScColumn` stay in Calc as the persistent
  workbook model
- the new engine module owns more of the computation, reference maintenance,
  and runtime behavior
- Calc retains thin bridge code plus document-model orchestration

That outcome is still a successful extraction, because the core spreadsheet
engine becomes concentrated and testable in one place.

## What Not To Extract Early

Avoid these early in the program:

- anything in `sc/source/ui/*`
- anything in `sc/source/filter/*`
- pivot-table-specific files
- drawing-related files
- UNO object implementations
- formula UI or service code not needed by Calc's engine extraction
- broad changes that try to migrate all `formula/` consumers at once

Moving those early would create churn without improving the engine boundary.

## Architectural Guardrails

To prevent the new module from becoming another mixed bag:

- keep public headers small and domain-oriented
- prefer interfaces and adapters over direct inclusion of large Calc headers
- do not let `spreadsheetengine` include UI or filter headers
- isolate shared external dependencies behind narrow wrappers when practical
- when copying from `formula/`, use a clear namespace or include-path boundary
  so Calc can target the new implementation without symbol ambiguity
- keep old Calc entry points stable until the new implementation is proven
- only move code once there is a test target that exercises it

## Practical First Ten Iterations

If this effort started tomorrow, a realistic first set of small iterations would
be:

1. Add the new module and empty library.
2. Expand the test runner to cover the agreed engine gate.
3. Create the copied `formula/` subtree inside `spread_engine_extract`.
4. Copy one small `formula` core file family and route Calc to it.
5. Move `ScCalcConfig` or equivalent config helpers.
6. Move `ScFormulaResult`.
7. Move `ScLookupCache`.
8. Move `ScSingleRefData` and `ScComplexRefData`.
9. Move `ScMatrix` support.
10. Move `ScInterpreterContext`.

By the end of those ten steps, the extraction would be real, the library would
have useful substance, and the risk would still be controlled.

## Exit Criteria

The extraction should be considered successful when:

- the core spreadsheet-engine logic lives primarily under
  `spread_engine_extract/`
- the `sc` module depends on the new engine module instead of owning most of
  the implementation directly
- Calc uses the copied compiler/token/grammar/runtime pieces under
  `spread_engine_extract/` instead of relying on the old shared `formula`
  implementation for its engine path
- the original `formula` module can still serve other LibreOffice applications
- the old Calc files are mostly bridges, hosts, or persistence/model code
- the spreadsheet unit-test gate still passes consistently throughout the
  migration
