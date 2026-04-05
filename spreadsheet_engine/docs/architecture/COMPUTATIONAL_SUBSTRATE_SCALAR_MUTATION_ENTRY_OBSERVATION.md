# Computational Substrate Scalar Mutation Entry Observation

Status: completed scalar-entry observation and classification note

## Purpose

This note records the observation and classification layer added for
[COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md).

The goal of this workstream was to make the remaining scalar-entry
broadcaster mismatch explicit enough to distinguish:

- canonicalization noise in live Calc broadcaster state
- missing or malformed expected broadcaster state in the engine-owned
  after-state
- true dependency or graph divergence

## What Was Added

The observation path now includes a dedicated broadcaster diagnostic
comparison in
[ComputationalShadowComparison.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx).

That comparison reports:

- exact broadcaster equality
- diagnostic ordering equivalence
- diagnostic deduplicated equivalence
- diagnostic empty-broadcaster-drop equivalence
- diagnostic listener-kind-ignored equivalence
- duplicate broadcaster counts
- duplicate listener counts
- empty broadcaster counts
- host-unknown listener counts
- a typed mismatch classification

The mutation-entry result surface now carries that diagnostic payload in
[ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx),
and the Calc proof lane in
[ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
asserts the current scalar-entry mismatch class explicitly instead of only
asserting `mbBroadcasterMatch == false`.

Standalone synthetic coverage in
[computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
now proves that the classifier can distinguish:

- exact broadcaster match
- duplicate broadcaster or listener materialization
- empty broadcaster persistence

## Current Classified Result

The admitted scalar `SetScalarValue` proof lane no longer reports an opaque
"broadcaster mismatch." It now classifies the remaining gap as:

- `missing_expected_broadcasters`

The current bounded evidence for that result is:

- expected cell broadcasters from the engine-owned scalar after-state: `0`
- live cell broadcasters after Calc realization: `2`
- expected area broadcasters: `0`
- live area broadcasters: `0`
- duplicate broadcaster count: `0`
- duplicate listener count: `0`
- empty broadcaster count: `0`
- host-unknown listener count: `0`

That means the remaining scalar-entry mismatch is not currently explained by:

- broadcaster ordering
- duplicate listener materialization
- empty broadcaster persistence
- unexpected host listeners

It is instead explained by a missing broadcaster surface in the predicted
engine-owned computational after-state.

## What This Changes

This observation layer narrows the next implementation question.

The convergence path no longer needs to guess whether live Calc realization
is introducing duplicate or empty broadcaster state. The immediate fix target
is the engine-side scalar authority after-state construction, which is not
currently carrying broadcaster records into the computational shadow even
though the graph after-state already proves those dependencies exactly.

## Boundaries Preserved

This workstream does not:

- widen the admitted scalar-entry surface
- weaken exact computational closeout requirements
- change queue or graph verification
- move rollback out of Calc

It only makes the remaining scalar-entry mismatch observable enough for the
next convergence workstream to change the right thing.
