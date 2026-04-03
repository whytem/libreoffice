# Computational Substrate Phase 3 Plan

Status: active implementation-ready Phase 3 plan

## Purpose

This document is the execution-ready plan for Phase 3 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

Phase 3 begins from the narrowed proceed boundary recorded in
[COMPUTATIONAL_SUBSTRATE_PHASE2_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE2_DECISION_RECORD.md).

Its job is to define and validate the first engine-owned execution-facing IR
boundary for the admitted computational-substrate subset so the project can
stop relying on `ScTokenArray` as the only plausible long-term execution
authority.

Phase 3 does not make the engine authoritative for live execution yet. It
establishes the IR model, lowering boundary, comparison rules, and safe
reference-update semantics required before any later authority pilot can be
credible.

## Phase 3 Goal

Introduce a narrowed engine-owned execution IR for the admitted Phase 2 graph
subset.

That means the engine must be able to:

- represent execution for the admitted formula-cell and formula-group subset
- lower current compiler output into that IR without durable Calc-owned token
  identity
- preserve execution-relevant reference shape for the admitted subset
- compare IR-backed execution behavior against the existing token-backed and
  replay-backed lanes

Phase 3 is successful only if the IR is real enough to become the future
execution authority candidate rather than a thin serialization wrapper around
Calc token containers.

## Entry Boundary

Phase 3 should start only from the subset validated by Phase 2:

- formula-cell graph nodes
- formula-group graph nodes
- normalized listener anchors
- cell and area broadcaster nodes
- broadcaster-to-listener edges
- graph-facing formula-tree and formula-track subsets
- rebuild-based graph maintenance for:
  - `SetValue`
  - formula edit
  - formula insertion
  - `ClearCell`
  - named-range rename in the standalone differential lane
- explicit delayed-state comparison for:
  - delayed listener startup
  - delayed broadcaster deletion
- representative structural rebuild coverage for:
  - single row insert
  - single column delete

The initial IR-supported mutation and execution set should therefore stay
aligned to that narrowed surface rather than widening immediately to
copy/move, clipboard, load-time, or broad structural authority.

## Non-Goals

Phase 3 should not attempt to:

- make the IR authoritative for all compiled formulas
- migrate `ScTokenArray` ownership out of Calc wholesale
- make listener, broadcaster, or graph maintenance authoritative
- move BASM slot layout or listener contexts into the engine
- broaden immediately to copy/move, clipboard, or `CalcAfterLoad` IR
  authority
- widen to external-reference cache ownership or other host-heavy services

## Required Deliverables

Phase 3 is complete only when all of the following exist:

1. an explicit engine-owned execution IR schema for the admitted subset
2. a stable lowering boundary from current compiler output into that IR
3. checked-in mapping rules that define which token concepts are preserved,
   normalized, or deferred
4. explicit reference-shape and structural-update semantics for the admitted
   subset
5. automated differential comparison between IR-backed expectations and the
   existing execution or replay baseline
6. a checked-in proceed/stop decision record for Phase 4

## Workstreams

### 3.1 Define The Narrowed Execution IR Contract

Freeze the exact semantic scope of the first engine-owned execution IR slice.

The contract should name:

- which formula-bearing cells and groups are admitted
- which token or execution concepts must survive lowering
- which current Calc token semantics are intentionally normalized
- which token features are explicitly deferred

The contract should also record what Phase 3 means by "execution authority
candidate" so later phases do not quietly drift back to `ScTokenArray` as the
real source of truth.

Required artifact:

- one checked-in Phase 3 IR contract note that defines the admitted semantic
  surface and the explicit defer list
- [COMPUTATIONAL_SUBSTRATE_PHASE3_IR_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE3_IR_CONTRACT.md)

### 3.2 Define The Engine-Owned IR Schema

Introduce the engine-owned IR types for the admitted subset.

The schema should represent:

- executable formula-body nodes for the admitted subset
- literal, scalar, reference, and range-shape carriers needed by that subset
- function or operator calls required by the admitted execution surface
- any graph-facing execution metadata needed for later authority phases

Recommended constraints:

- use engine-owned value-semantic ids and payloads
- keep Calc token pointers, pool indices, and container addresses out of
  durable IR identity
- keep the schema explicitly separate from `ScTokenArray` storage policy

Likely code homes:

- `spreadsheet_engine/inc/spreadsheetengine/detail/substrate/`
- `spreadsheet_engine/source/core/`

Required artifact:

- one checked-in schema or API note naming the Phase 3 IR types and their
  ownership rules

### 3.3 Build Lowering From Current Compiler Output Into The IR

Implement a stable lowering/import path from current compiler output into the
Phase 3 IR for the admitted subset.

