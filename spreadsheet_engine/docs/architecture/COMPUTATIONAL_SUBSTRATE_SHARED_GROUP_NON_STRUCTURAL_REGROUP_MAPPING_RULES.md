# Computational Substrate Shared-Group Non-Structural Regroup Mapping Rules

Status: frozen mapping rules for the exact regroup closeout

## Rule Family

The admitted regroup family is authored by one bounded rule family rather
than by host-observed after-topology.

## Rule 1: Candidate Surface

The candidate must already satisfy the carried-forward non-structural
shared-group gate:

- touched address shared before
- same-sheet shareable group
- named ranges stable
- observed after-state available

For regroup specifically:

- the touched address stays shared after
- the observed after touched-group identity differs from the before-group

## Rule 2: Preserve Stays Preserve

If the observed after touched-group identity matches the before-group
identity, the regroup path does not apply.

That shape remains the already-admitted same-text preserve family and still
requires byte-for-byte identical formula text.

## Rule 3: Engine-Authored Regroup Window

The regroup window begins with the touched pre-mutation group.

It may then extend only when the touched address is the anchor or tail of
that pre-mutation group, and only through a contiguous adjacent ordinary
formula run that:

- lives on the same sheet and column
- lowers successfully
- remains shareable
- compares joinably with the touched post-edit lowered formula

If the adjacent run would cross into another prior shared group, the regroup
path stops and the candidate remains deferred.

## Rule 4: Rebuild Inside The Window

Inside the regroup window the predictor:

- clears existing shared-group bindings
- lowers every formula cell in address order
- partitions contiguous joinable cells into exact runs
- materializes only runs longer than one cell as shared groups
- leaves single remaining formulas unshared

This rule family naturally covers:

- the new touched regroup run
- any exact residual untouched run from the prior group

## Rule 5: Admission Still Requires Exact Match

The regroup family admits only if the engine-authored predicted after-shadow
still matches the observed after-state exactly for:

- cell population
- formula-group topology
- dependency-driven observation state

If that exact match fails, the candidate remains out of contract.

## Forbidden Shortcuts

The regroup closeout explicitly forbids:

- extending the regroup window from host-observed group membership
- absorbing a prior shared group because live Calc merged it
- relabeling interior regroup as admitted without an engine-authored rule
- widening named-range or off-sheet behavior from same-sheet edge regroup

