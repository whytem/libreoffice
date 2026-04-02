# Host Service Facade Narrowing Plan

Status: active implementation-ready plan

## Purpose

The major extraction streams are complete:

- replay promotion and zero-fallback closeout
- recalc orchestration extraction
- execution-shell extraction
- engine-first Calc adoption
- host-boundary and token-boundary consolidation
- first-wave and second-wave direct engine-entry widening
- production-boundary tightening

What remains in Calc is now mostly intentional host ownership. The next useful
stream is not another broad extraction pass. It is to narrow the remaining
host-service packaging around those retained Calc-owned surfaces so the
boundary reads more clearly as "Calc owns host services, the engine owns
spreadsheet semantics."

This workstream is for the host-owned surfaces where Calc still does more
service packaging, projection, or value-shape orchestration than is really
necessary.

## What This Workstream Is For

This stream is successful when the retained host-owned production paths use
smaller, named compat facades for host services, while Calc continues to own:

- document storage and mutation
- formula-cell lifecycle and side effects
- token ownership and stack mutation
- external-reference cache ownership
- document/session/environment services
- UI, import/export, UNO, rendering, shell, and persistence

In practical terms, the focus is:

- inventorying the remaining host-service-heavy production paths
- separating true host ownership from packaging that can still be narrowed
- introducing smaller compat facades around those services
- removing repeated caller-side packaging in the touched scope
- preserving the current zero-fallback replay baseline

## Out Of Scope

This plan does not:

- move `ScDocument` storage or mutation into the engine
- move token ownership or stack mutation out of Calc
- move external-reference cache ownership into the engine
- move printer/path/environment inspection ownership into the engine
- reopen recalc authority, queue construction, or workbook-facade boundaries
- attempt a wholesale `ScInterpreter` rewrite
- change the zero-fallback replay contract

## Definition Of Done

This workstream is complete when all of the following are true:

1. the in-scope retained host-owned production paths are explicitly inventoried
   and classified
2. repeated host-service packaging in the touched scope is collapsed into
   named compat facades or clearly retained host-only wrappers
3. at least one external-reference service path, one add-in host-service path,
   and one host-heavy inspection path are narrowed measurably
4. no touched caller still mixes spreadsheet semantics with ad-hoc host
   service packaging when an explicit facade exists
5. the promoted replay corpus still reports `0` cached fallback under the
   strict summary assertion gate
6. Calc, add-in, and standalone validation lanes stay green for every touched
   slice

## Boundary Rules

Every slice in this stream must respect the current project boundary:

- keep host-service ownership in Calc
- narrow packaging, do not relocate ownership
- prefer one named compat facade per host-service cluster
- avoid creating Calc-local semantic fallbacks
- do not widen engine public API only to mirror host internals
- document any retained host-only wrapper explicitly

## Workstreams

### 1. Freeze The Remaining Host-Service Inventory

Build an explicit inventory of the retained host-owned production paths whose
packaging still looks heavier than necessary.

For each candidate, record:

- owning Calc or add-in file
- host service being packaged
- closest existing compat/helper surface
- whether the gap is projection, context assembly, cache access, formatting,
  or lifecycle
- whether the right outcome is `ready now`, `needs small seam`,
  `intentionally host-only`, or `defer`
- required validation lanes

Primary target files:

