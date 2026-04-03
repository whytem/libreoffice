/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdlib>
#include <cstring>
#include <optional>
#include <utility>

#include <document.hxx>

#include <spreadsheetengine/compat/libreoffice/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/DependencyGraphShadowBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/ExecutionIrBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcQueueExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>
#include <spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrComparison.hxx>

namespace spreadsheetengine::compat::libreoffice::substrateauthority
{

enum class PilotResultKind : sal_uInt8
{
    Disabled,
    RejectedOutOfContract,
    RejectedDirtyBaseline,
    RolledBackVerificationFailure,
    Applied,
    AppliedNormalizedEquivalent
};

struct PilotResult
{
    PilotResultKind meKind = PilotResultKind::Disabled;
    spreadsheetengine::detail::substrate::AuthorityPilotTransition maTransition;
    std::optional<recalcshadow::ShadowComparison> moQueueComparison;
    std::optional<spreadsheetengine::detail::substrate::DependencyGraphShadowComparison>
        moGraphComparison;
    std::optional<spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison>
        moIrComparison;
};

namespace detail
{

[[nodiscard]] inline bool isRuntimeEnabled(const ScDocument& rDoc)
{
    if (rDoc.GetAutoCalc())
        return false;

    const char* pToggle = std::getenv("SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY");
    return pToggle && *pToggle && std::strcmp(pToggle, "0") != 0;
}

[[nodiscard]] inline bool acceptsQueueComparison(
    const recalcshadow::ShadowComparison& rComparison,
    spreadsheetengine::detail::substrate::AuthorityVerificationMode)
{
    return rComparison.meKind == recalcshadow::ShadowComparisonKind::Exact;
}

[[nodiscard]] inline bool acceptsGraphComparison(
    const spreadsheetengine::detail::substrate::DependencyGraphShadowComparison& rComparison,
    spreadsheetengine::detail::substrate::AuthorityVerificationMode eMode)
{
    if (eMode == spreadsheetengine::detail::substrate::AuthorityVerificationMode::Exact)
        return rComparison.meKind
               == spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind::Exact;
    return rComparison.meKind
           != spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind::Mismatch;
}

[[nodiscard]] inline bool acceptsIrComparison(
    const spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison& rComparison,
    spreadsheetengine::detail::substrate::AuthorityVerificationMode eMode)
{
    if (eMode == spreadsheetengine::detail::substrate::AuthorityVerificationMode::Exact)
        return rComparison.meKind
               == spreadsheetengine::detail::substrate::ExecutionIrComparisonKind::Exact;
    return rComparison.meKind
           != spreadsheetengine::detail::substrate::ExecutionIrComparisonKind::Mismatch;
}

[[nodiscard]] inline bool anyNormalizedEquivalent(const PilotResult& rResult)
{
    return (rResult.moGraphComparison
            && rResult.moGraphComparison->meKind
                   == spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind::
                       NormalizedEquivalent)
           || (rResult.moIrComparison
               && rResult.moIrComparison->meKind
                      == spreadsheetengine::detail::substrate::ExecutionIrComparisonKind::
                          NormalizedEquivalent);
}

[[nodiscard]] inline spreadsheetengine::detail::substrate::AuthorityPilotInput
prepareAuthorityInput(
    const spreadsheetengine::detail::substrate::ComputationalWorkbookShadow& rComputationalShadow,
    const spreadsheetengine::detail::substrate::DependencyGraphShadow& rGraphShadow,
    const spreadsheetengine::detail::substrate::ExecutionIrWorkbookShadow& rIrShadow,
    const spreadsheetengine::detail::facade::MutationEvent& rMutation,
    const spreadsheetengine::detail::facade::WorkbookFacade& rAfterFacade, bool bCleanBaseline)
{
    spreadsheetengine::detail::substrate::AuthorityPilotInput aInput;
    aInput.maComputationalShadow = rComputationalShadow;
    aInput.maGraphShadow = rGraphShadow;
    aInput.maIrShadow = rIrShadow;
    aInput.maMutation = rMutation;
    aInput.mbCleanBaseline = bCleanBaseline;

    switch (rMutation.meKind)
    {
        case spreadsheetengine::detail::facade::MutationKind::SetScalarValue:
            aInput.moScalarValueAfter = rAfterFacade.getCellDescriptor(rMutation.maAddress).maValue;
            break;
        case spreadsheetengine::detail::facade::MutationKind::SetFormula:
            if (const auto oFormula = rAfterFacade.getFormulaCellDescriptor(rMutation.maAddress))
                aInput.moFormulaCachedValueAfter = oFormula->maCachedValue;
            break;
        default:
            break;
    }

    return aInput;
}

} // namespace detail

class ScopedComputationalAuthority
{
    bool mbCaptured = false;
    bool mbCleanBaseline = false;
    spreadsheetengine::detail::substrate::ComputationalWorkbookShadow maComputationalShadow;
    spreadsheetengine::detail::substrate::DependencyGraphShadow maGraphShadow;
    spreadsheetengine::detail::substrate::ExecutionIrWorkbookShadow maIrShadow;

public:
    ScopedComputationalAuthority() = default;

