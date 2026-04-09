# Computational Substrate Master

Status: canonical current-state, roadmap, and blocker reference for the
computational-substrate program

## Purpose

This document is the primary way to understand the computational-substrate
effort.

It supersedes the large collection of individual `COMPUTATIONAL_SUB*.md`
phase, plan, contract, schema, evidence, implementation, and decision
documents as the recommended entry point for current understanding.

Use this document for:

- the settled current boundary
- the admitted rollout and ownership slice
- the remaining deferred frontier
- the roadmap and biggest blockers

Use the older detailed documents only for historical archaeology.

## Executive Summary

The first-stage extraction objective is effectively achieved.
The live authority-transfer objective is not.

Today:

- `spreadsheet_engine/` is already a real shared computation layer used by
  both standalone evaluation and Calc-backed execution paths
- the promoted FODS replay baseline is stable at zero cached fallback
- the computational-substrate program proved a bounded, exact, opt-in
  authority slice
- the program also proved a bounded ownership-complete admitted slice on top
  of that authority result
- production Calc still does not delegate general cell evaluation authority
  from `ScFormulaCell::InterpretTail` to the engine evaluator
- but a first real live evaluator family now delegates through
  `InterpretTail` under env-gated `observe`, `shadow`, and `authority`
  modes

What the program did not prove is equally important:

- it did not justify a broad host-core transplant
- it did not justify broad default-on rollout
- it did not justify broad storage, token-container, or workbook-wide
  authority transfer

The active strategy response to that gap is now:

- [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md)

The correct mental model is:

- successful shared-engine extraction
- plus successful narrow authority proof
- plus successful bounded ownership proof
- but still not broad computational-document replacement

## Verified Baseline

The standing promoted replay baseline is:

- `500` workbooks
- `50,661` formula cells
- `50,652` parsed formulas
- `0` cached-fallback cells
- `0` cached-fallback rate

Promoted replay families include:

- `addin`
- `array`
- `database`
- `date_time`
- `financial`
- `information`
- `logical`
- `mathematical`
- `spreadsheet`
- `statistical`
- `text`

This baseline remains a hard guardrail for any future widening work.

## Engine-Owned Today

The engine already owns the following broad surfaces:

- compiler and token infrastructure
- standalone workbook model, FODS loading, and formula evaluation
- dependency snapshots, invalidation planning, and recalc planning
- a substantial shared runtime used by Calc in production execution paths

On top of that first-stage extraction boundary, the bounded
computational-substrate program now also owns admitted-slice records for:

- mutable computational sidecar state
- resident admitted-slice cell storage
- resident admitted-slice wiring containers
- admitted-slice formula-cell lifetime decisions
- admitted-slice object-realization records
- admitted-slice rollback records
- admitted-slice raw-mutation and raw-document-mutation records
- admitted-slice live-apply plans
- admitted-slice primitive realization, rollback, and execution records

This ownership is real, but it is still admitted-slice ownership, not broad
document-core ownership.

## Calc-Retained Today

Calc still intentionally owns:

- broad `ScDocument` storage and mutation outside the admitted slice
- broad formula-cell object lifetime outside the admitted slice
- broad real formula evaluation through `ScFormulaCell::InterpretTail` and
  `ScInterpreter`, outside the bounded delegated evaluator family
- broad listener/broadcaster residency outside the admitted slice
- token-container construction and Calc-local token plumbing
- UI, UNO, import/export, rendering, persistence, and shell integration
- external-reference, environment, printer, path, and document-service
  integrations
- the retained host shell that executes engine-authored records on the
  admitted live slice

That retained host boundary is not a bug. It is the current settled design.

## Strategic Rebaseline

The program now needs an explicit split between:

- shared-engine extraction, which is materially successful
- live authority transfer inside Calc, which remains unfinished

The computational substrate should therefore be treated primarily as:

- a migration underwriter
- a comparator during shadow runs
- a fallback guardrail during real delegation

