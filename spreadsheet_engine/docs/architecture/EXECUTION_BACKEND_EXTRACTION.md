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

## Proposed Phases

### Phase 0: Freeze The Orchestration Boundary

Goals:

- keep the engine-owned recalc authority lane green
- document the exact handoff files and validation gates
- identify the first execution helpers that still duplicate between Calc and
  standalone

Deliverables:

- refreshed `PROJECT_STATUS.md`
- this implementation-ready milestone doc
- explicit file inventory for the first evaluator-shell slice

### Phase 1: Extract Shared Coercion And Scalar Execution Helpers

Goals:

- move duplicated scalar coercion and argument-normalization mechanics behind
  shared runtime or compat helpers

Deliverables:

- shared helper layer for numeric, text, logical, and reference coercion
- reduced duplication between `FormulaEvaluator` and `ScInterpreter`
- focused standalone and Calc parity tests

### Phase 2: Extract Token Walking / Stack Shell Helpers

Goals:

- move reusable execution-shell mechanics out of Calc while leaving Calc as
  the execution host

Deliverables:

- engine/compat helpers for stack walking and token dispatch support
- reduced Calc-local evaluator shell logic
- differential validation around stack-sensitive functions

### Phase 3: Extract Reference-Sensitive Execution Slices

Goals:

- move the first reference-sensitive execution families behind the stable
  orchestration boundary

Deliverables:

- explicit bridge APIs for host-only services
- extracted execution slices validated against Calc behavior
- updated ownership map for remaining Calc-only execution logic

## Validation Strategy

Every execution slice should satisfy all of the following:

- `make -j4 CppunitTest_sc_ucalc_dependency_shadow`
- `make -j4 CppunitTest_sc_ucalc_workbook_facade`
- standalone tests for touched helper/runtime code
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary`
- any focused Calc Cppunit lane touched by the slice
- `git diff --check`

## Critical Files

| File | Role |
| --- | --- |
| [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md) | Consolidated project state and roadmap |
| [RECALC_ORCHESTRATION_EXTRACTION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/RECALC_ORCHESTRATION_EXTRACTION.md) | Completed orchestration milestone and handoff boundary |
| [RecalcQueueExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/RecalcQueueExecution.hxx) | Calc bridge that consumes engine-owned recalc queue output |
| [FormulaEvaluator.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluator.cxx) | Standalone execution shell reference point |
| [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx) | Calc formula-cell execution host |
| [interpr*.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx) | Calc execution backend surface |
