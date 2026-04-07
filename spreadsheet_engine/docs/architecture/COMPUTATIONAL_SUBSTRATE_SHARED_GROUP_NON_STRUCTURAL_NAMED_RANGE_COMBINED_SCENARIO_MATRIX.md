# Computational Substrate Shared-Group Non-Structural Named-Range-Combined Scenario Matrix

Status: frozen scenario matrix for the bounded named-range-combined closeout

| Scenario | Shape | Expected result |
| --- | --- | --- |
| `sg_nr_preserve_single_area` | same-sheet shareable `SameTextPreserve` with one global single-area named range and same-sheet consumers only | candidate under test; deferred at closeout |
| `sg_nr_preserve_single_cell` | same-sheet shareable `SameTextPreserve` with one global single-cell named range | same bounded family; deferred at closeout |
| `sg_nr_member_exit` | same bounded named-range surface, but touched shared member exits the group | explicit defer and reject |
| `sg_nr_off_sheet_consumer` | bounded global single-area name but one relevant consumer lives on another sheet | explicit defer and reject |
| `sg_nr_sheet_local` | sheet-local named range | explicit defer and reject |
| `sg_nr_multi_area` | multi-area named range target | explicit defer and reject |
| `sg_nr_scope_ambiguous` | ambiguous local/global name resolution | explicit defer and reject |
| `sg_nr_descriptor_drift` | add, remove, rename, or retarget a participating name | explicit defer and reject |
| `sg_nr_regroup_merge` | regroup, merge, or multi-group shared-group topology change with named-range usage | explicit defer and reject |
| `sg_nr_repair_sensitive` | any repair-sensitive host normalization combined with named-range usage | explicit defer and reject |

## Closeout Result

This cycle admitted none of the named-range-combined scenarios.

The preserve candidate closed exactly in standalone proof but remained
deferred live, and the `sg_nr_member_exit` follow-on also remained deferred.
