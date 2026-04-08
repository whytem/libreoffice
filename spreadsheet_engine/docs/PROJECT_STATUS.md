# Spreadsheet Engine: Project Status

This file is the concise current-state snapshot for `spreadsheet_engine/`.

For the full computational-substrate boundary, admitted slice, roadmap, and
blockers, start with
[architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md).

## Project Objective

The long-term objective is to make `spreadsheet_engine/` the home for Calc's
spreadsheet-specific computation engine while keeping Calc as the document
and application host.

In practical terms, the engine is intended to own:

- formula compilation and compiler-host interfaces
- token modeling and execution-facing compiler output
- spreadsheet evaluation semantics
- dependency analysis, invalidation planning, and recalc planning
- standalone workbook loading and evaluation
- bounded admitted-slice authority and ownership where exact carry-through is
  proven

Calc is intended to remain responsible for:

- UI, import/export, rendering, UNO, and shell integration
- persistence and document-service integrations
- retained document-shell behavior outside the admitted slice

## Current Status At A Glance

The original extraction objective is effectively achieved.

Today:

- the engine builds standalone with CMake and also inside LibreOffice
- the engine owns the shared compiler and token-model surface
- the engine owns the standalone workbook model, FODS loader, parser, and
  evaluator
- the engine owns dependency snapshots, invalidation planning, and recalc
  planning
- Calc already consumes substantial engine-owned runtime and compat logic in
  production execution paths
- the computational-substrate program proved a bounded, exact, opt-in
  authority slice
- the program also proved a bounded ownership-complete admitted slice on top
  of that authority result

What has not been proven is equally important:

- broad default-on authority is not justified
- broad document-core replacement is not justified
- broad storage, listener, token-container, or workbook-wide ownership
  transfer is not justified

## Verified Baseline

The standing promoted replay baseline is:

- `500` workbooks
- `50,661` formula cells
- `50,652` parsed formulas
- `0` cached-fallback cells
- `0` cached-fallback rate

Enabled promoted replay families include:

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

This baseline remains the hard guardrail for future widening work.

## Current Computational Substrate Position

### Engine-Owned Today

The engine broadly owns:

- compiler and token infrastructure
- standalone workbook loading and evaluation
- dependency snapshots, invalidation planning, and recalc planning
- a substantial shared runtime used by Calc

Within the admitted slice, the engine also owns bounded end-to-end decision
records for:

- sidecar computational state
- admitted resident cell and wiring state
- admitted formula-cell lifetime decisions
- object realization, rollback, raw mutation, and live-apply records
- primitive realization, rollback, and execution records

### Calc-Retained Today

Calc still intentionally owns:

- broad `ScDocument` storage and mutation outside the admitted slice
- broad object lifetime outside the admitted slice
- broad listener and broadcaster residency outside the admitted slice
- token-container construction and Calc-local token plumbing
- UI, UNO, rendering, persistence, and document-service integration
- the retained host shell that executes engine-authored records on the
  admitted live slice

## Current Opt-In Narrow Rollout Surface

The admitted rollout surface currently includes:

- admitted scalar lifecycle authority
- admitted scalar mutation entry
- single-sheet `InsertRows`, `DeleteRows`, `InsertColumns`, and
  `DeleteColumns`
- exact same-sheet shareable shared-group structural `Preserve`, `Split`, and
  `Rebuild`
- exact same-sheet shareable shared-group non-structural member-exit
  `SetScalarValue`, `SetFormula`, and `ClearCell`
- exact same-sheet shareable same-text preserve, bounded regroup, bounded
  merge, bounded replacement-merge, and bounded one-sided insertion
- exact same-sheet shareable named-range-combined `SameTextPreserve`
  `SetFormula`
- exact same-sheet shareable named-range-combined `Regroup` and
  `OneSidedInsert` `SetFormula` on the bounded
  `GlobalSingleAreaSameSheet` surface
- exact same-sheet shareable named-range-combined `MemberExit`
  `SetScalarValue`, `SetFormula`, and `ClearCell` on the bounded
  `GlobalSingleAreaSameSheet` surface
