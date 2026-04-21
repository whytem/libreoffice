# Engine Entrypoint Widening Plan

Status: completed implementation and closeout record

## Purpose

The extraction, replay-promotion, recalc-orchestration, execution-shell,
engine-first adoption, token-boundary reduction, and production-boundary
tightening streams are complete.

The next implementation stream is to widen direct engine-entry use inside Calc
from the first bounded adoption slice to a larger set of production paths that
already have engine-owned semantics and a now-thin host boundary.

This is not a storage migration, a recalc-boundary rewrite, or a wholesale
replacement of `ScInterpreter`. It is a second-wave production adoption
program.

## Closeout Summary

This stream is complete.

It landed three concrete widening results:

- a named direct add-in financial entry seam in
  [FinancialAddInExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/FinancialAddInExecution.hxx)
  and adoption of the selected `AnalysisAddIn` callers in
  [financial.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/financial.cxx)
- a named direct bounded local `CELL(...)` inspection seam in
  [CellInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx)
  and default production adoption in
  [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- retirement of the touched caller-side financial packaging tail so the
  selected production paths now enter engine-owned semantics through one
  stable compat family instead of repeated inline orchestration

The bounded external-reference `CELL(...)` candidate was reassessed during the
closeout pass and left as a future widening candidate rather than forced into
this stream. External cache ownership and host-shaped address/file projection
still dominate that path enough that it belongs in a later fresh inventory,
not as unfinished residue inside this completed stream.

## What This Workstream Is For

This stream is successful when more production Calc evaluation paths enter
`spreadsheet_engine` directly through stable, named entry adapters while Calc
continues to own:

- document storage and mutation
- formula-cell lifecycle and side effects
- token ownership and stack mutation
- document/session services and external-reference cache ownership
- host-only environment and document-service inspection
- UI, import/export, UNO, rendering, shell, and persistence

In concrete terms, the focus is:

- building a fresh inventory of production Calc call sites that are now good
  candidates for direct engine entry
- selecting only those slices whose spreadsheet semantics are already
  engine-owned and whose host packaging is thin enough to audit
- widening direct engine entry by default for those slices
- removing superseded Calc-local orchestration in the touched scope

## Out Of Scope

This plan does not:

- move `ScDocument` storage or document mutation into the engine
- reopen the recalc authority or queue-construction boundary
- move token ownership or stack mutation out of Calc wholesale
- move host-only services such as `INFO(...)`, printer/path inspection, or
  external-reference cache ownership into the engine
- redesign threaded or OpenCL backends
- change the zero-fallback replay contract

## Definition Of Done

This workstream is complete when all of the following are true:

1. the in-scope production Calc call sites selected for this stream invoke
   bounded engine entry points directly by default
2. each adopted slice uses a named host adapter or direct compat seam rather
   than repeated inline Calc orchestration
3. superseded Calc-local orchestration in the touched scope is removed or
   reduced to thin host-only wrappers
4. the touched Calc code reads primarily as host lifecycle, storage, or
   service access rather than spreadsheet-semantic execution logic
5. the promoted replay corpus remains at `0` cached fallback under the strict
   summary assertion gate
6. Calc and standalone validation lanes remain green for every adopted slice

## Boundary Rules

Every slice in this stream must continue to respect the current project
boundary:

- keep storage, mutation, token ownership, and stack mutation in Calc
- keep host services explicit and localized at the adapter edge
- prefer thin entry adapters over broad new cross-layer plumbing
- prefer one engine-owned semantic implementation for spreadsheet behavior
- do not widen engine public API only to mirror Calc internals
- do not introduce a second Calc-local semantic implementation as a fallback

## Workstreams

### 1. Freeze The Widening Inventory

Build and maintain an explicit inventory of production Calc entry points that
are now plausible second-wave direct engine-entry adopters.

For each candidate, record:

- Calc entry point and owning file
- closest current engine runtime, evaluator, or compat entry surface
- whether the remaining gap is packaging, lifecycle, host service dependency,
  or true semantic missing piece
- whether the correct outcome is `ready now`, `needs small seam`,
  `intentionally host-only`, or `defer`
- required validation lanes

Primary target files:

- [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx)
- [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- [interpr2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr2.cxx)
- [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- [interpr5.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr5.cxx)
- compat headers under
  [spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice)

Completion criteria:

- there is no unclassified in-scope widening candidate in the touched surface
- each candidate has an owning later slice or an explicit retain/defer reason

### 2. Normalize Thin Host Adapters For The Second Wave

Converge the minimal host-side packaging needed to widen direct engine entry
without broadening the boundary again.

Target seam types:

- workbook/document context packaging
- scalar/reference/value-shape adaptation for direct evaluator entry
- explicit host service handoff for the still-host-owned services needed by
  the chosen slice
- result projection from engine values back to Calc stack/result shapes

Implementation rules:

- one explicit entry adapter is better than repeated inline orchestration
- adapters should be thin, named, and vocabulary-stable
- Calc-only types should stop at the adapter edge where practical

Validation:

- focused Calc Cppunit coverage for touched adapter entry points
- standalone coverage if shared helper surfaces change
- `git diff --check`

Completion criteria:

- the chosen direct-entry slices no longer require repeated inline context
  packaging
- the host adapter surface is small enough to audit easily

### 3. Widen Ready-Now Pure-Computation Entrypoints

Move the highest-confidence production paths first: bounded pure-computation
surfaces that already have proven engine semantics and low host-service
dependency.

Target categories:

- scalar function families already proven through engine runtime and evaluator
- production Calc call sites that still orchestrate these paths locally even
  though direct engine entry is now feasible
- add-in adoption paths whose local setup is now thinner than the semantic
  work they still perform

Implementation rules:

- prefer direct engine entry over adding another Calc-local wrapper
- keep Calc-side argument validation and error/result shape compatible
- do not widen into host-heavy services in this slice

Validation:

- focused Calc formula lanes for the touched functions
- standalone evaluator/runtime coverage for the same semantic surface
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

Completion criteria:

- the chosen pure-computation production paths route through engine entry
  points by default
- the touched local orchestration tail is removed or reduced to thin adapters

### 4. Widen Ready-Now Reference-Safe Entrypoints

After the first pure-computation slice is stable, adopt one or more bounded
reference-safe slices that use the already-extracted compat/value-shape
machinery without reopening broad token ownership.

Target categories:

- bounded lookup/reference or inspection paths whose reference-shape and
  host-service dependencies are already explicit
- call sites that already depend on extracted compat seams and now only need a
  thin direct-entry wrapper
- matrix-safe or range-safe inspection paths where result shaping is already
  shared

Implementation rules:

- keep token ownership and stack mutation in Calc
- reuse existing compat seams rather than adding overlapping ones
- keep host-only lookups explicit where still needed

Validation:

- focused Calc Cppunit coverage on the touched reference paths
- standalone lookup/reference/evaluator coverage
- strict replay summary assertion

Completion criteria:

- at least one additional bounded reference-safe production path uses direct
  engine entry
- retained Calc code in the touched area is clearly host orchestration only

### 5. Retire Superseded Orchestration In Scope

Once direct entry is the default path for the selected slices, remove or
isolate the old Calc-local orchestration.

Allowed outcomes:

- delete the local orchestration tail entirely
- keep only a thin host-only wrapper
- retain a helper only if its host-owned dependency is explicit and documented

Not acceptable:

- leaving full duplicate orchestration in place after the engine entry path is
  proven
- keeping ambiguous helper layers whose ownership cannot be explained

Validation:

- targeted Calc coverage on the former caller sites
- standalone coverage if shared entry surfaces changed
- diff hygiene and replay baseline check

Completion criteria:

- there are no silent duplicate orchestration paths in the touched scope
- any retained helper has an explicit host-only rationale

### 6. Lock The Baseline And Close The Stream

Turn the widened direct-entry result into a stable standing contract.

Required guardrails:

- one-shot `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- focused Calc lanes for every adopted direct-entry slice
- standalone evaluator/runtime lanes for the same surfaces
- doc/status updates that describe the widened direct-entry boundary in
  present tense
- clean diff hygiene

Completion criteria:

- validation is documented and repeatable
- the status docs describe the widened direct-entry boundary as current state
- any further widening work is clearly a new frontier rather than unfinished
  second-wave adoption cleanup

## Phased Implementation Approach

### Phase 1. Freeze And Publish The Widening Inventory

Status: complete

Create the explicit candidate table for production Calc entry points that are
now plausible second-wave direct engine-entry adopters.

Initial inventory targets:

| Candidate | Owning file(s) | Current boundary problem | Classification | Target outcome | Validation lanes | Later phase |
| --- | --- | --- | --- | --- | --- | --- |
| add-in financial callers already backed by shared engine runtime (`EFFECT`, `NOMINAL`, `DOLLARFR`, `DOLLARDE`, `CUMPRINC`, `CUMIPMT`, `TBILLEQ`, `TBILLPRICE`, `TBILLYIELD`) | [financial.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/financial.cxx), [analysisdefs.hxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysisdefs.hxx) | host setup is already normalized, but production callers still stop at helper-level adoption instead of using a named direct entry seam | `ready now` for the pure scalar subset, `needs small seam` for the null-date subset | add a direct add-in financial entry seam and adopt the proven pure/date cluster | `CppunitTest_scaddins_analysis`, replay summary | Phase 2 / 3 / 5 |
| bounded local-workbook `CELL(...)` subset (`COL`, `ROW`, `SHEET`, `ADDRESS`, `CONTENTS`, `TYPE`) | [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx), [CellInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx) | reference-shape and result shaping are already shared, but `ScCell()` still assembles the bounded request inline | `needs small seam` | move bounded local `CELL(...)` entry through one named direct helper | `CppunitTest_sc_ucalc_formula2`, `CppunitTest_sc_ucalc_shared_cases`, replay summary | Phase 2 / 4 |
| bounded external-reference `CELL(...)` subset (`COL`, `ROW`, `SHEET`, `ADDRESS`, `CONTENTS`, `TYPE`, external filename/format/color/parentheses`) | [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx), [CellInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx) | external cache ownership is intentionally host-only, and the remaining projection shell still carries enough host-shaped address/file/cache behavior that the payoff is lower than the selected local `CELL(...)` and add-in financial slices | `defer after reassessment` | leave unchanged in this completed stream and carry it only as a future widening candidate if a later fresh inventory still ranks it highly | `CppunitTest_sc_ucalc_formula2`, replay summary | defer |
| additional pure scalar text/date parsing surfaces adjacent to `VALUE`, `DATEVALUE`, `TIMEVALUE`, and `NUMBERVALUE` | [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx), [interpr2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr2.cxx) | direct entry is already adopted for the main production parsing slice and no adjacent production caller currently offers better payoff than the financial and `CELL(...)` targets | `defer` | leave unchanged for this stream unless a clearer production caller appears during implementation | `CppunitTest_sc_ucalc_formula2`, replay summary | defer |
| host-heavy inspection or environment surfaces such as `INFO(...)` and retained local `CELL(...)` document-service properties (`WIDTH`, `PREFIX`, `PROTECT`, `FORMAT`, `COLOR`, `PARENTHESES`, `FILENAME`, `COORD`) | [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx), [interpr5.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr5.cxx) | host service access dominates the path even after boundary tightening | `intentionally host-only` | retain as Calc-owned unless a very small future seam becomes obvious | focused Calc coverage only | retain |

Phase completion questions:

- Are all plausible second-wave direct-entry candidates explicitly classified?
- Does each candidate have a concrete later phase or explicit retain/defer
  reason?

Phase 1 closeout notes:

- the highest-value widening targets are now explicitly narrowed to:
  - shared add-in financial callers that still stop at helper-level adoption
  - the bounded local and external `CELL(...)` subsets that already have thin
    compat/runtime seams
- adjacent parsing surfaces are explicitly deferred for this stream because the
  remaining production payoff is lower than the financial and `CELL(...)`
  slices
- `INFO(...)` and the retained host-heavy `CELL(...)` property tail remain
  intentionally Calc-owned

### Phase 2. Normalize The Second-Wave Host Adapters

Status: complete

Converge the minimal adapter or context surface needed by the selected
second-wave entry slices.

Preferred first targets:

- repeated workbook/context packaging around already-engine-owned entry paths
- repeated result projection that can be expressed once at the adapter edge
- add-in packaging that now shares the same host-service inputs

Phase completion questions:

- Are repeated host/context setup steps replaced by one named adapter seam?
- Does the touched code stop leaking packaging details into semantic callers?

Phase 2 closeout notes:

- added the direct add-in financial entry seam in
  [FinancialAddInExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/FinancialAddInExecution.hxx)
- added the direct bounded local `CELL(...)` inspection seam in
  [CellInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx)
- added focused Calc and add-in coverage for both adapter surfaces

### Phase 3. Adopt The Next Pure-Computation Direct Entrypoints

Status: complete

Apply the new packaging discipline to the highest-confidence pure-computation
production paths.

Preferred execution order:

1. smallest adjacent parsing or scalar-evaluation slice already near existing
   direct-entry adoption
2. any add-in production path whose host-service context is already normalized
3. documentation of what remains intentionally helper-level after the slice

Phase completion questions:

- Are the touched pure-computation callers entering the engine directly now?
- Has the local orchestration tail in the touched scope gotten meaningfully
  smaller?

Phase 3 closeout notes:

- the selected `AnalysisAddIn` pure-computation financial callers now enter
  shared engine semantics through
  [FinancialAddInExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/FinancialAddInExecution.hxx)
- the adopted production callers are `EFFECT`, `NOMINAL`, `DOLLARFR`,
  `DOLLARDE`, `CUMPRINC`, `CUMIPMT`, `FVSCHEDULE`, `XIRR`, and `XNPV`

### Phase 4. Adopt The Next Reference-Safe Direct Entrypoint Slice

Status: complete

Use the already-extracted compat/value-shape machinery to widen one bounded
reference-safe production path.

Preferred execution order:

1. the narrowest reference-safe inspection or lookup slice already backed by
   compat seams
2. any adjacent result-shape or matrix-safe path that can reuse the same
   entry adapter
3. documentation of what remains intentionally host-only after the slice

Phase completion questions:

- Is the touched reference-safe path entering the engine directly by default?
- Is the retained Calc code now obviously host orchestration only?

Phase 4 closeout notes:

- the bounded local-workbook `CELL(...)` subset now enters the engine through
  the direct adapter in
  [CellInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx)
- `ScCell()` in
  [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
  now keeps only the host-heavy property tail and result pushing locally

### Phase 5. Remove Superseded Local Orchestration

Status: complete

Once direct entry is the default path for the selected slices, remove or
isolate the old caller-side orchestration.

Target outcome:

- one named direct-entry path per boundary cluster
- no ambiguous wrapper tail in the touched scope

Phase completion questions:

- Are multiple production callers reusing the same entry seam?
- Has the superseded local orchestration tail been removed or reduced to thin
  host wrappers?

Phase 5 closeout notes:

- the remaining selected add-in financial wrappers that still packaged
  null-date or date-mode context now reuse the same direct adapter family
- the adopted production callers in this pass are `ACCRINT`, `DURATION`,
  `YIELDMAT`, `TBILLEQ`, `TBILLPRICE`, and `TBILLYIELD`
- the external `CELL(...)` candidate was explicitly deferred instead of left
  as an ambiguous half-finished tail

### Phase 6. Revalidate And Close The Stream

Status: complete

After the widening slices land:

- rerun the standing Calc/add-in and standalone lanes
- rerun one-shot replay with `--assert-zero-fallback`
- update
  [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)
  and [README.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/README.md)
- record what remains intentionally helper-level or host-only after the stream

Phase completion questions:

- Is the widened direct-entry boundary easier to describe in present tense?
- Are future adoption questions clearly a new frontier rather than unfinished
  second-wave cleanup?

Phase 6 closeout notes:

- final validation stayed green across the touched add-in, Calc, and replay
  lanes
- the standing replay baseline remained:
  - `workbooks=500`
  - `formula_cells=50661`
  - `parsed_formulas=50652`
  - `cached_fallback_cells=0`
  - `cached_fallback_rate=0`
- the stream is now closed; any further widening should start from a fresh
  inventory rather than by carrying forward this record as an active plan

## Validation Contract

Every implementation slice in this stream should use the smallest validation
set that proves both direct-entry correctness and replay stability.

Per-slice default contract:

- focused Calc or add-in coverage for the touched production entry points
- standalone coverage if a shared helper or runtime surface changed
- `spreadsheetengine_fods_evaluator_tests` if evaluator-facing code changed
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
  whenever the touched slice changes shared evaluation behavior
- `git diff --check`

Recommended high-signal lanes:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- `CppunitTest_scaddins_analysis`
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

## Exit Criteria

This stream should only be considered complete when:

1. the remaining in-scope second-wave direct-entry candidates have been
   explicitly inventoried and classified
2. repeated host/context packaging in the touched scope has been collapsed
   into named seams
3. at least one additional pure-computation production path and one additional
   bounded reference-safe path use direct engine entry by default
4. no known ambiguous local orchestration tail remains in the touched scope
5. the promoted replay corpus still reports:
   - `workbooks=500`
   - `formula_cells=50661`
   - `parsed_formulas=50652`
   - `cached_fallback_cells=0`
   - `cached_fallback_rate=0`
6. the status docs describe the widened direct-entry boundary as current state
   and make the next frontier explicit

At that point, the project can reassess whether the next frontier should be:

- another bounded direct-entry widening stream
- tighter production compiler-path adoption
- or continued maintenance of the now-wider direct-entry boundary
