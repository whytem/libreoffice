# Computational Substrate Global Named-Range Admission Decision Record

Status: complete closeout decision for the global named-range admission plan

## Decision

Do not widen the live opt-in rollout.

The bounded global single-area named-range promotion cycle closes with a
narrower result:

- keep the bounded global single-area named-range slice out of the live
  rollout
- keep its exact standalone prediction and gated live-candidate lanes as proof
  surfaces
- keep off-sheet, local, multi-area, and scope-ambiguous classes deferred

## Why The Rollout Does Not Widen

The evidence is not strong enough to treat the bounded global single-area
slice as another admitted live authority class.

What the proof cycle did establish:

- the structural pilot can classify the bounded global single-area slice as an
  admitted candidate when a dedicated gate is enabled
- exact standalone target rewriting is proven for the bounded same-sheet slice
- explicit-sheet-prefix target text is preserved exactly on the standalone
  shadow surface
- dirty-baseline rejection remains deterministic
- deliberate divergence remains repair-detected and rollback-capable

What the proof cycle did not establish:

- an exact live apply on the bounded same-sheet candidate
- a homogeneous happy path that includes off-sheet global-name consumers
- enough evidence to claim that the global single-area slice is stable live
  rollout material

## Final Boundary

The live admitted rollout therefore remains:

- scalar lifecycle authority
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`
- ordinary scalar formulas only
- clean baseline only
- no shared groups
- no named-range-sensitive structural behavior

The only new settled conclusion is narrower than rollout admission:

- the bounded global single-area named-range slice now has:
  - exact standalone prediction evidence
  - a separate gated live-candidate path
  - a checked-in proof record showing why promotion still fails

## Classes Left Deferred

The following remain explicitly deferred:

- global named-range live rollout admission
- off-sheet global-name structural consumers
- sheet-local named-range structural behavior
- multi-area named ranges
- scope-ambiguous name sets
- shared-group-sensitive structural behavior
- sheet insert, delete, rename, or move
- broader storage migration

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_EQUIVALENCE_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_EQUIVALENCE_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_EVIDENCE.md)

## Next Adjacent Concern

The next step should again be a bounded reassessment before any widening.

The evidence does not support jumping straight from this result into:

- sheet-local named-range structural behavior
- or shared-group-sensitive structural behavior

The correct next question is narrower:

- whether the same-sheet global single-area slice can be made exact in the
  live authority path
- or whether named-range-sensitive structural behavior should remain outside
  the admitted rollout while the project turns to a different adjacent concern