It should no longer be treated as the main product if the goal remains
substantive relocation of authority into the standalone engine.

The recommended north-star transfer target is:

- `ScFormulaCell::InterpretTail` -> engine evaluator

That strategy reset is documented in
[COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md),
and the first concrete execution plan is
[COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md).

## Current Live Evaluator Delegation Slice

The first real `InterpretTail -> engine` migration pass is now complete.

That live delegated evaluator slice is:

- literal-only `VALUE`
- literal-only `DATEVALUE`
- literal-only `TIMEVALUE`
- literal-only `NUMBERVALUE`

The live seam is controlled by
`SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR` with:

- `observe`
- `shadow` or `shadowcompare`
- `authority`

The landed boundary is:

- `observe` records supported and fallback classification on the real
  AutoCalc `InterpretTail` path
- `shadow` records mismatch reasons while Calc still owns the result
- `authority` lets supported formulas bypass `ScInterpreter` and project the
  engine result directly into `ScFormulaCell`
- unsupported or out-of-contract formulas fall back explicitly and record a
  fallback reason

The closeout for that first switchover pass is:

- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_EVIDENCE.md)

## Current Admitted Authority And Rollout Slice

The current opt-in narrow rollout surface includes:

- admitted scalar lifecycle authority
- admitted scalar mutation entry
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`
- exact same-sheet shareable shared-group structural `Preserve`, `Split`,
  and `Rebuild`
- exact same-sheet shareable shared-group non-structural member-exit
  `SetScalarValue`, `SetFormula`, and `ClearCell`
- exact same-sheet shareable shared-group same-text preserve `SetFormula`
- exact same-sheet shareable bounded edge-regroup `SetFormula`
- exact same-sheet shareable bounded gap-closing merge `SetFormula`
- exact same-sheet shareable bounded replacement-merge `SetFormula`
- exact same-sheet shareable bounded one-sided adjacent insertion
  `SetFormula`
- exact same-sheet shareable named-range-combined `SameTextPreserve`
  `SetFormula` on the bounded `GlobalSingleAreaSameSheet` surface
- exact same-sheet shareable named-range-combined `Regroup` and
  `OneSidedInsert` `SetFormula` on that same bounded
  `GlobalSingleAreaSameSheet` surface
- exact same-sheet shareable named-range-combined `MemberExit`
  `SetScalarValue`, `SetFormula`, and `ClearCell` on that same bounded
  `GlobalSingleAreaSameSheet` surface
- exact same-sheet global single-area shift-only structural named-range
  `InsertRows`, `DeleteRows`, `InsertColumns`, and `DeleteColumns` for
  ordinary scalar formulas
- exact same-sheet shareable named-range-combined split-backed three-group
  `SetFormula` replay on that same bounded `GlobalSingleAreaSameSheet`
  named-range `Regroup` surface
- exact same-workbook one-consumer-sheet direct off-sheet shared-group
  non-structural `MemberExit` `SetScalarValue`, `SetFormula`, and
  `ClearCell`
- exact same-workbook one-consumer-sheet direct off-sheet shared-group
  formula-retained `SameTextPreserve` and `Regroup` `SetFormula`
- exact same-workbook one-consumer-sheet direct off-sheet host-uncategorized
  gap-closing insertion `SetFormula`
- exact same-workbook one-consumer-sheet off-sheet named-range-combined
  `SameTextPreserve`, `Regroup`, and `OneSidedInsert` `SetFormula` on the
  bounded `GlobalSingleAreaSingleConsumerSheet` surface
- exact same-workbook one-consumer-sheet off-sheet named-range-combined
  `MemberExit` `SetScalarValue`, `SetFormula`, and `ClearCell` on that same
  bounded `GlobalSingleAreaSingleConsumerSheet` surface

This slice remains intentionally constrained by:

- opt-in rollout and feature gates
- clean-baseline requirements
- exact queue, computational, graph, and IR verification
- explicit rollback on divergence

It should be treated as a bounded authority slice, not as proof that broad
default-on authority is ready.

## Current Ownership-Complete Admitted Slice

Within that admitted authority surface, the strongest settled conclusion is
that the engine can own the admitted-slice decision records end-to-end while
Calc executes the retained host shell around them.

At the current boundary, the engine can own:

- after-state planning
- dependency and recalc planning
- resident cell and wiring state on the admitted slice
- admitted formula-cell lifetime decisions
- object realization, rollback, raw mutation, and live-apply records
- primitive realization, rollback, and execution records

Calc still applies those records and remains the host of the underlying
document shell.

This is why the current boundary is best described as
ownership-complete-on-the-admitted-slice rather than broad host independence.

## Explicitly Deferred Frontier

The following remain outside the admitted rollout and outside the settled
ownership boundary:

- named-range-sensitive structural rollout beyond the admitted same-sheet
  global single-area shift-only surface
- shared-group-sensitive structural behavior outside the bounded exact
  same-sheet shareable slice
- repair-sensitive structural after-state divergence that is intentionally
  rollback-only
- off-sheet shared-group behavior outside the bounded one-consumer-sheet
  direct `MemberExit`, `SameTextPreserve`, `Regroup`, and host-uncategorized
  gap-closing insertion slices and the bounded named-range-combined
  `GlobalSingleAreaSingleConsumerSheet` `SameTextPreserve`, `Regroup`,
  `OneSidedInsert`, and `MemberExit` slice
- broad storage migration beyond the admitted slice
- broad token-container ownership transfer
- broad listener/broadcaster ownership transfer beyond the admitted slice
- workbook-wide or sheet-wide authority transfer

One important lesson from the completed attempts is that some synthetic
exact-modeling candidates are not real live-admission candidates. For
example, the bounded three-group collapse cycle closed as deferred because
live Calc keeps the far group separate.

The broader same-sheet widening rerun superseded the older same-surface
blocker-phase conclusion. With corrected frozen-snapshot live facade proof,
bounded named-range-combined `Regroup` and bounded named-range-combined
`OneSidedInsert` closed as real live families and are now admitted, while
broader non-edge regroup and merge attempts closed as normalization onto
the already-admitted member-exit path.

The follow-on split-outcome pass then froze the exact remaining same-sheet
three-group host shape. That pass proved the live outcome is a split-backed
`Regroup`, not a true one-group collapse. It also proved that mutation-entry
already carries that host-shaped regroup outcome exactly.

The direct split replay carry-through pass is now complete too. It closed
the old replay blocker by matching the exact named-range area-broadcaster
shape that live Calc uses for the bounded split-backed outcome, and direct
authority and lifecycle replay are now admitted on that same
`GlobalSingleAreaSameSheet` surface.

## Current Roadmap

The blocker-clearance program is now closed. Its final reassessment is in
[COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_DECISION_RECORD.md).

The roadmap should now stay tightly ordered around real authority-transfer
programs first, and bounded substrate widening only when it materially enables
them.

For an aggressive multi-blocker execution plan that attacks the full
remaining frontier as one staged program, see
[COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md).

For the focused pass that closed the repair-sensitive frontier, see
[COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_PLAN.md](COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_PLAN.md).
Its final decision is in
[COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_DECISION_RECORD.md).

The named-range structural rollout clearance pass is now complete:

- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_ROLLOUT_CLEARANCE_PLAN.md](COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_ROLLOUT_CLEARANCE_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_ROLLOUT_CLEARANCE_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_ROLLOUT_CLEARANCE_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_ROLLOUT_CLEARANCE_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_ROLLOUT_CLEARANCE_EVIDENCE.md)

### Priority 1: Expand InterpretTail Evaluator Delegation By Capability Cluster

The first live switchover phase is now complete.

The highest-value next move is the next evaluator capability wave:

- keep the live `InterpretTail` routing seam
- expand the delegated family from literal-only text parsing to the next
  host-backed capability cluster
- keep fallback-first delegation and shadow comparison as the migration bar

The completed first-pass closeout is:

- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_EVIDENCE.md)

### Priority 2: Shared-Group-Sensitive Structural Behavior Outside The Bounded Slice

Further bounded substrate widening remains useful only when it directly
enables the evaluator migration or removes a concrete fallback reason from the
live authority-transfer program.

## Biggest Remaining Blockers

The main blockers are now clear and concrete.

### 1. InterpretTail Delegation Is Still Too Narrow

The biggest live-authority blocker is no longer the total absence of
delegation. It is the narrowness of the currently delegated family.

- `ScFormulaCell::InterpretTail` now contains a real engine-routing seam
- but the authoritative family is still limited to literal-only text-parsing
  formulas
- workbook-local reference and named-range inputs still mostly fall back to
  Calc

If the program wants substantive authority relocation, the next work must
expand this seam by capability cluster.

### 2. Shared-Group-Sensitive Structural Behavior Outside The Bounded Slice

The structural slice is still intentionally narrow even without named ranges.

Broader shared-group-sensitive structural behavior outside the bounded exact
same-sheet shareable structural surface remains outside the admitted slice
until the engine can author the exact live after-topology for those classes
too.

### 3. Named-Range-Sensitive Structural Rollout Beyond The Admitted Shift Slice

The broad named-range structural blocker is now narrower than it was before,
not gone entirely.

The admitted structural named-range surface now includes the bounded
same-sheet global single-area shift-only lane. The retained named-range
structural frontier is:

- target-resize structural edits
- off-sheet named-range structural consumers
- sheet-local names
- multi-area names
- scope-ambiguous name sets

Those classes still need exact live host proof before they can widen.

## What Dropped Off The Blocker List

Repair-sensitive normalization is no longer a top blocker on the current
roadmap.

The bounded direct off-sheet gap-closing insertion surface is no longer a
top blocker either.

The closeout pass proved that the host-uncategorized direct off-sheet lane
was already exact on the existing bounded machinery:

- live workbook-facade host shape still labels the family as `None`
- direct authority now has the missing exact queue/graph/IR proof
- lifecycle and mutation-entry already close as exact live apply

That removes the last retained one-consumer-sheet direct off-sheet anomaly
from the blocker list.

The repair-sensitive closeout pass showed that the currently observed
same-sheet structural repair families are better described as:

- exact admitted families when the after-state closes exactly
- deterministic rollback with explicit structural reasons when the observed
  after-state diverges
- explicit rejects outside the exact slice

The closeout did not admit a new family, but it did remove the need for a
separate open-ended repair-sensitive widening program on the current
surface.

The retained host shell is still real, but the blocker-clearance Phase 4
pass showed that it is no longer a top opaque blocker on the admitted slice.

The runtime already treats it as an explicit execution-and-observation shell
around engine-authored records for realization, rollback, live apply,
primitive execution, and final verification.

That still limits any claim of broad host independence, but it is no longer
the first thing that needs to change for the next admitted-slice widening
pass.

## Working Rules

Any future widening should continue under the same rules:

- do not sacrifice the zero-fallback replay baseline
- prefer exact verification and explicit rollback over optimistic promotion
- do not accept hidden dual authority where Calc remains the real source of
  truth
- treat performance and memory regressions as architecture issues
- only widen one bounded family at a time
- update this master document when the boundary changes

## Historical Detail

The detailed `COMPUTATIONAL_SUB*.md` documents remain in the repository as
historical closeout records, experiments, and phase-by-phase archaeology.

They are no longer the recommended way to understand the current state.

For current understanding, use:

- this master document
- [COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md)
- [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)

For historical detail, use:

- [../archive/](../archive/)
- [../extraction-history/](../extraction-history/)
