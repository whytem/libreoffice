/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <algorithm>
#include <iostream>
#include <optional>
#include <vector>

#include <spreadsheetengine/detail/dependency/DependencySnapshot.hxx>
#include <spreadsheetengine/detail/dependency/InvalidationPlanner.hxx>
#include <spreadsheetengine/detail/workbook/InMemoryWorkbookFacade.hxx>

#include "TestSupport.hxx"

namespace
{

using spreadsheetengine::api::CellAddress;
using spreadsheetengine::api::CellValue;
using spreadsheetengine::detail::dependency::DependencyNodeKind;
using spreadsheetengine::detail::dependency::DirtyReason;
using spreadsheetengine::detail::dependency::DependencyNodeId;
using spreadsheetengine::detail::dependency::RebuildScopeKind;
using spreadsheetengine::detail::facade::FormulaCellKind;
using spreadsheetengine::detail::facade::InMemoryWorkbookFacade;
using spreadsheetengine::detail::facade::MutationEvent;
using spreadsheetengine::standalone::test::fail;

[[nodiscard]] bool containsAddress(
    const std::vector<spreadsheetengine::detail::dependency::DirtyFormulaCell>& rEntries,
    const CellAddress& rAddress)
{
    return std::any_of(rEntries.begin(), rEntries.end(),
        [&rAddress](const auto& rEntry) { return rEntry.maAddress == rAddress; });
}

[[nodiscard]] bool containsNodeId(
    const std::vector<DependencyNodeId>& rEntries, DependencyNodeId aNodeId)
{
    return std::find(rEntries.begin(), rEntries.end(), aNodeId) != rEntries.end();
}

} // namespace

