# Computational Substrate Phase 0 Plan

Status: active implementation-ready Phase 0 plan

## Purpose

This document is the execution-ready plan for Phase 0 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

Phase 0 is the observability and scope-freeze gate for the larger
computational substrate program. Its purpose is to prove that the project can
observe, capture, and compare the live computational substrate in Calc
precisely enough to justify later shadow storage and authority work.

If that proof does not materialize, the larger program should stop here.

## Phase 0 Goal

Establish a precise, testable map of the live computational substrate that
would need to move.

That includes:

- formula tree
- broadcast track
- broadcast-area machine
- listener contexts
- broadcaster storage
- formula-cell listener lifecycle
- `ScTokenArray` construction, merge, and reference-adjustment surfaces

## Required Deliverables

Phase 0 is complete only when all of the following exist:

1. an explicit ownership inventory of the live computational substrate
2. a documented observable-state model for later shadow comparisons
3. stable runtime capture helpers for the critical live state
4. a representative mutation scenario matrix
5. at least one automated differential validation lane using the new captures
6. a checked-in proceed/stop decision record for Phase 1

## Workstreams

### 0.1 Inventory The Live Computational Substrate

Build an explicit ownership map for:

- formula tree and broadcast track state rooted in
  [document.hxx](/home/ubuntu/repos/libreoffice/sc/inc/document.hxx)
- broadcast-area machine usage rooted in
  [document.hxx](/home/ubuntu/repos/libreoffice/sc/inc/document.hxx)
- listener contexts in
  [listenercontext.hxx](/home/ubuntu/repos/libreoffice/sc/inc/listenercontext.hxx)
- broadcaster storage in
  [mtvelements.hxx](/home/ubuntu/repos/libreoffice/sc/inc/mtvelements.hxx)
  plus the relevant column implementations under `sc/source/core/data/`
- formula-cell listener lifecycle in
  [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
- `ScTokenArray` construction, merge, and reference-update surfaces in
  [token.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/token.cxx)
  and
  [compiler.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/compiler.cxx)

For each surface, classify it as:

- computational and candidate for migration
- host-only and retained
- mixed and needing a later seam

Required artifact:

- one checked-in reference document or appendix that records the full
  classification

### 0.2 Define The Observable State Model

Specify the exact live state that later phases must compare.

At minimum, define comparison shapes for:

- formula-tree membership
- broadcast-track membership
- broadcaster state by cell and by area
- listener registration shape
- recalculation queue membership and order
- engine-side dependency snapshot and recalc plan correspondence to the live
  graph

Required artifact:

- one explicit comparison schema document or equivalent checked-in design note
  that defines:
  - exact match
  - normalized-equivalent match
  - unacceptable divergence

### 0.3 Add Targeted Runtime Instrumentation

Instrument the Calc-owned computational state so it can be captured cheaply
and compared deterministically.

Likely targets:

- [GetBroadcasterState()](/home/ubuntu/repos/libreoffice/sc/inc/document.hxx#L2481)
- formula-tree and broadcast-track capture helpers near
  [document.hxx](/home/ubuntu/repos/libreoffice/sc/inc/document.hxx#L415)
- delayed listener and delayed broadcaster deletion behavior near
  [document10.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/document10.cxx#L415)
- listener registration and teardown flows near
  [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx#L5532)

Required artifact:

- stable capture helpers or debug-only snapshots suitable for automated tests

### 0.4 Build The Representative Mutation Scenario Set

Define the mutation scenarios that later phases must shadow exactly.

The minimum scenario set should include:

- `SetValue`
- `SetString`
- `SetFormula`
- `ClearCell`
- row and column insert/delete
- named-range edits
- copy/move and clipboard-driven listener rebuild cases
- load-time and `CalcAfterLoad` listener setup behavior

Required artifact:

- one checked-in scenario matrix that maps each mutation class to:
  - required captures
  - expected comparison outputs
  - owning validation lanes

### 0.5 Add Phase 0 Differential Validation Lanes

Phase 0 must end with real automated checks, not just documents and manual
inspection.

Minimum validation additions:

- broadcaster-state comparisons on representative mutations
- formula-tree and broadcast-track state comparisons after representative
  mutations
- coverage for delayed listener startup and delayed broadcaster deletion
- alignment checks between engine dependency snapshots/recalc plans and the
  captured live graph shape where those comparisons are already meaningful

Likely homes:

- Calc Cppunit lanes adjacent to the current dependency-shadow and
  workbook-facade coverage
- targeted standalone-or-compat tests where capture helpers cross the engine
  boundary

### 0.6 Freeze The Phase 0 Decision Record

Phase 0 closes only when the project has one explicit answer to the question:

"Can later phases shadow the live computational substrate exactly enough to
justify authority work?"

Required closeout artifact:

- a short checked-in decision record that says one of:
  - proceed to Phase 1
  - proceed only on a narrower subset
  - stop the computational substrate program here

## Target Surfaces

The first implementation sweep for Phase 0 should inspect and likely touch:

- [document.hxx](/home/ubuntu/repos/libreoffice/sc/inc/document.hxx)
- [listenercontext.hxx](/home/ubuntu/repos/libreoffice/sc/inc/listenercontext.hxx)
- [mtvelements.hxx](/home/ubuntu/repos/libreoffice/sc/inc/mtvelements.hxx)
- [document10.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/document10.cxx)
- [document.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/document.cxx)
- [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
- [column2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/column2.cxx)
- [token.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/token.cxx)
- [compiler.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/compiler.cxx)
- current dependency-shadow and workbook-facade test lanes under `sc/qa/unit/`
  and `spreadsheet_engine/tests/unit/`

## Validation Contract

Phase 0 should explicitly require:

- `git diff --check`
- current dependency-shadow and workbook-facade test lanes
- at least one new automated capture-and-compare lane for broadcaster or
  formula-tree state
- one-shot `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

Where Phase 0 instrumentation crosses the engine boundary, the touched
standalone or compat lanes should also be rerun.

## Exit Criteria

Phase 0 is successful only if all of the following are true:

1. the migration scope is explicitly inventoried
2. the live computational state is observable through stable capture helpers
3. exact and normalized-equivalent comparison rules are documented
4. representative mutation scenarios are defined and automated
5. at least one real differential validation lane is in place
6. the project can make a justified proceed or stop decision for Phase 1

If any of those are missing, then the correct outcome is to keep working in
Phase 0 rather than to start shadow storage work prematurely.