- [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- [interpr5.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr5.cxx)
- [analysis.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysis.cxx)
- [analysisdefs.hxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysisdefs.hxx)
- compat headers under
  [spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice)

Completion criteria:

- there is no unclassified in-scope host-service candidate in the touched
  surface
- each candidate has an owning later slice or an explicit retain/defer reason

### 2. Narrow External-Reference Service Packaging

Target the retained external-reference service shell first.

Likely focus:

- external inspection/projection paths adjacent to the deferred external
  `CELL(...)` candidate
- repeated external cache fetch/projection setup
- external filename/address/value-shape projection that can be expressed once
  behind a compat facade while cache ownership stays in Calc

Implementation rules:

- keep cache ownership, session lookup, and document linkage in Calc
- narrow only packaging and projection
- prefer a named external inspection/projection facade over repeated caller
  logic

Completion criteria:

- the touched external-reference callers stop duplicating projection logic
- retained local code reads as cache ownership or host orchestration only

### 3. Narrow Add-In Date And Holiday Service Packaging

Target the remaining host-owned add-in service assembly around null-date and
holiday inputs.

Likely focus:

- host null-date retrieval
- holiday-list expansion from add-in/UNO inputs
- repeated service context assembly for add-in calendar/workday/date callers

Implementation rules:

- keep live document null-date ownership in Calc/add-ins
- keep UNO/add-in surface ownership in Calc/add-ins
- narrow repeated assembly into one explicit host-service context seam

Completion criteria:

- the touched add-in callers use one named host-service context facade
- repeated null-date/holiday packaging gets measurably smaller

### 4. Narrow Host-Heavy Inspection And Environment Packaging

Target the retained information/inspection shell that is still intentionally
host-owned but may still be over-packaged.

Likely focus:

- bounded `INFO(...)` projection paths
- retained local `CELL(...)` host-heavy property tail
- document-service inspection around printer/path/format/environment helpers

Implementation rules:

- keep host-heavy inspection ownership in Calc
- narrow projection and service assembly only where it clarifies the boundary
- do not force pure-engine ownership where the service is inherently host-only

Completion criteria:

- the touched inspection paths use smaller named service facades
- retained local code is more obviously host-only and less mixed with
  projection semantics

### 5. Retire Superseded Host-Service Wrappers In Scope

Once the new facades are in place, remove or isolate the old packaging tail.

Allowed outcomes:

- delete the superseded wrapper entirely
- keep only a thin host-only facade
- retain a helper only if its ownership is explicit and documented

Not acceptable:

- leaving both old and new packaging layers active in the same scope
- keeping ambiguous wrappers whose ownership cannot be explained

Completion criteria:

- there is one clear facade path per touched host-service cluster
- no silent duplicate host-service packaging remains in the touched scope

### 6. Lock The Baseline And Close The Stream

Turn the narrowed host-service boundary into a stable standing contract.

Required closeout work:

- rerun focused Calc and add-in lanes for every touched slice
- rerun standalone evaluator coverage where shared helpers changed
- rerun one-shot replay with `--assert-zero-fallback`
- update status and architecture docs in present tense
- record explicit defers and retained host-only surfaces after the stream

Completion criteria:

- the narrowed host-service boundary is documented as current state
- future work is clearly a new bounded frontier rather than unfinished facade
  cleanup

## Phased Implementation Approach

### Phase 1. Freeze And Publish The Host-Service Inventory

Status: complete

Create the explicit candidate table for retained host-service-heavy production
paths.

Initial inventory targets:

| Candidate | Owning file(s) | Current boundary problem | Classification | Target outcome | Validation lanes | Later phase |
| --- | --- | --- | --- | --- | --- | --- |
| bounded external-reference `CELL(...)` subset and adjacent projection helpers | [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx), [CellInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx) | external cache ownership is correctly host-owned, but projection and formatting still look broader than necessary | `needs small seam` | narrow external inspection/projection behind one compat facade while keeping cache ownership in Calc | `CppunitTest_sc_ucalc_formula2`, replay summary | Phase 2 |
| add-in null-date and holiday-list service assembly | [analysis.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysis.cxx), [analysisdefs.hxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysisdefs.hxx) | repeated host-service assembly still appears in multiple add-in entry clusters | `needs small seam` | converge onto one explicit host-service context seam | `CppunitTest_scaddins_analysis`, replay summary | Phase 3 |
| retained host-heavy `CELL(...)` property and bounded `INFO(...)` projection tail | [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx), [interpr5.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr5.cxx) | ownership is intentionally host-only, but service projection can still be narrowed | `needs small seam` | move the touched projection/setup behind smaller named host facades | `CppunitTest_sc_ucalc_formula2`, replay summary | Phase 4 |
| pure environment services with no spreadsheet-semantic payoff outside Calc | [interpr5.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr5.cxx) and supporting document-service files | ownership is fully host-only and packaging is already minimal enough | `intentionally host-only` | retain in Calc and document why | focused Calc coverage only | retain |

Phase 1 closeout notes:

- the external-reference candidate is now narrowed to the production shell in
  [ScCellExternal()](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
  plus the adjacent filename/address/format projection helpers in
  [CellInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx)
- the add-in service candidate is confirmed as the remaining null-date and
  holiday-list context assembly in
  [analysisdefs.hxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysisdefs.hxx)
  and
  [analysis.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysis.cxx)
- the host-heavy inspection candidate is confirmed as the retained local
  `CELL(...)` property tail in
  [ScCell()](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
  and the bounded `INFO(...)` projection path in
  [ScInfo()](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr5.cxx)
- pure environment services with no spreadsheet-semantic payoff outside Calc
  are explicitly retained as host-only for this stream

### Phase 2. Narrow The External-Reference Facade

Status: complete

Adopt the smallest useful external-reference service seam.

Preferred execution order:

1. external `CELL(...)` classification/projection helper packaging
2. external filename/address/value-shape projection reuse
3. documentation of what remains intentionally cache-owned afterward

Phase 2 closeout notes:

- the bounded external `CELL(...)` service shell in
  [ScCellExternal()](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
  now routes `COL`, `ROW`, `SHEET`, `ADDRESS`, `FILENAME`, `CONTENTS`, and
  `TYPE` through
  [DirectExternalCellInspectionAdapter](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx)
- external cache ownership, token extraction, and the remaining
  format/color/parentheses projection stay in Calc as the intentionally
  host-owned tail for this slice
- focused external-reference coverage now lives in
  [testExternalRef()](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx)
  and validates the narrowed `CELL(...)` projection subset directly

### Phase 3. Narrow The Add-In Service Context Facade

Status: complete

Collapse repeated add-in host-service assembly into one named context seam.

Preferred execution order:

1. null-date service packaging
2. holiday-list expansion packaging
3. context reuse in the selected add-in entry clusters

Phase 3 closeout notes:

- the remaining add-in null-date and holiday packaging now converges on
  [AddInDateServiceContext](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysisdefs.hxx)
  and the paired
  [getAddInDateServiceContext()](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysisdefs.hxx)
  helpers
- [analysis.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysis.cxx)
  now reuses that single seam for `WORKDAY`, `NETWORKDAYS`, `YEARFRAC`, and
  `WEEKNUM`
- [financial.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/financial.cxx)
  now builds its date-mode and null-date adapters from the same add-in host
  context instead of assembling separate null-date paths

### Phase 4. Narrow Host-Heavy Inspection Projection

Status: complete

Apply the same discipline to the retained host-heavy inspection shell.

Preferred execution order:

1. retained local `CELL(...)` host property projection
2. bounded `INFO(...)` service projection where a small seam clarifies the
   boundary
3. explicit documentation of what remains host-only after the slice

Phase 4 closeout notes:

- the retained local `CELL(...)` host-only property tail now runs through
  [DirectHostCellInspectionAdapter](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx)
  instead of open-coded projection in
  [ScCell()](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- the bounded `INFO(...)` projection path now routes through
  [DirectInfoInspectionAdapter](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InfoInspectionExecution.hxx),
  leaving [ScInfo()](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr5.cxx)
  with only request assembly and result pushing
- the intentionally retained host-only tail after this slice is the service
  ownership itself, not the caller-side projection logic

### Phase 5. Remove Superseded Host-Service Wrappers

Status: complete

Delete or isolate the old packaging tail now that the selected facades are in
place.

Target outcome:

- one named host-service facade per touched boundary cluster
- no ambiguous wrapper layers in the touched scope

Phase 5 closeout notes:

- the superseded add-in wrapper layer in
  [analysisdefs.hxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysisdefs.hxx)
  is gone: `FinancialDateContext`, `WorkdayHostContext`,
  `getFinancialDateContext()`, `getWorkdayHostContext()`, and the old
  `getNullDateParts()` helper no longer sit alongside the new
  `AddInDateServiceContext` seam
- the remaining TBILL host-date callers in
  [financial.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/financial.cxx)
  now use the add-in date-service context directly
- the old `isUnavailableInfoKind()` helper was retired from
  [InfoInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InfoInspectionExecution.hxx)
  because the direct info adapter now owns that projection decision

### Phase 6. Revalidate And Close The Stream

After the narrowing slices land:

- rerun the standing Calc/add-in and standalone lanes
- rerun one-shot replay with `--assert-zero-fallback`
- update
  [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)
  and [README.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/README.md)
- record what remains intentionally host-only after the stream

## Validation Contract

Per-slice default contract:

- focused Calc or add-in coverage for the touched production paths
- standalone coverage if a shared helper or compat surface changed
- `spreadsheetengine_fods_evaluator_tests` if evaluator-facing behavior changed
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

1. the retained host-service-heavy production paths in scope are explicitly
   inventoried and classified
2. repeated host-service packaging in the touched scope has been collapsed
   into named facades
3. at least one external-reference path, one add-in service path, and one
   host-heavy inspection path are narrowed materially
4. no known ambiguous host-service wrapper tail remains in the touched scope
5. the promoted replay corpus still reports:
   - `workbooks=500`
   - `formula_cells=50661`
   - `parsed_formulas=50652`
   - `cached_fallback_cells=0`
   - `cached_fallback_rate=0`
6. the status docs describe the narrowed host-service boundary as current
   state and make the next frontier explicit

At that point, the project can reassess whether the next frontier should be:

- another bounded direct-entry widening stream
- tighter compiler-path adoption around the retained host boundary
- or continued maintenance of the narrowed facade boundary
