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

The first-stage shared-engine extraction objective is effectively achieved.
The live authority-transfer objective is not.

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
- production Calc still does not delegate general cell evaluation authority
  from `ScFormulaCell::InterpretTail` to the engine evaluator
- but a real bounded live evaluator capability cluster now delegates through
  `InterpretTail` under env-gated `observe`, `shadow`, and `authority`
  modes

What has not been proven is equally important:

- broad default-on authority is not justified
- broad document-core replacement is not justified
- broad storage, listener, token-container, or workbook-wide ownership
  transfer is not justified

The active strategy response to that gap is now recorded in:

- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md)

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
- broad real formula evaluation outside the bounded delegated evaluator
  family
- token-container construction and Calc-local token plumbing
- UI, UNO, rendering, persistence, and document-service integration
- the retained host shell that executes engine-authored records on the
  admitted live slice

## Current Live Evaluator Delegation Slice

The first two `ScFormulaCell::InterpretTail -> engine` migration waves are
now landed.

The live delegated evaluator family now includes:

- widened host-backed text parsing for:
  - `VALUE`
  - `DATEVALUE`
  - `TIMEVALUE`
  - `NUMBERVALUE`
- bounded workbook-local lookup and index routing for:
  - `MATCH`
  - `XMATCH`
  - `LOOKUP`
  - `VLOOKUP`
  - `HLOOKUP`
  - `INDEX`

The live seam is controlled by
`SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR` with:

- `observe`
- `shadow` or `shadowcompare`
- `authority`

At the current boundary:

- `observe` records supported and fallback classification on the real
  AutoCalc seam
- `shadow` records mismatch reasons while Calc still owns the final result
- `authority` lets supported formulas bypass `ScInterpreter`
- unsupported or out-of-contract formulas fall back explicitly to Calc

At the current boundary, the live delegated seam accepts:

- literals
- single-cell references
- single-cell workbook-global names
- single-cell sheet-local names
- simple scalar expression trees built from bounded concatenation and scalar
  unary/binary operators
- bounded single-area workbook-local lookup/index reads whose result remains
  scalar or single-cell

The completed closeouts are:

- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_DECISION_RECORD.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_EVIDENCE.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_EVIDENCE.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_PLAN.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_DECISION_RECORD.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_EVIDENCE.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_EVIDENCE.md)

The immediate next evaluator pass is now
[architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_PLAN.md).

The new Calc-backed replay runner froze the current live-corpus baseline at:

- `interpret_tail_authoritative_total=0`
- `interpret_tail_authoritative_fallback_total=303`
- `interpret_tail_fallback_parse_failure=303`

So the next job is to turn the existing delegated cluster into real
authoritative replay-corpus usage before widening function breadth again.

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
- exact same-sheet global single-area shift-only structural named-range
  `InsertRows`, `DeleteRows`, `InsertColumns`, and `DeleteColumns`
- exact same-sheet shareable named-range-combined split-backed three-group
  `SetFormula` replay on the bounded `GlobalSingleAreaSameSheet`
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
  `MemberExit` `SetScalarValue`, `SetFormula`, and `ClearCell` on the
  bounded `GlobalSingleAreaSingleConsumerSheet` surface

This remains an opt-in, exact-verification boundary with rollback on
divergence, not a broad live-authority flip.

## Explicitly Deferred Frontier

The following remain deferred:

- named-range-sensitive structural rollout beyond the admitted same-sheet
  global single-area shift-only surface
- repair-sensitive structural after-state divergence that is intentionally
  rollback-only
- off-sheet shared-group behavior outside the bounded one-consumer-sheet
  direct `MemberExit`, `SameTextPreserve`, `Regroup`, and host-uncategorized
  gap-closing insertion slices and the bounded named-range-combined
  `GlobalSingleAreaSingleConsumerSheet` `SameTextPreserve`, `Regroup`,
  `OneSidedInsert`, and `MemberExit` slice
- broad storage migration beyond the admitted slice
- broad token-container and listener ownership transfer
- workbook-wide or sheet-wide authority transfer

