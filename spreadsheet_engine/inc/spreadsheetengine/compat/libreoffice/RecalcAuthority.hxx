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
#include <unordered_set>
#include <utility>
#include <vector>

#include <sal/log.hxx>
#include <osl/diagnose.h>

#include <dociter.hxx>
#include <document.hxx>
#include <formulacell.hxx>
#include <table.hxx>

#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>

namespace spreadsheetengine::compat::libreoffice::recalcauthority
{

enum class PilotResultKind : sal_uInt8
{
    Disabled,
    SkippedDirtyBaseline,
    FallbackUnderScheduling,
    RolledBackVerificationFailure,
    Applied
};

struct PilotResult
{
    PilotResultKind meKind = PilotResultKind::Disabled;
    spreadsheetengine::detail::dependency::RecalcPlan maPlan;
    std::optional<recalcshadow::ShadowComparison> moComparisonBefore;
    std::optional<recalcshadow::ShadowComparison> moComparisonAfter;
};

namespace detail
{

[[nodiscard]] inline bool isRuntimeEnabled(const ScDocument& rDoc)
{
    if (rDoc.GetAutoCalc())
        return false;

    const char* pToggle = std::getenv("SPREADSHEET_ENGINE_RECALC_AUTHORITY");
    return pToggle && *pToggle && std::strcmp(pToggle, "0") != 0;
}

[[nodiscard]] inline bool isStrictRuntimeEnabled()
{
    const char* pToggle = std::getenv("SPREADSHEET_ENGINE_RECALC_AUTHORITY_STRICT");
    return pToggle && *pToggle && std::strcmp(pToggle, "0") != 0;
}

struct FormulaStateSnapshot
{
    std::vector<api::CellAddress> maTreeOrder;
    std::vector<api::CellAddress> maTrackOrder;
    std::vector<api::CellAddress> maDirtyOnly;
};

[[nodiscard]] inline std::vector<ScFormulaCell*> collectFormulaCells(const ScDocument& rDoc)
{
    std::vector<ScFormulaCell*> aCells;
    for (SCTAB nTab = 0; nTab < rDoc.GetTableCount(); ++nTab)
    {
        ScCellIterator aIter(const_cast<ScDocument&>(rDoc),
            ScRange(0, 0, nTab, rDoc.MaxCol(), rDoc.MaxRow(), nTab));
        for (bool bHas = aIter.first(); bHas; bHas = aIter.next())
        {
            if (aIter.getType() != CELLTYPE_FORMULA)
                continue;
            if (ScFormulaCell* pCell = aIter.getFormulaCell())
                aCells.push_back(pCell);
        }
    }
    return aCells;
}

[[nodiscard]] inline FormulaStateSnapshot captureFormulaState(const ScDocument& rDoc)
{
    FormulaStateSnapshot aSnapshot;
    const auto aActualTree
        = recalcshadow::detail::collectFormulaTreeAddresses(rDoc);
    aSnapshot.maTreeOrder = aActualTree;

    for (ScFormulaCell* pCell : collectFormulaCells(rDoc))
    {
        const auto aAddress = toApiCellAddress(pCell->aPos);
        if (rDoc.IsInFormulaTrack(pCell))
            aSnapshot.maTrackOrder.push_back(aAddress);
        else if (pCell->GetDirty() || pCell->NeedsInterpret())
            aSnapshot.maDirtyOnly.push_back(aAddress);
    }

    return aSnapshot;
}

[[nodiscard]] inline bool isCleanBaseline(const FormulaStateSnapshot& rSnapshot)
{
    return rSnapshot.maTreeOrder.empty() && rSnapshot.maTrackOrder.empty()
           && rSnapshot.maDirtyOnly.empty();
}

[[nodiscard]] inline ScFormulaCell* getFormulaCell(ScDocument& rDoc, const api::CellAddress& rAddress)
{
    ScTable* pTable = rDoc.FetchTable(rAddress.mnSheet);
    if (!pTable)
        return nullptr;
    return pTable->GetFormulaCell(rAddress.mnColumn, rAddress.mnRow);
}

inline void clearRuntimeFormulaState(ScDocument& rDoc)
{
    for (ScFormulaCell* pCell : collectFormulaCells(rDoc))
    {
        if (rDoc.IsInFormulaTrack(pCell))
            rDoc.RemoveFromFormulaTrack(pCell);
        if (rDoc.IsInFormulaTree(pCell))
            rDoc.RemoveFromFormulaTree(pCell);
        if (pCell->GetDirty() || pCell->NeedsInterpret())
            pCell->ResetDirty();
    }
}

inline void appendQueueAddressToTree(ScDocument& rDoc, const api::CellAddress& rAddress)
{
    ScFormulaCell* pCell = getFormulaCell(rDoc, rAddress);
    if (!pCell)
        return;

    if (rDoc.IsInFormulaTrack(pCell))
        rDoc.RemoveFromFormulaTrack(pCell);
    if (rDoc.IsInFormulaTree(pCell))
        rDoc.RemoveFromFormulaTree(pCell);

    pCell->SetDirtyVar();
    rDoc.PutInFormulaTree(pCell);
}

inline void restoreFormulaState(ScDocument& rDoc, const FormulaStateSnapshot& rSnapshot)
{
    clearRuntimeFormulaState(rDoc);

    for (const auto& rAddress : rSnapshot.maTreeOrder)
        appendQueueAddressToTree(rDoc, rAddress);

    for (const auto& rAddress : rSnapshot.maTrackOrder)
    {
        ScFormulaCell* pCell = getFormulaCell(rDoc, rAddress);
        if (!pCell)
            continue;
        pCell->SetDirtyVar();
        rDoc.AppendToFormulaTrack(pCell);
    }

    for (const auto& rAddress : rSnapshot.maDirtyOnly)
    {
        ScFormulaCell* pCell = getFormulaCell(rDoc, rAddress);
        if (!pCell)
            continue;
        if (!rDoc.IsInFormulaTree(pCell) && !rDoc.IsInFormulaTrack(pCell))
            pCell->SetDirtyVar();
    }
}

inline void applyRecalcPlan(ScDocument& rDoc,
    const spreadsheetengine::detail::dependency::RecalcPlan& rPlan)
{
    clearRuntimeFormulaState(rDoc);

    for (const auto& rEntry : rPlan.maQueue)
        appendQueueAddressToTree(rDoc, rEntry.maAddress);
}

inline void assertSafeComparison(const recalcshadow::ShadowComparison& rComparison,
    const char* pContext)
{
    if (!isStrictRuntimeEnabled())
        return;

    const bool bSafe = rComparison.meKind != recalcshadow::ShadowComparisonKind::UnderScheduling;
    OSL_ENSURE(bSafe, pContext ? pContext : "recalc authority comparison failed");
}

inline void logPilotResult(const PilotResult& rResult, const char* pContext)
{
    const char* pLabel = pContext ? pContext : "mutation";
    switch (rResult.meKind)
    {
        case PilotResultKind::Disabled:
            return;
        case PilotResultKind::SkippedDirtyBaseline:
            SAL_INFO("sc.spreadsheetengine",
                "recalc authority skipped dirty baseline for " << pLabel);
            return;
        case PilotResultKind::FallbackUnderScheduling:
            SAL_WARN("sc.spreadsheetengine",
                "recalc authority fallback under-scheduling for " << pLabel);
            return;
        case PilotResultKind::RolledBackVerificationFailure:
            SAL_WARN("sc.spreadsheetengine",
                "recalc authority rolled back verification failure for " << pLabel);
            return;
        case PilotResultKind::Applied:
            SAL_INFO("sc.spreadsheetengine",
                "recalc authority applied for " << pLabel << " queue="
                                                << rResult.maPlan.maQueue.size());
            return;
    }
}

} // namespace detail

class ScopedRecalcAuthority
{
    bool mbCaptured = false;
    bool mbCleanBaseline = false;
    spreadsheetengine::detail::dependency::DependencySnapshot maSnapshot;
    detail::FormulaStateSnapshot maBaselineState;

public:
    ScopedRecalcAuthority() = default;

