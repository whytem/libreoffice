# Computational Substrate Shared-Group Widening Contract

Status: frozen widening contract

## Purpose

This document freezes the exact boundary for the shared-group widening
cycle defined in
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_PLAN.md).

The goal of this cycle is not broad rollout. It is to answer one bounded
question:

- can the ownership-complete admitted slice be widened to include a
  meaningful shared-group structural class without reopening already-settled
  ownership seams

## Carried-Forward Admitted Mutation Vocabulary

This cycle keeps the current admitted mutation vocabulary fixed:

- `SetScalarValue`
- `SetFormula`
- `ClearCell`
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`

No other mutation classes are in scope.

## Shared-Group Workbook Classes Under Reassessment

This cycle reassesses only workbooks whose distinguishing feature is
shared-formula-group behavior on the existing mutation vocabulary.

The representative classes are:

- preserve-existing-group cases
- split-group cases
- rebuild-group cases
- repair-detected or host-repair-sensitive cases

The reassessment stays narrow:

- single-sheet only
- clean baseline only
- no named-range-sensitive behavior
- no sheet insert, delete, rename, or move
- no copy, move, clipboard, load-time, or undo-like flows

## Fixed In-Scope Shared-Group Surface

The widening surface is limited to:

- ordinary scalar workbook content plus shared-formula groups
- shared-group anchors and members already visible through the workbook
  facade
- same-sheet group preserve, split, rebuild, and repair behavior caused by
  the admitted mutation vocabulary
- the computational, graph, queue, realization, rollback, and verification
  effects that follow from those shared-group transitions

## Explicitly Out Of Scope

This cycle does not attempt to:

- widen into named-range-sensitive structural behavior
- widen into sheet-local or cross-sheet structural behavior
- widen into sheet insert, delete, rename, or move
- widen into shared-group behavior combined with broader document flows
- reopen current-slice ownership of resident storage, resident wiring,
  lifetime, mutation-entry, realization, rollback, verification, primitive
  execution, or primitive host-call identity
- weaken exact queue, computational, graph, replay, realization, rollback,
  or verification requirements
- claim broad `ScDocument` independence

## Retained Calc Host Surfaces

This cycle still assumes the bounded host surfaces retained after admitted
slice ownership closeout:

- low-level Calc object and document execution outside the explicit
  engine-authored admitted slice
- shared-group lifecycle and repair paths that cannot yet be represented
  through the existing engine-owned mutation, realization, rollback, and
  verification seams
- any host-only repair or re-canonicalization logic that forces divergence
  from the bounded engine-authored shared-group pilot

If shared-group behavior depends on those retained host surfaces in a way
that the current seams cannot express, the cycle must stop and record that
reason explicitly.

## Success, Hybrid, And Defer Definitions

This cycle may close in exactly one of three ways.

### Success

Bounded shared-group behavior is admitted only if all of the following hold
on a meaningful same-sheet shared-group subset:

- the subset stays inside the carried-forward mutation vocabulary
- preserve, split, rebuild, or repair outcomes are explicit rather than
  hidden
- after-mutation shared-group topology is predicted by the engine from the
  before-state by shifting surviving members and partitioning them into
  explicit contiguous runs rather than by copying host-observed topology
- resident cell state, resident wiring state, lifetime, realization,
  rollback, and verification still close exactly
- the retained hybrid reason is not needed for the admitted subset

### Hybrid Or Validation-Only

The cycle closes as hybrid or validation-only if:

- shared-group behavior can be exercised and classified through the current
  engine-owned seams
- but at least one narrow shared-group lifecycle, regroup, or repair concern
  still requires host-observed or host-owned behavior

In that case the closeout must name the one bounded reason precisely.

### Defer

The cycle closes as deferred if:

- shared-group outcomes cannot be represented honestly through the current
  seams
- or the proof cycle cannot find any meaningful subset that keeps exact
  queue, computational, graph, replay, realization, rollback, and
  verification behavior

## Required Validation Standard

Every candidate shared-group outcome in this cycle must continue to satisfy
the standing validation contract:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

The standing replay baseline must remain exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`
