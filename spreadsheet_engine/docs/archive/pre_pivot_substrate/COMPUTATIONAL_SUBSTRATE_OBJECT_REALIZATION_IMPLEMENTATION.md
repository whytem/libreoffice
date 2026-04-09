# Computational Substrate Object Realization Implementation

Status: implemented object-realization path

## Purpose

This note records the engine-authored realization path added for
[COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md).

The implementation goal for this workstream is intentionally narrow:

- introduce one explicit engine-authored realization record for the admitted
  slice
- have Calc realize live objects from that record instead of implicitly
  coordinating three separate resident stores at each call site
- keep rollback and raw mutation APIs in Calc

## Landed Engine-Authored Surface

The new compat surface lives in:

- [ComputationalSubstrateObjectRealization.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx)

It now defines:

- `AdmittedObjectRealization`
- `ObjectRealizationResultKind`
- `ObjectRealizationResult`
- `buildAdmittedObjectRealization(...)`
- `realizeAdmittedObjectRealization(...)`

The engine-authored realization record is value-semantic and bundles the
already-proven resident state that live realization needs:

- admitted formula-cell lifetime
- admitted scalar cell storage
- admitted resident wiring containers

## Realization Flow

The landed realization path is:

1. build an `AdmittedObjectRealization` record from the mutable resident
   substrate state
2. realize admitted formula-cell lifetime from that record
3. mirror admitted scalar cell storage from that record
4. `CalcAll()` the live document to stabilize formula payload and dependent
   live state
5. realize admitted listener/broadcaster, formula-tree, and formula-track
   participation from that same record

The important boundary change is that Calc no longer receives those admitted
live realization inputs as three unrelated resident stores at the main call
site. It now consumes one engine-authored realization record.

## Mutation-Entry Integration

The admitted mutation-entry path in
[ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
now uses the new object-realization record for both:

- realized after-state application
- rollback to the captured before-state

It also now records:

- `moObjectRealizationObservation`

inside the admitted mutation-entry result so live object-realization status is
visible alongside queue, computational, graph, IR, and broadcaster
comparisons.

## Landed Proof Updates

The Calc differential proof lanes now use the engine-authored record in the
new object-realization observation tests in
[ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx).

Those lanes now prove:

- the admitted lifecycle exact case closes through
  `AdmittedObjectRealization`
- removing one realized live formula object after that step is classified as
  `missing_realized_objects`
- admitted mutation-entry applied results now carry an explicit
  object-realization observation and are required to classify as `exact`

## Boundary Kept Intact

This workstream still does not claim:

- host-independent mutation APIs
- rollback migration out of Calc
- shared-group, named-range, off-sheet, or sheet-wide realization
- broad `ScDocument` host independence

Calc remains the host for raw mutation entry and final rollback. The landed
change is that admitted live realization is now more explicitly engine-
authored on the bounded slice.