    explicit ScopedComputationalAuthority(ScDocument& rDoc, bool bCapture)
    {
        if (!bCapture)
            return;

        const CalcWorkbookFacade aFacade(rDoc, 0);
        maComputationalShadow = buildComputationalWorkbookShadow(aFacade, rDoc);
        maGraphShadow = buildDependencyGraphShadow(aFacade, rDoc);
        maIrShadow = buildExecutionIrWorkbookShadow(aFacade, rDoc);
        mbCleanBaseline = recalcqueue::isCleanFormulaState(recalcqueue::captureFormulaState(rDoc));
        mbCaptured = true;
    }

    [[nodiscard]] static ScopedComputationalAuthority captureIfRuntimeEnabled(ScDocument& rDoc)
    {
        return ScopedComputationalAuthority(rDoc, detail::isRuntimeEnabled(rDoc));
    }

    [[nodiscard]] bool isCaptured() const { return mbCaptured; }
    [[nodiscard]] bool canApplyAuthority() const { return mbCaptured && mbCleanBaseline; }

    [[nodiscard]] std::optional<PilotResult> apply(
        ScDocument& rDoc, const spreadsheetengine::detail::facade::MutationEvent& rMutation) const
    {
        if (!mbCaptured)
            return std::nullopt;

        PilotResult aResult;
        const sal_Int64 nAfterGeneration = maComputationalShadow.maSnapshot.mnGeneration + 1;
        const CalcWorkbookFacade aAfterFacade(rDoc, nAfterGeneration);
        aResult.maTransition = spreadsheetengine::detail::substrate::buildAuthorityPilotTransition(
            detail::prepareAuthorityInput(maComputationalShadow, maGraphShadow, maIrShadow,
                rMutation, aAfterFacade, mbCleanBaseline));

        switch (aResult.maTransition.meVerdict)
        {
            case spreadsheetengine::detail::substrate::AuthorityPilotVerdict::RejectedDirtyBaseline:
                aResult.meKind = PilotResultKind::RejectedDirtyBaseline;
                return aResult;
            case spreadsheetengine::detail::substrate::AuthorityPilotVerdict::RejectedOutOfContract:
                aResult.meKind = PilotResultKind::RejectedOutOfContract;
                return aResult;
            case spreadsheetengine::detail::substrate::AuthorityPilotVerdict::Applicable:
            case spreadsheetengine::detail::substrate::AuthorityPilotVerdict::Applied:
            case spreadsheetengine::detail::substrate::AuthorityPilotVerdict::NormalizedEquivalent:
            case spreadsheetengine::detail::substrate::AuthorityPilotVerdict::RolledBack:
                break;
        }

        const auto aStateBeforeApply = recalcqueue::captureFormulaState(rDoc);
        recalcqueue::applyRecalcPlan(rDoc, aResult.maTransition.maRecalcPlan);

        const CalcWorkbookFacade aVerifiedFacade(rDoc, nAfterGeneration);
        aResult.moQueueComparison = recalcshadow::detail::comparePlanToDocument(
            aResult.maTransition.maRecalcPlan, aVerifiedFacade, rDoc);

        const auto aObservation = makeComputationalObservationState(
            substrateobs::collectLiveComputationalState(rDoc));
        const auto aLiveComputationalShadow
            = spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow(
                aVerifiedFacade, aObservation);
        aResult.moGraphComparison = spreadsheetengine::detail::substrate::compareDependencyGraphShadow(
            aResult.maTransition.maGraphAfter, aLiveComputationalShadow, aObservation);
        aResult.moIrComparison = spreadsheetengine::detail::substrate::compareExecutionIrWorkbookShadow(
            aResult.maTransition.maIrAfter, buildExecutionIrWorkbookShadow(aLiveComputationalShadow, rDoc));

        const bool bAccepted = detail::acceptsQueueComparison(
                                   *aResult.moQueueComparison,
                                   aResult.maTransition.maVerification.meQueueMode)
            && detail::acceptsGraphComparison(
                *aResult.moGraphComparison, aResult.maTransition.maVerification.meGraphMode);

        if (!bAccepted)
        {
            recalcqueue::restoreFormulaState(rDoc, aStateBeforeApply);
            aResult.meKind = PilotResultKind::RolledBackVerificationFailure;
            return aResult;
        }

        aResult.meKind = detail::anyNormalizedEquivalent(aResult)
                             ? PilotResultKind::AppliedNormalizedEquivalent
                             : PilotResultKind::Applied;
        return aResult;
    }
};

} // namespace spreadsheetengine::compat::libreoffice::substrateauthority

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
