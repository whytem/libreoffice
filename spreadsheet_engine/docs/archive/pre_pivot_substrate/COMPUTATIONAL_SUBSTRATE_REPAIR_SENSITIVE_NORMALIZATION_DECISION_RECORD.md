# Computational Substrate Repair-Sensitive Normalization Decision Record

Status: completed closeout decision

## Decision

Close the repair-sensitive normalization pass as executed.

Do not promote a new repair-sensitive admitted family.

Do retire repair-sensitive normalization from the top-blocker list for the
current roadmap.

## What This Pass Settled

The currently observed repair-sensitive surface on the bounded same-sheet
structural lane is now explicit enough to classify cleanly:

- exact structural families remain exact and already admitted
- repair-sensitive divergence closes as deterministic rollback with explicit
  structural reason codes
- out-of-scope cases remain explicit rejects rather than hidden cleanup

That means the current repair-sensitive frontier no longer hides a real
live widening target on the surface we just re-checked.

## New Stable Repair Reasons

The structural pass now distinguishes:

- `structural_formula_reference_update_mismatch`
- `structural_named_range_reference_update_mismatch`
- `structural_shared_group_reference_update_mismatch`

These reasons are a better architectural boundary than the old generic
`structural_reference_update_mismatch` string because they match the three
actual repair buckets the proof ladder exercises.

## Result Against The Plan

The plan allowed two acceptable end states:

- identify and admit a real exact widening target
- or prove that the current repair-sensitive surface is a rollback/reject
  boundary rather than a real widening frontier

The second outcome is what the evidence supports.

## Roadmap Consequence

The next meaningful widening blocker is no longer repair-sensitive
normalization.

The roadmap should now move to:

1. bounded off-sheet shared-group dependency closure
2. only after that, reassess the retained same-sheet multi-group collapse
   boundary

If a future live family appears to require actual repair-sensitive
normalization, it should return as a named bounded family rather than as a
generic “repair-sensitive” program bucket.

## Validation Used For This Decision

- `spreadsheetengine_computational_substrate_tests`
- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

