# Computational Substrate Shared-Group Non-Structural Frontier Scenario Matrix

Status: frozen scenario matrix for the broader non-structural shared-group frontier closeout

## Purpose

This matrix records how the broader non-structural shared-group frontier was
resolved at closeout.

## Admitted Exact Family

| Family | Representative Shape | Expected Outcome |
| --- | --- | --- |
| Same-text preserve | `SetFormula` on an already-shared address with identical formula text and unchanged same-sheet shareable group identity | Exact admit |

Representative positions for this family:

- anchor member
- interior member
- tail member

The landed proof is on the representative already-shared same-sheet preserve
shape, and the admitted mapping rules are position-symmetric inside the
group-preserve window.

## Deferred Or Rejected Families

| Family | Representative Shape | Closeout Outcome |
| --- | --- | --- |
| Regroup | `SetFormula` changes text and the touched cell remains shared in a different run identity | Reject and defer |
| Merge | `SetFormula` inserted into a blank gap or replacement collapses adjacent runs into one group | Reject and defer |
| Named-range-combined | shared-group edit whose exact closure depends on named-range identity or retargeting | Reject and defer |
| Repair-sensitive normalization | host regrouping or cleanup changes shareability, anchor, or partitioning beyond the engine-authored rule family | Defer |
| Off-sheet | shared-group mutation whose exact closure depends on off-sheet consumers or broader workbook surface | Reject and defer |

## Representative Checked Cases

The closeout evidence records these concrete proof buckets:

- same-text preserve lifecycle exact
- same-text preserve mutation-entry exact
- regroup lifecycle rejected
- merge lifecycle rejected
- merge mutation-entry rejected
- named-range plus off-sheet authority rejected
- off-sheet consumer authority rejected

## Carried-Forward Baseline

The earlier admitted member-exit matrix remains valid and unchanged:

- `SetScalarValue` member exit
- `SetFormula` member exit
- `ClearCell` member exit

Those cases are not reclassified by this frontier cycle.
