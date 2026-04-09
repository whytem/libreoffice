# Computational Substrate Object Realization Observation

Status: implemented observation and classification note

## Purpose

This note records the observation and classification layer added for
[COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md).

The implementation goal for this workstream is diagnostic, not yet
authoritative:

- make admitted-slice live object-realization drift explicit
- classify it into stable bounded categories
- keep the classification surface small enough to support exact proof lanes
  on the admitted slice

## Landed Observation Surface

The new compat surface is:

- [ComputationalSubstrateObjectRealization.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx)

It introduces:

- `ObjectRealizationObservationKind`
- `ObjectRealizationObservation`
- `classifyObjectRealizationObservation(...)`
- `toString(ObjectRealizationObservationKind)`

The classifier consumes the already-existing admitted proof outputs:

- formula-cell lifetime realization result
- cell-storage mirror result
- wiring realization result
- queue comparison
- computational comparison
- graph comparison
- broadcaster canonicalization comparison

## Classified Result Families

The landed observation layer distinguishes these bounded result families:

- `exact`
  - all realization steps apply
  - queue is exact
  - computational comparison is a full match
  - graph comparison is a full match
  - broadcaster canonicalization is exact
- `ordering_only`
  - live realization differs only by admitted ordering
  - queue and broadcaster ordering are the active non-exact surfaces
- `missing_realized_objects`
  - the live host realization is missing formula objects or broadcaster
    materialization that the engine-authored resident state expects
- `host_only_repair_or_reconstruction`
  - the live host layer introduces duplicate, empty, or otherwise
    reconstructive broadcaster behavior that is not part of the resident
    engine state
- `queue_or_state_mismatch`
  - the divergence is larger than pure host reconstruction and must still be
    treated as a real proof failure
- `out_of_contract`
  - one of the admitted realization stages rejects before differential
    comparison is meaningful

## Landed Proof Lanes

Two proof layers now exercise the observation path:

- standalone classification checks in
  [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- Calc differential cases in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

The Calc differential cases currently prove:

- an exact admitted lifecycle realization case classifies as `exact`
- removing a realized admitted formula object after realization classifies as
  `missing_realized_objects`

The standalone classifier checks also freeze the mapping for:

- `ordering_only`
- `host_only_repair_or_reconstruction`
- `queue_or_state_mismatch`
- `out_of_contract`

## Boundary Kept Intact

This workstream does not yet claim:

- a new engine-authored realization record
- a new live realization path in mutation entry
- rollback migration out of Calc
- widening beyond the admitted scalar and structural slice

It only ensures the next implementation sweep can measure object-realization
drift honestly instead of treating all non-exact live behavior as one opaque
failure bucket.
