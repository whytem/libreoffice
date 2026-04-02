# Production Boundary Tightening Plan

Status: active implementation-ready plan

## Purpose

The extraction, replay-promotion, recalc-orchestration, execution-shell,
engine-first adoption, host-boundary consolidation, token-boundary reduction,
and first direct-entry adoption streams are complete.

The next implementation stream is to tighten the remaining production
compiler/evaluation boundary around surfaces that are intentionally still
host-owned.

This is not a replay-promotion effort, a storage migration, or a wholesale
interpreter rewrite. It is a production-boundary quality program.

## What This Workstream Is For

This stream is successful when the remaining production Calc/add-in paths that
still package host services around spreadsheet semantics are:

- explicitly inventoried and classified
- narrowed to thin, named host adapters where practical
- no longer carrying repeated ad hoc packaging or orchestration
- easier to explain as either `host-only` or `engine-entry packaging`
- validated against the standing zero-fallback replay baseline

In concrete terms, the focus is:

- retained host-heavy information and inspection paths
- external-reference and session-backed evaluation packaging
- document null-date, holiday-list, and similar host-service packaging around
  otherwise engine-owned semantics
- production compiler/evaluation entry plumbing that is still broader than it
  needs to be

## Out Of Scope

This plan does not:

- move document storage, mutation, or token ownership out of Calc
- move true host services such as `INFO(...)`, printer/path inspection, or
  external-reference cache ownership into the standalone engine
- reopen the recalc authority boundary
- replace `ScInterpreter` wholesale
- redesign threaded or OpenCL execution
- change the zero-fallback replay contract

## Definition Of Done

This workstream is complete when all of the following are true:

1. the in-scope production compiler/evaluation entry points touching retained
   host-owned surfaces are explicitly classified as `host-only`, `needs thin
   adapter`, or `defer`
2. repeated Calc/add-in packaging of host services is collapsed into named,
   auditable adapters or context providers
3. the touched production paths use the smallest practical Calc-owned shell
   around engine semantics
4. superseded local orchestration in the touched scope is removed or isolated
   behind clearly host-only wrappers
5. the promoted replay corpus remains at `0` cached fallback under the strict
   summary assertion gate
6. the remaining retained Calc-owned surfaces are easier to describe in
   present tense as intentional host boundary rather than legacy packaging

## Boundary Rules

Every slice in this stream must continue to respect the settled project
boundary:

- keep storage, mutation, token ownership, and stack mutation in Calc
- keep host services explicit and localized at the adapter edge
- prefer thin context adapters over inlined packaging at multiple call sites
- keep spreadsheet semantics engine-owned where they already are engine-owned
- do not hide host lookups inside generic semantic helpers
- do not introduce duplicate Calc-local semantic implementations as a fallback

## Workstreams

### 1. Freeze The Remaining Production-Boundary Inventory

Build and maintain an explicit inventory of the remaining production
compiler/evaluation entry points that still package retained host-owned
services around spreadsheet semantics.

For each candidate, record:

- Calc/add-in entry point and owning file
- whether the surface is primarily host service, packaging, or semantics
- closest current engine runtime, evaluator, or compat entry surface
- whether the correct outcome is `host-only`, `needs thin adapter`, or `defer`
- required validation lanes

Primary target files:

