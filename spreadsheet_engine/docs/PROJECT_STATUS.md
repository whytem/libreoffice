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
- exact same-sheet shareable named-range-combined split-backed three-group
  `SetFormula` replay on the bounded `GlobalSingleAreaSameSheet`
  named-range `Regroup` surface
- exact same-workbook one-consumer-sheet direct off-sheet shared-group
  non-structural `MemberExit` `SetScalarValue`, `SetFormula`, and
  `ClearCell`
- exact same-workbook one-consumer-sheet direct off-sheet shared-group
  formula-retained `SameTextPreserve` and `Regroup` `SetFormula`
- exact same-workbook one-consumer-sheet off-sheet named-range-combined
  `SameTextPreserve`, `Regroup`, and `OneSidedInsert` `SetFormula` on the
  bounded `GlobalSingleAreaSingleConsumerSheet` surface
- exact same-workbook one-consumer-sheet off-sheet named-range-combined
  `MemberExit` `SetScalarValue`, `SetFormula`, and `ClearCell` on the
  bounded `GlobalSingleAreaSingleConsumerSheet` surface

This remains an opt-in, exact-verification boundary with rollback on
divergence, not a broad live-authority flip.

## Explicitly Deferred Frontier

The following remain deferred:

- named-range-sensitive structural rollout
- repair-sensitive structural after-state divergence that is intentionally
  rollback-only
- off-sheet shared-group behavior outside the bounded one-consumer-sheet
  direct `MemberExit`, `SameTextPreserve`, and `Regroup` slices and the
  bounded named-range-combined `GlobalSingleAreaSingleConsumerSheet`
  `SameTextPreserve`, `Regroup`, `OneSidedInsert`, and `MemberExit` slice
- broad storage migration beyond the admitted slice
- broad token-container and listener ownership transfer
- workbook-wide or sheet-wide authority transfer

## Next Roadmap Category

The broader same-sheet single-cell widening pass is now materially closed on
the current bounded surface, and the follow-on split-outcome pass is closed
too.

The recommended order is now:

1. isolate the remaining direct off-sheet
   gap-closing insertion surface where live Calc merges after-topology but
   does not expose a stable mutation-family classification
2. only after that, revisit named-range-sensitive structural rollout if the
   roadmap still wants another admitted-slice expansion

The now-completed split-outcome pass is
[architecture/COMPUTATIONAL_SUBSTRATE_SAME_SHEET_NAMED_RANGE_SPLIT_OUTCOME_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_SAME_SHEET_NAMED_RANGE_SPLIT_OUTCOME_PLAN.md).
Its final decision is in
[architecture/COMPUTATIONAL_SUBSTRATE_SAME_SHEET_NAMED_RANGE_SPLIT_OUTCOME_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_SAME_SHEET_NAMED_RANGE_SPLIT_OUTCOME_DECISION_RECORD.md).

The now-completed split replay carry-through pass is
[architecture/COMPUTATIONAL_SUBSTRATE_SAME_SHEET_SPLIT_REPLAY_CARRY_THROUGH_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_SAME_SHEET_SPLIT_REPLAY_CARRY_THROUGH_PLAN.md).
Its final decision is in
[architecture/COMPUTATIONAL_SUBSTRATE_SAME_SHEET_SPLIT_REPLAY_CARRY_THROUGH_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_SAME_SHEET_SPLIT_REPLAY_CARRY_THROUGH_DECISION_RECORD.md).

The staged program plan for attacking those blockers together is
[architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md).

The completed closeout for the final bounded off-sheet surface pass is
[architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_FINAL_SURFACE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_FINAL_SURFACE_PLAN.md).
Its final decision is in
[architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_FINAL_SURFACE_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_FINAL_SURFACE_DECISION_RECORD.md).

The active focused follow-on pass for the retained direct off-sheet
gap-closing insertion surface is
[architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_GAP_INSERTION_SURFACE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_GAP_INSERTION_SURFACE_PLAN.md).

The now-completed repair-sensitive closeout pass is
[architecture/COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_PLAN.md).
Its final decision is in
[architecture/COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_REPAIR_SENSITIVE_NORMALIZATION_DECISION_RECORD.md).

The now-completed off-sheet pass is
[architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_DEPENDENCY_CLOSURE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_DEPENDENCY_CLOSURE_PLAN.md).
Its final decision is in
[architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_DEPENDENCY_CLOSURE_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_DEPENDENCY_CLOSURE_DECISION_RECORD.md).

The now-completed off-sheet named-range and merge pass is
[architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_NAMED_RANGE_AND_MERGE_CARRY_THROUGH_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_NAMED_RANGE_AND_MERGE_CARRY_THROUGH_PLAN.md).
Its final decision is in
[architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_NAMED_RANGE_AND_MERGE_CARRY_THROUGH_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_NAMED_RANGE_AND_MERGE_CARRY_THROUGH_DECISION_RECORD.md).

The earlier same-surface blocker-phase conclusion has now been superseded by
the broader same-sheet widening rerun. Corrected frozen-snapshot live facade
proof showed that bounded named-range-combined `Regroup` and
`OneSidedInsert` are real live families and they are now admitted, while
broader non-edge regroup and merge attempts normalize onto the already-
admitted member-exit path.

The split-outcome follow-on pass then froze the retained same-sheet
three-group host shape more precisely. Live Calc does not expose a real
one-group collapse there. It exposes a split-backed `Regroup` with the far
participant group still separate. Mutation-entry now carries that host-shaped
regroup lane exactly.

The direct split replay carry-through pass is now closed too. The old
same-sheet replay blocker is gone: direct authority and lifecycle replay now
carry that exact split-backed named-range host shape on the bounded
`GlobalSingleAreaSameSheet` surface.

The repair-sensitive normalization pass is now closed. The current
repair-sensitive structural probes are explicit deterministic rollback
buckets with stable reasons, not a hidden widening frontier. The pass did
not admit a new family, but it did remove repair-sensitive normalization
from the top-blocker list for the current roadmap.

The off-sheet dependency closure, broader formula-retained, named-range,
and final-surface passes are now closed too. Together they admitted the
bounded one-consumer-sheet direct off-sheet shared-group `MemberExit`,
`SameTextPreserve`, and `Regroup` families plus the bounded off-sheet
named-range-combined `GlobalSingleAreaSingleConsumerSheet`
`SameTextPreserve`, `Regroup`, `OneSidedInsert`, and `MemberExit`
families. The combined off-sheet blocker is now gone; the remaining
off-sheet question is narrower still: direct gap-closing insertion where
live Calc merges after-topology but does not expose a stable mutation-
family classification, while bounded replacement attempts normalize to the
already-admitted direct `Regroup` lane.

The retained host shell and the old blocker-clearance program are both now
historical closeouts rather than active top blockers. The same-sheet split
replay pass, repair-sensitive closeout, and bounded off-sheet widening
passes have all landed. The current roadmap is therefore narrower and more
concrete:

- direct off-sheet gap-closing insertion where live Calc merges
  after-topology but still leaves mutation-family classification at `None`
- named-range-sensitive structural rollout if another admitted-slice
  expansion is still desired after the off-sheet gap surface is resolved

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
