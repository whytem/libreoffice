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

Today:

- `spreadsheet_engine/` is already a real shared computation layer used by
  both standalone evaluation and Calc-backed execution paths
- the promoted FODS replay baseline is stable at zero cached fallback
- the computational-substrate program proved a bounded, exact, opt-in
  authority slice
- the program also proved a bounded ownership-complete admitted slice on top
  of that authority result

What the program did not prove is equally important:

- it did not justify a broad host-core transplant
- it did not justify broad default-on rollout
- it did not justify broad storage, token-container, or workbook-wide
  authority transfer

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
- broad listener/broadcaster residency outside the admitted slice
- token-container construction and Calc-local token plumbing
- UI, UNO, import/export, rendering, persistence, and shell integration
- external-reference, environment, printer, path, and document-service
  integrations
- the retained host shell that executes engine-authored records on the
  admitted live slice

That retained host boundary is not a bug. It is the current settled design.

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
- exact same-workbook one-consumer-sheet direct off-sheet shared-group
  non-structural `MemberExit` `SetScalarValue`, `SetFormula`, and
  `ClearCell`
- exact same-workbook one-consumer-sheet direct off-sheet shared-group
  formula-retained `SameTextPreserve` and `Regroup` `SetFormula`
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

- named-range-sensitive structural rollout
- shared-group-sensitive structural behavior outside the bounded exact
  same-sheet shareable slice
- named-range-combined multi-group collapse attempts that live Calc keeps
  as far-group-separate or split outcomes
- repair-sensitive structural after-state divergence that is intentionally
  rollback-only
- off-sheet shared-group behavior outside the bounded one-consumer-sheet
  direct `MemberExit`, `SameTextPreserve`, and `Regroup` slices and the
  bounded named-range-combined `GlobalSingleAreaSingleConsumerSheet`
  `SameTextPreserve`, `Regroup`, `OneSidedInsert`, and `MemberExit` slice
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

## Current Roadmap

The blocker-clearance program is now closed. Its final reassessment is in
[COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_DECISION_RECORD.md).

The roadmap should now stay tightly ordered around the remaining widening
frontiers.

For an aggressive multi-blocker execution plan that attacks the full
remaining frontier as one staged program, see
[COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md).

For the focused pass that closed the repair-sensitive frontier, see
[COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_PLAN.md](COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_PLAN.md).
Its final decision is in
[COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_DECISION_RECORD.md).

### Priority 1: Broader Off-Sheet Shared-Group Behavior

The direct one-consumer-sheet off-sheet `MemberExit`,
`SameTextPreserve`, and `Regroup` lanes are admitted, and the bounded
one-consumer-sheet off-sheet named-range-combined
`GlobalSingleAreaSingleConsumerSheet` `SameTextPreserve`, `Regroup`,
`OneSidedInsert`, and `MemberExit` lane is admitted too.

The completed closeout for the last combined off-sheet blocker is in
[COMPUTATIONAL_SUBSTRATE_OFF_SHEET_FINAL_SURFACE_PLAN.md](COMPUTATIONAL_SUBSTRATE_OFF_SHEET_FINAL_SURFACE_PLAN.md),
with the final decision in
[COMPUTATIONAL_SUBSTRATE_OFF_SHEET_FINAL_SURFACE_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_OFF_SHEET_FINAL_SURFACE_DECISION_RECORD.md).

The pass also established two important off-sheet host-shape facts:

- bounded direct replacement attempts normalize to the already-admitted
  direct `Regroup` surface
- the only retained one-consumer-sheet off-sheet anomaly is the direct
  gap-closing insertion where live Calc merges after-topology but does not
  expose a stable mutation-family classification

### Priority 2: Reassess The Retained Same-Sheet Deferred Boundary

Only after off-sheet work should the project revisit
the retained same-sheet multi-group collapse boundary. The current live
proof shows a far-group-separate outcome, so this remains an explicit defer
rather than a near-term widening target.

### Priority 3: Reassess Broad Ownership Expansion

Only after the above should the project revisit any question of broader
storage or host-surface transfer.

The current program has already proven a meaningful bounded authority and
ownership result. It does not need to force a larger migration to count as a
success.

## Biggest Remaining Blockers

The main blockers are now clear and concrete.

### 1. Exact After-State Authoring For The Retained Same-Sheet Collapse Boundary

The broader same-sheet single-cell widening pass materially reduced this
frontier, but one explicit same-sheet deferred shape remains:
named-range-combined multi-group collapse that live Calc keeps as a
far-group-separate or split outcome.

This is no longer the best next blocker to attack, but if it is revisited
later the bar stays the same: the engine-authored after-state must match
the live host exactly rather than collapsing more aggressively than Calc.

### 2. Narrow Off-Sheet Gap-Closing Insertion Surface

The old combined off-sheet blocker is now mostly closed:

- off-sheet named-range-combined `MemberExit` is admitted
- direct replacement attempts normalize to admitted direct `Regroup`

The remaining narrow off-sheet question is the bounded direct gap-closing
insertion surface where live Calc exposes merged after-topology but leaves
the mutation-family classification at `None`.

That is no longer a broad off-sheet carry-through blocker. It is now a
host-shape surfacing problem on an otherwise bounded one-consumer-sheet
surface.

## What Dropped Off The Blocker List

Repair-sensitive normalization is no longer a top blocker on the current
roadmap.

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
