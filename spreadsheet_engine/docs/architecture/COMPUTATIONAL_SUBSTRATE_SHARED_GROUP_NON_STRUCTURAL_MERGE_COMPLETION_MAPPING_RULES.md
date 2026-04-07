# Computational Substrate Shared-Group Non-Structural Merge Completion Mapping Rules

Status: frozen mapping note for the same-sheet merge-completion closeout

## Participant Mapping

The admitted one-sided insert family maps:

- one blank touched address `T`
- one adjacent prior same-column shareable group `A`
- one exact after-group `M`

The participating prior group must be:

- immediately below `T` for upward insertion
- immediately above `T` for downward insertion

No second prior shared group may participate in the admitted window.

## Admitted Rebuild Window

The engine-authored rebuild window is:

- upward one-sided insert: from `T.row` through `A.end`
- downward one-sided insert: from `A.anchor` through `T.row`

## Exact After-Group Matching

The observed-after touched group is admissible only when it matches the
bounded authored shape:

- upward one-sided insert:
  - after-group anchor must equal `T`
  - after-group end must equal `A.end`
  - after-group length must equal `A.length + 1`
- downward one-sided insert:
  - after-group anchor must equal `A.anchor`
  - after-group end must equal `T`
  - after-group length must equal `A.length + 1`

This exact match forbids broader multi-participant collapse from riding the
one-sided insert lane.

## Rebuild Rule

Inside the admitted rebuild window the engine must:

- create the inserted formula cell in the predicted shadow
- clear existing shared bindings inside the rebuild window
- lower formulas by address
- repartition contiguous compatible runs
- materialize only exact rebuilt shared runs

Final admission still requires exact predicted-versus-observed topology
match across addresses and shared-group identity.

## Forbidden Mappings

The mapping rules do not allow:

- borrowing host-observed after-topology as authority
- absorbing a second prior shared group through the one-sided insert lane
- widening into named-range, repair-sensitive, or off-sheet behavior
