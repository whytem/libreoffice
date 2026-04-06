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

#include <spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx>
#include <spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx>
#include <spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx>
#include <spreadsheetengine/detail/substrate/StructuralPilotBuilder.hxx>

namespace spreadsheetengine::detail::substrate
{

enum class MutationEntryPath : std::uint8_t
{
    Authority,
    Lifecycle,
    Structural
};

struct MutationEntryRequest
{
    facade::MutationEvent maMutation;
    std::optional<api::CellValue> moScalarValueAfter;
    std::optional<api::CellValue> moFormulaCachedValueAfter;

    [[nodiscard]] constexpr bool operator==(const MutationEntryRequest& rOther) const = default;

    [[nodiscard]] static MutationEntryRequest setScalarValue(
        const api::CellAddress& rAddress, const api::CellValue& rValue)
    {
        MutationEntryRequest aRequest;
        aRequest.maMutation = facade::MutationEvent::setScalarValue(rAddress);
        aRequest.moScalarValueAfter = rValue;
        return aRequest;
    }

    [[nodiscard]] static MutationEntryRequest setFormula(
        const api::CellAddress& rAddress, api::StringView rFormula,
        std::optional<api::CellValue> oCachedValueAfter = std::nullopt)
    {
        MutationEntryRequest aRequest;
        aRequest.maMutation = facade::MutationEvent::setFormula(rAddress, rFormula);
        aRequest.moFormulaCachedValueAfter = oCachedValueAfter;
        return aRequest;
    }

    [[nodiscard]] static MutationEntryRequest clearCell(const api::CellAddress& rAddress)
    {
        MutationEntryRequest aRequest;
        aRequest.maMutation = facade::MutationEvent::clearCell(rAddress);
        return aRequest;
    }

    [[nodiscard]] static MutationEntryRequest insertRows(
        facade::SheetId nSheet, api::RowIndex nRow, sal_Int32 nCount)
    {
        MutationEntryRequest aRequest;
        aRequest.maMutation = facade::MutationEvent::insertRows(nSheet, nRow, nCount);
        return aRequest;
    }

    [[nodiscard]] static MutationEntryRequest deleteRows(
        facade::SheetId nSheet, api::RowIndex nRow, sal_Int32 nCount)
    {
        MutationEntryRequest aRequest;
        aRequest.maMutation = facade::MutationEvent::deleteRows(nSheet, nRow, nCount);
        return aRequest;
    }

    [[nodiscard]] static MutationEntryRequest insertColumns(
        facade::SheetId nSheet, api::ColumnIndex nColumn, sal_Int32 nCount)
    {
        MutationEntryRequest aRequest;
        aRequest.maMutation = facade::MutationEvent::insertColumns(nSheet, nColumn, nCount);
        return aRequest;
    }

    [[nodiscard]] static MutationEntryRequest deleteColumns(
        facade::SheetId nSheet, api::ColumnIndex nColumn, sal_Int32 nCount)
    {
        MutationEntryRequest aRequest;
        aRequest.maMutation = facade::MutationEvent::deleteColumns(nSheet, nColumn, nCount);
        return aRequest;
    }
};

struct MutationEntryBuildInput
{
    ComputationalWorkbookShadow maComputationalShadow;
    DependencyGraphShadow maGraphShadow;
    ExecutionIrWorkbookShadow maIrShadow;
    MutationEntryRequest maRequest;
    std::optional<ComputationalWorkbookShadow> moObservedAfterComputationalShadow;
    std::optional<ExecutionIrWorkbookShadow> moObservedAfterIrShadow;
    StructuralPilotBuildMode meStructuralBuildMode = StructuralPilotBuildMode::AuthorityOnly;
    bool mbAllowNamedRangeAdmission = false;
    bool mbAllowSharedGroupNonStructuralAdmission = false;
    bool mbCleanBaseline = false;

