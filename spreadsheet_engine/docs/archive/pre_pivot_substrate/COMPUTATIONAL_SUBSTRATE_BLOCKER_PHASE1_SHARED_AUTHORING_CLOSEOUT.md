# Computational Substrate Blocker Phase 1 Shared Authoring Closeout

Status: completed closeout for blocker-clearance Phase 1

## Scope

This closeout covers Phase 1 of
[COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md):
shared after-state authoring for broader same-sheet shared-group families.

## What Was Tested

The phase focused on the narrowest deferred same-sheet named-range-combined
families that looked closest to the current admitted slice:

- regroup attempt
- gap-closing merge attempt
- replacement-merge attempt
- one-sided insert attempt

Each attempt was checked against live Calc facade behavior rather than only
synthetic in-memory expectations.

## Result

Phase 1 closed without an admitted-slice expansion.

The important finding is that these bounded named-range-combined same-sheet
attempts do not currently behave like distinct live regroup or merge
families in Calc. On the live facade surface they remain on the bounded
named-range same-sheet boundary, but the observed mutation classification
normalizes to `SameTextPreserve` rather than producing a distinct
regroup/merge/replacement-merge/one-sided-insert family.

That means the attempted widening was not a trustworthy admitted-slice
expansion. It was a synthetic-model widening that did not match the live
host shape closely enough to promote.

## Decision

The broader named-range-combined same-sheet regroup and merge attempt
families remain deferred.

The authority gate stays unchanged:

- admitted named-range-combined `SameTextPreserve`
- admitted named-range-combined `MemberExit`
- deferred broader named-range-combined regroup, merge, and collapse

## Why This Matters

This phase still cleared useful uncertainty.

- The blocker is no longer “we have not looked at these families.”
- The blocker is now explicit: live Calc does not currently expose these
  attempts as distinct broadened families on the bounded facade surface.
- Future widening has to start from real live-shape canonicalization, not
  from synthetic group-topology expectations.

## Evidence

The retained live proof added in this phase is in:

- `sc/qa/unit/ucalc_workbook_facade.cxx`

The phase also reran the existing computational proof ladder after backing
out the non-closing admission attempt:

- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_dependency_shadow`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_computational_substrate_tests`
- `git diff --check`

## Follow-On

The next blocker phase should not retry the same synthetic widening.
Instead it should move to Phase 2 and make repair-sensitive normalization
explicit, while keeping the Phase 1 finding recorded in the blocker matrix
and master roadmap.
