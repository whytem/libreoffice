/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <algorithm>
#include <cstdlib>
#include <vector>

#include <spreadsheetengine/detail/dependency/DependencySnapshot.hxx>
#include <spreadsheetengine/detail/dependency/InvalidationPlanner.hxx>
#include <spreadsheetengine/detail/dependency/RecalcPlanner.hxx>
#include <spreadsheetengine/detail/workbook/InMemoryWorkbookFacade.hxx>

#include "TestSupport.hxx"

namespace
{

using spreadsheetengine::api::CellAddress;
using spreadsheetengine::api::CellValue;
using spreadsheetengine::detail::dependency::DirtyReason;
using spreadsheetengine::detail::dependency::RebuildScopeKind;
using spreadsheetengine::detail::dependency::RecalcGroupPolicy;
using spreadsheetengine::detail::dependency::RecalcSeedKind;
using spreadsheetengine::detail::facade::FormulaCellKind;
using spreadsheetengine::detail::facade::InMemoryWorkbookFacade;
using spreadsheetengine::detail::facade::MutationEvent;
using spreadsheetengine::standalone::test::fail;

[[nodiscard]] bool containsQueueAddress(
    const std::vector<spreadsheetengine::detail::dependency::RecalcQueueEntry>& rEntries,
    const CellAddress& rAddress)
{
    return std::any_of(rEntries.begin(), rEntries.end(),
        [&rAddress](const auto& rEntry) { return rEntry.maAddress == rAddress; });
}

[[nodiscard]] bool containsSeedAddress(
    const std::vector<spreadsheetengine::detail::dependency::RecalcSeed>& rSeeds,
    const CellAddress& rAddress, DirtyReason eReason)
{
    return std::any_of(rSeeds.begin(), rSeeds.end(),
        [&rAddress, eReason](const auto& rSeed) {
            return rSeed.meKind == RecalcSeedKind::FormulaCell && rSeed.moAddress
                   && *rSeed.moAddress == rAddress && rSeed.meReason == eReason;
        });
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
        FormulaCellKind::SharedGroupMember); // E1
    aFacade.setFormulaCell({ nSheet, 4, 1 }, u"=A2", CellValue::number(20.0),
        FormulaCellKind::SharedGroupMember); // E2
    aFacade.addFormulaGroup({ nSheet, 4, 0 }, 2, true);
    aFacade.addNamedRange(u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$A$1:$A$2");

    const auto aSnapshot = buildDependencySnapshot(aFacade);
    const auto aValueInvalidation
        = planInvalidation(aSnapshot, MutationEvent::setScalarValue({ nSheet, 0, 0 }));
    const auto aValuePlan = buildRecalcPlan(aSnapshot, aValueInvalidation);

    if (!containsSeedAddress(aValuePlan.maSeeds, { nSheet, 1, 0 }, DirtyReason::ScalarValueChanged)
        || !containsSeedAddress(aValuePlan.maSeeds, { nSheet, 4, 0 }, DirtyReason::ScalarValueChanged))
    {
        return fail("recalc_planner", "direct recalc seeds mismatch");
    }

    if (!std::any_of(aValuePlan.maSeeds.begin(), aValuePlan.maSeeds.end(),
            [](const RecalcSeed& rSeed) {
                return rSeed.meKind == RecalcSeedKind::NamedRange
                       && rSeed.meReason == DirtyReason::ScalarValueChanged;
            }))
    {
        return fail("recalc_planner", "named range seed missing");
    }

    if (aValuePlan.maQueue.size() != 4)
        return fail("recalc_planner", "queue size mismatch for scalar edit");

    if (!(aValuePlan.maQueue[0].maAddress == CellAddress { nSheet, 1, 0 })
        || !(aValuePlan.maQueue[1].maAddress == CellAddress { nSheet, 4, 0 })
        || !(aValuePlan.maQueue[2].maAddress == CellAddress { nSheet, 3, 0 })
        || !(aValuePlan.maQueue[3].maAddress == CellAddress { nSheet, 2, 0 }))
    {
        return fail("recalc_planner", "queue order mismatch for scalar edit");
    }

    if (aValuePlan.maQueue[3].mnDependencyDepth != 1)
        return fail("recalc_planner", "transitive dependency depth mismatch");

    const auto aGroupIt = std::find_if(aValuePlan.maQueue.begin(), aValuePlan.maQueue.end(),
        [nSheet](const RecalcQueueEntry& rEntry) {
            return rEntry.maAddress == CellAddress { nSheet, 4, 0 };
        });
    if (aGroupIt == aValuePlan.maQueue.end()
        || aGroupIt->meGroupPolicy != RecalcGroupPolicy::PreserveSharedGroup
        || !aGroupIt->moSharedGroupAnchor
        || !(*aGroupIt->moSharedGroupAnchor == CellAddress { nSheet, 4, 0 })
        || aGroupIt->mnSharedGroupLength != 2)
    {
        return fail("recalc_planner", "shared group metadata mismatch");
    }

    spreadsheetengine::detail::facade::NamedRangeDescriptor aBefore;
    aBefore.maId = { 0, std::nullopt };
    aBefore.maName = u"Metrics";
    aBefore.maBaseAddress = { nSheet, 0, 0 };
    aBefore.maTargetExpression = u"$A$1:$A$2";
    auto aAfter = aBefore;
    aAfter.maName = u"Metrics2";

    const auto aNamedRangeInvalidation
        = planInvalidation(aSnapshot, MutationEvent::renameNamedRange(aBefore, aAfter));
    const auto aNamedRangePlan = buildRecalcPlan(aSnapshot, aNamedRangeInvalidation);

    if (aNamedRangePlan.maRebuildScopes.empty()
        || aNamedRangePlan.maRebuildScopes.front().meKind != RebuildScopeKind::NamedRanges)
    {
        return fail("recalc_planner", "named range rebuild scope mismatch");
    }

    if (!containsQueueAddress(aNamedRangePlan.maQueue, { nSheet, 3, 0 }))
        return fail("recalc_planner", "named range dependent queue mismatch");

    const auto aStructuralInvalidation
        = planInvalidation(aSnapshot, MutationEvent::insertRows(nSheet, 1, 1));
    const auto aStructuralPlan = buildRecalcPlan(aSnapshot, aStructuralInvalidation);

    if (!aStructuralPlan.mbRequiresSnapshotRebuild || !aStructuralPlan.mbUsedConservativeWidening)
        return fail("recalc_planner", "structural rebuild flags mismatch");

    if (!std::any_of(aStructuralPlan.maSeeds.begin(), aStructuralPlan.maSeeds.end(),
            [](const RecalcSeed& rSeed) {
                return rSeed.meKind == RecalcSeedKind::StructuralRebuild
                       && rSeed.moRebuildScopeKind
                       && *rSeed.moRebuildScopeKind == RebuildScopeKind::Workbook;
            }))
    {
        return fail("recalc_planner", "structural rebuild seed mismatch");
    }

    if (aStructuralPlan.maQueue.size() != 5)
        return fail("recalc_planner", "structural queue size mismatch");

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
