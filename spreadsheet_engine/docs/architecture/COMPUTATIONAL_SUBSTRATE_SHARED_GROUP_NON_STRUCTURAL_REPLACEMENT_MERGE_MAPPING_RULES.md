# Computational Substrate Shared-Group Non-Structural Replacement-Merge Mapping Rules

Status: frozen mapping note for the exact replacement-merge closeout

## Participant Mapping

The admitted replacement-merge family maps:

- one touched pre-group `T`
- one adjacent prior same-column shareable group `A`
- one exact after-group `M` that includes the touched address

The touched address must be:

- the anchor of `T` when `A` is above
- the tail of `T` when `A` is below

## Admitted Rebuild Window

The engine-authored rebuild window is:

- upward replacement merge: from `A.anchor` through `T.end`
- downward replacement merge: from `T.anchor` through `A.end`

No additional prior shared groups may participate in that window.

## Exact After-Group Matching

The observed-after touched group is admissible only when it matches the
bounded authored shape:

- upward replacement merge:
  - after-group anchor must equal `A.anchor`
  - after-group end must be at or below the touched address and at or above
    `T.end`
- downward replacement merge:
  - after-group end must equal `A.end`
  - after-group anchor must be at or above the touched address and at or
    below `T.anchor`

These bounds allow the untouched remainder of `T` to stay outside `M`.

## Rebuild Rule

Inside the admitted rebuild window the engine must:

- mutate the touched cell formula in the predicted shadow
- clear existing shared bindings inside the rebuild window
- lower formulas by address
- repartition contiguous compatible runs
- materialize only exact rebuilt shared runs

Final admission still requires exact predicted-versus-observed topology
match across addresses and shared-group identity.

## Forbidden Mappings

The mapping rules do not allow:

- borrowing host-observed after-topology as authority
- absorbing both sides of the touched group in one admission step
- widening into named-range, repair-sensitive, or off-sheet behavior
