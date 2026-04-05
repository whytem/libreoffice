# Computational Substrate Final Rollback Scenario Matrix

Status: frozen final-rollback scenario matrix

## Purpose

This note freezes the representative admitted-slice rollback scenarios for
[COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md).

The matrix exists to keep the proof cycle focused on the rollback behavior
that sits immediately around already-engine-owned resident and realized
state:

- rollback of admitted scalar mutation entry
- rollback of admitted formula replace and remove realization
- rollback of admitted narrow structural mutations
- rollback of admitted listener, broadcaster, formula-tree, and
  formula-track participation

It does not widen the workbook slice. It only classifies which admitted
rollback cases can count toward settled authority, which remain
validation-only evidence, and which must still be rejected or deferred.

## Scenario Classes

### Candidate For Settled Rollback Authority

The following admitted cases are candidates for settled engine-authored
rollback if they restore exactly:

- rollback after failed scalar overwrite verification on an ordinary scalar
  precedent chain
- rollback after failed formula replace at a stable admitted address
- rollback after failed `ClearCell` removal of an admitted ordinary scalar
  formula cell
- rollback after admitted `InsertRows` and `DeleteRows` divergence where
  ordinary scalar formula cells move but stay inside the admitted slice
- rollback after admitted `InsertColumns` and `DeleteColumns` divergence
  where ordinary scalar formula cells move but stay inside the admitted
  slice
- rollback of direct listener cases where one broadcaster node feeds one or
  more admitted formula-cell listeners
- rollback of transitive invalidation chains that remain inside the admitted
  scalar workbook slice

### Validation-Only Evidence

The following cases may be exercised as evidence but do not, by themselves,
justify a proceed decision:

- ordering-only rollback restore differences that still preserve exact
  computational and graph closure
- bounded rollback flows where live Calc execution must temporarily recreate
  an admitted `ScFormulaCell` before returning to the exact admitted
  baseline
- bounded rollback flows where duplicate or empty broadcasters appear during
  restore but the final restored state canonicalizes exactly
- explicit reject or repair-triggering induced divergence used only to prove
  the retained host shell still rolls back cleanly

### Explicit Reject Or Defer

The following cases remain outside the promotion surface for this cycle:

- shared-group-sensitive rollback
- named-range-sensitive rollback
- sheet-local or scope-ambiguous named-range consumers
- off-sheet structural consumers outside the admitted slice
- sheet insert, delete, rename, or move
- copy, move, clipboard, import, load-time, or undo-like flows
- any rollback path that requires Calc-only hidden state not representable
  by an engine-authored rollback record

## Representative Matrix

| Scenario | Mutation Class | Rollback Focus | Classification |
| --- | --- | --- | --- |
| Failed overwrite of a scalar precedent feeding one ordinary formula | `SetScalarValue` | exact restore of value, listener set, and formula-tree state | candidate |
| Failed overwrite of a scalar precedent feeding a transitive chain | `SetScalarValue` | queue, graph, and broadcaster restore | candidate |
| Failed ordinary scalar formula replacement | `SetFormula` | formula-cell lifetime restore at stable address | candidate |
| Failed clear of an ordinary scalar formula cell | `ClearCell` | formula-cell recreate plus wiring restore | candidate |
| Failed row insertion through an admitted scalar formula slice | `InsertRows` | moved formula-cell and listener restore | candidate |
| Failed row deletion through an admitted scalar formula slice | `DeleteRows` | removed and shifted formula-cell restore | candidate |
| Failed column insertion through an admitted scalar formula slice | `InsertColumns` | moved formula-cell and broadcaster restore | candidate |
| Failed column deletion through an admitted scalar formula slice | `DeleteColumns` | removed and shifted formula-cell restore | candidate |
| Queue ordering drift with otherwise exact restored state | any admitted class | rollback ordering diagnosis | validation-only |
| Duplicate or empty broadcaster materialization during restore | any admitted class | rollback canonicalization diagnosis | validation-only |
| Induced rollback failure proving host repair response | any admitted class | rollback observation and defer path | validation-only |
| Shared-group rollback restore | any | shared-group host identity | defer |
| Named-range-sensitive rollback restore | any | named-range-sensitive dependency shape | defer |
| Off-sheet or sheet-wide structural rollback | structural | widened workbook-scope host behavior | defer |

## Success Interpretation

The matrix should be read under these rules:

- candidate cases may support a proceed decision only if they restore with
  exact queue, computational, and graph results plus explicit engine-authored
  rollback records
- validation-only cases may support diagnosis and evidence, but they do not
  independently justify widening the settled rollback boundary
- reject or defer cases must stay out of the rollback path for this cycle

## Guardrails

This matrix is violated if implementation work:

- silently widens beyond the admitted mutation classes
- treats shared-group or named-range-sensitive cases as part of the same
  rollback proof surface
- accepts non-exact queue, computational, or graph outcomes as success
- reclassifies a defer case as a candidate without a new explicit plan
