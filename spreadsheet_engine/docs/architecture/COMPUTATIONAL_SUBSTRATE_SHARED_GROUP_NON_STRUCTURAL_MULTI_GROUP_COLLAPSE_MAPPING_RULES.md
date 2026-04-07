# Computational Substrate Shared-Group Non-Structural Multi-Group Collapse Mapping Rules

Status: frozen mapping note for the exact multi-group collapse closeout

## Participant Mapping

For the bounded synthetic family, the engine identifies:

1. one touched middle shareable group `M`
2. one contiguous shareable group immediately above `M`
3. one contiguous shareable group immediately below `M`

The exact synthetic rebuild window is:

- `start = above.anchor.row`
- `end = below.anchor.row + below.length - 1`

## Exact Synthetic After-State Rule

The synthetic family closes only when the observed-after group:

- anchors at the upper participant anchor
- ends at the lower participant tail
- covers the full span of all three participant groups
- includes the touched address

## Live Promotion Rule

Promotion to the admitted slice required the live host to expose the same
full-span one-group after-topology.

That condition did not close.

## Defer Rule

If the live host after-state:

- leaves the far participant group separate
- produces more than one after-group in the participant span
- or otherwise diverges from the full-span one-group topology

then the family remains deferred even if the synthetic authored model is
exact in standalone proof.
