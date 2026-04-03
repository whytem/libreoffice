# Computational Substrate Phase 0 Decision Record

Status: completed Phase 0 decision record

## Question

Can later phases shadow the live computational substrate exactly enough to
justify authority work?

## Decision

Proceed to Phase 1 on a narrower subset.

## Why

Phase 0 produced all required artifacts:

- explicit ownership inventory in
  [COMPUTATIONAL_SUBSTRATE_PHASE0_INVENTORY.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_INVENTORY.md)
- explicit observable-state model in
  [COMPUTATIONAL_SUBSTRATE_PHASE0_OBSERVABLE_STATE_MODEL.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_OBSERVABLE_STATE_MODEL.md)
- stable capture helpers in
  [ComputationalSubstrateObservation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObservation.hxx)
- representative mutation matrix in
  [COMPUTATIONAL_SUBSTRATE_PHASE0_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_SCENARIO_MATRIX.md)
- automated differential validation in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

The new validation lane now demonstrates stable capture and comparison for:

- scalar precedent edits
- formula text edits
- delayed listener startup
- delayed broadcaster deletion

That is enough to justify moving forward.

## Narrowed Proceed Scope

Phase 1 should start only on the subset that Phase 0 now observes cleanly:

- formula-tree membership and order
- formula-track membership and order
- normalized broadcaster-state shape
- listener registration shape as projected from broadcaster state
- dependency and recalc-plan correspondence on the current safe mutation set

The first Phase 1 shadow storage and graph work should prioritize:

- scalar edits
- formula edits and insertions
- clears
- named-range changes already covered by existing dependency-shadow lanes

## Explicit Defers

Phase 0 does not justify immediate broad authority work for:

- full structural edit coverage across all row and column cases
- copy/move and clipboard rebuild behavior as an authority candidate
- load-time `CalcAfterLoad` listener setup as an authority candidate
- BASM slot-layout fidelity beyond the normalized broadcaster-state surface
- direct migration of Calc-owned containers such as `ScTokenArray`,
  `ColumnBlockPositionSet`, or the current slot-machine layout

Those surfaces stay in observed or deferred status until later phases prove
they can be shadowed with the same precision.

## Risk Gate Outcome

The Phase 0 risk gate is passed, but only for the narrowed subset above.

That means:

- Phase 1 may begin
- Phase 1 should still be staged and differential
- any future widening of the substrate authority candidate set must add its own
  capture-and-compare evidence before it is treated as safe
