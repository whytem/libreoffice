# Computational Substrate Named-Range Structural Rollout Clearance Plan

Status: active execution plan for clearing the retained named-range-sensitive
structural rollout blocker

## Purpose

This pass revisits the current top structural widening blocker recorded in
[COMPUTATIONAL_SUBSTRATE_MASTER.md](COMPUTATIONAL_SUBSTRATE_MASTER.md):

- named-range-sensitive structural rollout is still outside the admitted slice

The goal is not to solve every named-range-sensitive structural class at once.
The goal is to clear the blocker honestly by promoting the bounded live class
that already has:

- exact standalone structural prediction
- a dedicated live candidate gate
- clean deterministic reject behavior for off-sheet, local, multi-area, and
  ambiguous cases

## Promotion Target

This pass targets only the bounded same-sheet global single-area structural
surface:

- global named ranges only
- single-area targets only
- ordinary scalar formulas only
- same-sheet structural edits only
- clean baseline only
- `InsertRows`
- `DeleteRows`
- `InsertColumns`
- `DeleteColumns`

This pass is complete only if that surface either:

- joins the admitted structural rollout with exact live proof
- or is explicitly reclosed again with checked-in proof showing why exact live
  replay still fails

## Non-Goals

This pass must not claim admission for:

- off-sheet named-range structural consumers
- sheet-local names
- multi-area names
- scope-ambiguous name sets
- shared-group-sensitive structural behavior
- repair-sensitive structural divergence
- sheet insert, delete, rename, or move

## Workstreams

### 1. Freeze The Candidate Surface

Check that the bounded candidate is still the same one previously isolated by
the older named-range structural passes:

- same-sheet
- global
- single-area
- ordinary scalar
- structural row and column edits only

Required result:

- one checked-in plan freezing the exact promotion target and retained rejects

### 2. Prove Exact Live Carry-Through

Strengthen the current proof surface from “candidate gate reaches the bridge”
to “bounded same-sheet live structural apply is exact.”

This workstream should:

- upgrade the existing live Calc proof from loose admission-style assertions to
  exact admitted-slice assertions
- add any missing representative same-sheet cases needed to justify admitting
  the bounded structural named-range surface rather than one isolated mutation
  sample
- preserve explicit same-sheet/off-sheet separation

Required result:

- checked-in exact live proof for the bounded same-sheet candidate
- retained explicit off-sheet rejection proof

### 3. Patch Runtime Only If Exactness Still Fails

If the strengthened proof exposes a real mismatch, fix only the narrowest
runtime seam needed for the bounded same-sheet candidate.

If the strengthened proof already closes exactly, do not force a production
change that the runtime no longer needs.

Required result:

- either a bounded runtime patch or an explicit proof-backed “no runtime patch
  required” conclusion

### 4. Close Out The Structural Boundary

Update the current-state docs so they describe the structural boundary
accurately after the proof cycle.

The closeout must say clearly:

- whether the bounded same-sheet global single-area named-range structural
  surface is now admitted
- which named-range structural classes remain deferred afterward
- what the next structural blocker becomes once this one is closed

Required artifacts:

- one checked-in decision record
- updates to the master doc, status doc, and architecture README

## Validation Contract

The minimum closeout validation is:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

## Exit Criteria

This pass is complete only when:

- the bounded same-sheet named-range structural promotion target is frozen
- the live proof says exactly whether the bounded same-sheet surface is now
  admitted
- the master current-state docs reflect the actual post-pass boundary
- the next structural blocker is narrower than the current broad
  named-range-sensitive rollout blocker