    explicit ScopedRecalcAuthority(const ScDocument& rDoc, bool bCapture)
    {
        if (!bCapture)
            return;

        const CalcWorkbookFacade aFacade(rDoc, 0);
        maSnapshot = spreadsheetengine::detail::dependency::buildDependencySnapshot(aFacade);
        maBaselineState = detail::captureFormulaState(rDoc);
        mbCleanBaseline = detail::isCleanBaseline(maBaselineState);
        mbCaptured = true;
    }

    [[nodiscard]] static ScopedRecalcAuthority captureIfRuntimeEnabled(const ScDocument& rDoc)
    {
        return ScopedRecalcAuthority(rDoc, detail::isRuntimeEnabled(rDoc));
    }

    [[nodiscard]] bool isCaptured() const { return mbCaptured; }
    [[nodiscard]] bool canApplyAuthority() const { return mbCaptured && mbCleanBaseline; }

    [[nodiscard]] std::optional<PilotResult> apply(
        ScDocument& rDoc, const spreadsheetengine::detail::facade::MutationEvent& rMutation) const
    {
        if (!mbCaptured)
            return std::nullopt;

        PilotResult aResult;
        const auto aInvalidationPlan
            = spreadsheetengine::detail::dependency::planInvalidation(maSnapshot, rMutation);
        aResult.maPlan
            = spreadsheetengine::detail::dependency::buildRecalcPlan(maSnapshot, aInvalidationPlan);

        const CalcWorkbookFacade aAfterFacade(rDoc, 0);
        aResult.moComparisonBefore
            = recalcshadow::detail::comparePlanToDocument(aResult.maPlan, aAfterFacade, rDoc);
        detail::assertSafeComparison(*aResult.moComparisonBefore, "recalc authority before apply");

        if (!mbCleanBaseline)
        {
            aResult.meKind = PilotResultKind::SkippedDirtyBaseline;
            return aResult;
        }

        if (aResult.moComparisonBefore->meKind
            == recalcshadow::ShadowComparisonKind::UnderScheduling)
        {
            aResult.meKind = PilotResultKind::FallbackUnderScheduling;
            return aResult;
        }

        const auto aActualBefore = detail::captureFormulaState(rDoc);
        detail::applyRecalcPlan(rDoc, aResult.maPlan);

        const CalcWorkbookFacade aAppliedFacade(rDoc, 0);
        aResult.moComparisonAfter
            = recalcshadow::detail::comparePlanToDocument(aResult.maPlan, aAppliedFacade, rDoc);
        if (aResult.moComparisonAfter->meKind != recalcshadow::ShadowComparisonKind::Exact)
        {
            detail::restoreFormulaState(rDoc, aActualBefore);
            aResult.meKind = PilotResultKind::RolledBackVerificationFailure;
            return aResult;
        }

        aResult.meKind = PilotResultKind::Applied;
        return aResult;
    }

    template <typename EventBuilder>
        requires std::invocable<EventBuilder, const CalcWorkbookFacade&>
    [[nodiscard]] std::optional<PilotResult> apply(
        ScDocument& rDoc, EventBuilder&& rEventBuilder) const
    {
        if (!mbCaptured)
            return std::nullopt;

        const CalcWorkbookFacade aAfterFacade(rDoc, 0);
        return apply(rDoc, std::forward<EventBuilder>(rEventBuilder)(aAfterFacade));
    }

    void logAndApply(ScDocument& rDoc,
        const spreadsheetengine::detail::facade::MutationEvent& rMutation,
        const char* pContext) const
    {
        if (const auto oResult = apply(rDoc, rMutation))
            detail::logPilotResult(*oResult, pContext);
    }

    template <typename EventBuilder>
        requires std::invocable<EventBuilder, const CalcWorkbookFacade&>
    void logAndApply(ScDocument& rDoc, EventBuilder&& rEventBuilder, const char* pContext) const
    {
        if (const auto oResult = apply(rDoc, std::forward<EventBuilder>(rEventBuilder)))
            detail::logPilotResult(*oResult, pContext);
    }
};

} // namespace spreadsheetengine::compat::libreoffice::recalcauthority

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