int main()
{
    using namespace spreadsheetengine::detail::dependency;

    InMemoryWorkbookFacade aFacade;
    const auto nSheet = aFacade.addSheet(u"Sheet1");

    aFacade.setCell({ nSheet, 0, 0 }, CellValue::number(10.0)); // A1
    aFacade.setCell({ nSheet, 0, 1 }, CellValue::number(20.0)); // A2
    aFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1", CellValue::number(10.0)); // B1
    aFacade.setFormulaCell({ nSheet, 2, 0 }, u"=B1", CellValue::number(10.0)); // C1
    aFacade.setFormulaCell({ nSheet, 3, 0 }, u"=SUM(Metrics)", CellValue::number(30.0)); // D1
    aFacade.setFormulaCell({ nSheet, 4, 0 }, u"=A1", CellValue::number(10.0),
        FormulaCellKind::SharedGroupMember);
    aFacade.setFormulaCell({ nSheet, 4, 1 }, u"=A2", CellValue::number(20.0),
        FormulaCellKind::SharedGroupMember);
    aFacade.addFormulaGroup({ nSheet, 4, 0 }, 2, true);
    aFacade.addNamedRange(u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$A$1:$A$2");

    const auto aSnapshot = buildDependencySnapshot(aFacade);

    if (aSnapshot.maReport.mnFormulaNodeCount != 5)
        return fail("dependency_snapshot", "formula node count mismatch");
    if (aSnapshot.maReport.mnNamedRangeNodeCount != 1)
        return fail("dependency_snapshot", "named range node count mismatch");
    if (aSnapshot.maReport.mnReverseDependencyEdgeCount != 2)
        return fail("dependency_snapshot", "reverse dependency count mismatch");

    const auto oNameNode = aSnapshot.findNamedRangeNode({ 0, std::nullopt });
    if (!oNameNode)
        return fail("dependency_snapshot", "named range node lookup failed");

    const auto oB1Node = aSnapshot.findFormulaCellNode({ nSheet, 1, 0 });
    const auto oD1Node = aSnapshot.findFormulaCellNode({ nSheet, 3, 0 });
    const auto oE2Node = aSnapshot.findFormulaCellNode({ nSheet, 4, 1 });
    if (!oB1Node || !oD1Node || !oE2Node)
        return fail("dependency_snapshot", "formula node lookup failed");

    const auto& rB1Deps = aSnapshot.getDependencies(*oB1Node);
    if (rB1Deps.size() != 1 || rB1Deps.front().maSource.meKind != DependencySourceKind::Cell
        || !(rB1Deps.front().maSource.maCellAddress == CellAddress { nSheet, 0, 0 }))
    {
        return fail("dependency_snapshot", "B1 dependency mismatch");
    }

    const auto& rD1Deps = aSnapshot.getDependencies(*oD1Node);
    if (rD1Deps.size() != 1 || rD1Deps.front().maSource.meKind != DependencySourceKind::NamedRange
        || !(rD1Deps.front().maSource.maNamedRangeId == spreadsheetengine::detail::facade::NamedRangeId { 0, std::nullopt }))
    {
        return fail("dependency_snapshot", "D1 named range dependency mismatch");
    }

    const auto& rB1Reverse = aSnapshot.getReverseDependents(*oB1Node);
    if (!containsNodeId(rB1Reverse, *aSnapshot.findFormulaCellNode({ nSheet, 2, 0 })))
        return fail("dependency_snapshot", "B1 reverse dependency mismatch");

    const auto& rNameReverse = aSnapshot.getReverseDependents(*oNameNode);
    if (!containsNodeId(rNameReverse, *oD1Node))
        return fail("dependency_snapshot", "named range reverse dependency mismatch");

    const auto* pE2Node = aSnapshot.getNode(*oE2Node);
    if (!pE2Node || !pE2Node->moSharedGroupAnchor
        || !(*pE2Node->moSharedGroupAnchor == CellAddress { nSheet, 4, 0 })
        || pE2Node->mnSharedGroupLength != 2 || !pE2Node->mbShareableGroup)
    {
        return fail("dependency_snapshot", "shared group normalization mismatch");
    }

    const auto aValuePlan = planInvalidation(
        aSnapshot, MutationEvent::setScalarValue({ nSheet, 0, 0 }));
    if (!containsAddress(aValuePlan.maDirtyFormulaCells, { nSheet, 1, 0 })
        || !containsAddress(aValuePlan.maDirtyFormulaCells, { nSheet, 2, 0 })
        || !containsAddress(aValuePlan.maDirtyFormulaCells, { nSheet, 3, 0 })
        || !containsAddress(aValuePlan.maDirtyFormulaCells, { nSheet, 4, 0 }))
    {
        return fail("dependency_invalidation", "value change invalidation mismatch");
    }
    if (containsAddress(aValuePlan.maDirtyFormulaCells, { nSheet, 4, 1 }))
        return fail("dependency_invalidation", "unexpected dirty shared member");

    const auto aFormulaPlan = planInvalidation(
        aSnapshot, MutationEvent::setFormula({ nSheet, 1, 0 }, u"=A2"));
    if (!containsAddress(aFormulaPlan.maDirtyFormulaCells, { nSheet, 1, 0 })
        || !containsAddress(aFormulaPlan.maDirtyFormulaCells, { nSheet, 2, 0 }))
    {
        return fail("dependency_invalidation", "formula change invalidation mismatch");
    }
    if (aFormulaPlan.maRebuildScopes.empty()
        || aFormulaPlan.maRebuildScopes.front().meKind != RebuildScopeKind::FormulaCell)
    {
        return fail("dependency_invalidation", "formula change rebuild scope mismatch");
    }

    spreadsheetengine::detail::facade::NamedRangeDescriptor aBefore;
    aBefore.maId = { 0, std::nullopt };
    aBefore.maName = u"Metrics";
    aBefore.maBaseAddress = { nSheet, 0, 0 };
    aBefore.maTargetExpression = u"$A$1:$A$2";
    spreadsheetengine::detail::facade::NamedRangeDescriptor aAfter = aBefore;
    aAfter.maName = u"Metrics2";

    const auto aRenameNamePlan = planInvalidation(
        aSnapshot, MutationEvent::renameNamedRange(aBefore, aAfter));
    if (!containsAddress(aRenameNamePlan.maDirtyFormulaCells, { nSheet, 3, 0 })
        || aRenameNamePlan.maRebuildScopes.empty()
        || aRenameNamePlan.maRebuildScopes.front().meKind != RebuildScopeKind::NamedRanges)
    {
        return fail("dependency_invalidation", "named range rename invalidation mismatch");
    }

    const auto aStructuralPlan
        = planInvalidation(aSnapshot, MutationEvent::insertRows(nSheet, 2, 1));
    if (!aStructuralPlan.mbRequiresSnapshotRebuild
        || aStructuralPlan.maRebuildScopes.empty()
        || aStructuralPlan.maRebuildScopes.front().meKind != RebuildScopeKind::Workbook)
    {
        return fail("dependency_invalidation", "structural rebuild mismatch");
    }
    if (aStructuralPlan.maDirtyFormulaCells.size() != 5)
        return fail("dependency_invalidation", "structural dirty set mismatch");

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
