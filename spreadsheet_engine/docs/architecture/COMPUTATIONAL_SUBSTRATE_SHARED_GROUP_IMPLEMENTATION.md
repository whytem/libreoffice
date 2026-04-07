# Computational Substrate Shared-Group Implementation

Status: bounded structural plus non-structural shared-group slice implemented, including exact regroup, gap-merge, replacement-merge, one-sided insert closeout, admitted bounded named-range preserve, and a deferred multi-group-collapse closeout

## What Landed

The shared-group widening cycle now has one bounded live structural family,
one bounded live non-structural member-exit family, five additional
non-structural frontier admissions on top of the ownership-complete admitted
slice, and one additional bounded multi-group-collapse proof cycle that
closed as deferred.

The workstream is now closed out by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MULTI_GROUP_COLLAPSE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MULTI_GROUP_COLLAPSE_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MULTI_GROUP_COLLAPSE_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MULTI_GROUP_COLLAPSE_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MULTI_GROUP_COLLAPSE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MULTI_GROUP_COLLAPSE_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NAMED_RANGE_LIVE_OWNERSHIP_DECISION_RECORD.md)

The implementation stays intentionally narrow:

- it widens the live admitted slice only for exact same-sheet shareable
  shared-group structural topology cases
- that exact family now includes bounded `Preserve`, `Split`, and `Rebuild`
  outcomes
- it also widens the live admitted slice for exact same-sheet shareable
  non-structural member-exit `SetScalarValue`, `SetFormula`, and
  `ClearCell` cases
- it now also widens the live admitted slice for exact same-sheet shareable
  same-text preserve `SetFormula` on an already-shared member
- it now also widens the live admitted slice for exact same-sheet shareable
  edge-regroup `SetFormula`
- it now also widens the live admitted slice for exact same-sheet shareable
  gap-closing merge `SetFormula`
- it now also widens the live admitted slice for exact same-sheet shareable
  edge replacement-merge `SetFormula`
- it now also widens the live admitted slice for exact same-sheet shareable
  one-sided adjacent insertion `SetFormula`
- it now also widens the live admitted slice for exact same-sheet shareable
  named-range-combined `SameTextPreserve` `SetFormula` on the bounded
  `GlobalSingleAreaSameSheet` surface
- it keeps multi-group collapse, broader named-range-combined classes,
  off-sheet, repair-sensitive, and non-edge regroup shared-group classes
  outside live admission

## Main Runtime Surfaces

The landed path is centered in:

- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)
- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
- [LifecyclePilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx)
- [StructuralPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/StructuralPilotBuilder.hxx)
- [ComputationalSubstrateAuthority.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateAuthority.hxx)
- [ComputationalSubstrateLifecycle.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateLifecycle.hxx)
- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ComputationalSubstrateStructural.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateStructural.hxx)
- [ComputationalSubstrateRollout.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollout.hxx)

## Facade-Side Shared-Group Surfaces

The facade consumer layer now exposes stable shared-group helpers for:

- collection of unique `FormulaGroupDescriptor` values through the workbook
  facade
- explicit transition classification between before and after group
  descriptors

That means tests and evidence can now describe shared-group outcomes as:

- `Preserve`
- `Rebuild`
- `Split`
- or `None`

without relying on hidden Calc-local interpretation.

## Structural Shared-Group Gate

The structural pilot recognizes a dedicated shared-group slice when all of
the following hold:

- the mutation stays inside the already-admitted structural vocabulary
- the workbook has no named-range-sensitive structural behavior
- the shared-group behavior stays single-sheet and shareable on the mutated
  sheet
- the shape stays inside ordinary-formula plus shared-group-member cells
- either the before or after shadow carries shared-group identity

This slice is guarded by:

- `SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL=1`
- `SPREADSHEET_ENGINE_COMPUTATIONAL_SHARED_GROUP=1`

Within that gate, the behavior now splits cleanly:

- validation mode still exercises bounded non-exact, regrouping, and
  repair-sensitive shared-group outcomes
- authority mode admits exact same-sheet shareable structural topology
  candidates

The adjacent non-structural shared-group member-exit slice is now guarded by:

- `SPREADSHEET_ENGINE_COMPUTATIONAL_SHARED_GROUP_NON_STRUCTURAL=1`

Within that gate, the admitted family is:

