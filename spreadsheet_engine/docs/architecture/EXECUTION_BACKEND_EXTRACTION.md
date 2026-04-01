# Execution Backend Extraction

## Purpose

The recalculation-orchestration milestone is complete. The next extraction
program starts from a stable authority boundary:

- engine owns invalidation planning and recalc queue construction
- Calc consumes engine queue output through a compat bridge
- Calc still owns storage mutation and actual formula execution

This milestone moves execution semantics out of Calc in small, validated
slices while preserving that boundary.

## Objectives

1. extract evaluator-shell mechanics behind the settled scheduler boundary
2. keep Calc as the storage and runtime host while execution logic migrates
3. validate every slice against existing standalone, Calc, and replay lanes
4. avoid re-opening queue/scheduling authority questions during execution work

## In Scope

- token walking and stack mechanics that remain Calc-owned
- coercion and argument normalization shared between Calc and standalone
- reference-sensitive execution helpers that can move without changing storage
- explicit compat bridges where Calc still needs host-owned services

## Out Of Scope

- replacing `ScDocument` storage
- changing listener/broadcaster ownership
- altering the now-settled recalc queue/scheduling boundary
- OpenCL or threaded backend redesign

## Starting Boundary

The starting handoff from the completed recalc-orchestration milestone is:

- engine-owned queue output represented by `RecalcPlan`
- Calc queue consumption isolated in
  `compat/libreoffice/RecalcQueueExecution.hxx`
- authoritative planner/scheduler pilot validated for non-structural,
  structural, and named-range mutations

## Current Status

This milestone is now active with Phases 0 through 5 complete and the next
bounded execution-shell slice narrowed to the remaining post-Phase-5
token-walking, jump, and name/string-reference flows.

- Phase 0 is complete: the orchestration/execution boundary is explicitly
  documented, the first helper duplication inventory is frozen, and the Phase 1
  validation contract is now an executed baseline rather than a proposed lane
- Phase 1 is complete: engine-owned scalar coercion and text-position
  normalization helpers now exist, standalone consumes them directly, and Calc
  reaches the same normalization path through thin adapters in `ScInterpreter`
- Phase 2 is complete: Calc's operator-dispatch shell now routes comparison
  operators, logical folds, unary matrix/scalar operators, and synthetic binary
  dispatch through shared interpreter-shell helpers with focused Calc coverage
- Phase 3 is complete: the first reference-sensitive execution slices now move
  through explicit compat bridges for `MATCH` / `XMATCH` execution plus
  `ADDRESS` / `OFFSET` / `INDEX` planning and formatting, while Calc still
  owns stack movement and host-only services
- Phase 4 is complete: `LOOKUP`, `VLOOKUP`, `HLOOKUP`, and `XLOOKUP` now route
  bounded traversal, slice planning, and result shaping through the expanded
  compat lookup bridge while Calc keeps stack mutation and host-only services
- Phase 5 is complete: `ROW`, `COLUMN`, `ROWS`, `COLUMNS`, `AREAS`, `SHEET`,
  and `SHEETS` now route bounded reference-shape and sheet/cardinality logic
  through shared helpers in standalone plus Calc compat bridges, leaving the
  remaining Calc-owned shell narrowed to later token-walking and jump/name
  flows

The starting baseline for this milestone is:

- `500` promoted replay workbooks
- `50,661` formula cells
- `0` cached-fallback cells
- completed recalc-orchestration phases through the queue-consumption handoff

## Proposed Phases

### Phase 0: Freeze The Orchestration Boundary

Status: **Complete**

Goals:

- keep the engine-owned recalc authority lane green
- document the exact handoff files and validation gates
- identify the first execution helpers that still duplicate between Calc and
  standalone
- freeze the first safe extraction surface before helper code starts moving

Deliverables:

- refreshed `PROJECT_STATUS.md`
- this implementation-ready milestone doc
- explicit file inventory for the first evaluator-shell slice
- explicit validation contract for the first helper extraction

Phase 0 shipped as four concrete workstreams.

#### 0.1 Boundary Inventory

Map the exact line between "orchestration now owned by the engine" and
"execution still owned by Calc".

Required outputs:

- authoritative file list for the completed orchestration side
- authoritative file list for the remaining execution shell
- short ownership note for each key boundary file

