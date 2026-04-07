# Computational Substrate Formula-Group Listener-Anchor Live Support Mapping Rules

Status: frozen mapping note for the FormulaGroup listener-anchor live-support closeout

## Listener-Anchor Identity

`FormulaGroup` listener-anchor identity remains:

- kind = `FormulaGroup`
- anchor address = shared-top formula-cell address
- length = shared-group length

`FormulaCell` listener-anchor identity remains unchanged.

## Live Resolution Rule

Live realization now resolves a `FormulaGroup` anchor by:

- locating the shared-top formula cell at the anchor address
- requiring that the cell is still an admitted non-matrix formula cell
- requiring that the cell is still the shared top
- requiring that the live shared length matches the stored length
- resolving the live shared-top block pointer from the sheet column store

That resolved shared-top block pointer is then replayed through Calc's own
group-listening helper instead of degrading the anchor into individual cell
listeners.

## Replay Rule

For a resolved `FormulaGroup` anchor, live wiring now:

- clears existing admitted live wiring
- replays the group listener through shared-group listening
- skips redundant per-cell listener-edge application for formula cells that
  belong to the replayed group
- continues to apply ordinary listener edges explicitly

## Exactness Rule

The comparison standard is unchanged:

- broadcaster identity must still match exactly for an exact result
- queue, computational, graph, and final verification still decide whether
  the surface is exact
- listener-kind mismatch is not ignored for admission

## Reject Rule

`HostUnknown` remains a hard reject:

- live store validation rejects it
- live wiring realization rejects it
- the reason remains `listener_anchor_out_of_contract`
