/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>
#include <set>

#include <document.hxx>

#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollout.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/DependencyGraphShadowBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/ExecutionIrBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcQueueExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/compat/libreoffice/WorkbookFacade.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrComparison.hxx>
#include <spreadsheetengine/detail/substrate/StructuralPilotBuilder.hxx>

namespace spreadsheetengine::compat::libreoffice::substratestructural
{

enum class StructuralResultKind : sal_uInt8
{
    Disabled,
    RejectedOutOfContract,
    RejectedDirtyBaseline,
    RolledBackVerificationFailure,
    RepairDetected,
    Applied,
    AppliedNormalizedEquivalent
};

struct StructuralResult
{
    StructuralResultKind meKind = StructuralResultKind::Disabled;
    spreadsheetengine::detail::substrate::StructuralPilotTransition maTransition;
    std::optional<recalcshadow::ShadowComparison> moQueueComparison;
    std::optional<spreadsheetengine::detail::substrate::ComputationalShadowComparison>
        moComputationalComparison;
    std::optional<spreadsheetengine::detail::substrate::DependencyGraphShadowComparison>
        moGraphComparison;
    std::optional<spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison>
        moIrComparison;
};

namespace detail
{

[[nodiscard]] inline bool isRuntimeEnabled(const ScDocument& rDoc)
{
    return substraterollout::isSurfaceEnabled(rDoc, substraterollout::RolloutSurface::Structural);
}

[[nodiscard]] inline bool acceptsQueueComparison(const recalcshadow::ShadowComparison& rComparison)
{
    return rComparison.meKind == recalcshadow::ShadowComparisonKind::Exact;
}

[[nodiscard]] inline bool acceptsComputationalComparison(
    const spreadsheetengine::detail::substrate::ComputationalShadowComparison& rComparison,
    spreadsheetengine::detail::substrate::StructuralVerificationMode)
{
    return rComparison.mbFullMatch;
}

[[nodiscard]] inline bool acceptsGraphComparison(
    const spreadsheetengine::detail::substrate::DependencyGraphShadowComparison& rComparison,
    spreadsheetengine::detail::substrate::StructuralVerificationMode eMode)
{
    if (eMode == spreadsheetengine::detail::substrate::StructuralVerificationMode::Exact)
        return rComparison.meKind
               == spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind::Exact;
    return rComparison.meKind
           != spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind::Mismatch;
}

[[nodiscard]] inline bool anyNormalizedEquivalent(const StructuralResult& rResult)
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

[[nodiscard]] inline StructuralResultKind classifyVerifiedStructuralResult(
    const StructuralResult& rResult)
{
    switch (rResult.maTransition.meVerdict)
    {
        case spreadsheetengine::detail::substrate::StructuralPilotVerdict::RejectedOutOfContract:
            return StructuralResultKind::RejectedOutOfContract;
        case spreadsheetengine::detail::substrate::StructuralPilotVerdict::RejectedDirtyBaseline:
            return StructuralResultKind::RejectedDirtyBaseline;
        case spreadsheetengine::detail::substrate::StructuralPilotVerdict::RepairDetected:
            return StructuralResultKind::RepairDetected;
        case spreadsheetengine::detail::substrate::StructuralPilotVerdict::RolledBack:
            return StructuralResultKind::RolledBackVerificationFailure;
        case spreadsheetengine::detail::substrate::StructuralPilotVerdict::Applicable:
        case spreadsheetengine::detail::substrate::StructuralPilotVerdict::Applied:
        case spreadsheetengine::detail::substrate::StructuralPilotVerdict::NormalizedEquivalent:
            break;
    }

    if (!rResult.moQueueComparison || !rResult.moComputationalComparison || !rResult.moGraphComparison)
        return StructuralResultKind::RolledBackVerificationFailure;

    if (!acceptsQueueComparison(*rResult.moQueueComparison)
        || !acceptsComputationalComparison(*rResult.moComputationalComparison,
            rResult.maTransition.maVerification.meComputationalMode)
        || !acceptsGraphComparison(
            *rResult.moGraphComparison, rResult.maTransition.maVerification.meGraphMode))
    {
        return StructuralResultKind::RolledBackVerificationFailure;
    }

    return anyNormalizedEquivalent(rResult) ? StructuralResultKind::AppliedNormalizedEquivalent
                                            : StructuralResultKind::Applied;
}

[[nodiscard]] inline spreadsheetengine::detail::substrate::StructuralPilotInput
prepareStructuralInput(
    const spreadsheetengine::detail::substrate::ComputationalWorkbookShadow& rComputationalShadow,
    const spreadsheetengine::detail::substrate::DependencyGraphShadow& rGraphShadow,
    const spreadsheetengine::detail::substrate::ExecutionIrWorkbookShadow& rIrShadow,
    const spreadsheetengine::detail::facade::MutationEvent& rMutation,
    const spreadsheetengine::detail::facade::WorkbookFacade& rAfterFacade, ScDocument& rDoc,
    bool bCleanBaseline)
{
    spreadsheetengine::detail::substrate::StructuralPilotInput aInput;
    aInput.maComputationalShadow = rComputationalShadow;
    aInput.maGraphShadow = rGraphShadow;
    aInput.maIrShadow = rIrShadow;
    aInput.maMutation = rMutation;
    aInput.mbCleanBaseline = bCleanBaseline;
    aInput.maObservedAfterComputationalShadow = buildComputationalWorkbookShadow(rAfterFacade, rDoc);
    aInput.maObservedAfterIrShadow = buildExecutionIrWorkbookShadow(
        aInput.maObservedAfterComputationalShadow, rDoc);
    return aInput;
}

inline void applyScalarCellValue(ScDocument& rDoc, const ScAddress& rAddress,
    const spreadsheetengine::api::CellValue& rValue)
{
    if (rValue.isNumber() || rValue.isBoolean())
        rDoc.SetValue(rAddress, rValue.mfNumber);
    else if (rValue.isText())
        rDoc.SetString(rAddress, toLibreOfficeString(rValue.maString));
    else
        rDoc.SetEmptyCell(rAddress);
}

inline void restoreSheetFromComputationalShadow(ScDocument& rDoc,
    const spreadsheetengine::detail::substrate::ComputationalWorkbookShadow& rShadow,
    spreadsheetengine::api::SheetId nSheet)
{
    CalcWorkbookFacade aFacade(rDoc, rShadow.maSnapshot.mnGeneration);
    std::set<spreadsheetengine::api::CellAddress,
        spreadsheetengine::detail::substrate::detail::AddressLess>
        aDesiredAddresses;

    for (const auto& rSheet : rShadow.maSheets)
    {
        if (rSheet.maSheet.mnId != nSheet)
            continue;

        for (const auto& rCell : rSheet.maCells)
        {
            aDesiredAddresses.insert(rCell.maId.maAddress);
            const ScAddress aAddress = toLibreOfficeAddress(rCell.maId.maAddress);
            if (rCell.moFormula)
                rDoc.SetString(aAddress, toLibreOfficeString(rCell.moFormula->maFormulaSource));
            else
                applyScalarCellValue(rDoc, aAddress, rCell.maCell.maValue);
        }
    }

    aFacade.visitCells(nSheet, [&](const spreadsheetengine::detail::facade::CellDescriptor& rCell) {
        if (aDesiredAddresses.find(rCell.maAddress) != aDesiredAddresses.end())
            return true;

        rDoc.SetEmptyCell(toLibreOfficeAddress(rCell.maAddress));
        return true;
    });
}

inline void rollbackStructuralMutation(ScDocument& rDoc,
    const spreadsheetengine::detail::substrate::StructuralPilotTransition& rTransition,
    const recalcqueue::FormulaStateSnapshot& rBeforeState,
    const spreadsheetengine::detail::substrate::ComputationalWorkbookShadow& rBeforeShadow)
{
    for (auto it = rTransition.maSyncActions.rbegin(); it != rTransition.maSyncActions.rend(); ++it)
    {
        switch (it->meKind)
        {
            case spreadsheetengine::detail::substrate::StructuralSyncActionKind::InsertRows:
            {
                rDoc.DeleteRow(ScRange(0, it->mnStartRow, it->mnSheet, rDoc.MaxCol(),
                    it->mnStartRow + it->mnCount - 1, it->mnSheet));
                break;
            }
            case spreadsheetengine::detail::substrate::StructuralSyncActionKind::DeleteRows:
            {
                rDoc.InsertRow(ScRange(0, it->mnStartRow, it->mnSheet, rDoc.MaxCol(),
                    it->mnStartRow + it->mnCount - 1, it->mnSheet));
                break;
            }
            case spreadsheetengine::detail::substrate::StructuralSyncActionKind::InsertColumns:
            {
                rDoc.DeleteCol(ScRange(it->mnStartColumn, 0, it->mnSheet,
                    it->mnStartColumn + it->mnCount - 1, rDoc.MaxRow(), it->mnSheet));
                break;
            }
            case spreadsheetengine::detail::substrate::StructuralSyncActionKind::DeleteColumns:
            {
                rDoc.InsertCol(ScRange(it->mnStartColumn, 0, it->mnSheet,
                    it->mnStartColumn + it->mnCount - 1, rDoc.MaxRow(), it->mnSheet));
                break;
            }
        }
    }

    for (const auto& rSync : rTransition.maSyncActions)
        restoreSheetFromComputationalShadow(rDoc, rBeforeShadow, rSync.mnSheet);
    recalcqueue::restoreFormulaState(rDoc, rBeforeState);
}

} // namespace detail

class ScopedComputationalStructural
{
    bool mbCaptured = false;
    bool mbCleanBaseline = false;
    spreadsheetengine::detail::substrate::ComputationalWorkbookShadow maComputationalShadow;
    spreadsheetengine::detail::substrate::DependencyGraphShadow maGraphShadow;
    spreadsheetengine::detail::substrate::ExecutionIrWorkbookShadow maIrShadow;

public:
    ScopedComputationalStructural() = default;

