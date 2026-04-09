# Computational Substrate Shared-Group Named-Range Live Ownership Mapping Rules

Status: frozen closeout mapping rules for bounded named-range-combined live ownership

## Dependency Snapshot Rules

- named-range target expressions are parsed through the normal formula parser
  first
- if that parse fails, bounded bare reference text is retried directly as:
  - one cell reference
  - one range reference
- if both paths fail, the target remains opaque and the slice is not
  admitted

## Listener-Anchor Rules

- formula nodes always retain their `FormulaCell` anchor identity
- shared-group common dependencies originating from named-range resolution
  project onto the owning `FormulaGroup` anchor
- member-local direct dependencies keep the `FormulaCell` anchor

## Exact Comparison Rules

- exact lifecycle requires zero dependency opacity plus exact queue,
  computational, graph, and IR verification
- exact mutation entry requires the same exact surface after live apply and
  final verification
- descriptor drift, off-sheet expansion, or repair-sensitive normalization
  remain out of contract
