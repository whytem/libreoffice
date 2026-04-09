# Computational Substrate Object Realization Scenario Matrix

Status: frozen object-realization scenario matrix

## Purpose

This note freezes the representative admitted-slice scenarios for
[COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md).

The matrix exists to keep the proof cycle focused on live realization
behavior that sits immediately above already-engine-owned resident state:

- formula-cell create, replace, and remove realization
- listener and broadcaster realization from resident wiring
- formula-tree and formula-track live participation

It does not widen the workbook slice. It only classifies which admitted
realization cases can count toward settled authority, which remain
validation-only evidence, and which must still be rejected or deferred.

## Scenario Classes

### Candidate For Settled Object-Realization Authority

The following admitted cases are candidates for settled engine-authored
object realization if they close exactly:

- scalar overwrite that preserves the same admitted formula population and
  only changes invalidation wiring
- formula replace on an ordinary scalar formula cell where the admitted
  formula address stays stable
- clear-cell removal of an ordinary scalar formula cell with exact listener,
  tree, and track cleanup
- admitted `InsertRows` and `DeleteRows` cases where ordinary scalar formula
  cells move but stay inside the already-admitted structural slice
- admitted `InsertColumns` and `DeleteColumns` cases where ordinary scalar
  formula cells move but stay inside the already-admitted structural slice
- direct listener cases where one broadcaster node feeds one or more admitted
  formula-cell listeners
- transitive invalidation chains that remain inside the admitted scalar
  workbook slice

### Validation-Only Evidence

The following cases may be exercised as evidence but do not, by themselves,
justify settled object-realization widening:

- ordering-only differences in formula-tree or formula-track realization that
  still preserve exact queue, computational, and graph comparison
- bounded object-recreate flows where live Calc realization must tear down
  and recreate an admitted `ScFormulaCell` but ends in the same exact
  admitted state
- bounded cleanup flows where the resident wiring store is exact but the live
  host layer briefly materializes duplicate or empty broadcasters before the
  final exact state is restored
- explicit rollback-triggering induced divergence used to prove the retained
  rollback contract

### Explicit Reject Or Defer

The following cases remain outside the promotion surface for this cycle:

- shared-group-sensitive formula realization
- named-range-sensitive realization
- sheet-local or scope-ambiguous named-range consumers
- off-sheet structural consumers outside the admitted slice
- sheet insert, delete, rename, or move
- copy, move, clipboard, import, load-time, or undo-like flows
- any case that requires Calc-only host identity not representable by an
  engine-authored realization record

## Representative Matrix

| Scenario | Mutation Class | Realization Focus | Classification |
| --- | --- | --- | --- |
| Overwrite scalar precedent feeding one ordinary formula | `SetScalarValue` | listener and broadcaster reuse, exact formula-tree ordering | candidate |
| Overwrite scalar precedent feeding a transitive chain | `SetScalarValue` | listener propagation plus exact queue/graph closure | candidate |
| Replace an ordinary scalar formula in place | `SetFormula` | formula-cell replace realization at stable address | candidate |
| Clear an ordinary scalar formula cell | `ClearCell` | formula-cell remove realization plus wiring cleanup | candidate |
| Insert rows through an admitted scalar formula slice | `InsertRows` | create and move realized formula cells plus tree/track update | candidate |
| Delete rows through an admitted scalar formula slice | `DeleteRows` | remove and move realized formula cells plus tree/track update | candidate |
| Insert columns through an admitted scalar formula slice | `InsertColumns` | create and move realized formula cells plus wiring update | candidate |
| Delete columns through an admitted scalar formula slice | `DeleteColumns` | remove and move realized formula cells plus wiring update | candidate |
| Ordering-only tree or track drift with otherwise exact state | any admitted class | queue and graph order classification | validation-only |
| Duplicate or empty broadcaster materialization that normalizes away | any admitted class | broadcaster canonicalization diagnosis | validation-only |
| Induced divergence to prove rollback remains available | any admitted class | rollback and repair-detected host response | validation-only |
| Shared-group formula object realization | any | shared-group host identity | defer |
| Named-range-sensitive realization path | any | named-range object and dependency shape | defer |
| Off-sheet or sheet-wide structural realization | structural | widened workbook-scope host behavior | defer |

## Success Interpretation

The matrix should be read under these rules:

- candidate cases may support a proceed decision only if they close with
  exact queue, computational, and graph results plus explicit engine-authored
  realization records
- validation-only cases may support diagnosis and evidence, but they do not
  independently justify widening the settled live boundary
- reject or defer cases must stay out of the realization path for this cycle

## Guardrails

This matrix is violated if implementation work:

- silently widens beyond the admitted mutation classes
- treats shared-group or named-range-sensitive cases as part of the same
  proof surface
- accepts non-exact queue, computational, or graph outcomes as success
- reclassifies a defer case as a candidate without a new explicit plan