    explicit ScopedComputationalStructural(ScDocument& rDoc, bool bCapture)
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

    [[nodiscard]] static ScopedComputationalStructural captureIfRuntimeEnabled(ScDocument& rDoc)
    {
        return ScopedComputationalStructural(rDoc, detail::isRuntimeEnabled(rDoc));
    }

    [[nodiscard]] bool isCaptured() const { return mbCaptured; }
    [[nodiscard]] bool canApplyStructural() const { return mbCaptured && mbCleanBaseline; }

    [[nodiscard]] std::optional<StructuralResult> apply(
        ScDocument& rDoc, const spreadsheetengine::detail::facade::MutationEvent& rMutation) const
    {
        return run(rDoc, rMutation,
            spreadsheetengine::detail::substrate::StructuralPilotBuildMode::AuthorityOnly);
    }

    [[nodiscard]] std::optional<StructuralResult> validateCandidate(
        ScDocument& rDoc, const spreadsheetengine::detail::facade::MutationEvent& rMutation) const
    {
        return run(rDoc, rMutation,
            spreadsheetengine::detail::substrate::StructuralPilotBuildMode::Validation);
    }

private:
    [[nodiscard]] std::optional<StructuralResult> run(ScDocument& rDoc,
        const spreadsheetengine::detail::facade::MutationEvent& rMutation,
        spreadsheetengine::detail::substrate::StructuralPilotBuildMode eBuildMode) const
    {
        if (!mbCaptured)
            return std::nullopt;

        StructuralResult aResult;
        const sal_Int64 nAfterGeneration = maComputationalShadow.maSnapshot.mnGeneration + 1;
        const CalcWorkbookFacade aAfterFacade(rDoc, nAfterGeneration);
        aResult.maTransition = spreadsheetengine::detail::substrate::buildStructuralPilotTransition(
            detail::prepareStructuralInput(maComputationalShadow, maGraphShadow, maIrShadow,
                rMutation, aAfterFacade, rDoc, mbCleanBaseline),
            aAfterFacade, makeComputationalObservationState(substrateobs::collectLiveComputationalState(rDoc)),
            eBuildMode);

        switch (aResult.maTransition.meVerdict)
        {
            case spreadsheetengine::detail::substrate::StructuralPilotVerdict::RejectedDirtyBaseline:
                aResult.meKind = StructuralResultKind::RejectedDirtyBaseline;
                return aResult;
            case spreadsheetengine::detail::substrate::StructuralPilotVerdict::RejectedOutOfContract:
                aResult.meKind = StructuralResultKind::RejectedOutOfContract;
                return aResult;
            case spreadsheetengine::detail::substrate::StructuralPilotVerdict::Applicable:
            case spreadsheetengine::detail::substrate::StructuralPilotVerdict::Applied:
            case spreadsheetengine::detail::substrate::StructuralPilotVerdict::NormalizedEquivalent:
            case spreadsheetengine::detail::substrate::StructuralPilotVerdict::RolledBack:
            case spreadsheetengine::detail::substrate::StructuralPilotVerdict::RepairDetected:
                break;
        }

        const auto aStateBeforeVerify = recalcqueue::captureFormulaState(rDoc);
        recalcqueue::applyRecalcPlan(rDoc, aResult.maTransition.maRecalcPlan);

        const CalcWorkbookFacade aVerifiedFacade(rDoc, nAfterGeneration);
        aResult.moQueueComparison = recalcshadow::detail::comparePlanToDocument(
            aResult.maTransition.maRecalcPlan, aVerifiedFacade, rDoc);

        const auto aObservation = makeComputationalObservationState(
            substrateobs::collectLiveComputationalState(rDoc));
        const auto aLiveComputationalShadow
            = spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow(
                aVerifiedFacade, aObservation);
        aResult.moComputationalComparison
            = spreadsheetengine::detail::substrate::compareComputationalShadow(
                aResult.maTransition.maComputationalAfter, aVerifiedFacade, aObservation);
        aResult.moGraphComparison = spreadsheetengine::detail::substrate::compareDependencyGraphShadow(
            aResult.maTransition.maGraphAfter, aLiveComputationalShadow, aObservation);
        aResult.moIrComparison = spreadsheetengine::detail::substrate::compareExecutionIrWorkbookShadow(
            aResult.maTransition.maIrAfter, buildExecutionIrWorkbookShadow(aLiveComputationalShadow, rDoc));

        aResult.meKind = detail::classifyVerifiedStructuralResult(aResult);
        if (aResult.meKind == StructuralResultKind::RolledBackVerificationFailure
            || aResult.meKind == StructuralResultKind::RepairDetected)
        {
            detail::rollbackStructuralMutation(
                rDoc, aResult.maTransition, aStateBeforeVerify, maComputationalShadow);
        }

        return aResult;
    }
};

} // namespace spreadsheetengine::compat::libreoffice::substratestructural

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