- `SetScalarValue` through the authority lane
- member-exit `SetFormula` through the lifecycle lane
- `ClearCell` through the lifecycle lane
- same-text preserve `SetFormula` through the lifecycle and mutation-entry
  lanes
- edge-regroup `SetFormula` through the lifecycle and mutation-entry lanes
- gap-closing merge `SetFormula` through the lifecycle and mutation-entry
  lanes
- edge replacement-merge `SetFormula` through the lifecycle and
  mutation-entry lanes
- one-sided adjacent insertion `SetFormula` through the lifecycle and
  mutation-entry lanes
- bounded named-range-combined same-text preserve `SetFormula` through the
  lifecycle and mutation-entry lanes

but only when the touched same-sheet shareable shared-group member exits the
group and surviving members can be repartitioned into exact contiguous runs,
or when identical formula-text replacement preserves the same shareable group
identity exactly, or when a touched edge member regroups through a bounded
adjacent ordinary-formula run without absorbing another prior shared group,
or when a blank gap between exactly two adjacent shareable groups can be
authored as one exact merged after-group, or when an already-shared edge
member can absorb exactly one adjacent prior shared group through a bounded
engine-authored rebuild window, or when a blank touched address can extend
exactly one adjacent prior shared group by one cell through a bounded
engine-authored rebuild window.

## Engine-Predicted Structural Topology

The structural predictor no longer stops at preserve-only group shifting.

For the admitted shared-group structural slice, it now:

- shifts each before-state shared-group member through the structural
  mutation independently
- drops members removed by the mutation
- partitions the surviving shifted members into explicit contiguous vertical
  runs
- materializes only runs longer than one cell as rebuilt shared groups
- leaves single surviving formulas unshared

That one engine-authored rule family naturally covers:

- `Preserve`: one contiguous shifted run with the original length
- `Split`: fewer or no surviving shared runs after the mutation
- `Rebuild`: one or more surviving shared runs whose anchor or length changes

## Validation Fallback Boundary

The host-observed shared-group topology overlay still exists, but only as a
bounded validation fallback.

It is now used only when:

- the shared-group slice is running in validation mode
- the engine-predicted shared-group topology does not exactly match the
  observed after-state

That keeps non-exact regrouping and repair-sensitive classes visible to the
pilot without relabeling them as admitted live authority.

## Authority Carry-Through

The admitted shared-group structural slice now carries through the full
authority-shaped after-state rather than stopping at candidate
classification.

The landed path:

- overlays live observed cell and formula payloads by address while keeping
  shared-group identity engine-authored
- rehydrates per-cell formula-tree and formula-track flags from the predicted
  observation state
- builds IR after-state group bindings from the predicted computational
  shadow, not from observed shared-group topology
- keeps the admitted after-shadow materializable into the mutable substrate
  facade without stale pre-shift formula source text

That is why the preserve, split, and rebuild slices can now close exact
queue, computational, graph, and IR checks on the live narrow rollout path.

The non-structural member-exit slice now closes through the same carry-through
shape:

- the predictor removes the touched member from the before-group
- surviving members are repartitioned into exact contiguous runs
- observation state is rehydrated from the predicted dependency snapshot and
  recalc plan
- mutable-substrate facade state is materialized from the admitted
  after-shadow so shared-group bindings stay exact after apply
- mutation entry no longer rejects resident non-matrix shared formulas on
  this admitted slice

The frontier closeouts add five more bounded rules:

- same-text preserve `SetFormula` is admitted only when the touched address
  remains inside the same shareable group identity
- edge-regroup `SetFormula` is admitted only when the engine can rebuild the
  touched pre-mutation group plus an adjacent ordinary-formula run into an
  exact predicted after-topology
- gap-closing merge `SetFormula` is admitted only when the engine can create
  the inserted formula cell in the predicted shadow and rebuild the two
  adjacent prior shareable groups into one exact merged after-topology
- edge replacement-merge `SetFormula` is admitted only when the engine can
  rebuild the touched pre-group plus exactly one adjacent prior shared group
  into a bounded exact after-topology that still includes the touched
  address
- one-sided adjacent insertion `SetFormula` is admitted only when the engine
  can create the inserted formula cell in the predicted shadow and rebuild
  exactly one adjacent prior shared group into a bounded exact
  prior-length-plus-one after-topology
