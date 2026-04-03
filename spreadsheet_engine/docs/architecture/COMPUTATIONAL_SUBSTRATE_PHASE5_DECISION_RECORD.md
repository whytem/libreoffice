# Computational Substrate Phase 5 Decision Record

Status: completed closeout record

## Decision

Proceed to Phase 6 only on a narrower lifecycle-authoritative subset.

Phase 5 is a successful proceed decision, but it is not a broad admission of
formula lifecycle authority.

The admitted Phase 5 subset is:

- direct scalar formula insertion
- scalar formula replacement that preserves single-cell shape
- scalar formula removal through `ClearCell`
- clean-baseline entry only
- exact queue verification
- exact computational verification
- exact graph verification
- execution-IR comparison retained as observation data

Phase 6 should inherit only that admitted surface unless it proves additional
widening deliberately.

## What Phase 5 Proved

Phase 5 proved that the engine can be the real source of lifecycle transitions
for the narrow scalar subset rather than merely shadowing Calc after the fact.

On the admitted subset, the engine now owns:

- lifecycle mutation classification for formula insertion, replacement, and
  removal
- lifecycle transition construction
- engine-authored post-mutation computational shadow
- engine-authored post-mutation dependency graph
- engine-authored post-mutation recalc queue
- explicit host synchronization actions for the narrowed scalar subset
- explicit rejection, rollback, and repair-detected verdicts

Calc still applies the host-visible mutation APIs and remains the owner of:

- document storage
- formula-cell object lifetime
- live listener and broadcaster containers
- the final host verification and rollback surface

That is acceptable for Phase 5 because the phase goal was not full host
replacement. It was to prove that the engine can originate the admitted
lifecycle answer and detect when Calc diverges from it.

## What Remains Deferred

The following lifecycle classes remain deferred after Phase 5:

- shared-group creation, split, merge, and repair
- structural-edit lifecycle
- named-range lifecycle authority in the live path
- copy, move, clipboard, load-time, and undo-like lifecycle behavior
- any path that depends on retained Calc listener or broadcaster ownership in
  ways the engine does not yet model directly

These are not partially admitted by implication. They remain explicit defer
surfaces.

## Execution-IR Gate Decision

Execution-IR comparison should remain observational in Phase 6.

Reason:

- the admitted Phase 5 lifecycle path already proved exact queue,
  computational, and graph verification
- no new evidence from Phase 5 justifies upgrading IR observation into a hard
  rollback gate yet
- the next phase should first prove structural and reference-update widening
  on the narrowed admitted subset before tightening the IR gate

## Unacceptable Divergence Patterns

The following remain unacceptable and should continue to trigger rejection,
rollback, or repair-detected outcomes:

- dirty-baseline entry into lifecycle authority
- mutation classes outside the admitted scalar formula subset
- queue mismatch after admitted lifecycle application
- graph mismatch after admitted lifecycle application
- computational-after mismatch that indicates silent lifecycle repair or
  under-modeled host behavior

## Proceed Boundary For Phase 6

Phase 6 may proceed, but only from this narrower boundary:

- scalar lifecycle authority already admitted in Phase 5
- execution-IR comparison remains observational
- shared groups stay deferred
- structural widening must be admitted incrementally rather than assumed

If Phase 6 cannot preserve those guardrails while widening into structural or
reference-update behavior, the program should narrow again rather than treating
Phase 5 as blanket evidence for broader lifecycle authority.
