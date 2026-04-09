# Computational Substrate Named-Range Structural Implementation

Status: complete implementation note for the named-range widening plan

## Purpose

This note records what Phase 4 actually wired into the structural pilot.

The goal of this phase was not live rollout widening. It was to let
named-range-sensitive structural cases run through the existing authority
harness in validation mode while preserving the current admitted rollout
boundary.

## What Landed

The validation-only pilot now extends the structural proof path in three
specific ways:

- the structural builder can recognize a bounded named-range-sensitive slice
  during validation builds
- single-area named-range targets can be shifted and compared on the
  computational shadow surface
- Calc differential tests can run global named-range structural candidates
  through `validateCandidate(...)` without enabling the live rollout path

The main code surfaces are:

- [StructuralPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/StructuralPilotBuilder.hxx)
- [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Validation-Only Slice That Is Implemented

The current implementation admits only the bounded global single-area slice
into the validation build path.

What now works in validation mode:

- row and column insert/delete structural mutations
- ordinary scalar formulas
- single-area named-range targets
- exact target rewriting for names whose source text stays inside the simple
  absolute A1 single-area form

What remains outside the implementation surface even in validation mode:

- multi-area names
- sheet-local names
- shared-group-sensitive structural behavior
- scope-ambiguous name sets

## Important Working Rules

The pilot still keeps live authority unchanged:

- `apply(...)` remains limited to the already admitted narrow rollout surface
- named-range-sensitive structural cases must use `validateCandidate(...)`
- validation-only cases still return exact verdict categories through the same
  rollback-capable bridge

For named-range prediction, the pilot now:

- preserves whether the original target expression used an explicit sheet
  prefix
- rewrites only the single-area target expression
- keeps the observed after-state descriptor identity and base-address data
  supplied by the facade

That is deliberate. In this phase, target rewriting is predicted; broader
named-range descriptor authority is not yet claimed.

## Phase-4 Result

Phase 4 now leaves the proof cycle in a useful state:

- global single-area named-range structural cases can enter the validation
  harness
- sheet-local cases still reject cleanly instead of silently drifting into the
  pilot
- multi-area cases still reject cleanly

That is enough to start the evidence pass in the next workstream, where the
project can record exact-match, rollback, and defer outcomes honestly instead
of assuming that every named-range class behaves the same.