- true `FormulaGroup` listener anchors are now live-owned on the already
  admitted shared-group path through resident wiring replay
- `HostUnknown` listener anchors remain explicitly out of contract
- named-range-combined same-text preserve is now admitted only when direct
  target-expression dependencies close without opacity and named-range
  common dependencies project to exact `FormulaGroup` listener anchors while
  member-local direct references remain on exact `FormulaCell` anchors
- bounded three-participant multi-group collapse remains deferred because
  the exact authored full-span one-group topology closes only in standalone
  proof; live Calc three-group attempts keep the far participant group
  separate
- multi-group collapse, broader named-range-combined, repair-sensitive,
  off-sheet, and broader non-edge regroup or merge classes remain rejected
  as deferred frontier classes instead of passing through the ordinary
  formula paths

## Test Coverage Added

The checked-in coverage now includes:

- facade consumer coverage for shifted structural `Preserve`, `Split`, and
  `Rebuild` classification
- facade consumer coverage for non-structural shared-group `Split` and
  `Rebuild` classification
- standalone proof that exact structural split prediction does not depend on
  observed shared-group topology
- standalone proof that exact structural rebuild prediction does not depend
  on observed shared-group topology
- standalone proof that exact structural preserve, split, and rebuild
  authority candidates close exact computational, graph, and IR state
- standalone proof that exact non-structural shared-group authority and
  lifecycle candidates close exact computational, graph, and IR state
- standalone proof that exact non-structural shared-group regroup candidates
  close exact computational state without borrowing observed topology
- standalone proof that exact non-structural shared-group merge candidates
  close exact computational state without borrowing observed topology
- standalone proof that exact non-structural shared-group replacement-merge
  candidates close exact computational state without borrowing observed
  topology
- standalone proof that exact non-structural shared-group one-sided insert
  candidates close exact computational state without borrowing observed
  topology
- facade consumer coverage for synthetic non-structural shared-group
  multi-group-collapse classification
- standalone proof that exact synthetic non-structural shared-group
  three-participant collapse candidates close exact computational state
  without borrowing observed topology
- standalone retained reject coverage for four-plus-group collapse
- structural shared-group rejection when the dedicated candidate gate is off
- structural shared-group preserve, split, and rebuild live-apply coverage
  when the dedicated shared-group gate is on
- non-structural shared-group authority and lifecycle live-apply coverage
  when the dedicated non-structural shared-group gate is on
- non-structural shared-group mutation-entry coverage on the same bounded
  slice
- non-structural shared-group regroup lifecycle and mutation-entry coverage
  on the same bounded slice
- non-structural shared-group merge lifecycle and mutation-entry coverage on
  the same bounded slice
- non-structural shared-group replacement-merge lifecycle and mutation-entry
  coverage on the same bounded slice
- non-structural shared-group one-sided insert lifecycle and mutation-entry
  coverage on the same bounded slice
- facade consumer coverage for bounded named-range-combined same-text
  preserve and off-sheet defer classification
- standalone proof that exact named-range-combined same-text preserve
  closes exact computational, graph, and IR state
- Calc dependency-snapshot proof that bounded named-range target
  expressions no longer create opaque nodes or edges
- live lifecycle proof that bounded named-range-combined same-text preserve
  now applies
- live mutation-entry proof that bounded named-range-combined same-text
  preserve now applies
- standalone retained reject coverage for bounded named-range-combined
  member-exit
- Calc facade proof that live three-group attempts keep the far group
  separate instead of producing one full-span collapsed group
- non-structural shared-group lifecycle and mutation-entry proof that live
  three-group attempts keep the far group separate
- live authority and mutation-entry retained reject proof for bounded
  named-range-combined member-exit
- retained reject coverage for `HostUnknown` listener anchors on the live
  wiring path
- standalone shared-group object-realization proof that `FormulaGroup`
  listener anchors now apply without out-of-contract reject while still
  exposing a narrower `computational_mismatch` restore gap
- standalone shared-group rollback proof that `FormulaGroup` listener
  anchors now apply without out-of-contract reject while still exposing a
  narrower `missing_restored_objects` restore gap
- structural shared-group-plus-named-range defer coverage
- structural shared-group repair-detected coverage when the group shape is
  perturbed after the structural mutation

The result is still bounded shared-group admission, not a broad shared-group
rollout.
