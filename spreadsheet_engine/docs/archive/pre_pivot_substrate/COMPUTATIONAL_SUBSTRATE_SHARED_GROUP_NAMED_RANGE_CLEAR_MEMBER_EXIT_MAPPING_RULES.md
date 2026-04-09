# Computational Substrate Shared-Group Named-Range Clear Member-Exit Mapping Rules

Status: frozen mapping note for bounded named-range-combined `ClearCell`
member-exit closeout

## Exact Ownership Rule

For bounded named-range-combined `ClearCell` `MemberExit`, the planner must
author the post-clear named-range broadcaster surface exactly as live Calc
does.

That means:

- the surviving shared after-group keeps its shared-group identity
- direct ordinary named-range consumers keep their ordinary area-listener
  anchors
- the surviving shared after-group does not contribute a named-range
  area-listener anchor on this bounded lane

## Why This Rule Matters

The previous blocker was not live host cleanup. It was the planner adding a
named-range `FormulaGroup` area listener that live Calc does not expose
after the bounded clear.

## Exactness Consequence

The lane admits only when the predicted broadcaster, graph, and IR surfaces
all close on that bounded post-clear rule.
