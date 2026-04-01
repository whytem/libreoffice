# Recalculation Orchestration Extraction Plan

## Purpose

This document turns the next logical post-convergence milestone into an
implementation-ready plan:

- promote the completed workbook facade plus dependency/invalidation planner
  into the substrate for engine-owned recalculation planning
- keep Calc as the storage owner and formula-execution owner initially
- move dirty-set and queue/scheduling policy toward engine authority in small,
  shadow-validated slices

This is intentionally a separate milestone because it is now the main boundary
between "shared runtime is complete enough" and "the engine owns more of Calc's
production calculation semantics."

## Executive Summary

The right next move is an **engine-owned recalculation orchestrator**, not more
pure-computation extraction and not a storage rewrite.

The milestone should:

1. define engine-owned recalc-plan and scheduling types
2. consume workbook-facade snapshots plus invalidation-planner output
3. run in shadow mode inside Calc and compare against Calc's current recalc
   behavior
4. become authoritative first for selected safe mutation families
5. hand off later to execution-backend extraction once queue ownership is
   stable

The milestone should **not**:

- replace `ScDocument` storage
- replace `ScFormulaCell` ownership wholesale
- rewrite `ScInterpreter` in one pass
- move OpenCL/threading concerns into the engine core
- conflate dependency planning with listener/broadcaster side effects

## Why This Is The Next Milestone

The completed prerequisites are now in place:

- shared compiler and compiled replay switchover
- runtime modularization and major pure-computation convergence with Calc
- Calc-backed workbook facade
- dependency snapshotting and invalidation planning
- full promoted FODS replay corpus at zero cached fallback:
  - `500` workbooks
  - `50,661` formula cells
  - `0` cached-fallback cells

That means the next clean boundary is no longer function coverage or replay
promotion. It is the policy layer that decides:

- what becomes dirty after a mutation
- what order recalculation work runs in
- when grouped/shared execution is preserved or broken apart
- how recalculation reaches a stable completion state

## Problem Statement

Today Calc still owns the authoritative recalculation loop through a spread of
Calc-specific structures and side effects:

- dirty-bit setting on formula-bearing cells
- recalc queue creation and mutation-triggered enqueue policy
- formula-group fallback and regroup behavior during recalculation
- listener/broadcaster side effects connected to edits
- execution ordering and completion semantics

The engine already owns the model inputs needed to reason about this:

- workbook-facade snapshots
- canonical compiled/tokenized formula representations
- dependency relationships and reverse-dependency indexing
- invalidation planning from normalized mutations

If scheduler extraction is skipped and execution work is attempted first, the
project will keep core recalculation semantics trapped inside Calc-owned queue
and dirty-state logic. That would make later authority shifts harder and force
duplicate work.

## Current Status

Already complete and available for this milestone:

- workbook facade contracts and Calc-backed adapters
- mutation event vocabulary
- dependency snapshot builder
- invalidation planner
- structural rebuild scopes and conservative widening behavior
- shadow auditing hooks for selected Calc mutation paths
- zero-fallback standalone replay on the promoted corpus

What remains Calc-owned and is the target of this milestone:

- authoritative dirty-set ownership
- recalc queue and scheduling order
- grouped-formula preservation/fallback decisions
- mutation-to-recalc orchestration

## Milestone Definition

### What this milestone must achieve

By the end of this milestone:

- `spreadsheet_engine` owns recalc-plan and scheduler data structures
- engine planner output can be compared against Calc's live recalculation
  behavior in shadow mode
- selected low-risk mutation families use engine-owned dirty planning
- selected low-risk mutation families use engine-owned queue/scheduling policy
- later execution-backend extraction has a stable orchestration boundary to
  target

### What this milestone does not require

This milestone does **not** require:

- making the engine the workbook storage authority
- replacing listener/broadcaster plumbing wholesale
- extracting all of `ScInterpreter`
- extracting threaded or OpenCL execution backends
- corpus-wide lexical token parity beyond the existing maintenance lanes

## Scope Boundary

### In Scope

- engine-owned recalc-plan, queue-entry, and scheduling-result types
- queue construction from invalidation-planner output
- group-aware scheduling metadata
- shadow comparison of dirty sets and queue ordering
- authoritative pilot paths for selected safe edits
- Calc adapters that feed live workbook/mutation data into the engine planner

### Explicitly Out Of Scope

- storage ownership changes
- UI/rendering/persistence/UNO
- full listener graph replacement
- direct execution ownership for all formulas
- backend resource management

## Design Principles

### 1. Planner first, authority second

The engine should first prove it can model recalculation behavior in shadow
before becoming authoritative for production side effects.

### 2. Keep Calc as host and executor initially

Calc continues to own mutation execution, cell storage, and formula execution
while the engine takes over planning and scheduling policy in narrow slices.

### 3. Prefer explicit recalc state

Queue seeds, causes, structural-rebuild reasons, group policies, and completion
status should be represented in engine-owned types rather than inferred from
scattered Calc state.

