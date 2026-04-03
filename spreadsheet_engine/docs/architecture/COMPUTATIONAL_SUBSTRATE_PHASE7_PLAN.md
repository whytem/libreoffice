# Computational Substrate Phase 7 Plan

Status: active execution-ready plan for Phase 7

## Purpose

This document is the execution-ready plan for Phase 7 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

Phase 7 begins from the narrowed proceed boundary recorded in
[COMPUTATIONAL_SUBSTRATE_PHASE6_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE6_DECISION_RECORD.md).

Its job is not to widen authority casually. Its job is to decide whether the
computational substrate program has now produced a credible long-term Calc and
engine boundary, and if so, to define that boundary precisely enough to guide
real adoption or to justify stopping cleanly.

Phase 7 is therefore both an architecture and evidence phase. It must produce
an explicit answer to what should become engine-owned, what should remain
host-owned, how that boundary could roll out, and what would cause the program
to stop instead of forcing adoption.

## Phase 7 Goal

Produce an evidence-backed host-boundary re-cut and rollout decision for the
computational substrate program.

That means Phase 7 must:

- define the best candidate long-term Calc and engine boundary using the
  surfaces actually proven through Phase 6
- separate what is ready for engine ownership from what should remain
  intentionally host-owned
- describe a rollout shape that does not rely on hidden dual authority
- establish whether correctness, performance, memory, and operational clarity
  are strong enough to justify broader adoption
- record a proceed, narrow, or stop decision that later work can treat as the
  governing boundary

Phase 7 is successful only if it leaves the project with a clear and durable
architectural answer rather than a vague promise of future rollout.

## Entry Boundary

Phase 7 should start only from the narrower Phase 6 admitted subset that was
actually proven:

- the admitted Phase 5 scalar lifecycle-authoritative subset remains the
  foundation
- structural authority is admitted only for:
  - single-sheet `InsertRows`
  - single-sheet `DeleteColumns`
  - the ordinary-scalar-formula slice with no shared groups or named ranges
- queue comparison is exact
- computational comparison is exact
- graph comparison is exact
- reference-update divergence is treated as repair-detected and unacceptable
- rejection, rollback, and repair-detected outcomes remain first-class

Phase 7 must not assume broad structural authority from that evidence. It must
treat the Phase 6 admitted slice as the maximum proven authority surface at
entry.

## Non-Goals

Phase 7 should not attempt to:

- force broad rollout before the evidence supports it
- re-open settled first-stage extraction streams
- silently widen structural authority beyond the Phase 6 admitted surface
- move UI, UNO, rendering, import/export, persistence, or document-shell
  concerns into `spreadsheet_engine/`
- move printer, path, environment, or other inherently host-bound services
  into the engine
- treat retained Calc ownership as failure when it is the cleaner boundary
- leave the project in a dual-authority state with no explicit stop or narrow
  option

## Required Deliverables

Phase 7 is complete only when all of the following exist:

1. a checked-in boundary contract naming the candidate long-term engine-owned
   and host-owned surfaces
2. a checked-in ownership map that classifies every remaining major
   computational surface as:
   - ready for engine authority
   - ready only for bounded pilot rollout
   - intentionally host-owned
   - deferred
3. a checked-in rollout matrix for workbook and mutation classes
4. checked-in evidence for correctness, performance, memory, and operational
   clarity on representative scenarios
5. a checked-in rollback and deactivation strategy for any candidate rollout
6. a checked-in final decision record that says whether the computational
   substrate program should:
   - proceed into rollout
   - proceed only on a narrower subset
   - stop as an exploratory architecture effort

## Workstreams

### 7.1 Freeze The Phase 7 Decision Contract

Freeze the exact question Phase 7 is allowed to answer before any rollout
recommendation drifts wider than the proven Phase 6 surface.

The contract should name:

- the entry boundary inherited from Phase 6
- what counts as a valid boundary re-cut in this phase
- which outcomes are allowed:
  - broad proceed
  - narrow proceed
  - stop
- which evidence categories are mandatory before any rollout recommendation is
  considered credible

Required artifact:

- one checked-in Phase 7 decision contract note

### 7.2 Produce The Final Ownership And Boundary Map

Produce the concrete candidate long-term boundary for the computational
substrate program.

This map should classify, at minimum:

- computational storage
- formula tree
- formula track
- listener and broadcaster semantics
- structural mutation classes
- execution-facing IR authority
- retained `ScTokenArray` or Calc token-plumbing seams
- retained host-only document or environment services

Every major surface should be marked as:

- ready for engine authority
- ready only for bounded rollout
- intentionally host-owned
- deferred

Required artifacts:

- one checked-in final ownership map
- one checked-in retained host-service list or equivalent boundary note