The lowering boundary should define:

- what is imported directly
- what is normalized on entry
- what remains host-only or token-only and is therefore deferred

The first lowering lane may be rebuild-only, but it must be deterministic and
must not treat `ScTokenArray` as hidden long-term identity.

Required artifact:

- one checked-in lowering API plus at least one standalone or Calc-backed lane
  that exercises IR lowering on representative formulas from the admitted
  subset

### 3.4 Add Reference-Shape And Structural-Update Rules

Define and validate the reference-update semantics needed by the admitted IR
subset.

This workstream should cover:

- scalar references
- range references
- formula-group-related reference shape where admitted
- the representative single-row insert and single-column delete cases already
  carried forward from Phase 2

If some `ScTokenArray`-era update behavior is too Calc-shaped to admit, it
should be recorded explicitly as deferred rather than silently ignored.

Required artifact:

- one checked-in mapping or evidence note for Phase 3 reference-update and
  structural-adjustment behavior on the admitted subset

### 3.5 Add IR Differential Validation Lanes

Wire the Phase 3 IR into automated differential validation.

At minimum, validation should compare:

- lowered IR shape against the admitted compile input
- IR-backed reference-shape expectations after representative mutations
- IR-backed execution-relevant shape against existing replay and evaluator
  expectations on the admitted subset

Phase 3 does not need to replace the existing evaluator or compiler lanes, but
it must prove that the IR boundary stays aligned with them.

Required artifact:

- one checked-in differential surface with explicit verdicts for exact,
  normalized-equivalent, and mismatched IR states

### 3.6 Freeze Phase 3 Closeout And The Proceed Decision

Phase 3 closes only with a checked-in decision record that says one of:

- proceed to Phase 4
- proceed only on a narrower IR subset
- stop the computational substrate program here

The closeout must explicitly classify:

- what is now admitted into the engine-owned execution IR
- what remains deferred to later phases
- what still fundamentally depends on Calc token containers or host-only
  services

Required artifact:

- one checked-in Phase 3 decision record with explicit proceed or stop
  reasoning

## Target Surfaces

The first implementation sweep for Phase 3 should expect to introduce or
touch:

- new IR and lowering code under
  `spreadsheet_engine/inc/spreadsheetengine/detail/substrate/`
- new implementation files under
  `spreadsheet_engine/source/core/`
- completed computational-substrate surfaces:
  - [ComputationalShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ComputationalShadow.hxx)
  - [DependencyGraphShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/DependencyGraphShadow.hxx)
  - [DependencyGraphShadowBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/DependencyGraphShadowBuilder.hxx)
  - [DependencyGraphShadowComparison.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx)
- compiler and token-facing boundary surfaces under Calc and compat layers
- standalone tests under `spreadsheet_engine/tests/unit/`
- Calc validation lanes under:
  - [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)
  - [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Recommended Execution Order

Phase 3 should run in this order:

1. freeze the narrowed IR contract and defer map
2. define the IR schema
3. build deterministic lowering from current compiler output
4. define reference-shape and structural-update rules for the admitted subset
5. add IR differential validation lanes
6. freeze the Phase 3 decision record

This ordering matters because the project should not widen the lowering or
update logic before the IR contract is explicit.

## Validation Contract

Phase 3 should explicitly require:

- `git diff --check`
- targeted computational-substrate cases in `CppunitTest_sc_ucalc_workbook_facade`
- targeted graph and IR cases in `CppunitTest_sc_ucalc_dependency_shadow`
- new standalone IR, lowering, or reference-update lanes under
  `spreadsheet_engine/tests/unit/`
- compiler-path validation for the admitted lowering surface
- differential tests for:
  - formula edit
  - formula insertion
  - `ClearCell`
  - named-range rename on the admitted subset
  - representative row insert
  - representative column delete
- standing graph-shadow and computational-shadow lanes that overlap the
  touched surfaces
- one-shot
  `spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

Where lowering or IR helpers cross the current compiler or evaluator boundary,
the affected standalone and Calc-backed lanes should also be rerun.

## Exit Criteria

Phase 3 is successful only if all of the following are true:

1. the engine has an explicit execution IR for the admitted subset
2. current compiler output can be lowered into that IR without hidden Calc
   container identity
3. the admitted reference-update and representative structural cases remain
   coherent in the IR boundary
4. exact or intentionally-normalized IR differential comparisons are
   automated and green
5. the project can make a justified proceed, narrow-proceed, or stop decision
   for Phase 4

If the only viable Phase 3 result is a thin wrapper that still treats
`ScTokenArray` as the real authority, then the correct outcome is to stop or
narrow the program here rather than pushing forward with a false engine-owned
IR boundary.
