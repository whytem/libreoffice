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

#include <sal/log.hxx>
#include <osl/diagnose.h>

#include <document.hxx>

#include <spreadsheetengine/compat/libreoffice/RecalcQueueExecution.hxx>
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
    recalcqueue::FormulaStateSnapshot maBaselineState;

public:
    ScopedRecalcAuthority() = default;

    explicit ScopedRecalcAuthority(const ScDocument& rDoc, bool bCapture)
    {
        if (!bCapture)
            return;

        const CalcWorkbookFacade aFacade(rDoc, 0);
        maSnapshot = spreadsheetengine::detail::dependency::buildDependencySnapshot(aFacade);
        maBaselineState = recalcqueue::captureFormulaState(rDoc);
        mbCleanBaseline = recalcqueue::isCleanFormulaState(maBaselineState);
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
        const CalcWorkbookFacade aAfterFacade(rDoc, 0);
        aResult.maPlan
            = recalcshadow::detail::buildComparisonPlan(maSnapshot, rMutation, aAfterFacade);
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

        const auto aActualBefore = recalcqueue::captureFormulaState(rDoc);
        recalcqueue::applyRecalcPlan(rDoc, aResult.maPlan);

        const CalcWorkbookFacade aAppliedFacade(rDoc, 0);
        aResult.moComparisonAfter
            = recalcshadow::detail::comparePlanToDocument(aResult.maPlan, aAppliedFacade, rDoc);
        if (aResult.moComparisonAfter->meKind != recalcshadow::ShadowComparisonKind::Exact)
        {
            recalcqueue::restoreFormulaState(rDoc, aActualBefore);
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
