# Computational Substrate Off-Sheet Dependency Closure Decision Record

Status: completed closeout for the bounded off-sheet widening pass

## Decision

Admit the bounded direct off-sheet shared-group non-structural
`MemberExit` slice, and keep broader off-sheet behavior deferred.

The newly admitted slice is:

- same workbook
- one mutation sheet plus one direct consumer sheet
- shareable shared-group non-structural `MemberExit`
- `SetScalarValue`, `SetFormula`, and `ClearCell`
- exact authority, lifecycle, mutation-entry, queue, graph, and IR proof

## Why It Closed

The real blocker was narrower than the old roadmap wording suggested.

Bounded direct off-sheet consumers were failing because cross-sheet
references like `Data.B1` were being treated as `missing_named_reference`
inside dependency-snapshot construction, which turned the candidate into
`opaque_dependency_surface`.

Once that raw-reference fallback was added, the bounded one-consumer-sheet
member-exit family closed exactly.

## What Did Not Close

This pass did not justify admission for:

- off-sheet named-range-combined families as a distinct live boundary
- off-sheet same-text preserve
- off-sheet regroup
- off-sheet merge or replacement-merge
- structural off-sheet named-range rollout
- multi-consumer-sheet off-sheet behavior
- workbook-wide off-sheet authority

Those remain deferred because this pass only proved the bounded direct
consumer member-exit lane.

## Current Off-Sheet Boundary

The off-sheet frontier is now narrower:

- admitted:
  one-consumer-sheet direct off-sheet shared-group member-exit
- deferred:
  off-sheet formula-retained families and off-sheet named-range-combined
  families

## Next Logical Blocker

The next off-sheet widening target should stay adjacent to this result:

- either bounded direct off-sheet `SameTextPreserve`
- or bounded direct off-sheet `Regroup`

Named-range-combined off-sheet work should wait until the live facade and
shadow surfaces expose that boundary distinctly enough to prove it as a real
off-sheet family rather than collapsing it back onto the same-sheet lane.
