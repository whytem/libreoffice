# Computational Substrate Shared-Group Non-Structural Scenario Matrix

Status: frozen scenario matrix for the non-structural shared-group admission
cycle

## Classification Legend

- `candidate`: intended live-admission candidate for this cycle
- `reject`: stays out of the candidate path and should reject deterministically
- `defer`: remains outside this cycle and must not be inferred from candidate
  success

## Representative Cases

| Scenario ID | Mutation | Before Shape | Expected Outcome | Classification |
| --- | --- | --- | --- | --- |
| `sg_ns_set_value_tail_collapse` | `SetScalarValue` on tail member | one 2-row shareable group | no surviving shared groups | `candidate` |
| `sg_ns_set_value_anchor_rebuild` | `SetScalarValue` on anchor | one 3-row shareable group | surviving 2-row rebuilt group | `candidate` |
| `sg_ns_set_value_middle_split` | `SetScalarValue` on interior member | one 3-row shareable group | surviving formulas split into singletons | `candidate` |
| `sg_ns_clear_tail_collapse` | `ClearCell` on tail member | one 2-row shareable group | no surviving shared groups | `candidate` |
| `sg_ns_clear_anchor_rebuild` | `ClearCell` on anchor | one 3-row shareable group | surviving 2-row rebuilt group | `candidate` |
| `sg_ns_clear_middle_split` | `ClearCell` on interior member | one 3-row shareable group | surviving formulas split into singletons | `candidate` |
| `sg_ns_set_formula_anchor_rebuild` | `SetFormula` on anchor with different formula text | one 3-row shareable group | edited cell ordinary, lower 2-row rebuilt group | `candidate` |
| `sg_ns_set_formula_middle_split` | `SetFormula` on interior member with different formula text | one 3-row shareable group | edited cell ordinary, surviving formulas split into singletons | `candidate` |
| `sg_ns_set_formula_tail_collapse` | `SetFormula` on tail member with different formula text | one 2-row shareable group | edited cell ordinary, no surviving shared groups | `candidate` |
| `sg_ns_set_formula_same_text_preserve` | `SetFormula` on member with same formula text | one shareable group | host may preserve shared grouping | `reject` |
| `sg_ns_named_range_combined` | any candidate mutation on shared-group formula using named ranges | shared-group plus named-range-sensitive formulas | outside this cycle | `reject` |
| `sg_ns_non_shareable_group` | any candidate mutation on non-shareable group | non-shareable group | outside this cycle | `reject` |
| `sg_ns_multi_group_regroup` | any candidate mutation whose exact answer depends on regroup/merge across prior groups | multiple interacting groups | broader regroup problem | `defer` |
| `sg_ns_off_sheet_group` | any candidate mutation where relevant shared-group behavior crosses sheets | off-sheet dependency in shared-group topology | outside bounded slice | `defer` |
| `sg_ns_repair_sensitive_divergence` | any candidate mutation where host after-state requires extra repair not represented in the shadow | perturbed host-only repair | rollback or defer only | `defer` |

## Minimum Proof Coverage

The cycle is not complete unless checked-in tests cover at least:

- one `SetScalarValue` candidate that collapses a 2-row group
- one `SetScalarValue` candidate that rebuilds a 3-row group
- one `ClearCell` candidate that splits a 3-row group
- one `SetFormula` candidate that rebuilds or splits a 3-row group while
  leaving the edited cell ordinary
- one gate-off or same-text `SetFormula` rejection case
- one mutation-entry live-apply proof on an authority-path candidate
- one mutation-entry live-apply proof on a lifecycle-path candidate
- one rollback-capable or reject proof for an out-of-scope shared-group case

## Promotion Boundary

Success on the `candidate` rows does not justify widening into:

- preserve-only same-text replacement
- merge or regroup across prior groups
- named-range-combined shared-group behavior
- off-sheet or broader workbook classes
