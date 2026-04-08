# Computational Substrate Blocker Scenario Matrix

Status: frozen scenario matrix for the four-blocker clearance program

## Purpose

This matrix records the main scenario families that the blocker-clearance
program must evaluate.

## Same-Sheet Shared-Group Authoring

| Scenario | Family | Current state | Target outcome |
| --- | --- | --- | --- |
| `sg_nr_regroup_same_sheet` | named-range-combined regroup | live attempt normalized to preserve; deferred as distinct family | admit only if real live family closes exactly |
| `sg_nr_merge_same_sheet` | named-range-combined merge | live attempt normalized to preserve; deferred as distinct family | admit only if real live family closes exactly |
| `sg_nr_replacement_merge_same_sheet` | named-range-combined replacement merge | live attempt normalized to preserve; deferred as distinct family | admit only if real live family closes exactly |
| `sg_nr_one_sided_insert_same_sheet` | named-range-combined one-sided insert | live attempt normalized to preserve; deferred as distinct family | admit only if real live family closes exactly |
| `sg_nr_multi_group_collapse_same_sheet` | named-range-combined multi-group collapse | deferred | admit only if live Calc actually collapses |
| `sg_non_edge_regroup_same_sheet` | broader non-edge regroup | deferred | admit if exact |
| `sg_non_edge_merge_same_sheet` | broader non-edge merge | deferred | admit if exact |

## Repair-Sensitive Normalization

| Scenario | Family | Current state | Target outcome |
| --- | --- | --- | --- |
| `repair_structural_named_range` | structural repair-sensitive named-range | explicit `RepairDetected` rollback | keep explicit rollback unless exact normalization closes |
| `repair_shared_group_regroup` | regroup with host cleanup sensitivity | explicit reject-by-rule | admit only if exact authored after-state closes |
| `repair_shared_group_merge` | merge with host cleanup sensitivity | explicit reject-by-rule | admit only if exact authored after-state closes |
| `repair_listener_broadcaster` | listener or broadcaster cleanup drift | retained host-only cleanup diagnostic | normalize only if canonical engine-owned rule closes |

## Off-Sheet Dependency Closure

| Scenario | Family | Current state | Target outcome |
| --- | --- | --- | --- |
| `sg_nr_preserve_off_sheet` | named-range-combined off-sheet preserve | explicit deferred boundary | admit bounded slice only with exact cross-sheet closure |
| `sg_nr_member_exit_off_sheet` | named-range-combined off-sheet member exit | explicit deferred boundary | admit bounded slice only with exact cross-sheet closure |
| `sg_off_sheet_direct_consumer` | off-sheet direct shared-group consumer | authority reject-by-rule | admit bounded slice only with exact cross-sheet closure |
| `sg_off_sheet_multi_sheet_spill` | broader workbook-wide spill | explicitly fenced | keep fenced |

## Host-Shell Boundary

| Scenario | Family | Current state | Target outcome |
| --- | --- | --- | --- |
| `host_ir_expected_after` | expected IR rebuilt through host shell | mixed | canonical engine-authored expected IR |
| `host_live_apply_records` | live-apply records versus host state | mixed | canonical engine-authored record set |
| `host_final_verification_source` | final verification source of truth | mixed | engine-authored on admitted slice |
| `host_opaque_dependency_reject` | generic host-surface rejection | generic reject | explicit retained-host rule |

## Standing Fences

These remain fenced unless a later exact proof explicitly closes them:

- workbook-wide authority transfer
- broad default-on rollout
- broad document-core ownership transfer
- UI, UNO, rendering, persistence, and shell migration
