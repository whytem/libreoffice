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
#include <concepts>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <utility>
#include <vector>

#include <sal/log.hxx>

#include <address.hxx>
#include <document.hxx>

#include <spreadsheetengine/compat/libreoffice/MutationTranslator.hxx>
#include <spreadsheetengine/compat/libreoffice/WorkbookFacade.hxx>
#include <spreadsheetengine/detail/dependency/DependencySnapshot.hxx>
#include <spreadsheetengine/detail/dependency/InvalidationPlanner.hxx>

namespace spreadsheetengine::compat::libreoffice::dependencyshadow
{

enum class ShadowComparisonKind : sal_uInt8
{
    Disabled,
    Exact,
    ConservativeSuperset,
    UnderInvalidation
};

struct ShadowComparison
{
    ShadowComparisonKind meKind = ShadowComparisonKind::Disabled;
    sal_Int32 mnPredictedFormulaCount = 0;
    sal_Int32 mnActualFormulaCount = 0;
    sal_Int32 mnMissingFormulaCount = 0;
    sal_Int32 mnExtraFormulaCount = 0;
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

[[nodiscard]] inline bool isRuntimeEnabled(const ScDocument& rDoc)
{
    if (rDoc.GetAutoCalc())
        return false;

    const char* pToggle = std::getenv("SPREADSHEET_ENGINE_DEPENDENCY_SHADOW");
    return pToggle && *pToggle && std::strcmp(pToggle, "0") != 0;
}

[[nodiscard]] inline std::vector<api::CellAddress> normalizeAddresses(
    std::vector<api::CellAddress> aAddresses)
{
    std::sort(aAddresses.begin(), aAddresses.end(), AddressLess {});
    aAddresses.erase(std::unique(aAddresses.begin(), aAddresses.end()), aAddresses.end());
    return aAddresses;
}

[[nodiscard]] inline std::vector<api::CellAddress> collectDirtyFormulaAddresses(
    const CalcWorkbookFacade& rFacade)
{
    std::vector<api::CellAddress> aAddresses;
    rFacade.visitAllFormulaCells([&aAddresses](
                                     const spreadsheetengine::detail::facade::FormulaCellDescriptor&
                                         rDesc) {
        if (rDesc.mbDirty || rDesc.mbNeedsRecalc)
            aAddresses.push_back(rDesc.maId.maAddress);
        return true;
    });
    return normalizeAddresses(std::move(aAddresses));
}

[[nodiscard]] inline std::vector<api::CellAddress> collectPredictedFormulaAddresses(
    const spreadsheetengine::detail::dependency::InvalidationPlan& rPlan)
{
    std::vector<api::CellAddress> aAddresses;
    aAddresses.reserve(rPlan.maDirtyFormulaCells.size());
    for (const auto& rEntry : rPlan.maDirtyFormulaCells)
        aAddresses.push_back(rEntry.maAddress);
    return normalizeAddresses(std::move(aAddresses));
}

[[nodiscard]] inline ShadowComparison comparePlanToFacade(
    const spreadsheetengine::detail::dependency::DependencySnapshot& rSnapshot,
    const spreadsheetengine::detail::facade::MutationEvent& rMutation,
    const CalcWorkbookFacade& rAfterFacade)
{
    const auto aPlan = spreadsheetengine::detail::dependency::planInvalidation(
        rSnapshot, rMutation);
    const auto aPredicted = collectPredictedFormulaAddresses(aPlan);
    const auto aActual = collectDirtyFormulaAddresses(rAfterFacade);

    std::vector<api::CellAddress> aMissing;
    std::vector<api::CellAddress> aExtra;
    std::set_difference(aActual.begin(), aActual.end(), aPredicted.begin(), aPredicted.end(),
        std::back_inserter(aMissing), AddressLess {});
    std::set_difference(aPredicted.begin(), aPredicted.end(), aActual.begin(), aActual.end(),
        std::back_inserter(aExtra), AddressLess {});

    ShadowComparison aComparison;
    aComparison.mnPredictedFormulaCount = static_cast<sal_Int32>(aPredicted.size());
    aComparison.mnActualFormulaCount = static_cast<sal_Int32>(aActual.size());
    aComparison.mnMissingFormulaCount = static_cast<sal_Int32>(aMissing.size());
    aComparison.mnExtraFormulaCount = static_cast<sal_Int32>(aExtra.size());
    aComparison.mbRequiresSnapshotRebuild = aPlan.mbRequiresSnapshotRebuild;
    aComparison.mbUsedConservativeWidening = aPlan.mbUsedConservativeWidening;

    if (!aMissing.empty())
        aComparison.meKind = ShadowComparisonKind::UnderInvalidation;
    else if (!aExtra.empty())
        aComparison.meKind = ShadowComparisonKind::ConservativeSuperset;
    else
        aComparison.meKind = ShadowComparisonKind::Exact;

    return aComparison;
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
                "dependency shadow exact for " << pLabel << " predicted="
                                               << rComparison.mnPredictedFormulaCount
                                               << " actual="
                                               << rComparison.mnActualFormulaCount);
            return;
        case ShadowComparisonKind::ConservativeSuperset:
            SAL_INFO("sc.spreadsheetengine",
                "dependency shadow conservative for " << pLabel << " predicted="
                                                      << rComparison.mnPredictedFormulaCount
                                                      << " actual="
                                                      << rComparison.mnActualFormulaCount
                                                      << " extra="
                                                      << rComparison.mnExtraFormulaCount
                                                      << " rebuild="
                                                      << rComparison.mbRequiresSnapshotRebuild);
            return;
        case ShadowComparisonKind::UnderInvalidation:
            SAL_WARN("sc.spreadsheetengine",
                "dependency shadow under-invalidation for " << pLabel << " predicted="
                                                            << rComparison.mnPredictedFormulaCount
                                                            << " actual="
                                                            << rComparison.mnActualFormulaCount
                                                            << " missing="
                                                            << rComparison.mnMissingFormulaCount
                                                            << " rebuild="
                                                            << rComparison.mbRequiresSnapshotRebuild);
            return;
    }
}

} // namespace detail

