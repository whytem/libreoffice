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
- exact same-sheet shareable named-range-combined `MemberExit`
  `SetScalarValue`, `SetFormula`, and `ClearCell` on that same bounded
  `GlobalSingleAreaSameSheet` surface

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
- broader named-range-combined regroup, merge, and collapse
- broader non-edge regroup and merge shared-group behavior
- repair-sensitive host normalization
- off-sheet shared-group consumers and dependency closure
- broad storage migration beyond the admitted slice
- broad token-container ownership transfer
- broad listener/broadcaster ownership transfer beyond the admitted slice
- workbook-wide or sheet-wide authority transfer

One important lesson from the completed attempts is that some synthetic
exact-modeling candidates are not real live-admission candidates. For
example, the bounded three-group collapse cycle closed as deferred because
live Calc keeps the far group separate.

## Current Roadmap

The roadmap should stay tightly ordered.

For an aggressive multi-blocker execution plan that attacks the full
remaining frontier as one staged program, see
[COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md).

### Priority 1: Same-Surface Shared-Group Widening

The best next work is still on the same bounded authority surface:

- broader same-sheet named-range-combined regroup, merge, and collapse
  classes, but only when the engine can author the exact live after-state
- broader same-sheet non-edge shared-group regroup and merge classes

This is the highest-value next category because it expands the admitted slice
without opening a new host surface.

### Priority 2: Repair-Sensitive Behavior

After the same-surface frontier, the next meaningful step is
repair-sensitive normalization.

That work is harder because it requires either:

- exact engine modeling of Calc cleanup and regroup behavior

or:

- a narrower admitted class that avoids hidden host normalization entirely

### Priority 3: Off-Sheet Shared-Group Behavior

Off-sheet consumers and dependencies are a larger boundary jump.

They should come after same-sheet widening and repair-sensitive work because
they broaden authority scope materially more than the current frontier.

### Priority 4: Reassess Broad Ownership Expansion

Only after the above should the project revisit any question of broader
storage or host-surface transfer.

The current program has already proven a meaningful bounded authority and
ownership result. It does not need to force a larger migration to count as a
success.

## Biggest Blockers

The main blockers are now clear and concrete.

### 1. Exact After-State Authoring For Broader Shared-Group Families

The engine still needs exact after-state authoring for broader regroup,
merge, and collapse families.

The bar is not "synthetic modeling looks plausible." The bar is
"the engine-authored after-state matches live Calc exactly."

### 2. Repair-Sensitive Host Normalization

Some classes still depend on host cleanup behavior that is not yet modeled
as an exact engine-owned rule family.

Until that normalization becomes explicit, those cases should stay deferred.

### 3. Off-Sheet Dependency Closure

Cross-sheet consumers broaden the authority surface significantly.

The blocker is not just formula evaluation. It is exact dependency,
broadcaster, queue, and rollback closure without accidentally promoting a
much larger workbook-wide surface.

### 4. Retained Host Shell Boundaries

The engine can now author much more of the admitted slice, but Calc still
owns the broad document shell and the retained host application surfaces.

That retained boundary limits any claim that broad computational-document
ownership is already justified.

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
