# Computational Substrate Phase 1 Structural Widening Evidence

Status: active Phase 1 structural evidence artifact

## Question

Can the Phase 1 computational shadow survive representative structural edits
without depending on Calc pointer identity or slot-layout ownership?

## Evidence Admitted In Phase 1

Phase 1 now admits two representative rebuild-based structural cases:

- single row insert
- single column delete

The validation lane is
[ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
in `testComputationalShadowRepresentativeStructuralWidening`.

## Why These Cases Are Enough For Phase 1

The Phase 1 shadow is still non-authoritative and rebuild-based.

For this phase, the structural question is not performance. It is whether the
normalized shadow model can survive address-shifting mutations while remaining:

- pointer-free
- descriptor-based
- reconstructible from the facade plus normalized live observation capture

The admitted row and column cases demonstrate that the current shadow can do
that on representative structural edits.

## What Remains Deferred

This evidence does not yet admit:

- broad structural authority
- copy/move or clipboard rebuild paths
- full row/column structural breadth
- BASM slot-layout fidelity
- token-container migration

Those remain later-phase questions even though the representative Phase 1
widening gate is now passed for rebuild-based shadowing.
