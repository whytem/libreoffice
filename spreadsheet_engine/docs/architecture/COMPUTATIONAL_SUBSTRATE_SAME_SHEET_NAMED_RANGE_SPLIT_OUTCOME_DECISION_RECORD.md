# Computational Substrate Same-Sheet Named-Range Split-Outcome Decision Record

Status: completed closeout for the bounded same-sheet named-range
split-outcome pass

## Decision

Do not admit a new distinct same-sheet named-range-combined
multi-group-collapse family.

Do record the bounded same-sheet named-range three-group attempt as
partially cleared:

- live mutation-entry now closes exactly on that host shape
- it closes as normalization onto the already-admitted bounded named-range
  `Regroup` lane
- direct authority and lifecycle replay of the split-backed host shape
  remain deferred

## Why

The pass started from the older assumption that the remaining blocker was a
three-group collapse into one after-group.

The live facade proof froze a different host contract:

- the bounded named-range surface is still `GlobalSingleAreaSameSheet`
- the mutation family is `Regroup`
- the transition kind is `Split`
- the far participant group remains separate

Once that exact host shape was frozen, the proof ladder closed in three
different ways:

- engine-only standalone modeling matched the split-backed host shape exactly
- live mutation-entry applied that same shape exactly and verified cleanly
- direct authority and lifecycle replay of that same shape still rolled back
  on exact verification

That means the blocker was real, but narrower than the old synthetic model.
It also means the practical admitted slice did widen, but not as a new
distinct family admission. The widening is a normalization result on the
already-admitted named-range `Regroup` lane.

## Exact Result

The bounded same-sheet named-range three-group attempt now closes as:

- live Calc host shape: `Regroup` plus `Split`
- mutation-entry: exact apply
- standalone exactness: exact
- authority replay: retained defer
- lifecycle replay: retained defer

## Current Same-Sheet Boundary

The same-sheet named-range-combined admitted boundary now includes:

- bounded `SameTextPreserve`
- bounded `Regroup`
- bounded `OneSidedInsert`
- bounded `MemberExit`
- bounded three-group split-backed attempts on mutation-entry, as
  normalization onto the admitted `Regroup` lane

The retained same-sheet named-range boundary is now narrower:

- direct authority replay of the split-backed three-group host shape
- direct lifecycle replay of the split-backed three-group host shape
- any true same-sheet multi-group collapse family that would require the far
  participant group to collapse into one after-group

## Next Logical Blocker

If the roadmap continues on the same-sheet surface, the next blocker is no
longer "does three-group collapse exist."

It is:

- can direct authority and lifecycle replay carry the exact live
  split-backed named-range host shape without verification rollback

If the roadmap reprioritizes by distinct-family expansion instead, the next
remaining candidate is the retained direct off-sheet gap-closing insertion
surface where live Calc merges the after-topology but still does not expose
stable mutation-family classification.
