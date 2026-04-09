# Computational Substrate Shared-Group Non-Structural Merge Mapping Rules

Status: frozen mapping rules for the exact merge closeout

## Rule Family

The admitted merge family is authored by one bounded rule family rather than
by host-observed after-topology.

## Rule 1: Candidate Surface

The candidate must already satisfy the carried-forward non-structural
shared-group gate:

- same-sheet only
- shareable groups only
- named ranges stable
- observed after-state available

For merge specifically:

- the touched address is blank before
- the touched address is formula and shared after
- exactly one shareable same-column group exists immediately above
- exactly one shareable same-column group exists immediately below

## Rule 2: Gap-Closing Only

If the touched address is adjacent to only one prior shared group, the merge
path does not apply.

If the touched edit begins from an already-shared member and absorbs another
prior shared group, the gap-merge path also does not apply.

Those shapes remain deferred or rejected outside this bounded family.

## Rule 3: Engine-Authored Merge Window

The merge window begins with the upper prior shareable group and ends with
the lower prior shareable group.

It always includes:

- the full upper group
- the touched inserted formula cell
- the full lower group

The engine does not widen that window from host-observed after-group
membership.

## Rule 4: Rebuild Inside The Window

Inside the merge window the predictor:

- creates the inserted formula cell in the predicted shadow
- clears existing shared-group bindings
- lowers every formula cell in address order
- partitions contiguous joinable cells into exact runs
- materializes only runs longer than one cell as shared groups

For the admitted family, that rebuild must produce exactly one merged run
across the full window.

## Rule 5: Admission Still Requires Exact Match

The merge family admits only if the engine-authored predicted after-shadow
still matches the observed after-state exactly for:

- cell population
- formula-group topology
- dependency-driven observation state

If that exact match fails, the candidate remains out of contract.

## Forbidden Shortcuts

The merge closeout explicitly forbids:

- extending the merge window from host-observed group membership
- treating one-sided insertion as merge because live Calc grouped it
- relabeling replacement-driven merge as admitted without an engine-authored
  rule family
- widening named-range or off-sheet behavior from same-sheet gap merge