Inventory result:

- `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/RecalcAuthority.hxx`
- `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/RecalcQueueExecution.hxx`
- `spreadsheet_engine/source/core/FormulaEvaluator.cxx`
- `spreadsheet_engine/source/core/FormulaEvaluatorInternals.hxx`
- `spreadsheet_engine/source/core/FormulaEvaluatorOperators.cxx`
- `sc/source/core/data/formulacell.cxx`
- `sc/source/core/tool/interpr1.cxx`
- `sc/source/core/tool/interpr2.cxx`
- `sc/source/core/tool/interpr3.cxx`
- `sc/source/core/tool/interpr4.cxx`
- `sc/source/core/tool/interpr6.cxx`
- `sc/source/core/tool/interpr7.cxx`

Ownership map:

- `RecalcAuthority.hxx`: engine-owned authority/pilot surface that verifies
  Calc dirty-plan and queue application against engine expectations
- `RecalcQueueExecution.hxx`: Calc bridge that consumes engine-owned recalc
  queue output; this is the settled orchestration handoff
- `FormulaEvaluator.cxx`: standalone evaluator entrypoint, replay policy, and
  top-level orchestration for execution semantics that are already engine-owned
- `FormulaEvaluatorInternals.hxx`: current concentration point for evaluator
  helper context, materializers, and shared execution support that still needs
  further extraction in later phases
- `FormulaEvaluatorOperators.cxx`: engine-owned unary/binary/reference operator
  shell, now clearly separated from Calc queue/scheduling concerns
- `formulacell.cxx`: Calc execution host for formula-cell lifecycle, storage,
  and recalc-side effects; still intentionally Calc-owned
- `interpr1.cxx`, `interpr2.cxx`, `interpr3.cxx`, `interpr4.cxx`,
  `interpr6.cxx`, `interpr7.cxx`: Calc stack-driven execution shell and
  remaining host-shaped evaluator backend, including token walking, stack
  mutation, and reference/range traversal

The outcome is now an explicit handoff map, not a speculative redesign.

#### 0.2 Helper Duplication Inventory

Produce a concrete inventory of execution-shell duplication that still exists
between Calc and standalone.

The inventory is grouped by helper family:

- scalar coercion:
  - number coercion
  - boolean coercion
  - string coercion
  - date-serial coercion
- positional and integral argument normalization
- scalarization of single-cell references and scalar matrices
- host-mediated formatting and error shaping
- stack-shell helpers that are not yet candidates for extraction

Frozen answers from the first inventory pass:

- which helpers already exist in `FormulaEvaluatorInternals.hxx`
- which corresponding behaviors still live as `Get*()` /
  `Get*WithDefault()` / `Pop*()` flows inside `ScInterpreter`
- which behaviors are pure enough for shared extraction now
- which behaviors are still tightly bound to Calc stack state and must wait
  for Phase 2

Inventory result:

- Shared now:
  - ASCII uppercasing and numeric text parsing
  - scalar number coercion
  - scalar boolean coercion
  - scalar string coercion
  - whole-number normalization
  - one-based and non-negative text-position/index normalization
- Calc-owned but now routed through the shared normalization surface:
  - `ScInterpreter::CheckStringPositionArgument()`
  - `ScInterpreter::GetStringPositionArgument()`
- Still deferred because they remain stack-shaped or host-shaped:
  - `GetDouble()`, `GetString()`, `GetDoubleWithDefault()`, `GetByteString()`
  - `PopDoubleRef()`, `PopSingleRef()`, and other stack cursor mutation flows
  - reference-list and range walkers
  - short-circuit and jump shell behavior
  - host-mediated formatting/error shaping beyond the already-shared scalar
    value coercion

#### 0.3 Select The First Safe Extraction Surface

Choose the first Phase 1 surface only from helpers that satisfy all of these
rules:

- no queue/scheduling authority changes
- no storage ownership changes
- no listener/broadcaster interaction
- no need to migrate Calc stack walking in the same slice
- can be differential-tested directly in both standalone and Calc

The selected first safe surface was:

- scalar coercion and scalar argument normalization

The Phase 0 defer list, now frozen for later phases, is:

- token-walking helpers
- stack cursor movement
- interpreter-local dispatch shell mechanics
- reference-list and range walkers
- short-circuit jump behavior

