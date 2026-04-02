# Execution Shell Closeout Plan

## Purpose

This document turns the remaining "finish the execution shell extraction"
frontier into a concrete implementation plan.

The goal is not to move all of `ScInterpreter` into `spreadsheet_engine/`.
The goal is to finish moving the remaining spreadsheet-specific execution
semantics out of Calc while keeping Calc as the storage and application host.

## What "Finished" Means

The execution-shell extraction should be considered complete when all of the
following are true:

- spreadsheet-specific execution decisions live in engine-owned runtime code or
  in narrow compat bridges under `spreadsheet_engine/`
- Calc keeps only host responsibilities such as storage access, stack storage,
  document/session services, number-format services, printer/path access,
  external-document integration, and other unavoidable host-side side effects
- the remaining Calc-local interpreter code is clearly host-shaped rather than
  a second spreadsheet-semantics implementation
- the promoted replay corpus remains at `0` cached-fallback cells
- there are no known in-scope duplicated implementations between standalone and
  Calc for the migrated execution surface

This closeout is therefore about boundary quality, not about reducing the raw
line count of `ScInterpreter` at any cost.

## Current Remaining Scope

After the completed execution-backend slices, the remaining execution shell is
concentrated in four areas:

1. broader token-walking and frame interpretation inside `ScInterpreter`
2. remaining reference/name/range traversal helpers that are still Calc-local
3. host-heavy information/property inspection tails, especially the remaining
   `CELL(...)` surface
4. residual duplication where Calc still carries spreadsheet-semantic helpers
   that should route through engine-owned runtime or compat code

These areas should be closed out in the order below.

## Boundary Rules

The plan must continue to respect the settled project boundary:

- keep `ScDocument`, formula-cell storage, listener wiring, and document
  mutation in Calc
- keep recalc authority and queue-construction boundaries unchanged
- do not rewrite `ScInterpreter` wholesale
- do not migrate UI, import/export, UNO, rendering, or threaded/OpenCL backend
  responsibilities into the engine
- prefer small compat seams plus shared runtime over broad cross-layer
  entanglement

## Workstreams

### Workstream 1: Freeze The Remaining Shell Inventory

Status: **Complete**

Before landing more extraction slices, produce and keep an explicit inventory of
what is still Calc-owned in the execution shell.

Required outputs:

- a classified list of remaining Calc-local execution clusters under:
  - `sc/source/core/tool/interpr1.cxx`
  - `sc/source/core/tool/interpr2.cxx`
  - `sc/source/core/tool/interpr4.cxx`
  - `sc/source/core/tool/interpr8.cxx`
- an ownership label for each cluster:
  - `move to shared runtime`
  - `move to compat bridge`
  - `keep host-only`
- a first-pass mapping from each cluster to its future validation lane

The point of this workstream is to prevent the remaining shell from becoming a
moving target while later slices land.

Primary file targets:

