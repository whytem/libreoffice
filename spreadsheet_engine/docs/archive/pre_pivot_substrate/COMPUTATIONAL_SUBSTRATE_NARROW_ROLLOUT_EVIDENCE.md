# Computational Substrate Narrow Rollout Evidence

Status: complete bounded rollout evidence note for the narrow rollout plan

## Purpose

This note freezes the bounded rollout evidence for the admitted computational
substrate authority slice after the first opt-in rollout wiring and the first
bolder widening validation pass.

It exists to answer four questions before the rollout decision workstream:

- does the admitted rollout slice still behave correctly under the live gate
- is the gate and override model operationally understandable
- do the first widening candidates strengthen or weaken the rollout case
- is there any immediate evidence that the opt-in gate introduces material
  cost on the bounded validation lane

## Correctness On The Admitted Slice

The admitted rollout slice remains correct on the standing bounded validation
surface:

- scalar lifecycle authority still applies exactly on the admitted subset
- admitted structural authority still applies exactly for:
  - single-sheet `InsertRows`
  - single-sheet `DeleteColumns`
- dirty-baseline, out-of-contract, rollback, and repair-detected verdicts
  remain explicit rather than becoming hidden host behavior

The rollout-specific gate coverage is checked by:

- `testComputationalNarrowRolloutDisabledByDefault`
- `testComputationalNarrowRolloutLifecycleEnabledByUmbrella`
- `testComputationalNarrowRolloutStructuralEnabledByUmbrella`
- `testComputationalNarrowRolloutDeleteRowEnabledByUmbrella`
- `testComputationalNarrowRolloutInsertColumnEnabledByUmbrella`
- `testComputationalNarrowRolloutStructuralOverrideBeatsUmbrella`

The standing applied and divergence lanes remain in:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_ir_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`

## Operational Clarity

The rollout remains operationally narrow and easy to deactivate:

- one umbrella gate enables the admitted rollout:
  - `SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT`
- the existing per-surface overrides still win when explicitly set:
  - `SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY`
  - `SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE`
  - `SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL`
- the gate is still disabled by default
- rollback and repair-detected verdicts remain first-class result kinds in the
  live compat surface rather than being folded into generic failure

That means the rollout can still be:

- enabled narrowly
- debugged from explicit result shapes
- narrowed again quickly if a later lane regresses

## Performance And Memory Observations

The bounded bake-time sample was taken on the same Calc differential lane with
the gate off and then with the umbrella rollout gate on:

- gate off:
  - command: `make -j1 CppunitTest_sc_ucalc_dependency_shadow`
  - elapsed: `8.88s`
  - max RSS: `251400 KB`
- gate on:
  - command:
    `SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT=1 make -j1 CppunitTest_sc_ucalc_dependency_shadow`
  - elapsed: `8.79s`
  - max RSS: `251548 KB`

Interpretation:

- no meaningful performance regression is visible in this bounded unit-lane
  sample
- memory stayed effectively flat in the same sample
- the rollout is still opt-in, so there is no default-on product-wide cost
  implied by this note

This is not a benchmark program. It is a bounded operational sample intended
to catch obvious rollout cost regressions.

## Effect Of The Bolder Widening Candidates

The first widening pass strengthened the rollout case rather than weakening
it.

Per
[COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_EVIDENCE.md):

- `DeleteRows` currently meets the proof threshold on the narrow scalar slice
- `InsertColumns` currently meets the proof threshold on the narrow scalar
  slice

That means the bounded rollout now has:

- a stable admitted surface
- a clean gate model
- two adjacent structural classes that appear promotable rather than deferred

## Standing Replay Baseline

The standing replay baseline remains unchanged:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

## Workstream Interpretation

This note supports the closeout posture that was taken in the final rollout
decision:

- keep the rollout opt-in and bounded
- do not narrow it again based on current bounded bake evidence
- widen the admitted structural rollout by one step to include `DeleteRows`
  and `InsertColumns`