#### 0.4 Freeze The Validation Contract

The exact validation contract for Phase 1 is now frozen and executed.

Required gates:

- `make -j4 CppunitTest_sc_ucalc_dependency_shadow`
- `make -j4 CppunitTest_sc_ucalc_workbook_facade`
- at least one focused Calc formula/execution lane for touched helpers
- standalone tests for new helper/runtime code
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary`
- `git diff --check`

Executed closeout lane:

- `make -j4 CppunitTest_sc_ucalc_dependency_shadow`
- `make -j4 CppunitTest_sc_ucalc_workbook_facade`
- `make -j4 CppunitTest_sc_ucalc_formula2`
- `cmake --build spreadsheet_engine/build_check --target spreadsheetengine_execution_tests spreadsheetengine_fods_evaluator_tests spreadsheetengine_fods_replay_tests -j4`
- `spreadsheet_engine/build_check/spreadsheetengine_execution_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_evaluator_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary`
- `git diff --check`

Completion criteria for Phase 0:

- the orchestration/execution handoff files are explicitly documented
- the first helper extraction surface is named and bounded
- the initial defer list is named so later phases do not sprawl
- the Phase 1 validation contract is frozen in this document

### Phase 1: Extract Shared Coercion And Scalar Execution Helpers

Status: **Complete**

Goals:

- move duplicated scalar coercion and argument-normalization mechanics behind
  shared runtime or compat helpers
- make `FormulaEvaluator` one client of the helper layer rather than its owner
- let Calc call the same helper layer through thin adapters while keeping Calc
  as the execution host

Deliverables:

- shared helper layer for numeric, text, and logical coercion plus
  argument/index normalization
- reduced duplication between `FormulaEvaluator` and `ScInterpreter`
- focused standalone and Calc parity tests
- explicit defer list for the remaining stack-shell work

Phase 1 shipped as six concrete workstreams.

#### 1.1 Define The Shared Coercion Vocabulary

Implemented outcome:

An engine-owned helper surface now exists in
`spreadsheet_engine/inc/spreadsheetengine/runtime/ScalarCoercion.hxx`. It is
explicit about coercion intent, not Calc stack shape.

The landed helper vocabulary covers:

- coerce scalar to number
- coerce scalar to boolean
- coerce scalar to string
- normalize whole-number / index / position arguments
- shared text-position validation used by both standalone and Calc

Deferred from this first helper layer:

- date-serial coercion, because the remaining behavior split is still too
  host-shaped to claim as a safe Phase 1 extraction
- reference and matrix scalarization, because the current remaining cases still
  depend on Calc stack and reference iteration semantics

The helper API returns engine-shaped results:

- successful scalar result
- spreadsheet error result
- explicit "unsupported here" result where stack-sensitive behavior still
  belongs to Calc

Preferred file areas:

- `spreadsheet_engine/inc/spreadsheetengine/runtime/`
- `spreadsheet_engine/source/core/`
- `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/` for thin Calc
  adapters only

#### 1.2 Move Standalone `FormulaEvaluator` To The Shared Layer

Implemented outcome:

`FormulaEvaluator` now consumes the shared helper layer instead of keeping the
first implementation evaluator-private.

Primary targets:

- `spreadsheet_engine/source/core/FormulaEvaluatorInternals.hxx`
- `spreadsheet_engine/source/core/FormulaEvaluator*.cxx`
- `spreadsheet_engine/inc/spreadsheetengine/detail/FormulaEvaluator.hxx`

Implemented in:

- `spreadsheet_engine/source/core/CoreRuntimeUtils.hxx`
- `spreadsheet_engine/source/core/FormulaEvaluatorUtils.hxx`
- `spreadsheet_engine/source/core/FormulaEvaluatorInternals.hxx`
- `spreadsheet_engine/source/core/FormulaEvaluatorText.cxx`
- `spreadsheet_engine/cmake/sources/PublicHeaders.cmake`

#### 1.3 Add Thin Calc Adapters

Implemented outcome:

Calc now reaches the same normalization behavior through thin adapters without
moving stack walking.

That means:

- Calc still pops arguments from the interpreter stack
- Calc converts popped values into the shared helper input shape
- Calc calls the shared helper for coercion and normalization decisions
- Calc maps shared helper output back to legacy `Push*()` / error behavior

Primary targets:

- `sc/source/core/tool/interpr*.cxx`
- `sc/source/core/inc/interpre.hxx`
- `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`

Implemented in:

- `sc/source/core/inc/interpre.hxx`
- existing `interpr1.cxx` text-family call sites through
  `GetStringPositionArgument()`

The key rule remains:

- adapt stack state at the boundary, not inside the shared helper

#### 1.4 Migrate The First Bounded Helper Families

The first bounded families that landed were:

1. numeric coercion and whole-number argument normalization
2. boolean coercion for scalar evaluator inputs
3. string coercion and text-position/index normalization

This phase explicitly continued to avoid:

- reference-list traversal
- matrix iteration beyond simple scalarization
- dynamic reference expansion
- stack-sensitive short-circuit control flow

#### 1.5 Add Direct Differential Tests

Phase 1 now has direct tests that prove the new helper layer, not just indirect
replay.

Implemented test shapes:

- standalone unit tests for helper semantics
- focused Calc Cppunit coverage for the same coercion cases
- side-by-side parity cases for representative functions that depend on the
  migrated helpers

The landed parity surface is:

- string/index normalization: `LEFT`, `RIGHT`, `MID`, `SEARCH`, `REPLACE`,
  `SUBSTITUTE`

Implemented in:

- `spreadsheet_engine/tests/unit/execution_api_tests.cxx`
- `spreadsheet_engine/tests/unit/fods_evaluator_tests.cxx`
- `sc/qa/unit/ucalc_formula2.cxx`

#### 1.6 Record The Remaining Defer List For Phase 2

The explicit defer list for Phase 2 is now frozen.

Calc-owned after Phase 1 because it still depends on stack shell behavior:

- token walking
- stack cursor mutation
- opcode dispatch shell mechanics
- jump/short-circuit stack behavior
- range/list/reference iteration that is still interpreter-shaped
- date-serial coercion paths that still depend on Calc-side host shaping
- reference/matrix scalarization paths that still depend on interpreter-shaped
  stack/reference state

Completion criteria for Phase 1, now met:

- the new shared coercion helper layer exists in engine-owned code
- `FormulaEvaluator` uses it for the migrated coercion families
- Calc uses thin adapters into the same helper layer for the migrated families
- no scheduler/orchestration ownership changes are introduced
- direct standalone and Calc differential tests cover the migrated helpers
- replay remains at `0` cached-fallback cells on the promoted corpus

### Phase 2: Extract Token Walking / Stack Shell Helpers

Status: **Complete**

Goals:

- move reusable execution-shell mechanics out of Calc while leaving Calc as
  the execution host

Deliverables:

- engine/compat helpers for stack walking and token dispatch support
- reduced Calc-local evaluator shell logic
- differential validation around stack-sensitive functions

Implemented outcome:

Phase 2 landed the first bounded stack-shell extraction surface rather than a
full `ScInterpreter` rewrite. Calc now routes the operator-dispatch shell
through extracted helpers for:

- scalar and matrix comparison dispatch (`=`, `<>`, `<`, `>`, `<=`, `>=`)
- stack-driven logical folds (`AND`, `OR`, `XOR`)
- unary matrix/scalar operator dispatch (`NOT`, unary minus)
- synthetic binary dispatch for percent-sign execution

Implemented in:

- `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpreterDispatch.hxx`
- `sc/source/core/inc/interpre.hxx`
- `sc/source/core/tool/interpr1.cxx`
- `sc/qa/unit/ucalc_formula2.cxx`

What Phase 2 intentionally did not move:

- general token walking across the full opcode dispatcher
- short-circuit and jump stack behavior
- reference-list traversal beyond the operator/logical shell
- host-mediated reference and matrix iteration outside the bounded operator
  surface

Completion criteria for Phase 2, now met:

- the first operator-shell helper family is extracted behind a compat/shared
  dispatch surface
- Calc-local duplication in `interpr1.cxx` is materially reduced for the
  migrated operators
- focused Calc tests cover scalar, range, matrix, and synthetic-dispatch cases
- the full standalone and replay validation baseline remains green

### Phase 3: Extract Reference-Sensitive Execution Slices

Status: **Complete**

Goals:

- move the first reference-sensitive execution families behind the stable
  orchestration boundary

Deliverables:

- explicit bridge APIs for host-only services
- extracted execution slices validated against Calc behavior
- updated ownership map for remaining Calc-only execution logic

Phase 3 shipped as three concrete workstreams.

#### 3.1 Extract Explicit Reference/Lookup Compat Bridges

The first bounded reference-sensitive helpers now live behind dedicated compat
surfaces instead of remaining as duplicated local logic inside `ScInterpreter`.

Landed bridge surfaces:

- `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/LookupExecution.hxx`
- `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx`
- `spreadsheet_engine/inc/spreadsheetengine/runtime/ReferenceText.hxx`

What moved behind those bridges:

- `MATCH` / `XMATCH` vector materialization and index resolution
- `ADDRESS` string formatting with Calc-shaped sheet-prefix behavior
- `OFFSET` range planning
- `INDEX` reference-area planning and area normalization

These bridges are intentionally header-only because they depend on Calc-owned
host symbols and need to compile in Calc translation units rather than inside
`libspreadsheetenginelo`.

#### 3.2 Rewire The First Calc Reference-Sensitive Families

Calc now consumes the bounded bridges in:

- `sc/source/core/tool/interpr1.cxx`
- `sc/source/core/inc/interpre.hxx`

The migrated Calc entry points are:

- `ScMatch()` / `ScXMatch()`
- `ScAddressFunc()`
- `ScOffset()`
- `ScIndex()`

Calc still owns:

- stack cursor movement and stack typing
- external-name/name-resolution plumbing outside the bounded bridge surface
- matrix/reference traversal outside the migrated `MATCH` / `XMATCH` /
  `ADDRESS` / `OFFSET` / `INDEX` slice

#### 3.3 Remove Duplicate Standalone Helper Tails

Phase 3 also removed leftover duplicate reference-text helpers from:

- `spreadsheet_engine/source/core/FormulaEvaluatorInternals.hxx`
- `spreadsheet_engine/source/core/CompiledFormulaInflation.cxx`

Those paths now use the shared `runtime/ReferenceText.hxx` helper layer.

Completion criteria for Phase 3, now met:

- the first reference-sensitive execution helpers are extracted behind explicit
  compat bridges
- Calc consumes those bridges for a bounded live function slice without moving
  storage or queue authority
- standalone reference/lookup tests, focused Calc lanes, evaluator tests, and
  one-shot replay summary remain green
- the remaining Calc-owned reference shell is explicitly narrowed to the next
  bounded slice

### Phase 4: Expand Bounded Lookup And Reference Traversal

Status: **Complete**

Goals:

- move the next bounded lookup/reference walkers without reopening general
  token walking
- keep Calc host ownership for name resolution and document-backed services
- continue replacing duplicated vector/reference traversal one family at a time

Target surface:

- `LOOKUP`, `VLOOKUP`, `HLOOKUP`, and `XLOOKUP` traversal helpers
- bounded reference/name-resolution helpers that still sit directly in
  `ScInterpreter`
- additional workbook-address/string-reference glue only where a compat bridge
  is clearly narrower than the existing Calc shell

Phase 4 shipped as four concrete workstreams.

#### 4.1 Expand The Lookup Compat Bridge Surface

The first task is to widen the new compat bridge layer just enough to support
bounded lookup traversal without forcing Calc storage or stack mechanics into
the engine.

Primary targets:

- `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/LookupExecution.hxx`
- `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx`
- `spreadsheet_engine/source/core/FormulaEvaluatorLookup.cxx`

Required bridge capabilities:

- shared lookup-vector materialization for one-dimensional and tabular inputs
- shared result-slice planning for `VLOOKUP` / `HLOOKUP` / `XLOOKUP`
- bounded not-found and approximate-match shaping that stays aligned with Calc
- narrow reference/name helpers only where the lookup family cannot stay
  coherent without them

Bridge rules:

- the bridge must stay compat-shaped and may depend on Calc host symbols
- do not move `ScDocument` ownership, token arrays, or general stack walking
- prefer header-only compat helpers if the implementation depends directly on
  Calc symbols that do not belong in `libspreadsheetenginelo`

#### 4.2 Rewire Calc Lookup Entry Points One Family At A Time

Calc should consume the expanded bridge in tightly bounded slices rather than
through a single all-at-once rewrite.

Primary Calc targets:

- `sc/source/core/tool/interpr1.cxx`
- `sc/source/core/inc/interpre.hxx`

Recommended execution order:

1. `LOOKUP`
2. `VLOOKUP` / `HLOOKUP`
3. `XLOOKUP`

For each adoption step:

- keep Calc-owned stack mutation local to `ScInterpreter`
- route lookup-input materialization and match/result planning through the
  compat bridge
- preserve existing Calc error shaping and document-backed behavior where host
  semantics still matter

#### 4.3 Keep Standalone And Calc On The Same Lookup Core

Phase 4 should reduce duplication, not create a second lookup implementation.

Primary standalone targets:

- `spreadsheet_engine/source/core/FormulaEvaluatorLookup.cxx`
- `spreadsheet_engine/source/core/FormulaEvaluatorInternals.hxx`

Required outcome:

- the standalone evaluator and Calc compat bridge should both rely on the same
  underlying lookup runtime contracts
- any newly extracted planner/materializer helper should be shared between
  standalone and Calc where feasible
- if a helper cannot be shared yet, the reason should be explicitly documented
  as a Calc-host constraint rather than left as accidental duplication

#### 4.4 Validation And Closeout Contract

Completion criteria for Phase 4, now met:

- the migrated lookup families are executing through the compat bridge in Calc
- standalone lookup evaluation still uses the same authoritative lookup runtime
- the replay baseline remains fully green
- the remaining Calc-owned lookup/reference shell is narrowed to the next
  bounded slice instead of left as an open-ended tail

Focused validation lane for every Phase 4 slice:

- `make -j4 CppunitTest_sc_ucalc_shared_cases`
- `make -j4 CppunitTest_sc_ucalc_formula2`
- `make -j4 CppunitTest_sc_ucalc_dependency_shadow`
- `make -j4 CppunitTest_sc_ucalc_workbook_facade`
- `cmake --build spreadsheet_engine/build_check --target spreadsheetengine_lookup_tests spreadsheetengine_reference_tests spreadsheetengine_fods_evaluator_tests spreadsheetengine_fods_replay_tests -j4`
- `spreadsheet_engine/build_check/spreadsheetengine_lookup_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_reference_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_evaluator_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary`
- `git diff --check`

Concrete file/test inventory for Phase 4:

- Calc execution surface:
  - `sc/source/core/tool/interpr1.cxx`
  - `sc/source/core/inc/interpre.hxx`
- Standalone execution surface:
  - `spreadsheet_engine/source/core/FormulaEvaluatorLookup.cxx`
- Compat bridges:
  - `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/LookupExecution.hxx`
  - `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx`
- Focused Calc tests:
  - `sc/qa/unit/ucalc_shared_cases.cxx`
  - `sc/qa/unit/ucalc_formula2.cxx`
- Focused standalone tests:
  - `spreadsheet_engine/tests/unit/lookup_api_tests.cxx`
  - `spreadsheet_engine/tests/unit/fods_evaluator_tests.cxx`

Phase 4 explicit defer list:

- general token walking across the remaining opcode dispatcher
- short-circuit and jump-shell behavior
- broader name-resolution compilation flows outside the bounded lookup slice
- storage-sensitive database and matrix families
- any recalc-authority, listener, or queue-boundary changes

### Phase 5: Extract Bounded Token-Walking And Reference-Shape Shell Helpers

Status: **Complete**

Goals:

- move the next bounded Calc-owned execution shell after completed lookup
  traversal without widening into a full opcode-dispatch rewrite
- extract reusable reference-shape and scalar/area-selection helpers that still
  duplicate between Calc and standalone
- keep Calc as the stack and storage host while shared helpers take over the
  decision logic

Target surface:

- bounded token-walking helpers that decide which reference/scalar form a
  function consumes
- reference cardinality and shape helpers used by `ROW`, `COLUMN`, `ROWS`,
  `COLUMNS`, `AREAS`, `SHEET`, and `SHEETS`
- narrow name/reference materialization glue only where it is directly needed
  by those adopters and the already-landed lookup/reference bridge

Phase 5 landed as four concrete workstreams.

#### 5.1 Freeze The Bounded Stack/Reference Surface

The first task is to freeze exactly which remaining Calc shell behaviors this
phase is allowed to move.

Primary targets:

- `sc/source/core/tool/interpr1.cxx`
- `sc/source/core/inc/interpre.hxx`
- `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx`
- `spreadsheet_engine/source/core/FormulaEvaluatorOperators.cxx`
- `spreadsheet_engine/source/core/FormulaEvaluator.cxx`

Required boundary decisions:

- in scope:
  - stack-adapted selection between scalar, single-reference, double-reference,
    and bounded ref-list inputs
  - reference cardinality and row/column/area-count shaping
  - bounded sheet-count / sheet-index shaping where host-backed sheet metadata
    is already available through Calc
- out of scope:
  - `INDIRECT`
  - jump/short-circuit token walking
  - general opcode-dispatch-table extraction
  - structural reference rewrites and name-compilation flows

Landed outcome:

- the bounded Phase 5 surface stayed limited to `ROW`, `COLUMN`, `ROWS`,
  `COLUMNS`, `AREAS`, `SHEET`, and `SHEETS`
- the remaining Calc-owned shell is now explicitly the post-Phase-5
  token-walking and jump/name tail rather than another vague mixed bucket

#### 5.2 Extract Shared Reference-Shape Helpers Behind Compat Bridges

The next task is to put the selected reference-shape decisions behind explicit
shared or compat-owned helpers.

Primary targets:

- `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx`
- `spreadsheet_engine/inc/spreadsheetengine/runtime/ReferenceText.hxx`
- `spreadsheet_engine/source/core/FormulaEvaluatorOperators.cxx`
- `spreadsheet_engine/source/core/FormulaEvaluatorSpreadsheet.cxx`

Required helper surface:

- shared result types for:
  - scalarized reference information
  - row-count / column-count / area-count results
  - sheet-index / sheet-count results
- compat helpers that:
  - consume Calc-owned reference state
  - compute bounded cardinality/shape results
  - return Calc-shaped error/unsupported outcomes without owning stack mutation

Landed helper surface:

- prefer shared engine helpers for pure counting/shape logic
- keep host-backed sheet/document queries in compat helpers
- do not introduce a second standalone-only implementation for the same
  reference-shape logic

Concrete landed pieces:

- shared reference-shape/count helpers in `spreadsheet_engine/api/Reference.hxx`
- Calc compat adapters in
  `spreadsheet_engine/compat/libreoffice/ReferenceExecution.hxx`
- standalone reference-range materialization reused through
  `FormulaEvaluator::resolveReferenceRangeText(...)`

#### 5.3 Rewire Calc And Standalone Adopters In A Narrow Order

Calc should adopt the new helpers in an explicit order from least risky to more
reference-sensitive.

Recommended execution order:

1. `ROW` / `COLUMN`
2. `ROWS` / `COLUMNS`
3. `AREAS`
4. `SHEET` / `SHEETS`

Primary Calc targets:

- `sc/source/core/tool/interpr1.cxx`
- `sc/source/core/inc/interpre.hxx`

Primary standalone targets:

- `spreadsheet_engine/source/core/FormulaEvaluatorSpreadsheet.cxx`
- `spreadsheet_engine/source/core/FormulaEvaluator.cxx`

Landed outcome:

- Calc entry points keep stack mutation local
- the shared helper layer owns the reference-shape decision logic
- standalone uses the same helper surface where feasible
- any remaining Calc-only logic is documented as a host or stack constraint,
  not left as accidental duplication

Adopter order as landed:

1. `ROW` / `COLUMN`
2. `ROWS` / `COLUMNS`
3. `AREAS`
4. `SHEET` / `SHEETS`

#### 5.4 Validation And Closeout Contract

Completion criteria for Phase 5:

- the selected reference-shape families execute through extracted helper logic
  instead of Calc-local duplicated shell code
- standalone and Calc rely on the same bounded helper surface for the migrated
  shape/count semantics wherever feasible
- the remaining Calc-owned execution shell is narrowed to post-Phase-5 token
  walking and jump/name flows, not another vague mixed tail
- the replay baseline remains fully green

Phase 5 closeout status:

- complete
- validated across the full focused Calc and standalone lane below
- one-shot replay baseline remains `500` workbooks, `50,661` formula cells,
  `0` cached-fallback cells

Focused validation lane for every Phase 5 slice:

- `make -j4 CppunitTest_sc_ucalc_shared_cases`
- `make -j4 CppunitTest_sc_ucalc_formula2`
- `make -j4 CppunitTest_sc_ucalc_dependency_shadow`
- `make -j4 CppunitTest_sc_ucalc_workbook_facade`
- `cmake --build spreadsheet_engine/build_check --target spreadsheetengine_lookup_tests spreadsheetengine_reference_tests spreadsheetengine_fods_evaluator_tests spreadsheetengine_fods_replay_tests -j4`
- `spreadsheet_engine/build_check/spreadsheetengine_lookup_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_reference_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_evaluator_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary`
- `git diff --check`

Concrete file/test inventory for Phase 5:

- Calc execution surface:
  - `sc/source/core/tool/interpr1.cxx`
  - `sc/source/core/inc/interpre.hxx`
- Standalone execution surface:
  - `spreadsheet_engine/source/core/FormulaEvaluatorSpreadsheet.cxx`
  - `spreadsheet_engine/source/core/FormulaEvaluatorOperators.cxx`
- Compat/runtime helpers:
  - `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx`
  - `spreadsheet_engine/inc/spreadsheetengine/runtime/ReferenceText.hxx`
- Focused Calc tests:
  - `sc/qa/unit/ucalc_shared_cases.cxx`
  - `sc/qa/unit/ucalc_formula2.cxx`
- Focused standalone tests:
  - `spreadsheet_engine/tests/unit/reference_api_tests.cxx`
  - `spreadsheet_engine/tests/unit/fods_evaluator_tests.cxx`

Phase 5 explicit defer list:

- `INDIRECT` and string-to-reference compilation
- jump/short-circuit token walking
- broad opcode-dispatch-table extraction
- storage-sensitive database/matrix families
- queue authority, listener ownership, or workbook-storage changes

## Validation Strategy

Every execution slice should satisfy all of the following:

- `make -j4 CppunitTest_sc_ucalc_dependency_shadow`
- `make -j4 CppunitTest_sc_ucalc_workbook_facade`
- standalone tests for touched helper/runtime code
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary`
- any focused Calc Cppunit lane touched by the slice
- `git diff --check`

