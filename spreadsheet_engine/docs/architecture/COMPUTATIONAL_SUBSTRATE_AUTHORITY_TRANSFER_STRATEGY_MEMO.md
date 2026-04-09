# Computational Substrate Authority Transfer Strategy Memo

Status: active strategy memo for the next phase of the computational-substrate
program

## Purpose

This memo responds to the gap between the original program objective and the
current computational-substrate trajectory.

The shared-engine extraction work has delivered real value. The live
authority-transfer work has not yet done the same. The program therefore needs
an explicit strategy change if the goal remains substantive relocation of Calc
computation authority into `spreadsheet_engine/`.

## Diagnosis

The current codebase supports three conclusions at the same time:

- first-stage shared-engine extraction is real
- the computational substrate is a strong bounded verification and replay
  framework
- the current admitted-slice widening strategy is not, by itself, a plausible
  path to broad authority relocation

The load-bearing evidence is straightforward:

- the substrate remains disabled for normal AutoCalc sessions in
  [ComputationalSubstrateRollout.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollout.hxx)
- production Calc formula evaluation still runs through
  [ScFormulaCell::InterpretTail](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
  and constructs `ScInterpreter`
- Calc production paths consume engine helper plans such as
  [makeLoadTrackingPlan](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
  and [makeGroupInterpretFallbackPlan](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx),
  but not substrate authority/lifecycle as production control flow
- the engine already has a substantial standalone evaluator in
  [FormulaEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/FormulaEvaluator.hxx)
  and its `FormulaEvaluator*.cxx` family
- the current docs already concede that broad default-on authority and broad
  ownership transfer are not justified in
  [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)
  and [COMPUTATIONAL_SUBSTRATE_MASTER.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md)

The practical implication is that the substrate is presently serving as:

- an exact verifier
- a replay oracle
- a bounded mutation-side authority probe

It is not yet serving as the primary mechanism that moves real Calc
computation authority into the standalone engine.

## Strategic Decision

Choose one strategy explicitly:

- `Option A`: keep the original goal and convert the substrate into a migration
  underwriter
- `Option B`: keep the current verification architecture and restate the goal
  as bounded behavioral prediction/verification

This memo recommends `Option A`.

The program should keep the original goal, but stop treating conjunction-based
admitted-slice widening as the main product. The main product should become a
real production authority transfer, with the substrate relegated to the role
of guardrail, comparator, and fallback policy engine.

## Program Rebaseline

The program should be split into two tracks with different definitions of
success.

### Track 1: Shared-Engine Extraction

This track is already materially successful.

It includes:

- compiler and token infrastructure
- standalone workbook loading
- standalone formula evaluation
- dependency snapshots, invalidation planning, and recalc planning
- shared runtime families already consumed by Calc

Track 1 should be described as effectively achieved.

### Track 2: Live Authority Transfer Inside Calc

This track is not yet achieved.

Its success condition is not “another bounded admitted slice.” Its success
condition is that real Calc production paths delegate meaningful authority to
the engine.

Examples of real success:

- `ScFormulaCell::InterpretTail` delegates evaluation to the engine for a
  measurable production family
- Calc delegates recalc planning to the engine for a measurable production
  family
- Calc deletes or permanently retires meaningful portions of duplicated logic
  because the engine path is authoritative

## Recommended Architectural Change

The north-star migration target should be:

- [ScFormulaCell::InterpretTail](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
  -> engine evaluator switchover

That is the highest-leverage authority transfer because it affects real live
evaluation, not just mutation planning or sidecar verification.

The engine evaluator should become the production evaluator for a bounded but
real family of formulas, with Calc fallback for unsupported or behavior-risky
cases.

The substrate then changes role:

- before: the thing being widened
- after: the comparator and safety net that underwrites switchover

## Tactical Recommendations

### 1. Replace Exact-Or-Rollback As The Migration Bar

Exact carry-through remains appropriate for verifier mode.

It is too strict as the only rule for migration mode. A real migration needs
three classes:

- exact-match required
- documented acceptable delta
- unsupported and fall back to Calc

Without that split, the current policy is effectively a no-migration policy.

### 2. Remove The Blanket AutoCalc Veto For Observe And Shadow Modes

The current `GetAutoCalc()` short-circuit is acceptable only if the substrate
is explicitly test-only.

If the goal remains authority relocation, the runtime needs real session
signal. Introduce rollout modes such as:

- `Off`
- `Observe`
- `ShadowCompare`
- `AuthoritativeWithFallback`

`Observe` and `ShadowCompare` should be allowed in normal interactive Calc
sessions.

### 3. Stop Optimizing For One-Conjunction Admissions

The program should stop treating highly specific conjunctions as the primary
unit of success.

Instead, route whole capability families through the engine with fallback.
Examples:

- workbook-local scalar formulas with no external or macro dependencies
- pure runtime-function families already converged onto shared engine
  implementations
- reference and named-range families once the host-backed evaluator path
  handles them

### 4. Make Production Routing Metrics The Main Dashboard

The important metrics are no longer only replay exactness and admitted-slice
enumeration.

The program should track:

- percentage of live evaluations routed through the engine
- fallback reason histogram
- mismatch reason histogram during shadow runs
- number of production Calc callsites retired or bypassed
- number of duplicated Calc algorithms deleted or permanently quarantined

### 5. Treat Runtime-Free Passes As Harness Work, Not Migration Progress

Passes with no production routing change are still useful, but they should be
classified honestly as:

- verifier improvement
- replay coverage expansion
- migration enabler

They should not be presented as meaningful authority relocation unless they
change a real production path.

### 6. Reduce Document Overhead

The architecture docs have become too expensive relative to runtime movement.

Going forward, the active set should be:

- one master current-state document
- one strategy memo
- one active migration plan per transfer target
- short decision records only when the boundary materially changes

The rest should live in archive material or commit history.

## Recommended Next Program

The next active program should be:

1. adopt this strategy rebaseline in the top-level docs
2. create a concrete
   [InterpretTail-to-engine-evaluator switchover plan](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md)
3. introduce production `Observe` and `ShadowCompare` modes for evaluator
   switchover
4. route a narrow real formula family through the engine with fallback
5. expand by capability class, not conjunction count

## Success Condition

This strategy memo is successful only if it changes the execution model of the
program:

- from “prove more bounded substrate slices”
- to “use the substrate to underwrite real production delegation”

If the project is unwilling to make that change, then it should explicitly
choose verifier-only strategy `Option B` and restate the objective
accordingly.
