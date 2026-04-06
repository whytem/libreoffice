# Computational Substrate Shared-Group Scenario Matrix

Status: frozen scenario matrix

## Purpose

This matrix defines the representative shared-group cases that the widening
cycle must exercise.

It is intentionally narrower than "all shared-formula behavior." The matrix
is scoped only to the mutation vocabulary and workbook classes frozen in
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_CONTRACT.md).

## Classification Legend

- `candidate`: may be eligible for bounded admission if the exact proof
  holds
- `validation-only`: must be exercised and classified, but is not presumed
  safe for live admission in this cycle
- `deferred`: immediately outside the widening surface for this cycle

## Shared-Group Lifecycle Matrix

| Id | Mutation | Shared-group shape | Expected outcome class | Initial class |
| --- | --- | --- | --- | --- |
| `sg_scalar_member_overwrite` | `SetScalarValue` | overwrite a shared-group member with a scalar value | explicit split or host repair signal | `validation-only` |
| `sg_scalar_anchor_overwrite` | `SetScalarValue` | overwrite a shared-group anchor with a scalar value | explicit split or host repair signal | `validation-only` |
| `sg_formula_member_replace_same_shape` | `SetFormula` | replace a member with a formula that preserves the same shared pattern | preserve or rebuild with exact identity | `candidate` |
| `sg_formula_member_replace_new_shape` | `SetFormula` | replace a member with a formula that breaks shared-group identity | explicit split or repair signal | `validation-only` |
| `sg_formula_anchor_replace_same_shape` | `SetFormula` | replace an anchor with a formula that preserves group identity | preserve or rebuild with exact identity | `candidate` |
| `sg_formula_anchor_replace_new_shape` | `SetFormula` | replace an anchor with a formula that changes group semantics | explicit split or repair signal | `validation-only` |
| `sg_clear_member` | `ClearCell` | clear a shared-group member | explicit split or repair signal | `validation-only` |
| `sg_clear_anchor` | `ClearCell` | clear a shared-group anchor | explicit split or repair signal | `validation-only` |
| `sg_insert_rows_preserve` | `InsertRows` | row insertion outside the group that preserves same-sheet group identity | preserved group with shifted addresses | `candidate` |
| `sg_insert_rows_split` | `InsertRows` | row insertion through or adjacent to the group causing split behavior | explicit split or repair signal | `validation-only` |
| `sg_delete_rows_preserve` | `DeleteRows` | row deletion outside the group that preserves same-sheet group identity | preserved group with shifted addresses | `candidate` |
| `sg_delete_rows_rebuild` | `DeleteRows` | row deletion that forces group rebuild but keeps a meaningful same-sheet group | rebuild with normalized identity or repair signal | `validation-only` |
| `sg_insert_columns_preserve` | `InsertColumns` | column insertion outside the group preserving identity | preserved group with shifted references | `candidate` |
| `sg_insert_columns_split` | `InsertColumns` | column insertion that breaks shared-group membership | explicit split or repair signal | `validation-only` |
| `sg_delete_columns_preserve` | `DeleteColumns` | column deletion outside the group preserving identity | preserved group with shifted references | `candidate` |
| `sg_delete_columns_rebuild` | `DeleteColumns` | column deletion that rebuilds or shrinks the group | rebuild with normalized identity or repair signal | `validation-only` |

## Immediate Defers

The following classes stay out of contract for this cycle and should be
reported as deferred without implying future admission:

| Id | Reason for defer |
| --- | --- |
| `sg_cross_sheet_consumers` | shared-group effects coupled to off-sheet consumers are outside the single-sheet widening surface |
| `sg_named_range_combined` | shared-group behavior combined with named-range-sensitive structure remains a separate deferred front |
| `sg_sheet_level_edits` | sheet insert, delete, rename, and move are outside the carried-forward mutation vocabulary |
| `sg_copy_move_undo_load` | copy, move, clipboard, load-time, and undo-like flows remain outside this cycle |
| `sg_shared_group_plus_host_only_repair` | cases whose decisive outcome depends on host-only repair not representable through the current seams must remain deferred |

## Minimum Proof Coverage

The runtime and evidence cycle must cover at least:

- one preserve case from `SetFormula`
- one split case from scalar or clear mutation
- one preserve case from row or column structure
- one rebuild or repair-sensitive structural case
- one explicit deferred case

If any of those proof buckets is missing, the cycle is incomplete even if
individual tests are green.