- exact same-workbook one-consumer-sheet direct off-sheet shared-group
  non-structural `MemberExit` `SetScalarValue`, `SetFormula`, and
  `ClearCell`

This remains an opt-in, exact-verification boundary with rollback on
divergence, not a broad live-authority flip.

## Explicitly Deferred Frontier

The following remain deferred:

- named-range-sensitive structural rollout
- named-range-combined multi-group collapse attempts that live Calc keeps
  as far-group-separate or split outcomes
- repair-sensitive structural after-state divergence that is intentionally
  rollback-only
- off-sheet shared-group behavior outside the bounded one-consumer-sheet
  direct `MemberExit` slice
- broad storage migration beyond the admitted slice
- broad token-container and listener ownership transfer
- workbook-wide or sheet-wide authority transfer

## Next Roadmap Category

The broader same-sheet single-cell widening pass is now materially closed on
the current bounded surface.

The recommended order is now:

1. broader off-sheet formula-retained and named-range-combined shared-group
   consumers
2. only then reassess the retained same-sheet multi-group collapse boundary
   if new live evidence suggests a real expansion target

The staged program plan for attacking those blockers together is
[architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md).

The now-completed repair-sensitive closeout pass is
[architecture/COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_PLAN.md).
Its final decision is in
[architecture/COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_DECISION_RECORD.md).

The now-completed off-sheet pass is
[architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_DEPENDENCY_CLOSURE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_DEPENDENCY_CLOSURE_PLAN.md).
Its final decision is in
[architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_DEPENDENCY_CLOSURE_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_DEPENDENCY_CLOSURE_DECISION_RECORD.md).

The earlier same-surface blocker-phase conclusion has now been superseded by
the broader same-sheet widening rerun. Corrected frozen-snapshot live facade
proof showed that bounded named-range-combined `Regroup` and
`OneSidedInsert` are real live families and they are now admitted, while
broader non-edge regroup and merge attempts normalize onto the already-
admitted member-exit path.

The repair-sensitive normalization pass is now closed. The current
repair-sensitive structural probes are explicit deterministic rollback
buckets with stable reasons, not a hidden widening frontier. The pass did
not admit a new family, but it did remove repair-sensitive normalization
from the top-blocker list for the current roadmap.

The off-sheet dependency closure pass is now closed too. It admitted the
bounded one-consumer-sheet direct off-sheet shared-group `MemberExit`
family and narrowed the remaining off-sheet blocker to broader
formula-retained and named-range-combined families.

Phase 4 is now closed too. The retained host shell is now recorded as an
explicit admitted-slice execution and observation contract rather than as a
top opaque blocker. That does not widen the admitted slice by itself, but it
does move the remaining blocker list onto the three still-open technical
frontiers: broader same-sheet authoring, repair-sensitive normalization, and
off-sheet dependency closure.

Phase 5 has now closed the overall blocker-clearance program. The final
decision is that the program materially clarified and reduced the blocker
set, but it did not justify a broad-ownership follow-on. After the
repair-sensitive closeout pass, the current roadmap should move to bounded
off-sheet widening rather than another omnibus blocker program.

## Working Rules Going Forward

- treat the replay baseline as a non-negotiable guardrail
- widen only when the engine authors the exact live after-state
- prefer narrow, exact, well-proved slices over broad synthetic modeling
- keep rollback, verification, and retain/reject boundaries explicit
- treat individual historical `COMPUTATIONAL_SUB*.md` files as supporting
  detail, not the primary status surface

## Reference Material

- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md):
  canonical computational-substrate reference
- [architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md):
  ambitious staged blocker-clearance roadmap
- [architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_DECISION_RECORD.md):
  final blocker-program closeout and reassessment
- [architecture/README.md](architecture/README.md): architecture-doc entry
  point
- [archive/](archive/): archived milestone plans and closeout material
- [extraction-history/](extraction-history/): older extraction and migration
  notes