- [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx)
- [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- [analysis.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysis.cxx)
- [analysishelper.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysishelper.cxx)
- active docs under
  [spreadsheet_engine/docs/architecture](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture)

Completion criteria:

- there is no unclassified in-scope production-boundary candidate in the
  touched surface
- each candidate has an owning later slice or an explicit retain/defer reason

### 2. Isolate Host-Service Context Packaging

Converge the minimal host-side context packaging needed by retained host-owned
services that still surround engine semantics.

Target seam types:

- document null-date access
- holiday-list expansion and workday/calendar host inputs
- document/session inspection context
- bounded environment or external-reference session context

Implementation rules:

- package the host inputs once behind named adapters
- keep Calc/add-in types at the adapter edge where ownership is real
- avoid repeating host-service extraction inline at multiple semantic call
  sites

Validation:

- focused Calc/add-in Cppunit coverage for touched adapters
- standalone coverage if shared helper surfaces change
- `git diff --check`

Completion criteria:

- the touched production paths no longer perform repeated ad hoc host-service
  extraction inline
- host-service handoff becomes small enough to audit and explain easily

### 3. Tighten Information And Inspection Entry Surfaces

Reduce Calc-local production packaging around the retained information and
inspection paths that still mix host service lookups with engine-owned value
or result-shape logic.

Target categories:

- retained `CELL(...)` property tail
- `INFO(...)` result projection and bounded helper plumbing
- nearby inspection paths where semantics are already shared but host reads are
  still packaged broadly

Implementation rules:

- do not move true host/environment inspection into the engine
- do move result-shape, classification, or projection logic behind thin shared
  seams where practical
- keep the final retained local code obviously host-only

Validation:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- standalone evaluator coverage if shared inspection helpers change
- strict replay summary assertion

Completion criteria:

- the touched inspection/info paths have smaller Calc-owned shells
- retained helpers in the touched scope read as host service access, not mixed
  semantic utilities

### 4. Tighten External-Reference And Session-Backed Packaging

Narrow the production packaging around external-reference and session-backed
evaluation paths without moving ownership of caches or sessions out of Calc.

Target categories:

- external-reference fetch/projection wrappers
- session-backed lookup/inspection entry helpers
- bounded document-service packaging around externally sourced values

Implementation rules:

- keep external-reference cache ownership in Calc
- extract only the packaging/projection layer that is repeated or too broad
- prefer named compat/context seams over call-site-local shaping logic

Validation:

- focused Calc coverage for touched external/session paths
- standalone coverage if shared result-shape helpers change
- strict replay summary assertion

Completion criteria:

- the touched external/session-backed paths use named packaging seams
- remaining Calc-owned code is clearly cache/session ownership and document
  access, not duplicated semantic shaping

### 5. Tighten Production Compiler/Evaluation Entry Packaging

Audit the remaining production compiler/evaluation entry points whose
spreadsheet semantics are already engine-owned but whose host packaging is
still wider than necessary.

Target categories:

- direct-entry adapters that still repeat context preparation
- production compiler/evaluation call sites with similar context packaging at
  multiple entry points
- add-in adoption call sites whose host setup can now be normalized further

Implementation rules:

- prefer one small entry adapter over many inline setup paths
- keep compiler/evaluation entry vocabulary stable once the adapter is named
- do not widen engine public API just to mirror Calc internals

Validation:

- focused Calc/add-in coverage for touched entry points
- standalone coverage if shared entry surfaces change
- strict replay summary assertion

Completion criteria:

- repeated production entry packaging in the touched scope is collapsed
- retained caller-side code is mostly lifecycle, service acquisition, and
  result projection

### 6. Lock The Boundary And Close The Stream

Turn the tightened production boundary into a stable standing contract.

Required guardrails:

- one-shot
  `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- focused Calc/add-in lanes for every touched seam
- standalone evaluator/runtime coverage for shared helper changes
- doc/status updates that describe the boundary in present tense
- clean diff hygiene

Completion criteria:

- validation is documented and repeatable
- the status docs describe the remaining host-owned production boundary as
  intentional current state
- any further widening is clearly a new bounded frontier rather than unfinished
  cleanup from this stream

## Phased Implementation Approach

### Phase 1. Freeze And Publish The Production-Boundary Inventory

Create the explicit candidate table for production compiler/evaluation entry
points that still package retained host-owned services.

Initial inventory targets:

| Candidate | Owning file(s) | Current boundary problem | Target outcome | Validation lanes | Later phase |
| --- | --- | --- | --- | --- | --- |
| retained `CELL(...)` property tail (`FILENAME`, `WIDTH`, `PREFIX`, `PROTECT`, `FORMAT`, `COLOR`, `PARENTHESES`) | [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx) | host property reads mixed with result shaping and caller packaging | `needs thin adapter` | `CppunitTest_sc_ucalc_formula2`, replay summary | Phase 3 |
| `INFO(...)` environment/session inspection | [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx) | true host service, but result shaping and packaging may still be broader than needed | `host-only` with tighter projection seam | focused Calc coverage, replay summary | Phase 3 |
| external-reference projection and session-backed fetch helpers | [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx) | cache/session ownership is host-only, but surrounding packaging may be repeated or broad | `needs thin adapter` / partial retain | focused Calc coverage, replay summary | Phase 4 |
| add-in null-date and holiday-list packaging around shared financial/date semantics | [analysis.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysis.cxx), [analysishelper.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysishelper.cxx) | host inputs are real, but packaging may still be duplicated across adopted entry points | `needs thin adapter` | `CppunitTest_scaddins_analysis`, replay summary | Phase 2 / 5 |
| repeated direct-entry context setup around existing engine entry adapters | [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx), [interpr2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr2.cxx), add-in callers | similar workbook/service packaging repeated at several production callers | `needs thin adapter` | focused Calc/add-in coverage, replay summary | Phase 5 |

Phase completion questions:

- Are all in-scope remaining production-boundary candidates classified?
- Does each candidate have a concrete later phase or explicit retain reason?

### Phase 2. Extract Shared Host-Service Context Adapters

Introduce or tighten the named adapters/context providers needed by later
production-boundary slices.

Preferred first targets:

- null-date packaging
- holiday-list and calendar host inputs
- small document/session inspection contexts used by more than one entry point

Phase completion questions:

- Are repeated host-service reads replaced by one named adapter seam?
- Does the touched code stop leaking host packaging details into semantic
  callers?

### Phase 3. Shrink The Information/Inspection Boundary

Apply the new packaging discipline to retained information and inspection
surfaces.

Preferred execution order:

1. retained `CELL(...)` property tail
2. bounded `INFO(...)` result projection and caller packaging
3. any adjacent inspection helper that becomes obviously reducible after the
   first two slices

Phase completion questions:

- Is the touched inspection code now clearly split between host service access
  and semantic/result shaping?
- Has the Calc-owned shell gotten smaller without moving true host inspection
  into the engine?

### Phase 4. Shrink The External/Session Packaging Boundary

Apply the same discipline to external-reference and session-backed packaging.

Preferred execution order:

1. the narrowest repeated projection/helper seam in
   [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
2. any adjacent external/session-backed packaging that can reuse the same seam
3. documentation of what remains intentionally host-only after the slice

Phase completion questions:

- Is the touched code now mostly cache/session ownership plus named adapter
  calls?
- Are duplicated projection or shaping steps gone from the touched call sites?

### Phase 5. Collapse Residual Entry Packaging

With the host-service seams stable, collapse remaining repeated production
compiler/evaluation entry setup in the adopted areas.

Target outcome:

- one small adapter or context-preparation path per retained production
  boundary cluster
- no ambiguous wrapper tails in the touched scope

Phase completion questions:

- Are multiple production entry points reusing the same packaging seam?
- Has the superseded local orchestration tail been removed or reduced to thin
  host wrappers?

### Phase 6. Revalidate And Close The Stream

After the production-boundary slices land:

- rerun the standing Calc/add-in and standalone lanes
- rerun one-shot replay with `--assert-zero-fallback`
- update [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md) and [README.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/README.md)
- record what remains intentionally host-only after the stream

Phase completion questions:

- Is the remaining production boundary easier to describe in present tense?
- Are future questions clearly a new frontier rather than unfinished cleanup
  from this stream?

## Validation Contract

Every implementation slice in this stream should use the smallest validation
set that proves both boundary correctness and replay stability.

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

1. the remaining in-scope production compiler/evaluation boundary has been
   explicitly inventoried and classified
2. repeated host-service packaging in the touched scope has been collapsed
   into named seams
3. retained information, inspection, external-reference, and add-in host
   packaging surfaces in scope have smaller Calc/add-in shells than they do
   today
4. no known ambiguous local orchestration tail remains in the touched scope
5. the promoted replay corpus still reports:
   - `workbooks=500`
   - `formula_cells=50661`
   - `parsed_formulas=50652`
   - `cached_fallback_cells=0`
   - `cached_fallback_rate=0`
6. the status docs describe the post-stream boundary as current state and make
   the next frontier explicit

At that point, the project can reassess whether the next frontier should be:

- another bounded direct-entry widening stream
- tighter production compiler-path adoption
- or continued maintenance of the now-explicit host-owned production boundary
