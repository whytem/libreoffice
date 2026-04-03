/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <map>

#include <spreadsheetengine/api/ReferenceUpdate.hxx>
#include <spreadsheetengine/detail/dependency/DependencySnapshot.hxx>
#include <spreadsheetengine/detail/dependency/InvalidationPlanner.hxx>
#include <spreadsheetengine/detail/dependency/RecalcPlanner.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrComparison.hxx>
#include <spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx>
#include <spreadsheetengine/detail/substrate/StructuralPilot.hxx>

namespace spreadsheetengine::detail::substrate
{

namespace structuralbuilddetail
{

using spreadsheetengine::detail::substrate::detail::AddressLess;
using spreadsheetengine::detail::substrate::detail::collectShadowCellAddresses;
using spreadsheetengine::detail::substrate::detail::collectShadowFormulaGroups;
using spreadsheetengine::detail::substrate::detail::sortNamedRanges;

[[nodiscard]] inline api::refdata::SheetLimits makeStructuralSheetLimits()
{
    return { 1023, 65535, 15 };
}

[[nodiscard]] inline bool isOrdinaryScalarFormulaCell(const ShadowCellRecord& rCell)
{
    return rCell.hasFormula() && rCell.moFormula->meKind == facade::FormulaCellKind::Ordinary
           && !rCell.moFormulaGroup.has_value();
}

[[nodiscard]] inline bool isAdmittedStructuralSlice(const ComputationalWorkbookShadow& rShadow)
{
    if (!rShadow.maNamedRanges.empty() || !rShadow.maFormulaGroups.empty())
        return false;

    for (const auto& rSheet : rShadow.maSheets)
    {
        for (const auto& rCell : rSheet.maCells)
        {
            if (!rCell.hasFormula())
                continue;
            if (!isOrdinaryScalarFormulaCell(rCell))
                return false;
        }
    }

    return true;
}

[[nodiscard]] inline std::optional<api::CellAddress> shiftAddress(
    const facade::MutationEvent& rMutation, const api::CellAddress& rAddress)
{
    if (rAddress.mnSheet != rMutation.mnSheet)
        return rAddress;

    switch (rMutation.meKind)
    {
        case facade::MutationKind::InsertRows:
            if (rAddress.mnRow >= rMutation.maAddress.mnRow)
                return api::CellAddress { rAddress.mnSheet, rAddress.mnColumn,
                    static_cast<api::RowIndex>(rAddress.mnRow + rMutation.mnCount) };
            return rAddress;
        case facade::MutationKind::DeleteColumns:
            if (rAddress.mnColumn >= rMutation.maAddress.mnColumn
                && rAddress.mnColumn < rMutation.maAddress.mnColumn + rMutation.mnCount)
            {
                return std::nullopt;
            }
            if (rAddress.mnColumn >= rMutation.maAddress.mnColumn + rMutation.mnCount)
            {
                return api::CellAddress { rAddress.mnSheet,
                    static_cast<api::ColumnIndex>(rAddress.mnColumn - rMutation.mnCount),
                    rAddress.mnRow };
            }
            return rAddress;
        default:
            return std::nullopt;
    }
}

[[nodiscard]] inline std::optional<api::CellRange> shiftRange(
    const facade::MutationEvent& rMutation, api::CellRange aRange)
{
    const auto oPlan = makeExecutionIrStructuralUpdatePlan(rMutation, makeStructuralSheetLimits());
    if (!oPlan)
        return std::nullopt;

    const auto eResult = api::refupdate::updateReference(oPlan->meMode, oPlan->maWhere, oPlan->mnDx,
        oPlan->mnDy, oPlan->mnDz, 1023, 65535, 15, oPlan->mbExpandRefs, aRange);
    if (eResult == api::refupdate::UpdateResult::Invalid)
        return std::nullopt;
    return dependency::detail::normalizeRange(aRange);
}

[[nodiscard]] inline std::optional<ListenerAnchorId> shiftListenerAnchor(
    const facade::MutationEvent& rMutation, const ListenerAnchorId& rAnchor)
{
    if (rAnchor.meKind != ListenerAnchorKind::FormulaCell)
        return std::nullopt;

    const auto oShifted = shiftAddress(rMutation, rAnchor.maAnchor);
    if (!oShifted)
        return std::nullopt;

    ListenerAnchorId aShifted = rAnchor;
    aShifted.maAnchor = *oShifted;
    return aShifted;
}

inline void sortComputationalShadowForComparison(ComputationalWorkbookShadow& rShadow)
{
    for (auto& rSheet : rShadow.maSheets)
    {
        std::sort(rSheet.maCells.begin(), rSheet.maCells.end(),
            [](const ShadowCellRecord& rLeft, const ShadowCellRecord& rRight) {
                return AddressLess {}(rLeft.maId.maAddress, rRight.maId.maAddress);
            });
    }

    std::sort(rShadow.maFormulaTree.begin(), rShadow.maFormulaTree.end(), AddressLess {});
    std::sort(rShadow.maFormulaTrack.begin(), rShadow.maFormulaTrack.end(), AddressLess {});
    std::sort(rShadow.maCellBroadcasters.begin(), rShadow.maCellBroadcasters.end(),
        [](const CellBroadcasterRecord& rLeft, const CellBroadcasterRecord& rRight) {
            return AddressLess {}(rLeft.maBroadcaster, rRight.maBroadcaster);
        });
    std::sort(rShadow.maAreaBroadcasters.begin(), rShadow.maAreaBroadcasters.end(),
        [](const AreaBroadcasterRecord& rLeft, const AreaBroadcasterRecord& rRight) {
            if (!(rLeft.maBroadcaster.maStart == rRight.maBroadcaster.maStart))
                return AddressLess {}(rLeft.maBroadcaster.maStart, rRight.maBroadcaster.maStart);
            return AddressLess {}(rLeft.maBroadcaster.maEnd, rRight.maBroadcaster.maEnd);
        });
    for (auto& rRecord : rShadow.maCellBroadcasters)
        graphmapping::sortAndUnique(rRecord.maListeners, graphmapping::ListenerAnchorIdLess {});
    for (auto& rRecord : rShadow.maAreaBroadcasters)
        graphmapping::sortAndUnique(rRecord.maListeners, graphmapping::ListenerAnchorIdLess {});
}

[[nodiscard]] inline ComputationalWorkbookShadow buildPredictedStructuralComputationalShadow(
    const ComputationalWorkbookShadow& rBefore, const facade::MutationEvent& rMutation,
    const facade::WorkbookSnapshotInfo& rAfterSnapshot)
{
    ComputationalWorkbookShadow aPredicted = rBefore;
    aPredicted.maSnapshot = rAfterSnapshot;
    aPredicted.maFormulaGroups.clear();
    aPredicted.maNamedRanges.clear();
    aPredicted.maCellBroadcasters.clear();
    aPredicted.maAreaBroadcasters.clear();
    aPredicted.maFormulaTree.clear();
    aPredicted.maFormulaTrack.clear();

    for (auto& rSheet : aPredicted.maSheets)
        rSheet.maCells.clear();

    for (const auto& rSheet : rBefore.maSheets)
    {
        auto& rTargetSheet = aPredicted.maSheets[static_cast<std::size_t>(rSheet.maSheet.mnId)];
        for (const auto& rCell : rSheet.maCells)
        {
            const auto oShifted = shiftAddress(rMutation, rCell.maId.maAddress);
            if (!oShifted)
                continue;

            ShadowCellRecord aShifted = rCell;
            aShifted.maId.maAddress = *oShifted;
            aShifted.maCell.maAddress = *oShifted;
            if (aShifted.moFormula)
                aShifted.moFormula->maId.maAddress = *oShifted;
            rTargetSheet.maCells.push_back(std::move(aShifted));
        }
    }

    for (const auto& rAddress : rBefore.maFormulaTree)
    {
        if (const auto oShifted = shiftAddress(rMutation, rAddress))
            aPredicted.maFormulaTree.push_back(*oShifted);
    }

    for (const auto& rAddress : rBefore.maFormulaTrack)
    {
        if (const auto oShifted = shiftAddress(rMutation, rAddress))
            aPredicted.maFormulaTrack.push_back(*oShifted);
    }

    for (const auto& rBroadcaster : rBefore.maCellBroadcasters)
    {
        const auto oBroadcaster = shiftAddress(rMutation, rBroadcaster.maBroadcaster);
        if (!oBroadcaster)
            continue;

        CellBroadcasterRecord aShifted;
        aShifted.maBroadcaster = *oBroadcaster;
        for (const auto& rListener : rBroadcaster.maListeners)
        {
            if (const auto oListener = shiftListenerAnchor(rMutation, rListener))
                aShifted.maListeners.push_back(*oListener);
        }
        if (!aShifted.maListeners.empty())
            aPredicted.maCellBroadcasters.push_back(std::move(aShifted));
    }

    for (const auto& rBroadcaster : rBefore.maAreaBroadcasters)
    {
        const auto oBroadcaster = shiftRange(rMutation, rBroadcaster.maBroadcaster);
        if (!oBroadcaster)
            continue;

        AreaBroadcasterRecord aShifted;
        aShifted.maBroadcaster = *oBroadcaster;
        for (const auto& rListener : rBroadcaster.maListeners)
        {
            if (const auto oListener = shiftListenerAnchor(rMutation, rListener))
                aShifted.maListeners.push_back(*oListener);
        }
        if (!aShifted.maListeners.empty())
            aPredicted.maAreaBroadcasters.push_back(std::move(aShifted));
    }

    sortComputationalShadowForComparison(aPredicted);
    return aPredicted;
}

[[nodiscard]] inline bool matchesPredictedStructuralPopulation(
    const ComputationalWorkbookShadow& rPredicted,
    const ComputationalWorkbookShadow& rObserved)
{
    return collectShadowCellAddresses(rPredicted) == collectShadowCellAddresses(rObserved)
           && rPredicted.maFormulaTree == rObserved.maFormulaTree
           && rPredicted.maFormulaTrack == rObserved.maFormulaTrack
           && rPredicted.maCellBroadcasters == rObserved.maCellBroadcasters
           && rPredicted.maAreaBroadcasters == rObserved.maAreaBroadcasters
           && collectShadowFormulaGroups(rPredicted) == collectShadowFormulaGroups(rObserved)
           && sortNamedRanges(rPredicted.maNamedRanges) == sortNamedRanges(rObserved.maNamedRanges);
}

[[nodiscard]] inline ExecutionIrReferenceUpdateSummary updateShiftedFormulaReferences(
    ExecutionIrFormulaRecord& rFormula, const api::CellAddress& rOldPosition,
    const api::CellAddress& rNewPosition, const ExecutionIrStructuralUpdatePlan& rPlan)
{
    ExecutionIrReferenceUpdateSummary aSummary;
    const auto aLimits = makeStructuralSheetLimits();

    for (auto& rInstruction : rFormula.maInstructions)
    {
        api::refupdate::UpdateResult eResult = api::refupdate::UpdateResult::Nothing;
        bool bChanged = false;

        auto lUpdateSingle = [&](api::refdata::SingleRefData& rReference) {
            const auto aBefore = rReference;
            const auto aAbsolute
                = api::refdata::toAbsoluteAddress(rReference, aLimits, rOldPosition);
            api::CellRange aRange { aAbsolute, aAbsolute };
            eResult = api::refupdate::updateReference(rPlan.meMode, rPlan.maWhere, rPlan.mnDx,
                rPlan.mnDy, rPlan.mnDz, aLimits.mnMaxColumn, aLimits.mnMaxRow,
                aLimits.mnMaxSheet, rPlan.mbExpandRefs, aRange);
            api::refdata::setAddress(rReference, aLimits, aRange.maStart, rNewPosition);
            bChanged = rReference != aBefore;
        };

        auto lUpdateRange = [&](api::refdata::ComplexRefData& rReference) {
            const auto aBefore = rReference;
            auto aAbsolute = api::refdata::toAbsoluteRange(rReference, aLimits, rOldPosition);
            eResult = api::refupdate::updateReference(rPlan.meMode, rPlan.maWhere, rPlan.mnDx,
                rPlan.mnDy, rPlan.mnDz, aLimits.mnMaxColumn, aLimits.mnMaxRow,
                aLimits.mnMaxSheet, rPlan.mbExpandRefs, aAbsolute);
            api::refdata::setRange(rReference, aLimits, aAbsolute, rNewPosition);
            bChanged = rReference != aBefore;
        };

        switch (rInstruction.meKind)
        {
            case ExecutionIrInstructionKind::SingleReference:
            case ExecutionIrInstructionKind::ColumnRowNameReference:
                if (auto* pReference = std::get_if<api::refdata::SingleRefData>(&rInstruction.maPayload))
                    lUpdateSingle(*pReference);
                break;
            case ExecutionIrInstructionKind::RangeReference:
                if (auto* pReference = std::get_if<api::refdata::ComplexRefData>(&rInstruction.maPayload))
                    lUpdateRange(*pReference);
                break;
            case ExecutionIrInstructionKind::ExternalSingleReference:
                if (auto* pReference = std::get_if<ExecutionIrExternalSingleRefData>(&rInstruction.maPayload))
                    lUpdateSingle(pReference->maReference);
                break;
            case ExecutionIrInstructionKind::ExternalRangeReference:
                if (auto* pReference = std::get_if<ExecutionIrExternalDoubleRefData>(&rInstruction.maPayload))
                    lUpdateRange(pReference->maReference);
                break;
            default:
                break;
        }

        irrefdetail::foldUpdateResult(aSummary, eResult, bChanged);
    }

    return aSummary;
}

[[nodiscard]] inline ExecutionIrWorkbookShadow buildPredictedStructuralIrShadow(
    const StructuralPilotInput& rInput)
{
    ExecutionIrWorkbookShadow aPredicted;
    aPredicted.maSnapshot = rInput.maObservedAfterIrShadow.maSnapshot;
    aPredicted.maGrammar = rInput.maIrShadow.maGrammar;
    aPredicted.maFormulaGroups = rInput.maObservedAfterIrShadow.maFormulaGroups;
    aPredicted.maBuildFailures = rInput.maObservedAfterIrShadow.maBuildFailures;

    const auto oPlan
        = makeExecutionIrStructuralUpdatePlan(rInput.maMutation, makeStructuralSheetLimits());

    std::map<api::CellAddress, const ExecutionIrFormulaRecord*, AddressLess> aObservedAfterByAddress;
    for (const auto& rFormula : rInput.maObservedAfterIrShadow.maFormulaRecords)
        aObservedAfterByAddress[rFormula.maId.maAddress] = &rFormula;

    for (const auto& rFormula : rInput.maIrShadow.maFormulaRecords)
    {
        const auto oShifted = shiftAddress(rInput.maMutation, rFormula.maId.maAddress);
        if (!oShifted)
            continue;

        ExecutionIrFormulaRecord aPredictedRecord = rFormula;
        if (const auto itObserved = aObservedAfterByAddress.find(*oShifted);
            itObserved != aObservedAfterByAddress.end())
        {
            aPredictedRecord = *itObserved->second;
        }

        aPredictedRecord.maId.maAddress = *oShifted;
        if (oPlan)
        {
            aPredictedRecord.maInstructions = rFormula.maInstructions;
            const auto aSummary = updateShiftedFormulaReferences(
                aPredictedRecord, rFormula.maId.maAddress, *oShifted, *oPlan);
            (void)aSummary;
        }

        aPredictedRecord.mbInFormulaTree
            = std::find(rInput.maObservedAfterComputationalShadow.maFormulaTree.begin(),
                   rInput.maObservedAfterComputationalShadow.maFormulaTree.end(),
                   *oShifted)
              != rInput.maObservedAfterComputationalShadow.maFormulaTree.end();
        aPredictedRecord.mbInFormulaTrack
            = std::find(rInput.maObservedAfterComputationalShadow.maFormulaTrack.begin(),
                   rInput.maObservedAfterComputationalShadow.maFormulaTrack.end(),
                   *oShifted)
              != rInput.maObservedAfterComputationalShadow.maFormulaTrack.end();
        aPredicted.maFormulaRecords.push_back(std::move(aPredictedRecord));
    }

    irdetail::normalizeFormulaRecords(aPredicted.maFormulaRecords);
    return aPredicted;
}

} // namespace structuralbuilddetail

[[nodiscard]] inline StructuralPilotTransition buildStructuralPilotTransition(
    const StructuralPilotInput& rInput, const facade::WorkbookFacade& rAfterFacade,
    const ComputationalObservationState&)
{
    StructuralPilotTransition aTransition;
    aTransition.maInput = rInput;
    aTransition.maContract = structuraldetail::classifyStructuralMutation(rInput.maMutation);
    aTransition.maVerification = structuraldetail::makeStructuralVerification(aTransition.maContract);

    if (!aTransition.maContract.isAdmitted())
    {
        aTransition.meVerdict = StructuralPilotVerdict::RejectedOutOfContract;
        aTransition.maReason = u"mutation_out_of_contract";
        return aTransition;
    }

    if (aTransition.maContract.mbRequiresCleanBaseline && !rInput.mbCleanBaseline)
    {
        aTransition.meVerdict = StructuralPilotVerdict::RejectedDirtyBaseline;
        aTransition.maReason = u"dirty_baseline";
        return aTransition;
    }

    if (!structuralbuilddetail::isAdmittedStructuralSlice(rInput.maComputationalShadow)
        || !structuralbuilddetail::isAdmittedStructuralSlice(rInput.maObservedAfterComputationalShadow)
        || !rInput.maIrShadow.maBuildFailures.empty()
        || !rInput.maObservedAfterIrShadow.maBuildFailures.empty())
    {
        aTransition.meVerdict = StructuralPilotVerdict::RejectedOutOfContract;
        aTransition.maReason = u"structural_slice_out_of_contract";
        return aTransition;
    }

    StructuralSyncAction aSyncAction;
    aSyncAction.mnSheet = rInput.maMutation.mnSheet;
    aSyncAction.mnStartRow = rInput.maMutation.maAddress.mnRow;
    aSyncAction.mnStartColumn = rInput.maMutation.maAddress.mnColumn;
    aSyncAction.mnCount = rInput.maMutation.mnCount;
    switch (rInput.maMutation.meKind)
    {
        case facade::MutationKind::InsertRows:
            aSyncAction.meKind = StructuralSyncActionKind::InsertRows;
            break;
        case facade::MutationKind::DeleteColumns:
            aSyncAction.meKind = StructuralSyncActionKind::DeleteColumns;
            break;
        default:
            aTransition.meVerdict = StructuralPilotVerdict::RejectedOutOfContract;
            aTransition.maReason = u"unsupported_structural_sync";
            return aTransition;
    }

    const auto aPredictedComputational
        = structuralbuilddetail::buildPredictedStructuralComputationalShadow(
            rInput.maComputationalShadow, rInput.maMutation,
            rInput.maObservedAfterComputationalShadow.maSnapshot);
    if (!structuralbuilddetail::matchesPredictedStructuralPopulation(
            aPredictedComputational, rInput.maObservedAfterComputationalShadow))
    {
        aTransition.meVerdict = StructuralPilotVerdict::RejectedOutOfContract;
        aTransition.maReason = u"structural_population_mismatch";
        return aTransition;
    }

    aTransition.maIrAfter = structuralbuilddetail::buildPredictedStructuralIrShadow(rInput);
    const auto aIrComparison
        = compareExecutionIrWorkbookShadow(aTransition.maIrAfter, rInput.maObservedAfterIrShadow);
    if (aIrComparison.meKind == ExecutionIrComparisonKind::Mismatch)
    {
        aTransition.meVerdict = StructuralPilotVerdict::RepairDetected;
        aTransition.maReason = u"structural_reference_update_mismatch";
        aTransition.mbRequiresRollback = true;
        return aTransition;
    }

    if (const auto oPlan = makeExecutionIrStructuralUpdatePlan(
            rInput.maMutation, structuralbuilddetail::makeStructuralSheetLimits()))
    {
        for (const auto& rFormula : rInput.maIrShadow.maFormulaRecords)
        {
            StructuralReferenceUpdateRecord aReferenceUpdate;
            aReferenceUpdate.maBeforeId = rFormula.maId;
            if (const auto oShifted
                = structuralbuilddetail::shiftAddress(rInput.maMutation, rFormula.maId.maAddress))
            {
                aReferenceUpdate.moAfterId = ShadowCellId { *oShifted };
                auto aTmp = rFormula;
                aReferenceUpdate.maSummary = structuralbuilddetail::updateShiftedFormulaReferences(
                    aTmp, rFormula.maId.maAddress, *oShifted, *oPlan);
            }
            else
            {
                aReferenceUpdate.mbRemovedByStructure = true;
            }
            aTransition.maReferenceUpdates.push_back(std::move(aReferenceUpdate));
        }
    }

    aTransition.maDependencySnapshot = dependency::buildDependencySnapshot(rAfterFacade);
    if (aTransition.maDependencySnapshot.maReport.mnOpaqueNodeCount > 0
        || aTransition.maDependencySnapshot.maReport.mnOpaqueEdgeCount > 0)
    {
        aTransition.meVerdict = StructuralPilotVerdict::RejectedOutOfContract;
        aTransition.maReason = u"opaque_dependency_surface";
        return aTransition;
    }

    aTransition.maInvalidationPlan
        = dependency::planInvalidation(aTransition.maDependencySnapshot, rInput.maMutation);
    aTransition.maRecalcPlan
        = dependency::buildRecalcPlan(aTransition.maDependencySnapshot, aTransition.maInvalidationPlan);

    const auto aPredictedObservation = lifecyclebuilddetail::buildLifecycleObservationState(
        aTransition.maDependencySnapshot, aTransition.maRecalcPlan);
    aTransition.maComputationalAfter
        = buildComputationalWorkbookShadow(rAfterFacade, aPredictedObservation);
    aTransition.maGraphAfter
        = buildDependencyGraphShadow(aTransition.maComputationalAfter, aPredictedObservation);
    aTransition.maSyncActions.push_back(aSyncAction);
    aTransition.meVerdict = StructuralPilotVerdict::Applicable;
    aTransition.maReason = u"ready";
    return aTransition;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
