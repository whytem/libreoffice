# Computational Substrate Named-Range Structural Decision Record

Status: complete closeout decision for the named-range widening plan

## Decision

Do not widen the live opt-in rollout.

The named-range-sensitive structural proof cycle closes with a narrower result:

- keep named-range-sensitive structural behavior out of the admitted rollout
- keep the bounded global single-area class as a validation-only pilot surface
- explicitly defer sheet-local, multi-area, and scope-ambiguous named-range
  classes

## Why The Rollout Does Not Widen

The evidence is not strong enough to treat named-range-sensitive structural
behavior as another admitted live authority slice.

What the proof cycle did establish:

- the structural pilot can now run a bounded global single-area named-range
  case in validation mode
- exact target rewriting is proven on the standalone computational-shadow
  surface
- dirty-baseline rejection remains deterministic
- deliberate divergence remains repair-detected and rollback-capable

What the proof cycle did not establish:

- a broad named-range happy path across global, local, and multi-area classes
- enough homogeneous behavior to admit sheet-local names alongside globals
- enough evidence to claim that named-range-sensitive structural behavior is a
  stable live rollout expansion

## Final Boundary

The live admitted rollout remains:

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

- the bounded global single-area named-range slice now exists as a
  validation-only structural pilot surface

## Classes Left Deferred

The following remain explicitly deferred:

- sheet-local named-range structural behavior
- multi-area named ranges
- scope-ambiguous name sets
- shared-group-sensitive structural behavior
- sheet insert, delete, rename, or move
- broader storage migration

## Evidence

This decision is supported by:

- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_EVIDENCE.md)

## Next Adjacent Concern

The next step should be another bounded reassessment before any widening.

The evidence does not support jumping straight from this result into:

- shared-group-sensitive structural behavior
- or sheet-level structural authority

The correct next question is narrower:

- whether the bounded global single-area validation-only slice can ever meet
  live rollout standards
- or whether named-range-sensitive structural behavior should stay outside the
  admitted rollout entirely while the project moves on to a different adjacent
  concern
