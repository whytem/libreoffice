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

namespace spreadsheetengine::detail::substrate
{

enum class LifecycleMutationClass : std::uint8_t
{
    Admitted,
    ValidationOnly,
    Rejected
};

enum class LifecycleVerificationMode : std::uint8_t
{
    Exact,
    AllowNormalizedEquivalent
};

enum class LifecyclePilotVerdict : std::uint8_t
{
    RejectedOutOfContract,
    RejectedDirtyBaseline,
    Applicable,
    Applied,
    NormalizedEquivalent,
    RolledBack,
    RepairDetected
};

enum class LifecycleSyncActionKind : std::uint8_t
{
    InsertFormulaCell,
    ReplaceFormulaCell,
    RemoveFormulaCell
};

struct LifecyclePilotContract
{
    facade::MutationEvent maMutation;
    LifecycleMutationClass meMutationClass = LifecycleMutationClass::Rejected;
    bool mbRequiresCleanBaseline = true;
    bool mbRequiresFormulaLifecycleShape = true;
    bool mbAllowsNormalizedComputational = false;
    bool mbAllowsNormalizedGraph = false;
    bool mbObserveIrOnly = true;

    [[nodiscard]] constexpr bool operator==(const LifecyclePilotContract& rOther) const = default;
    [[nodiscard]] constexpr bool isAdmitted() const
    {
        return meMutationClass == LifecycleMutationClass::Admitted;
    }
};

struct LifecyclePilotVerification
{
    LifecycleVerificationMode meComputationalMode = LifecycleVerificationMode::Exact;
    LifecycleVerificationMode meGraphMode = LifecycleVerificationMode::Exact;
    bool mbObserveIrOnly = true;

    [[nodiscard]] constexpr bool operator==(const LifecyclePilotVerification& rOther) const
        = default;
};

struct LifecycleSyncAction
{
    LifecycleSyncActionKind meKind = LifecycleSyncActionKind::ReplaceFormulaCell;
    api::CellAddress maAddress;
    std::optional<api::String> moFormulaSource;
    std::optional<api::CellValue> moCachedValueAfter;
    bool mbFormulaPresentBefore = false;
    bool mbFormulaPresentAfter = false;

    [[nodiscard]] constexpr bool operator==(const LifecycleSyncAction& rOther) const = default;
};

struct LifecyclePilotInput
{
    ComputationalWorkbookShadow maComputationalShadow;
    DependencyGraphShadow maGraphShadow;
    ExecutionIrWorkbookShadow maIrShadow;
    facade::MutationEvent maMutation;
    std::optional<api::CellValue> moFormulaCachedValueAfter;
    bool mbCleanBaseline = false;

    [[nodiscard]] constexpr bool operator==(const LifecyclePilotInput& rOther) const = default;
};

struct LifecyclePilotTransition
{
    LifecyclePilotInput maInput;
    LifecyclePilotContract maContract;
    std::vector<LifecycleSyncAction> maSyncActions;
    ComputationalWorkbookShadow maComputationalAfter;
    DependencyGraphShadow maGraphAfter;
    ExecutionIrWorkbookShadow maIrAfter;
    dependency::DependencySnapshot maDependencySnapshot;
    dependency::InvalidationPlan maInvalidationPlan;
    dependency::RecalcPlan maRecalcPlan;
    LifecyclePilotVerification maVerification;
    LifecyclePilotVerdict meVerdict = LifecyclePilotVerdict::RejectedOutOfContract;
    api::String maReason;
    bool mbRequiresRollback = false;

    [[nodiscard]] constexpr bool operator==(const LifecyclePilotTransition& rOther) const
        = default;
    [[nodiscard]] constexpr bool isRejected() const
    {
        return meVerdict == LifecyclePilotVerdict::RejectedOutOfContract
               || meVerdict == LifecyclePilotVerdict::RejectedDirtyBaseline;
    }
};

namespace lifecycledetail
{

[[nodiscard]] inline LifecyclePilotContract classifyLifecycleMutation(
    const facade::MutationEvent& rMutation)
{
    LifecyclePilotContract aContract;
    aContract.maMutation = rMutation;

    switch (rMutation.meKind)
    {
        case facade::MutationKind::SetFormula:
        case facade::MutationKind::ClearCell:
            aContract.meMutationClass = LifecycleMutationClass::Admitted;
            return aContract;
        case facade::MutationKind::RenameNamedRange:
            aContract.meMutationClass = LifecycleMutationClass::ValidationOnly;
            return aContract;
        default:
            aContract.meMutationClass = LifecycleMutationClass::Rejected;
            return aContract;
    }
}

[[nodiscard]] inline LifecyclePilotVerification
makeLifecycleVerification(const LifecyclePilotContract& rContract)
{
    LifecyclePilotVerification aVerification;
    aVerification.meComputationalMode = rContract.mbAllowsNormalizedComputational
                                            ? LifecycleVerificationMode::AllowNormalizedEquivalent
                                            : LifecycleVerificationMode::Exact;
    aVerification.meGraphMode = rContract.mbAllowsNormalizedGraph
                                    ? LifecycleVerificationMode::AllowNormalizedEquivalent
                                    : LifecycleVerificationMode::Exact;
    aVerification.mbObserveIrOnly = rContract.mbObserveIrOnly;
    return aVerification;
}

} // namespace lifecycledetail

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