### 7.3 Define The Candidate Rollout Surface

Translate the boundary map into an explicit rollout candidate instead of a
theoretical architecture statement.

The rollout surface should specify:

- which workbook classes are candidates for broader engine-first authority
- which mutation classes are candidates for rollout
- which classes stay pilot-only
- which runtime toggles or authority gates would exist
- what rollback or deactivation path exists if rollout evidence regresses

Required artifact:

- one checked-in rollout matrix covering workbook classes, mutation classes,
  toggles, and defer boundaries

### 7.4 Add End-To-End Decision Evidence Lanes

Phase 7 must gather evidence that the candidate boundary is operationally
credible, not just semantically correct in isolated differentials.

This workstream should add or freeze decision evidence for:

- representative load/edit/recalc/end-to-end scenarios
- correctness comparison on the admitted authority subset
- representative scenarios that remain intentionally host-owned
- memory and recalc-performance capture on selected workloads
- operational notes about debuggability and boundary clarity

Required artifacts:

- one checked-in decision evidence note for correctness scenarios
- one checked-in performance and memory evidence note

### 7.5 Define Rollback, Deactivation, And Stop Conditions

Before any rollout recommendation is made, Phase 7 must make explicit how the
project backs out if the broader computational substrate boundary does not
hold up.

This workstream should define:

- runtime or build-time authority gates if rollout proceeds
- the conditions that trigger rollback to the pre-rollout boundary
- what metrics or divergence patterns force a stop decision
- what evidence is sufficient to say the exploratory program has succeeded or
  should close without broader rollout

Required artifact:

- one checked-in rollback and deactivation strategy note

### 7.6 Freeze The Phase 7 Closeout And Program Decision

Phase 7 closes only with a checked-in final decision record that says one of:

- proceed into rollout on a defined candidate boundary
- proceed only on a narrower candidate boundary
- stop the computational substrate program as an exploratory architecture
  effort

The closeout must explicitly classify:

- what the intended long-term Calc and engine boundary now is
- what remains intentionally host-owned
- what remains deferred
- whether rollout is justified now, later, or not at all
- what evidence was persuasive enough to support that answer

Required artifact:

- one checked-in final Phase 7 decision record

## Target Surfaces

The first implementation sweep for Phase 7 should expect to introduce or
touch:

- new or extended architecture notes under
  `spreadsheet_engine/docs/architecture/`
- the completed computational substrate decision records from Phases 0 through
  6
- standing validation and evidence lanes adjacent to:
  - [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
  - [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)
  - [ucalc_compile_diff.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_compile_diff.cxx)
- standalone build-check lanes under `spreadsheet_engine/build_check`
- top-level status and architecture index docs:
  - [COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md)
  - [README.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/README.md)
  - [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)

## Recommended Execution Order

Phase 7 should run in this order:

1. freeze the Phase 7 decision contract
2. produce the final ownership and boundary map
3. define the candidate rollout surface
4. add end-to-end decision evidence lanes
5. define rollback, deactivation, and stop conditions
6. freeze the Phase 7 closeout and program decision

## Validation Contract

Phase 7 validation should keep the standing computational substrate and replay
baselines green while adding rollout-decision evidence.

The minimum closeout contract should be:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_ir_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Additional Phase 7 evidence should include:

- representative load/edit/recalc/end-to-end scenarios on the admitted subset
- memory capture or comparison on selected workloads
- recalc-performance capture or comparison on selected workloads
- explicit documentation of any scenario that remains intentionally host-owned
  and why

Where Phase 7 adds rollout evidence lanes, they should exercise both:

- the strongest candidate rollout path
- the strongest candidate stop or narrow justification, if one exists

## Exit Criteria

Phase 7 should be considered complete only if all of the following are true:

1. the project has a checked-in candidate long-term Calc and engine boundary
2. retained host-owned surfaces are explicit rather than accidental
3. rollout candidates and defer surfaces are classified concretely
4. correctness evidence on representative scenarios is stable
5. memory and recalc-performance evidence are recorded, even if the decision
   is to stop or narrow
6. rollback and deactivation strategy are explicit
7. the checked-in final decision record says clearly whether the program
   should proceed broadly, proceed narrowly, or stop

Phase 7 should stop rather than recommend rollout if either of these becomes
true:

- the candidate boundary is more tangled operationally than the current Calc
  and engine split
- the evidence shows that correctness, memory, or recalc behavior is not good
  enough to justify broader adoption

## Phase 7 Definition Of Success

Phase 7 is a success if it leaves the project with a clear final architectural
answer for the computational substrate program.

That answer may be a broader rollout, a narrow rollout, or an honest stop.

It is not a success if it ends with only more pilot code and no explicit
boundary or decision.