For Phases 0 and 1 specifically, the minimum focused Calc lane should be the
lane that exercises the exact helper semantics being moved, not only
high-level replay.

Suggested focused Calc lanes for Phase 1:

- `make -j4 CppunitTest_sc_ucalc_formula`
- `make -j4 CppunitTest_sc_ucalc_formula2`

Suggested standalone additions for Phase 1:

- dedicated unit tests for the new coercion helper layer
- targeted evaluator tests proving `FormulaEvaluator` now consumes that layer

Suggested focused Calc lane for Phase 2:

- `make -j4 CppunitTest_sc_ucalc_formula2`

Suggested focused Calc lanes for Phase 3:

- `make -j4 CppunitTest_sc_ucalc_shared_cases`
- `make -j4 CppunitTest_sc_ucalc_formula2`

Suggested standalone additions for Phase 3:

- `spreadsheetengine_lookup_tests`
- `spreadsheetengine_reference_tests`

## Critical Files

| File | Role |
| --- | --- |
| [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md) | Consolidated project state and roadmap |
| [RECALC_ORCHESTRATION_EXTRACTION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/RECALC_ORCHESTRATION_EXTRACTION.md) | Completed orchestration milestone and handoff boundary |
| [RecalcQueueExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/RecalcQueueExecution.hxx) | Calc bridge that consumes engine-owned recalc queue output |
| [InterpreterDispatch.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpreterDispatch.hxx) | Shared compat support for Calc operator dispatch and logical-fold shell helpers |
| [LookupExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/LookupExecution.hxx) | Calc compat bridge for bounded `MATCH` / `XMATCH` execution |
| [ReferenceExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx) | Calc compat bridge for `ADDRESS` / `OFFSET` / `INDEX` planning and formatting |
| [FormulaEvaluator.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluator.cxx) | Standalone execution shell reference point |
| [FormulaEvaluatorInternals.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluatorInternals.hxx) | Current standalone coercion and evaluator-helper concentration point |
| [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx) | Calc formula-cell execution host |
| [interpr*.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx) | Calc execution backend surface |
