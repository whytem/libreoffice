# Computational Substrate Shared-Group Named-Range Member-Exit Scenario Matrix

Status: frozen scenario matrix for bounded named-range-combined scalar member-exit admission

## Admit Buckets

- standalone authority exactness for bounded `SetScalarValue`
  `MemberExit`
- live authority apply for the same bounded scalar family
- live mutation-entry apply for the same bounded scalar family

## Retained Reject Buckets

- live lifecycle reject for named-range-combined `SetFormula`
  `MemberExit`
- live authority reject for named-range-combined `ClearCell`
  `MemberExit`
- live mutation-entry reject for the same non-scalar member-exit families
- retained broader named-range-combined regroup, merge, collapse,
  repair-sensitive, and off-sheet reject buckets from prior cycles
