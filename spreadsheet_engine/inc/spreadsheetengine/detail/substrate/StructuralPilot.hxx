/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/detail/dependency/DependencySnapshot.hxx>
#include <spreadsheetengine/detail/dependency/InvalidationPlanner.hxx>
#include <spreadsheetengine/detail/dependency/RecalcPlanner.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadow.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadow.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIr.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrReferenceUpdate.hxx>

namespace spreadsheetengine::detail::substrate
{

enum class StructuralMutationClass : std::uint8_t
{
    Admitted,
    ValidationOnly,
    Rejected
};

enum class StructuralVerificationMode : std::uint8_t
{
    Exact,
    AllowNormalizedEquivalent
};

enum class StructuralPilotBuildMode : std::uint8_t
{
    AuthorityOnly,
    Validation
};

enum class StructuralPilotVerdict : std::uint8_t
{
    RejectedOutOfContract,
    RejectedDirtyBaseline,
    Applicable,
    Applied,
    NormalizedEquivalent,
    RolledBack,
    RepairDetected
};

enum class StructuralSyncActionKind : std::uint8_t
{
    InsertRows,
    DeleteRows,
    InsertColumns,
    DeleteColumns
};

struct StructuralPilotContract
{
    facade::MutationEvent maMutation;
    StructuralMutationClass meMutationClass = StructuralMutationClass::Rejected;
    bool mbRequiresCleanBaseline = true;
    bool mbRequiresScalarStructuralSlice = true;
    bool mbAllowsNormalizedComputational = false;
    bool mbAllowsNormalizedGraph = false;
    bool mbObserveIrOnly = true;

    [[nodiscard]] constexpr bool operator==(const StructuralPilotContract& rOther) const = default;
    [[nodiscard]] constexpr bool isAdmitted() const
    {
        return meMutationClass == StructuralMutationClass::Admitted;
    }
    [[nodiscard]] constexpr bool isAllowedInBuildMode(StructuralPilotBuildMode eMode) const
    {
        switch (eMode)
        {
            case StructuralPilotBuildMode::AuthorityOnly:
                return isAdmitted();
            case StructuralPilotBuildMode::Validation:
                return meMutationClass != StructuralMutationClass::Rejected;
        }

        return false;
    }
};

struct StructuralPilotVerification
{
    StructuralVerificationMode meComputationalMode = StructuralVerificationMode::Exact;
    StructuralVerificationMode meGraphMode = StructuralVerificationMode::Exact;
    StructuralVerificationMode meQueueMode = StructuralVerificationMode::Exact;
    bool mbObserveIrOnly = true;

    [[nodiscard]] constexpr bool operator==(const StructuralPilotVerification& rOther) const
        = default;
};

struct StructuralSyncAction
{
    StructuralSyncActionKind meKind = StructuralSyncActionKind::InsertRows;
    api::SheetId mnSheet = 0;
    api::RowIndex mnStartRow = 0;
    api::ColumnIndex mnStartColumn = 0;
    sal_Int32 mnCount = 0;

    [[nodiscard]] constexpr bool operator==(const StructuralSyncAction& rOther) const = default;
};

struct StructuralReferenceUpdateRecord
{
    ShadowCellId maBeforeId;
    std::optional<ShadowCellId> moAfterId;
    ExecutionIrReferenceUpdateSummary maSummary;
    bool mbRemovedByStructure = false;

    [[nodiscard]] constexpr bool operator==(const StructuralReferenceUpdateRecord& rOther) const
        = default;
};

struct StructuralPilotInput
{
    ComputationalWorkbookShadow maComputationalShadow;
    DependencyGraphShadow maGraphShadow;
    ExecutionIrWorkbookShadow maIrShadow;
    ComputationalWorkbookShadow maObservedAfterComputationalShadow;
    ExecutionIrWorkbookShadow maObservedAfterIrShadow;
    facade::MutationEvent maMutation;
    bool mbCleanBaseline = false;

    [[nodiscard]] constexpr bool operator==(const StructuralPilotInput& rOther) const = default;
};

struct StructuralPilotTransition
{
    StructuralPilotInput maInput;
    StructuralPilotContract maContract;
    std::vector<StructuralSyncAction> maSyncActions;
    std::vector<StructuralReferenceUpdateRecord> maReferenceUpdates;
    ComputationalWorkbookShadow maComputationalAfter;
    DependencyGraphShadow maGraphAfter;
    ExecutionIrWorkbookShadow maIrAfter;
    dependency::DependencySnapshot maDependencySnapshot;
    dependency::InvalidationPlan maInvalidationPlan;
    dependency::RecalcPlan maRecalcPlan;
    StructuralPilotVerification maVerification;
    StructuralPilotVerdict meVerdict = StructuralPilotVerdict::RejectedOutOfContract;
    api::String maReason;
    bool mbRequiresRollback = false;

    [[nodiscard]] constexpr bool operator==(const StructuralPilotTransition& rOther) const
        = default;
    [[nodiscard]] constexpr bool isRejected() const
    {
        return meVerdict == StructuralPilotVerdict::RejectedOutOfContract
               || meVerdict == StructuralPilotVerdict::RejectedDirtyBaseline;
    }
};

namespace structuraldetail
{

[[nodiscard]] inline StructuralPilotContract classifyStructuralMutation(
    const facade::MutationEvent& rMutation)
{
    StructuralPilotContract aContract;
    aContract.maMutation = rMutation;

    switch (rMutation.meKind)
    {
        case facade::MutationKind::InsertRows:
        case facade::MutationKind::DeleteColumns:
            aContract.meMutationClass = StructuralMutationClass::Admitted;
            return aContract;
        case facade::MutationKind::DeleteRows:
        case facade::MutationKind::InsertColumns:
            aContract.meMutationClass = StructuralMutationClass::ValidationOnly;
            return aContract;
        default:
            aContract.meMutationClass = StructuralMutationClass::Rejected;
            return aContract;
    }
}

[[nodiscard]] inline StructuralPilotVerification
makeStructuralVerification(const StructuralPilotContract& rContract)
{
    StructuralPilotVerification aVerification;
    aVerification.meComputationalMode = rContract.mbAllowsNormalizedComputational
                                            ? StructuralVerificationMode::AllowNormalizedEquivalent
                                            : StructuralVerificationMode::Exact;
    aVerification.meGraphMode = rContract.mbAllowsNormalizedGraph
                                    ? StructuralVerificationMode::AllowNormalizedEquivalent
                                    : StructuralVerificationMode::Exact;
    aVerification.meQueueMode = StructuralVerificationMode::Exact;
    aVerification.mbObserveIrOnly = rContract.mbObserveIrOnly;
    return aVerification;
}

} // namespace structuraldetail

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
