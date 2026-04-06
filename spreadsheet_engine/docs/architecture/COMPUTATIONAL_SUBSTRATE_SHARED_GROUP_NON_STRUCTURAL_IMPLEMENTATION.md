# Computational Substrate Shared-Group Non-Structural Implementation

Status: bounded non-structural member-exit shared-group slice implemented

## What Landed

The shared-group boundary now includes one bounded non-structural family in
addition to the previously admitted structural preserve/split/rebuild slice.

The landed non-structural family is:

- same-sheet shareable shared-group member-exit `SetScalarValue`
- same-sheet shareable shared-group member-exit `SetFormula`
- same-sheet shareable shared-group member-exit `ClearCell`

The implementation is closed out by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_DECISION_RECORD.md)

## Main Runtime Surfaces

The landed path is centered in:

- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
- [LifecyclePilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx)
- [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx)
- [ComputationalSubstrateAuthority.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateAuthority.hxx)
- [ComputationalSubstrateLifecycle.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateLifecycle.hxx)
- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ComputationalSubstrateRollout.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollout.hxx)

## Dedicated Gate

The non-structural slice is explicitly bounded by:

- `SPREADSHEET_ENGINE_COMPUTATIONAL_SHARED_GROUP_NON_STRUCTURAL=1`

It widens only the already-admitted authority and lifecycle rollout
surfaces, and it reaches mutation entry only when the explicit mutation-entry
gate is also enabled.

## Engine-Authored Member-Exit Topology

The non-structural predictor now authors after-state shared-group topology
instead of copying host-observed topology.

For the admitted slice it now:

- identifies the touched same-sheet shareable before-group
- removes the touched member from that before-group
- repartitions surviving pre-existing members into contiguous vertical runs
- materializes only runs longer than one cell as rebuilt shared groups
- leaves single surviving formulas ordinary
- represents the touched cell as scalar, empty, or ordinary formula
  according to the mutation kind

Observed-after shared-group topology is retained only as an exact validation
check for this bounded slice, not as the source of truth for the admitted
after-shadow.

## Authority And Lifecycle Carry-Through

The admitted non-structural slice now carries through the same exact
authority-style after-state closure as the admitted structural family.

The landed path:

- threads the observed-after computational shadow only for payload overlay
  and exact topology validation
- rehydrates formula-tree, formula-track, and broadcaster state from the
  predicted observation state
- rebuilds graph and IR from the engine-authored computational after-shadow
- keeps mutable-substrate facade state materialized from the admitted
  after-shadow so shared-group bindings do not go stale after apply

That is what allows the bounded non-structural slice to close exact queue,
computational, graph, and IR verification on the live compat path.

## Mutation Entry And Realization Carry-Through

The mutation-entry path now carries the same bounded non-structural slice.

The implementation work here was:

- thread the dedicated non-structural gate and observed-after computational
  shadow into mutation-entry authority and lifecycle builds
- widen admitted formula-cell lifetime and live wiring realization to accept
  non-matrix shared formulas already present in the live document
- keep scalar-cell mirroring scalar-only while allowing shared formulas to
  remain resident and verified through the realized after-state

That keeps the bounded mutation-entry live path exact without claiming broad
shared-group reconstruction authority outside the admitted slice.

## Coverage Added

The checked-in coverage now includes:

- facade-side non-structural shared-group split and rebuild classification
- standalone authority proof for bounded shared-group `SetScalarValue`
- standalone lifecycle proof for bounded shared-group `SetFormula`
- live narrow-rollout authority proof behind the dedicated non-structural
  gate
- live narrow-rollout lifecycle proof behind the dedicated non-structural
  gate
- mutation-entry authority and lifecycle proof on the same bounded slice
- deterministic gate-off rejection
- standing replay confirmation with zero cached fallback

## Remaining Deferred Boundary

The implementation stays intentionally narrow.

The following still remain deferred:

- same-text shared-group preserve replacements
- regroup or merge behavior across prior groups
- named-range-combined shared-group behavior
- repair-sensitive host-only regroup or normalization paths
- off-sheet or broader workbook classes