class ScopedInvalidationShadow
{
    bool mbCaptured = false;
    spreadsheetengine::detail::dependency::DependencySnapshot maSnapshot;

public:
    ScopedInvalidationShadow() = default;

    explicit ScopedInvalidationShadow(const ScDocument& rDoc, bool bCapture)
    {
        if (!bCapture)
            return;

        const CalcWorkbookFacade aFacade(rDoc, 0);
        maSnapshot = spreadsheetengine::detail::dependency::buildDependencySnapshot(aFacade);
        mbCaptured = true;
    }

    [[nodiscard]] static ScopedInvalidationShadow captureIfRuntimeEnabled(
        const ScDocument& rDoc)
    {
        return ScopedInvalidationShadow(rDoc, detail::isRuntimeEnabled(rDoc));
    }

    [[nodiscard]] bool isCaptured() const { return mbCaptured; }

    [[nodiscard]] std::optional<ShadowComparison> compare(
        const ScDocument& rDoc,
        const spreadsheetengine::detail::facade::MutationEvent& rMutation) const
    {
        if (!mbCaptured)
            return std::nullopt;

        const CalcWorkbookFacade aAfterFacade(rDoc, 0);
        return detail::comparePlanToFacade(maSnapshot, rMutation, aAfterFacade);
    }

    template <typename EventBuilder>
        requires std::invocable<EventBuilder, const CalcWorkbookFacade&>
    [[nodiscard]] std::optional<ShadowComparison> compare(
        const ScDocument& rDoc, EventBuilder&& rEventBuilder) const
    {
        if (!mbCaptured)
            return std::nullopt;

        const CalcWorkbookFacade aAfterFacade(rDoc, 0);
        return detail::comparePlanToFacade(
            maSnapshot, std::forward<EventBuilder>(rEventBuilder)(aAfterFacade), aAfterFacade);
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

} // namespace spreadsheetengine::compat::libreoffice::dependencyshadow

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
