# Computational Substrate Shared-Group Non-Structural Merge Scenario Matrix

Status: frozen scenario matrix for the exact merge closeout

| Scenario | Shape | Outcome |
| --- | --- | --- |
| `sg_merge_gap_two_group_short` | one blank gap row between two short same-column shareable groups | `admit` |
| `sg_merge_gap_two_group_long` | one blank gap row between longer same-column shareable groups | `admit` |
| `sg_merge_same_text_preserve` | already-shared touched member keeps the same group identity with identical text | `carried_forward_admit` |
| `sg_merge_edge_regroup` | already-shared edge member regroups with an ordinary run without absorbing a prior shared group | `carried_forward_admit` |
| `sg_merge_one_sided_insert` | blank-cell insertion adjacent to only one prior shareable group | `reject` |
| `sg_merge_replacement_driven` | shared-group formula replacement would absorb another prior shared group | `reject` |
| `sg_merge_multi_group_collapse` | more than two prior participants would collapse into one after-group | `reject` |
| `sg_merge_named_range_combined` | merge outcome depends on named-range drift | `reject` |
| `sg_merge_off_sheet_consumer` | merge candidate widens into off-sheet dependency closure | `defer` |

## Matrix Notes

- The admitted merge family is intentionally gap-closing only.
- The bounded family is two-before-to-one-after.
- One-sided insert, replacement-driven merge, multi-group collapse,
  named-range-combined, repair-sensitive, and off-sheet classes do not
  inherit admission from the gap-merge proof.