    [[nodiscard]] constexpr bool operator==(const MutationEntryBuildInput& rOther) const = default;
};

struct MutationEntryTransition
{
    MutationEntryBuildInput maInput;
    MutationEntryPath mePath = MutationEntryPath::Authority;
    std::optional<AuthorityPilotTransition> moAuthorityTransition;
    std::optional<LifecyclePilotTransition> moLifecycleTransition;
    std::optional<StructuralPilotTransition> moStructuralTransition;
    api::String maReason;

    [[nodiscard]] constexpr bool operator==(const MutationEntryTransition& rOther) const = default;
};

namespace mutationentrydetail
{

[[nodiscard]] inline bool isFormulaCellAtAddress(
    const ComputationalWorkbookShadow& rShadow, const api::CellAddress& rAddress)
{
    const ShadowCellRecord* pCell = rShadow.findCell(rAddress);
    return pCell && pCell->hasFormula();
}

[[nodiscard]] inline std::optional<MutationEntryPath> classifyMutationEntryPath(
    const MutationEntryBuildInput& rInput)
{
    switch (rInput.maRequest.maMutation.meKind)
    {
        case facade::MutationKind::SetScalarValue:
            return MutationEntryPath::Authority;
        case facade::MutationKind::SetFormula:
            return MutationEntryPath::Lifecycle;
        case facade::MutationKind::ClearCell:
            return isFormulaCellAtAddress(
                       rInput.maComputationalShadow, rInput.maRequest.maMutation.maAddress)
                       ? std::optional<MutationEntryPath>(MutationEntryPath::Lifecycle)
                       : std::optional<MutationEntryPath>(MutationEntryPath::Authority);
        case facade::MutationKind::InsertRows:
        case facade::MutationKind::DeleteRows:
        case facade::MutationKind::InsertColumns:
        case facade::MutationKind::DeleteColumns:
            return MutationEntryPath::Structural;
        default:
            return std::nullopt;
    }
}

[[nodiscard]] inline ComputationalWorkbookShadow makeObservedAfterComputationalShadow(
    const MutationEntryBuildInput& rInput, const facade::WorkbookFacade& rAfterFacade,
    const ComputationalObservationState& rAfterObservation)
{
    if (rInput.moObservedAfterComputationalShadow)
        return *rInput.moObservedAfterComputationalShadow;
    return buildComputationalWorkbookShadow(rAfterFacade, rAfterObservation);
}

[[nodiscard]] inline std::optional<api::CellValue> resolveScalarValueAfter(
    const MutationEntryBuildInput& rInput, const facade::WorkbookFacade& rAfterFacade)
{
    if (rInput.maRequest.moScalarValueAfter)
        return rInput.maRequest.moScalarValueAfter;
    if (rInput.maRequest.maMutation.meKind != facade::MutationKind::SetScalarValue)
        return std::nullopt;
    return rAfterFacade.getCellDescriptor(rInput.maRequest.maMutation.maAddress).maValue;
}

[[nodiscard]] inline std::optional<api::CellValue> resolveFormulaCachedValueAfter(
    const MutationEntryBuildInput& rInput, const facade::WorkbookFacade& rAfterFacade)
{
    if (rInput.maRequest.moFormulaCachedValueAfter)
        return rInput.maRequest.moFormulaCachedValueAfter;
    if (rInput.maRequest.maMutation.meKind != facade::MutationKind::SetFormula)
        return std::nullopt;

    const auto oFormula = rAfterFacade.getFormulaCellDescriptor(rInput.maRequest.maMutation.maAddress);
    if (!oFormula)
        return std::nullopt;
    return oFormula->maCachedValue;
}

} // namespace mutationentrydetail

[[nodiscard]] inline MutationEntryTransition buildMutationEntryTransition(
    const MutationEntryBuildInput& rInput, const facade::WorkbookFacade& rAfterFacade,
    const ComputationalObservationState& rAfterObservation = {})
{
    MutationEntryTransition aTransition;
    aTransition.maInput = rInput;

    const auto oPath = mutationentrydetail::classifyMutationEntryPath(rInput);
    if (!oPath)
    {
        aTransition.maReason = u"mutation_out_of_contract";
        return aTransition;
    }

    aTransition.mePath = *oPath;
    switch (*oPath)
    {
        case MutationEntryPath::Authority:
        {
            AuthorityPilotInput aAuthorityInput;
            aAuthorityInput.maComputationalShadow = rInput.maComputationalShadow;
            aAuthorityInput.maGraphShadow = rInput.maGraphShadow;
            aAuthorityInput.maIrShadow = rInput.maIrShadow;
            aAuthorityInput.maMutation = rInput.maRequest.maMutation;
            aAuthorityInput.moScalarValueAfter
                = mutationentrydetail::resolveScalarValueAfter(rInput, rAfterFacade);
            aAuthorityInput.moFormulaCachedValueAfter
                = mutationentrydetail::resolveFormulaCachedValueAfter(rInput, rAfterFacade);
            aAuthorityInput.moObservedAfterComputationalShadow = rInput.moObservedAfterComputationalShadow;
            aAuthorityInput.mbAllowSharedGroupNonStructuralAdmission
                = rInput.mbAllowSharedGroupNonStructuralAdmission;
            aAuthorityInput.mbCleanBaseline = rInput.mbCleanBaseline;
            aTransition.moAuthorityTransition = buildAuthorityPilotTransition(aAuthorityInput);
            aTransition.maReason = aTransition.moAuthorityTransition->maReason;
            return aTransition;
        }
        case MutationEntryPath::Lifecycle:
        {
            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = rInput.maComputationalShadow;
            aLifecycleInput.maGraphShadow = rInput.maGraphShadow;
            aLifecycleInput.maIrShadow = rInput.maIrShadow;
            aLifecycleInput.maMutation = rInput.maRequest.maMutation;
            aLifecycleInput.moFormulaCachedValueAfter
                = mutationentrydetail::resolveFormulaCachedValueAfter(rInput, rAfterFacade);
            aLifecycleInput.moObservedAfterComputationalShadow = rInput.moObservedAfterComputationalShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission
                = rInput.mbAllowSharedGroupNonStructuralAdmission;
            aLifecycleInput.mbCleanBaseline = rInput.mbCleanBaseline;
            aTransition.moLifecycleTransition = buildLifecyclePilotTransition(aLifecycleInput);
            aTransition.maReason = aTransition.moLifecycleTransition->maReason;
            return aTransition;
        }
        case MutationEntryPath::Structural:
        {
            if (!rInput.moObservedAfterIrShadow)
            {
                StructuralPilotTransition aRejectedTransition;
                aRejectedTransition.maInput.maComputationalShadow = rInput.maComputationalShadow;
                aRejectedTransition.maInput.maGraphShadow = rInput.maGraphShadow;
                aRejectedTransition.maInput.maIrShadow = rInput.maIrShadow;
                aRejectedTransition.maInput.maMutation = rInput.maRequest.maMutation;
                aRejectedTransition.maInput.maObservedAfterComputationalShadow
                    = mutationentrydetail::makeObservedAfterComputationalShadow(
                        rInput, rAfterFacade, rAfterObservation);
                aRejectedTransition.maInput.mbCleanBaseline = rInput.mbCleanBaseline;
                aRejectedTransition.meVerdict = StructuralPilotVerdict::RejectedOutOfContract;
                aRejectedTransition.maReason = u"missing_structural_ir_after";
                aTransition.moStructuralTransition = std::move(aRejectedTransition);
                aTransition.maReason = u"missing_structural_ir_after";
                return aTransition;
            }

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = rInput.maComputationalShadow;
            aStructuralInput.maGraphShadow = rInput.maGraphShadow;
            aStructuralInput.maIrShadow = rInput.maIrShadow;
            aStructuralInput.maMutation = rInput.maRequest.maMutation;
            aStructuralInput.maObservedAfterComputationalShadow
                = mutationentrydetail::makeObservedAfterComputationalShadow(
                    rInput, rAfterFacade, rAfterObservation);
            aStructuralInput.maObservedAfterIrShadow = *rInput.moObservedAfterIrShadow;
            aStructuralInput.mbCleanBaseline = rInput.mbCleanBaseline;
            aTransition.moStructuralTransition = buildStructuralPilotTransition(aStructuralInput,
                rAfterFacade, rAfterObservation, rInput.meStructuralBuildMode,
                rInput.mbAllowNamedRangeAdmission);
            aTransition.maReason = aTransition.moStructuralTransition->maReason;
            return aTransition;
        }
    }

    aTransition.maReason = u"mutation_out_of_contract";
    return aTransition;
}

[[nodiscard]] inline const dependency::RecalcPlan* findMutationEntryRecalcPlan(
    const MutationEntryTransition& rTransition)
{
    switch (rTransition.mePath)
    {
        case MutationEntryPath::Authority:
            return rTransition.moAuthorityTransition ? &rTransition.moAuthorityTransition->maRecalcPlan
                                                     : nullptr;
        case MutationEntryPath::Lifecycle:
            return rTransition.moLifecycleTransition ? &rTransition.moLifecycleTransition->maRecalcPlan
                                                     : nullptr;
        case MutationEntryPath::Structural:
            return rTransition.moStructuralTransition ? &rTransition.moStructuralTransition->maRecalcPlan
                                                      : nullptr;
    }
    return nullptr;
}

[[nodiscard]] inline const ComputationalWorkbookShadow* findMutationEntryComputationalAfter(
    const MutationEntryTransition& rTransition)
{
    switch (rTransition.mePath)
    {
        case MutationEntryPath::Authority:
            return rTransition.moAuthorityTransition
                       ? &rTransition.moAuthorityTransition->maComputationalAfter
                       : nullptr;
        case MutationEntryPath::Lifecycle:
            return rTransition.moLifecycleTransition
                       ? &rTransition.moLifecycleTransition->maComputationalAfter
                       : nullptr;
        case MutationEntryPath::Structural:
            return rTransition.moStructuralTransition
                       ? &rTransition.moStructuralTransition->maComputationalAfter
                       : nullptr;
    }
    return nullptr;
}

[[nodiscard]] inline const DependencyGraphShadow* findMutationEntryGraphAfter(
    const MutationEntryTransition& rTransition)
{
    switch (rTransition.mePath)
    {
        case MutationEntryPath::Authority:
            return rTransition.moAuthorityTransition ? &rTransition.moAuthorityTransition->maGraphAfter
                                                     : nullptr;
        case MutationEntryPath::Lifecycle:
            return rTransition.moLifecycleTransition ? &rTransition.moLifecycleTransition->maGraphAfter
                                                     : nullptr;
        case MutationEntryPath::Structural:
            return rTransition.moStructuralTransition ? &rTransition.moStructuralTransition->maGraphAfter
                                                      : nullptr;
    }
    return nullptr;
}

[[nodiscard]] inline const ExecutionIrWorkbookShadow* findMutationEntryIrAfter(
    const MutationEntryTransition& rTransition)
{
    switch (rTransition.mePath)
    {
        case MutationEntryPath::Authority:
            return rTransition.moAuthorityTransition ? &rTransition.moAuthorityTransition->maIrAfter
                                                     : nullptr;
        case MutationEntryPath::Lifecycle:
            return rTransition.moLifecycleTransition ? &rTransition.moLifecycleTransition->maIrAfter
                                                     : nullptr;
        case MutationEntryPath::Structural:
            return rTransition.moStructuralTransition ? &rTransition.moStructuralTransition->maIrAfter
                                                      : nullptr;
    }
    return nullptr;
}

[[nodiscard]] inline bool applyMutableMutationEntryTransition(
    MutableComputationalSubstrateState& rState, const MutationEntryTransition& rTransition)
{
    switch (rTransition.mePath)
    {
        case MutationEntryPath::Authority:
            return rTransition.moAuthorityTransition
                   && applyMutableAuthorityTransition(rState, *rTransition.moAuthorityTransition);
        case MutationEntryPath::Lifecycle:
            return rTransition.moLifecycleTransition
                   && applyMutableLifecycleTransition(rState, *rTransition.moLifecycleTransition);
        case MutationEntryPath::Structural:
            return rTransition.moStructuralTransition
                   && applyMutableStructuralTransition(rState, *rTransition.moStructuralTransition);
    }
    return false;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
