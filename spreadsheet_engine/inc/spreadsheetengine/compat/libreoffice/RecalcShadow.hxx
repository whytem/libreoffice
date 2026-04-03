/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <utility>
#include <vector>

#include <sal/log.hxx>

#include <dociter.hxx>
#include <document.hxx>
#include <formulacell.hxx>

#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateObservation.hxx>
#include <spreadsheetengine/compat/libreoffice/MutationTranslator.hxx>
#include <spreadsheetengine/compat/libreoffice/WorkbookFacade.hxx>
#include <spreadsheetengine/detail/dependency/DependencySnapshot.hxx>
#include <spreadsheetengine/detail/dependency/InvalidationPlanner.hxx>
#include <spreadsheetengine/detail/dependency/RecalcPlanner.hxx>

namespace spreadsheetengine::compat::libreoffice::recalcshadow
{

enum class ShadowComparisonKind : sal_uInt8
{
    Disabled,
    Exact,
    ConservativeSuperset,
    UnderScheduling,
    OrderMismatch,
    GroupMismatch
};

struct ShadowComparison
{
    ShadowComparisonKind meKind = ShadowComparisonKind::Disabled;
    sal_Int32 mnPredictedQueueCount = 0;
    sal_Int32 mnActualQueueCount = 0;
    sal_Int32 mnMissingQueueCount = 0;
    sal_Int32 mnExtraQueueCount = 0;
    sal_Int32 mnFirstOrderMismatchIndex = -1;
    sal_Int32 mnPredictedGroupCount = 0;
    sal_Int32 mnActualGroupCount = 0;
    sal_Int32 mnFirstGroupMismatchIndex = -1;
    std::optional<api::CellAddress> moPredictedOrderAddress;
    std::optional<api::CellAddress> moActualOrderAddress;
    std::optional<api::CellAddress> moPredictedGroupAnchor;
    std::optional<api::CellAddress> moActualGroupAnchor;
    bool mbRequiresSnapshotRebuild = false;
    bool mbUsedConservativeWidening = false;
};

namespace detail
{

struct AddressLess
{
    [[nodiscard]] bool operator()(const api::CellAddress& rLeft,
        const api::CellAddress& rRight) const
    {
        if (rLeft.mnSheet != rRight.mnSheet)
            return rLeft.mnSheet < rRight.mnSheet;
        if (rLeft.mnColumn != rRight.mnColumn)
            return rLeft.mnColumn < rRight.mnColumn;
        return rLeft.mnRow < rRight.mnRow;
    }
};

struct QueueGroup
{
    api::CellAddress maAnchor;
    sal_Int32 mnLength = 0;