## Next Roadmap Category

The roadmap now has two distinct tracks:

- shared-engine extraction, which is materially successful
- live authority transfer inside Calc, which is still active

The recommended next migration program is no longer “start the switchover.”
That first pass is now complete.

The recommended next move is:

1. convert the replay corpus from `0` authoritative / `303 parse_failure`
   into the first non-zero authoritative usage on the already-promoted
   `InterpretTail` function families
2. then broaden the live `InterpretTail` delegation family beyond the first
   bounded scalar-input and lookup/index capability cluster
3. keep using the substrate as migration underwriter, comparator, and fallback
   guardrail during that move
4. treat remaining bounded substrate widening as secondary unless it directly
   removes a live evaluator fallback reason

That focused next pass is
[architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_PLAN.md).

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

The now-completed focused pass for the bounded direct off-sheet
gap-closing insertion surface is
[architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_GAP_INSERTION_SURFACE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_GAP_INSERTION_SURFACE_PLAN.md).
Its final decision is in
[architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_GAP_INSERTION_SURFACE_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_OFF_SHEET_GAP_INSERTION_SURFACE_DECISION_RECORD.md).

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

The named-range structural rollout clearance pass is now closed too. The
bounded same-sheet global single-area shift-only structural named-range
surface is now admitted on live apply. That includes representative
`InsertRows`, `DeleteRows`, `InsertColumns`, and `DeleteColumns` cases when
the named-range target shifts without resizing. The retained named-range
structural boundary is now narrower: target-resize edits, off-sheet
consumers, local names, multi-area names, and ambiguous scope sets stay
deferred.

The repair-sensitive normalization pass is now closed. The current
repair-sensitive structural probes are explicit deterministic rollback
buckets with stable reasons, not a hidden widening frontier. The pass did
not admit a new family, but it did remove repair-sensitive normalization
from the top-blocker list for the current roadmap.

The off-sheet dependency closure, broader formula-retained, named-range,
final-surface, and gap-insertion passes are now closed too. Together they
admitted the bounded one-consumer-sheet direct off-sheet shared-group
`MemberExit`, `SameTextPreserve`, `Regroup`, and host-uncategorized
gap-closing insertion lanes plus the bounded off-sheet named-range-combined
`GlobalSingleAreaSingleConsumerSheet` `SameTextPreserve`, `Regroup`,
`OneSidedInsert`, and `MemberExit` families. The old combined off-sheet
blocker is now fully gone on the bounded one-consumer-sheet surface.

The retained host shell and the old blocker-clearance program are both now
historical closeouts rather than active top blockers. The same-sheet split
replay pass, repair-sensitive closeout, and bounded off-sheet widening
passes have all landed. The current roadmap is therefore:

- migrate a real production authority seam first:
  `ScFormulaCell::InterpretTail` -> engine evaluator
- use substrate comparison and fallback to underwrite that migration
- revisit additional bounded substrate widening only when it directly enables
  the migration program

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
- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md):
  strategy reset for moving from verifier-style widening to real authority
  transfer
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_PLAN.md):
  completed closeout for the first live evaluator switchover pass
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_DECISION_RECORD.md):
  final decision for the first live evaluator switchover pass
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_EVIDENCE.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_ENGINE_EVALUATOR_SWITCHOVER_EVIDENCE.md):
  evidence summary for the first live evaluator switchover pass
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_PLAN.md):
  completed closeout for the second live evaluator capability-cluster wave
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_DECISION_RECORD.md):
  final decision for the second live evaluator capability-cluster wave
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_EVIDENCE.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_EVIDENCE.md):
  evidence summary for the second live evaluator capability-cluster wave
- [architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md):
  ambitious staged blocker-clearance roadmap
- [architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_DECISION_RECORD.md](architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_DECISION_RECORD.md):
  final blocker-program closeout and reassessment
- [architecture/README.md](architecture/README.md): architecture-doc entry
  point
- [archive/](archive/): archived milestone plans and closeout material
- [extraction-history/](extraction-history/): older extraction and migration
  notes