- [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx)
- [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- [interpr2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr2.cxx)
- [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- [interpr8.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr8.cxx)
- [EXECUTION_BACKEND_EXTRACTION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/EXECUTION_BACKEND_EXTRACTION.md)

Exit criteria:

- every remaining execution-shell cluster is explicitly classified
- there is no unowned "miscellaneous interpreter tail" left in the plan
- each remaining cluster is attached to a concrete later workstream

Inventory result:

| File area | Remaining cluster | Ownership | Validation lane | Follow-on workstream |
| --- | --- | --- | --- | --- |
| `sc/source/core/tool/interpr4.cxx` | scalar-reference selection and implicit-intersection helpers (`DoubleRefToPosSingleRef`, `PopDoubleRefOrSingleRef`) | move to shared runtime plus compat bridge | `CppunitTest_sc_ucalc_formula2`, `spreadsheetengine_reference_tests`, replay summary | Workstream 2 / 3 |
| `sc/source/core/tool/interpr4.cxx` | matrix-condition and frame-conversion decisions (`ConvertMatrixJumpConditionToMatrix`, `ConvertMatrixParameters`, `PopDoubleRefPushMatrix`) | move to compat bridge | `CppunitTest_sc_ucalc_shared_cases`, `spreadsheetengine_execution_tests`, replay summary | Workstream 2 |
| `sc/source/core/tool/interpr4.cxx` | ref-list materialization and scalar/column-vector choice (`PopRefListPushMatrixOrRef`) | move to shared runtime plus compat bridge | `CppunitTest_sc_ucalc_shared_cases`, `spreadsheetengine_reference_tests`, replay summary | Workstream 3 |
| `sc/source/core/tool/interpr1.cxx` | host-heavy `CELL(...)` property tail and external variants (`FILENAME`, `FORMAT`, `WIDTH`, `PREFIX`, `PROTECT`, `COLOR`, `PARENTHESES`) | move subtype contract to shared runtime and compat bridge; keep host service access in Calc | `CppunitTest_sc_ucalc_formula2`, `spreadsheetengine_fods_evaluator_tests`, replay summary | Workstream 4 |
| `sc/source/core/tool/interpr1.cxx` | `INFO(...)` platform/session inspection | keep host-only | `information/fods` replay, focused Calc coverage | final closeout docs |
| `sc/source/core/tool/interpr2.cxx` | range-constructor and union token-container glue (`ScRangeFunc`, `ScUnionFunc`, `ScMultiArea`) | keep host-only because it mutates Calc token containers rather than expressing standalone runtime semantics | `CppunitTest_sc_ucalc_formula2`, replay summary | final closeout docs |
| `sc/source/core/tool/interpr4.cxx` | external-reference fetch and cache plumbing (`PopExternalSingleRef`, `PopExternalDoubleRef`, `GetExternalDoubleRef`) | keep host-only | focused Calc coverage, replay summary | final closeout docs |
| `sc/source/core/tool/interpr8.cxx` | no new independent execution cluster; remaining uses are adopters of extracted helpers | n/a | inherited from touched helper lanes | n/a |

This inventory is the frozen target for the rest of the closeout program.
Anything not listed above is now treated as either already extracted or
intentionally host-owned.

### Workstream 2: Extract The Broader Token-Walking And Frame Shell

Status: **Complete**

This is the highest-value remaining technical slice.

The target here is not raw stack mutation itself. The target is the
spreadsheet-semantic decision logic around token walking and frame
interpretation.

Extract these categories where feasible:

- parameter/frame interpretation rules
- scalar vs reference vs matrix dispatch decisions
- reference-list and area-selection planning
- result-shaping decisions that do not inherently depend on Calc storage
- token-walking decisions that can be expressed in engine-owned types without
  carrying Calc runtime state with them

Keep these Calc-owned:

- actual stack container mutation
- raw token cursor ownership
- document-backed value fetches
- host-specific side effects

Primary implementation pattern:

- shared helper or runtime code under `spreadsheet_engine/source/core/` or
  `spreadsheet_engine/inc/spreadsheetengine/runtime/`
- narrow Calc adapters under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`
- thin call-site rewiring in `interpr*.cxx`

Likely file targets:

- [FormulaEvaluator.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluator.cxx)
- [FormulaEvaluatorOperators.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluatorOperators.cxx)
- [FormulaEvaluatorInternals.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluatorInternals.hxx)
- [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx)
- [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- [interpr2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr2.cxx)
- [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- new compat helpers under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`

Exit criteria:

- the targeted token-walking decision helpers no longer exist only inside Calc
- standalone and Calc consume the same extracted planning/result-shaping logic
- the remaining Calc-local token shell is visibly narrower and more host-only

Closeout result:

- matrix-condition conversion policy now routes through
  `compat/libreoffice/MatrixFrameExecution.hxx`
- `ConvertMatrixJumpConditionToMatrix()` and the matrix/reflist branches inside
  `ConvertMatrixParameters()` no longer carry their spreadsheet-semantic
  eligibility rules inline in Calc
- the extracted frame-planning rules now have direct standalone unit coverage in
  `spreadsheetengine_execution_tests`

### Workstream 3: Extract The Remaining Reference/Name Traversal Shell

Status: **Complete**

This workstream closes the remaining reference-sensitive helpers that still
behave like spreadsheet semantics rather than like storage services.

Target areas:

- residual name-resolution decisions
- remaining string-reference classification and compilation helpers
- reference-list traversal policies
- range/area normalization and traversal rules
- any remaining bounded sheet/name/reference helper logic still duplicated
  between standalone and Calc

The preferred pattern is:

- engine-owned reference semantics in `runtime/` or evaluator support
- Calc-specific document/name/external-service access behind compat adapters

Primary file targets:

- [ReferenceExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx)
- [IndirectExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/IndirectExecution.hxx)
- [LookupExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/LookupExecution.hxx)
- [ReferenceText.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/runtime/ReferenceText.hxx)
- [FormulaEvaluatorLookup.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluatorLookup.cxx)
- [FormulaEvaluatorInformation.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluatorInformation.cxx)
- [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- [interpr8.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr8.cxx)

Exit criteria:

- remaining in-scope reference/name traversal rules are engine-owned or compat-owned
- Calc-local helpers in this area are reduced to document-backed service calls
- no duplicate standalone-vs-Calc semantic helpers remain for the migrated
  reference/name surface

Closeout result:

- scalar-reference selection now routes through shared helper planning in
  `api/Reference.hxx` and Calc compat adapters in
  `compat/libreoffice/ReferenceExecution.hxx`
- ref-list materialization policy (`keep list` vs `single ref` vs `column
  vector`) is now shared instead of being decided inline in `ScInterpreter`
- the remaining Calc-local code in these call sites is limited to stack
  mutation and document-backed cell fetches

### Workstream 4: Finish The Host-Heavy Inspection And Property Tail

Status: **Complete**

This workstream closes the remaining inspection/property shell that still sits
in Calc because it depends on host state.

The main explicit target is the remaining `CELL(...)` tail:

- `FILENAME`
- `FORMAT`
- `WIDTH`
- `PREFIX`
- `PROTECT`
- `COLOR`
- `PARENTHESES`
- external-reference variants

The goal is not to pretend these are pure runtime helpers. The goal is to move
their spreadsheet-semantic classification and result contract behind compat
helpers, while leaving printer, path, format table, and other host service
access inside Calc adapters.

If other information/property functions remain Calc-local for the same reason,
they should be folded into the same workstream.

Primary implementation pattern:

- subtype classification and result-shaping contract in shared runtime
- host-backed services in compat helpers
- thin Calc call sites

Likely file targets:

- [CellInspection.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/runtime/CellInspection.hxx)
- [CellInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx)
- [FormulaEvaluatorInformation.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluatorInformation.cxx)
- [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- Calc number-format, printer, and document helper call sites reached from
  `ScCell()`

Exit criteria:

- the full in-scope `CELL(...)` surface is either compat-owned or explicitly
  documented as permanently host-only
- the `CELL(...)` subtype contract is centralized instead of split across
  standalone and Calc
- replay and focused Calc tests remain green

Closeout status:

- complete
- `CELL(...)` subtype classification now lives in shared
  [CellInspection.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/runtime/CellInspection.hxx)
- Calc property-tail shaping now routes through
  [CellInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx)
- the remaining open shell is no longer the `CELL(...)` tail; it is the
  broader token-walking shell plus explicitly host-only information,
  token-container, and external-reference plumbing

### Workstream 5: Eliminate Residual Duplicate Implementations

Status: **Complete**

After the remaining shell helpers are extracted, perform a cleanup pass focused
on duplication.

This pass should:

- audit `FormulaEvaluator*` against remaining `ScInterpreter` helpers
- audit `scaddins` execution overlap where it is still in the same semantic
  surface
- remove dead Calc-local helper tails that became redundant after rewiring
- prefer engine-owned helper calls at Calc call sites wherever the semantics
  are already shared and validated

Primary file targets:

- [FormulaEvaluator.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluator.cxx)
- [FormulaEvaluatorInformation.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluatorInformation.cxx)
- [FormulaEvaluatorLookup.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluatorLookup.cxx)
- [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- [interpr2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr2.cxx)
- [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- [interpr8.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr8.cxx)

Exit criteria:

- no remaining known in-scope duplicate execution helpers exist between
  standalone and Calc
- any intentionally retained Calc-local helper is explicitly host-only
- the remaining interpreter shell is smaller and easier to explain

Closeout status:

- complete
- the dead Calc-local `ScCell()` branch tree for already-extracted
  `COL` / `ROW` / `SHEET` / `ADDRESS` / `CONTENTS` / `TYPE` handling is gone
- `ScCellExternal()` now dispatches through the shared `InfoKind` contract
  instead of duplicating keyword matching inline
- no known in-scope duplicate execution helpers remain for the extracted
  `CELL(...)` shell surface

### Workstream 6: Close Out The Execution Shell Program

Once the technical slices above are complete, perform the final closeout pass.

Required outputs:

- update [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)
  to reflect the new steady-state boundary
- update [EXECUTION_BACKEND_EXTRACTION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/EXECUTION_BACKEND_EXTRACTION.md)
  from "active extraction" to "completed boundary"
- refresh [README.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/README.md)
  so the next active frontier is clearly identified
- explicitly document what still remains Calc-owned and why

Exit criteria:

- the execution-shell extraction has a documented finished boundary
- the active architecture docs no longer describe the same shell work as open
- any remaining Calc-only code is intentionally host-owned, not just
  "not yet moved"

## Recommended Execution Order

The workstreams should be executed in this order:

1. Workstream 1: freeze the remaining-shell inventory
2. Workstream 2: broader token-walking and frame shell
3. Workstream 3: remaining reference/name traversal shell
4. Workstream 4: host-heavy inspection/property tail
5. Workstream 5: duplicate implementation cleanup
6. Workstream 6: final doc and boundary closeout

That order matters because the cleanup pass should happen only after the real
semantic migration is finished.

## Validation Strategy

### Minimum Validation For Every Slice

Every extraction slice in this plan should run all of the following:

- a focused Calc Cppunit lane that directly exercises the touched shell surface
- standalone tests for the new helper/runtime surface
- `spreadsheetengine_fods_evaluator_tests`
- `git diff --check`

### Standard Calc Validation Lanes

Use these as the default Calc regression lanes unless the slice is clearly
narrower:

- `make -j1 CppunitTest_sc_ucalc_formula2`
- `make -j1 CppunitTest_sc_ucalc_shared_cases`

Run these whenever the extraction slice touches the boundary around workbook,
dependency, or recalc behavior:

- `make -j1 CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CppunitTest_sc_ucalc_workbook_facade`

### Standard Standalone Validation Lanes

Use the touched subset plus the main evaluator/replay lanes:

- `cmake --build spreadsheet_engine/build_check --target spreadsheetengine_execution_tests -j4`
- `cmake --build spreadsheet_engine/build_check --target spreadsheetengine_lookup_tests -j4`
- `cmake --build spreadsheet_engine/build_check --target spreadsheetengine_reference_tests -j4`
- `cmake --build spreadsheet_engine/build_check --target spreadsheetengine_fods_evaluator_tests -j4`
- `cmake --build spreadsheet_engine/build_check --target spreadsheetengine_fods_replay_tests -j4`

Run the relevant executables after the build:

- `spreadsheet_engine/build_check/spreadsheetengine_execution_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_lookup_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_reference_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_evaluator_tests`

### Corpus-Wide Validation Requirement

Every meaningful slice should finish with:

- `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary`

That one-shot promoted-corpus summary remains the authoritative boundary
regression gate for this program.

### Final Closeout Validation

Before declaring the execution-shell extraction complete, run the full closeout
lane:

- `make -j1 CppunitTest_sc_ucalc_formula2`
- `make -j1 CppunitTest_sc_ucalc_shared_cases`
- `make -j1 CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CppunitTest_sc_ucalc_workbook_facade`
- `cmake --build spreadsheet_engine/build_check --target spreadsheetengine_execution_tests spreadsheetengine_lookup_tests spreadsheetengine_reference_tests spreadsheetengine_fods_evaluator_tests spreadsheetengine_fods_replay_tests -j4`
- `spreadsheet_engine/build_check/spreadsheetengine_execution_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_lookup_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_reference_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_evaluator_tests`
- `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary`
- `git diff --check`

## Final Exit Criteria

The remaining execution-shell extraction should only be considered complete
when all of the following are true:

1. The broader spreadsheet-semantic execution shell is no longer Calc-local.
   What remains in Calc is host-only stack, storage, and document-service work.

2. The remaining `CELL(...)` property tail and other host-heavy inspection
   surfaces are either compat-owned or explicitly documented as permanently
   host-only.

3. There are no known in-scope duplicate implementations between standalone and
   Calc for execution semantics covered by this plan.

4. The promoted replay baseline remains:
   - `500` workbooks
   - `50,661` formula cells
   - `0` cached-fallback cells
   - `0` cached-fallback rate

5. The architecture and status docs describe the resulting steady-state
   boundary rather than treating the same work as still open.

If these conditions are met, the execution-shell extraction can be considered
finished and the project can move on to the next boundary question from a
stable, well-defined baseline.