    [[nodiscard]] constexpr bool operator==(const QueueGroup& rOther) const = default;
};

[[nodiscard]] inline bool isRuntimeEnabled(const ScDocument& rDoc)
{
    if (rDoc.GetAutoCalc())
        return false;

    const char* pToggle = std::getenv("SPREADSHEET_ENGINE_RECALC_SHADOW");
    return pToggle && *pToggle && std::strcmp(pToggle, "0") != 0;
}

[[nodiscard]] inline std::vector<api::CellAddress> normalizeAddresses(
    std::vector<api::CellAddress> aAddresses)
{
    std::sort(aAddresses.begin(), aAddresses.end(), AddressLess {});
    aAddresses.erase(std::unique(aAddresses.begin(), aAddresses.end()), aAddresses.end());
    return aAddresses;
}

[[nodiscard]] inline std::vector<api::CellAddress> collectFormulaTreeAddresses(const ScDocument& rDoc)
{
    return substrateobs::collectFormulaTreeAddresses(rDoc);
}

[[nodiscard]] inline std::vector<api::CellAddress> collectPredictedQueueAddresses(
    const spreadsheetengine::detail::dependency::RecalcPlan& rPlan)
{
    std::vector<api::CellAddress> aAddresses;
    aAddresses.reserve(rPlan.maQueue.size());
    for (const auto& rEntry : rPlan.maQueue)
        aAddresses.push_back(rEntry.maAddress);
    return aAddresses;
}

[[nodiscard]] inline std::vector<QueueGroup> collectPredictedGroups(
    const spreadsheetengine::detail::dependency::RecalcPlan& rPlan)
{
    std::vector<QueueGroup> aGroups;
    for (const auto& rEntry : rPlan.maQueue)
    {
        if (!rEntry.moSharedGroupAnchor || rEntry.mnSharedGroupLength <= 1)
            continue;

        if (std::none_of(aGroups.begin(), aGroups.end(),
                [&rEntry](const QueueGroup& rGroup) {
                    return rGroup.maAnchor == *rEntry.moSharedGroupAnchor;
                }))
        {
            aGroups.push_back({ *rEntry.moSharedGroupAnchor, rEntry.mnSharedGroupLength });
        }
    }
    return aGroups;
}

[[nodiscard]] inline std::vector<QueueGroup> collectActualGroups(
    const CalcWorkbookFacade& rFacade, const std::vector<api::CellAddress>& rQueueAddresses)
{
    std::vector<QueueGroup> aGroups;
    for (const auto& rAddress : rQueueAddresses)
    {
        const auto oGroup = rFacade.getFormulaGroupDescriptor(rAddress);
        if (!oGroup || !oGroup->isValid())
            continue;

        if (std::none_of(aGroups.begin(), aGroups.end(),
                [&rAnchor = oGroup->maAnchor](const QueueGroup& rGroup) {
                    return rGroup.maAnchor == rAnchor;
                }))
        {
            aGroups.push_back({ oGroup->maAnchor, oGroup->mnLength });
        }
    }
    return aGroups;
}

[[nodiscard]] inline std::vector<api::CellAddress> filterPredictedToActualOrder(
    const std::vector<api::CellAddress>& rPredicted, const std::vector<api::CellAddress>& rActual)
{
    const auto aActualSorted = normalizeAddresses(rActual);
    std::vector<api::CellAddress> aFiltered;
    for (const auto& rAddress : rPredicted)
    {
        if (std::binary_search(aActualSorted.begin(), aActualSorted.end(), rAddress, AddressLess {}))
            aFiltered.push_back(rAddress);
    }
    return aFiltered;
}

[[nodiscard]] inline std::vector<QueueGroup> filterPredictedGroupsToActual(
    const std::vector<QueueGroup>& rPredicted, const std::vector<QueueGroup>& rActual)
{
    std::vector<QueueGroup> aFiltered;
    for (const auto& rGroup : rPredicted)
    {
        if (std::any_of(rActual.begin(), rActual.end(),
                [&rGroup](const QueueGroup& rActualGroup) {
                    return rActualGroup.maAnchor == rGroup.maAnchor;
                }))
        {
            aFiltered.push_back(rGroup);
        }
    }
    return aFiltered;
}

[[nodiscard]] inline ShadowComparison comparePlanToDocument(
    const spreadsheetengine::detail::dependency::RecalcPlan& rPlan,
    const CalcWorkbookFacade& rAfterFacade, const ScDocument& rDoc)
{
    const auto aPredicted = collectPredictedQueueAddresses(rPlan);
    const auto aActual = substrateobs::collectFormulaTreeAddresses(rDoc);
    const auto aPredictedSorted = normalizeAddresses(aPredicted);
    const auto aActualSorted = normalizeAddresses(aActual);

    std::vector<api::CellAddress> aMissing;
    std::vector<api::CellAddress> aExtra;
    std::set_difference(aActualSorted.begin(), aActualSorted.end(), aPredictedSorted.begin(),
        aPredictedSorted.end(), std::back_inserter(aMissing), AddressLess {});
    std::set_difference(aPredictedSorted.begin(), aPredictedSorted.end(), aActualSorted.begin(),
        aActualSorted.end(), std::back_inserter(aExtra), AddressLess {});

    const auto aPredictedFiltered = filterPredictedToActualOrder(aPredicted, aActual);
    const auto aPredictedGroupsFiltered
        = filterPredictedGroupsToActual(collectPredictedGroups(rPlan), collectActualGroups(rAfterFacade, aActual));
    const auto aActualGroups = collectActualGroups(rAfterFacade, aActual);

    ShadowComparison aComparison;
    aComparison.mnPredictedQueueCount = static_cast<sal_Int32>(aPredicted.size());
    aComparison.mnActualQueueCount = static_cast<sal_Int32>(aActual.size());
    aComparison.mnMissingQueueCount = static_cast<sal_Int32>(aMissing.size());
    aComparison.mnExtraQueueCount = static_cast<sal_Int32>(aExtra.size());
    aComparison.mnPredictedGroupCount = static_cast<sal_Int32>(aPredictedGroupsFiltered.size());
    aComparison.mnActualGroupCount = static_cast<sal_Int32>(aActualGroups.size());
    aComparison.mbRequiresSnapshotRebuild = rPlan.mbRequiresSnapshotRebuild;
    aComparison.mbUsedConservativeWidening = rPlan.mbUsedConservativeWidening;

    for (std::size_t nIndex = 0;
         nIndex < std::min(aPredictedFiltered.size(), aActual.size()); ++nIndex)
    {
        if (aPredictedFiltered[nIndex] != aActual[nIndex])
        {
            aComparison.mnFirstOrderMismatchIndex = static_cast<sal_Int32>(nIndex);
            aComparison.moPredictedOrderAddress = aPredictedFiltered[nIndex];
            aComparison.moActualOrderAddress = aActual[nIndex];
            break;
        }
    }

    for (std::size_t nIndex = 0;
         nIndex < std::min(aPredictedGroupsFiltered.size(), aActualGroups.size()); ++nIndex)
    {
        if (!(aPredictedGroupsFiltered[nIndex] == aActualGroups[nIndex]))
        {
            aComparison.mnFirstGroupMismatchIndex = static_cast<sal_Int32>(nIndex);
            aComparison.moPredictedGroupAnchor = aPredictedGroupsFiltered[nIndex].maAnchor;
            aComparison.moActualGroupAnchor = aActualGroups[nIndex].maAnchor;
            break;
        }
    }

    if (!aMissing.empty())
        aComparison.meKind = ShadowComparisonKind::UnderScheduling;
    else if (aComparison.mnFirstOrderMismatchIndex >= 0)
        aComparison.meKind = ShadowComparisonKind::OrderMismatch;
    else if (aComparison.mnFirstGroupMismatchIndex >= 0
             || aPredictedGroupsFiltered.size() != aActualGroups.size())
        aComparison.meKind = ShadowComparisonKind::GroupMismatch;
    else if (!aExtra.empty())
        aComparison.meKind = ShadowComparisonKind::ConservativeSuperset;
    else
        aComparison.meKind = ShadowComparisonKind::Exact;

    return aComparison;
}

[[nodiscard]] inline spreadsheetengine::detail::dependency::RecalcPlan buildComparisonPlan(
    const spreadsheetengine::detail::dependency::DependencySnapshot& rBeforeSnapshot,
    const spreadsheetengine::detail::facade::MutationEvent& rMutation,
    const CalcWorkbookFacade& rAfterFacade)
{
    auto aInvalidationPlan
        = spreadsheetengine::detail::dependency::planInvalidation(rBeforeSnapshot, rMutation);
    if (!aInvalidationPlan.mbRequiresSnapshotRebuild)
        return spreadsheetengine::detail::dependency::buildRecalcPlan(rBeforeSnapshot,
            aInvalidationPlan);

    const auto aAfterSnapshot
        = spreadsheetengine::detail::dependency::buildDependencySnapshot(rAfterFacade);
    const auto aAfterInvalidation
        = spreadsheetengine::detail::dependency::planInvalidation(aAfterSnapshot, rMutation);
    return spreadsheetengine::detail::dependency::buildRecalcPlan(aAfterSnapshot,
        aAfterInvalidation);
}

inline void logComparison(const ShadowComparison& rComparison, const char* pContext)
{
    const char* pLabel = pContext ? pContext : "mutation";
    switch (rComparison.meKind)
    {
        case ShadowComparisonKind::Disabled:
            return;
        case ShadowComparisonKind::Exact:
            SAL_INFO("sc.spreadsheetengine",
                "recalc shadow exact for " << pLabel << " predicted="
                                           << rComparison.mnPredictedQueueCount
                                           << " actual=" << rComparison.mnActualQueueCount);
            return;
        case ShadowComparisonKind::ConservativeSuperset:
            SAL_INFO("sc.spreadsheetengine",
                "recalc shadow conservative for " << pLabel << " predicted="
                                                  << rComparison.mnPredictedQueueCount
                                                  << " actual="
                                                  << rComparison.mnActualQueueCount
                                                  << " extra="
                                                  << rComparison.mnExtraQueueCount);
            return;
        case ShadowComparisonKind::UnderScheduling:
            SAL_WARN("sc.spreadsheetengine",
                "recalc shadow under-scheduling for " << pLabel << " predicted="
                                                      << rComparison.mnPredictedQueueCount
                                                      << " actual="
                                                      << rComparison.mnActualQueueCount
                                                      << " missing="
                                                      << rComparison.mnMissingQueueCount);
            return;
        case ShadowComparisonKind::OrderMismatch:
            SAL_WARN("sc.spreadsheetengine",
                "recalc shadow order mismatch for " << pLabel << " index="
                                                    << rComparison.mnFirstOrderMismatchIndex);
            return;
        case ShadowComparisonKind::GroupMismatch:
            SAL_WARN("sc.spreadsheetengine",
                "recalc shadow group mismatch for " << pLabel << " index="
                                                    << rComparison.mnFirstGroupMismatchIndex);
            return;
    }
}

} // namespace detail

class ScopedRecalcShadow
{
    bool mbCaptured = false;
    spreadsheetengine::detail::dependency::DependencySnapshot maSnapshot;

public:
    ScopedRecalcShadow() = default;

