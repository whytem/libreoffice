# Computational Substrate Shared-Group Non-Structural Regroup Scenario Matrix

Status: frozen scenario matrix for the exact regroup closeout

| Scenario | Shape | Outcome |
| --- | --- | --- |
| `sg_regroup_anchor_upward` | touched anchor formula changes and regroups with a contiguous ordinary formula run above | `admit` |
| `sg_regroup_tail_downward` | touched tail formula changes and regroups with a contiguous ordinary formula run below | `admit` |
| `sg_regroup_anchor_with_residual_tail_run` | anchor regroup changes the touched-group identity while the untouched tail remains an exact residual shared run | `admit` |
| `sg_regroup_tail_with_residual_head_run` | tail regroup changes the touched-group identity while the untouched head remains an exact residual shared run | `admit` |
| `sg_regroup_same_text_preserve` | touched cell stays in the same group identity with identical formula text | `carried_forward_admit` |
| `sg_regroup_member_exit` | touched cell leaves the group entirely | `carried_forward_admit` |
| `sg_regroup_interior` | touched interior member would need host regrouping beyond the bounded edge window | `defer` |
| `sg_regroup_merge_prior_group` | changed formula would absorb another prior shared group | `reject` |
| `sg_regroup_blank_insert_gap` | blank cell insertion adjacent to a shared group | `reject` |
| `sg_regroup_named_range_combined` | regroup outcome depends on named-range drift | `reject` |
| `sg_regroup_off_sheet_consumer` | regroup candidate widens into off-sheet dependency closure | `defer` |

## Matrix Notes

- The admitted regroup family is intentionally edge-only.
- Residual untouched members may remain shared if the bounded rebuild window
  can author them as exact contiguous runs.
- Merge, gap insert, named-range-combined, repair-sensitive, and off-sheet
  classes do not inherit admission from the edge-regroup proof.