### 4. Stay conservative on unsupported shapes

If planner fidelity is incomplete, prefer safe over-invalidation or queue
widening over under-recalculation.

## Proposed Phases

### Phase 0: Freeze Baseline And Instrumentation

Goals:

- keep the current `0 / 50,661` promoted-corpus fallback baseline green
- document the scheduler-extraction baseline in status docs
- identify the first safe mutation families for authoritative trials

Deliverables:

- refreshed `PROJECT_STATUS.md`
- milestone doc committed under `docs/architecture/`
- explicit validation lane list for scheduler-shadow work

### Phase 1: Engine-Owned Recalc Types

Goals:

- define engine-owned recalc planning vocabulary
- connect invalidation-plan output to scheduler input

Deliverables:

- recalc seed / dirty-cause / queue-entry / plan-result types
- group-policy and structural-rebuild markers
- standalone unit tests for queue construction on synthetic dependency graphs

Likely file area:

- `spreadsheet_engine/inc/spreadsheetengine/detail/dependency/`
- `spreadsheet_engine/source/core/`

### Phase 2: Shadow Scheduler In Calc

Goals:

- run engine recalc planning alongside Calc after representative edits
- compare queue membership, ordering, and group handling

Deliverables:

- Calc compat/shadow adapter for recalc planning
- dedicated Calc shadow tests for non-structural edits
- first diagnostic diff reporting for queue mismatches

Likely initial mutation families:

- `SetValue`
- `SetString`
- `SetFormula`
- `ClearCell`
- `ClearRange`

### Phase 3: Authoritative Dirty-Plan Pilot

Goals:

- let engine planner decide the authoritative dirty set for selected safe
  mutations while Calc still executes the recalculation

Deliverables:

- opt-in authority switch for selected mutation families
- runtime assertions comparing engine plan to Calc outcome
- rollback/debug path if mismatch is detected

### Phase 4: Authoritative Scheduling Pilot

Goals:

- move queue creation and ordering policy for the same safe mutation subset
  onto engine-owned scheduler outputs

Deliverables:

- engine-generated recalc queue consumed by Calc
- group/scalar scheduling policy validated in shadow then pilot mode
- stronger differential tests around ordering-sensitive cases

### Phase 5: Expand To Structural And Named-Range Mutations

Goals:

- widen authority from simple edits to the already-modeled structural and
  named-range cases

Deliverables:

- row/column insert/delete queue planning
- named-range mutation recalc planning
- broader shared-formula group stability coverage

### Phase 6: Hand Off To Execution-Backend Extraction

Goals:

- finish the orchestration milestone with a clean authority boundary for later
  evaluator-shell extraction

Completion handoff:

- scheduler output is engine-owned
- Calc remains storage and execution host
- next milestone can focus on token walking, coercion, and
  reference-sensitive execution slices

## Validation Strategy

Every phase should satisfy all of the following:

### Standalone / Engine Validation

- focused unit tests for recalc-plan construction
- promoted replay corpus stays at zero cached fallback
- compiled replay and compiler-maintenance lanes stay green

### Calc Validation

- shadow tests comparing dirty sets and queue ordering
- grouped/shared-formula behavior checks
- representative structural-edit validation before authority expansion

### Required Gates

- `make -j4 CppunitTest_sc_ucalc_workbook_facade`
- `make -j4 CppunitTest_sc_ucalc_dependency_shadow`
- existing compile-diff / formula lanes touched by the change
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary`
- `git diff --check`

## Completion Criteria

The milestone is complete when all of the following are true:

- engine-owned recalc-plan and scheduling types are stable
- selected safe mutation families use engine-owned dirty planning
- selected safe mutation families use engine-owned queue/scheduling policy
- Calc and engine shadow comparisons stay green on representative non-structural
  and structural cases
- the next execution-backend milestone can target a stable orchestration
  boundary instead of Calc-owned queue semantics

## Risks

- trying to replace Calc listeners too early
- widening authority before the shadow lane is stable
- extracting execution logic before queue ownership is settled
- allowing unsupported constructs such as dynamic references to appear precise
  when they still need conservative widening

## Critical Files

| File | Role |
| --- | --- |
| [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md) | Consolidated project state and roadmap |
| [CALC_BACKED_WORKBOOK_FACADE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/archive/CALC_BACKED_WORKBOOK_FACADE.md) | Archived prerequisite milestone |
| [DEPENDENCY_INVALIDATION_EXTRACTION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/archive/DEPENDENCY_INVALIDATION_EXTRACTION.md) | Archived prerequisite milestone |
| [DependencyShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/DependencyShadow.hxx) | Existing Calc shadow-audit bridge |
| [WorkbookFacade.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/WorkbookFacade.hxx) | Calc-backed workbook facade adapter |
| [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx) | Calc formula-cell ownership and dirty/recalc behavior |
| [document.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/document.cxx) | Workbook-level recalc orchestration host |