    explicit ScopedRecalcShadow(const ScDocument& rDoc, bool bCapture)
    {
        if (!bCapture)
            return;

        const CalcWorkbookFacade aFacade(rDoc, 0);
        maSnapshot = spreadsheetengine::detail::dependency::buildDependencySnapshot(aFacade);
        mbCaptured = true;
    }

    [[nodiscard]] static ScopedRecalcShadow captureIfRuntimeEnabled(const ScDocument& rDoc)
    {
        return ScopedRecalcShadow(rDoc, detail::isRuntimeEnabled(rDoc));
    }

    [[nodiscard]] bool isCaptured() const { return mbCaptured; }

    [[nodiscard]] std::optional<ShadowComparison> compare(
        const ScDocument& rDoc, const spreadsheetengine::detail::facade::MutationEvent& rMutation) const
    {
        if (!mbCaptured)
            return std::nullopt;

        const CalcWorkbookFacade aAfterFacade(rDoc, 0);
        return detail::comparePlanToDocument(
            detail::buildComparisonPlan(maSnapshot, rMutation, aAfterFacade), aAfterFacade, rDoc);
    }

    template <typename EventBuilder>
        requires std::invocable<EventBuilder, const CalcWorkbookFacade&>
    [[nodiscard]] std::optional<ShadowComparison> compare(
        const ScDocument& rDoc, EventBuilder&& rEventBuilder) const
    {
        if (!mbCaptured)
            return std::nullopt;

        const CalcWorkbookFacade aAfterFacade(rDoc, 0);
        return compare(rDoc, std::forward<EventBuilder>(rEventBuilder)(aAfterFacade));
    }

    void log(const ScDocument& rDoc,
        const spreadsheetengine::detail::facade::MutationEvent& rMutation,
        const char* pContext) const
    {
        if (const auto oComparison = compare(rDoc, rMutation))
            detail::logComparison(*oComparison, pContext);
    }

    template <typename EventBuilder>
        requires std::invocable<EventBuilder, const CalcWorkbookFacade&>
    void log(const ScDocument& rDoc, EventBuilder&& rEventBuilder, const char* pContext) const
    {
        if (const auto oComparison = compare(rDoc, std::forward<EventBuilder>(rEventBuilder)))
            detail::logComparison(*oComparison, pContext);
    }
};

} // namespace spreadsheetengine::compat::libreoffice::recalcshadow

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
