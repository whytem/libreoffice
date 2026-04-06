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

enum class AuthorityMutationClass : std::uint8_t
{
    Admitted,
    ValidationOnly,
    Rejected
};

enum class AuthorityVerificationMode : std::uint8_t
{
    Exact,
    AllowNormalizedEquivalent
};

enum class AuthorityPilotVerdict : std::uint8_t
{
    RejectedOutOfContract,
    RejectedDirtyBaseline,
    Applicable,
    Applied,
    NormalizedEquivalent,
    RolledBack
};

struct AuthorityPilotContract
{
    facade::MutationEvent maMutation;
    AuthorityMutationClass meMutationClass = AuthorityMutationClass::Rejected;
    bool mbRequiresCleanBaseline = true;
    bool mbAllowsNormalizedGraph = false;
    bool mbAllowsNormalizedQueue = false;
    bool mbAllowsNormalizedIr = false;

    [[nodiscard]] constexpr bool operator==(const AuthorityPilotContract& rOther) const = default;
    [[nodiscard]] constexpr bool isAdmitted() const
    {
        return meMutationClass == AuthorityMutationClass::Admitted;
    }
};

struct AuthorityPilotVerification
{
    AuthorityVerificationMode meGraphMode = AuthorityVerificationMode::Exact;
    AuthorityVerificationMode meQueueMode = AuthorityVerificationMode::Exact;
    AuthorityVerificationMode meIrMode = AuthorityVerificationMode::Exact;

    [[nodiscard]] constexpr bool operator==(const AuthorityPilotVerification& rOther) const
        = default;
};

struct AuthorityPilotInput
{
    ComputationalWorkbookShadow maComputationalShadow;
    DependencyGraphShadow maGraphShadow;
    ExecutionIrWorkbookShadow maIrShadow;
    facade::MutationEvent maMutation;
    std::optional<api::CellValue> moScalarValueAfter;
    std::optional<api::CellValue> moFormulaCachedValueAfter;
    std::optional<ComputationalWorkbookShadow> moObservedAfterComputationalShadow;
    bool mbAllowSharedGroupNonStructuralAdmission = false;
    bool mbCleanBaseline = false;

    [[nodiscard]] constexpr bool operator==(const AuthorityPilotInput& rOther) const = default;
};

struct AuthorityPilotTransition
{
    AuthorityPilotInput maInput;
    AuthorityPilotContract maContract;
    ComputationalWorkbookShadow maComputationalAfter;
    DependencyGraphShadow maGraphAfter;
    ExecutionIrWorkbookShadow maIrAfter;
    dependency::DependencySnapshot maDependencySnapshot;
    dependency::InvalidationPlan maInvalidationPlan;
    dependency::RecalcPlan maRecalcPlan;
    AuthorityPilotVerification maVerification;
    AuthorityPilotVerdict meVerdict = AuthorityPilotVerdict::RejectedOutOfContract;
    api::String maReason;
    bool mbRequiresRollback = false;

    [[nodiscard]] constexpr bool operator==(const AuthorityPilotTransition& rOther) const
        = default;
    [[nodiscard]] constexpr bool isRejected() const
    {
        return meVerdict == AuthorityPilotVerdict::RejectedOutOfContract
               || meVerdict == AuthorityPilotVerdict::RejectedDirtyBaseline;
    }
};

namespace authoritydetail
{

[[nodiscard]] inline AuthorityPilotContract classifyAuthorityMutation(
    const facade::MutationEvent& rMutation)
{
    AuthorityPilotContract aContract;
    aContract.maMutation = rMutation;

    switch (rMutation.meKind)
    {
        case facade::MutationKind::SetScalarValue:
        case facade::MutationKind::SetFormula:
        case facade::MutationKind::ClearCell:
            aContract.meMutationClass = AuthorityMutationClass::Admitted;
            return aContract;
        case facade::MutationKind::RenameNamedRange:
        case facade::MutationKind::InsertRows:
        case facade::MutationKind::DeleteColumns:
            aContract.meMutationClass = AuthorityMutationClass::ValidationOnly;
            return aContract;
        default:
            aContract.meMutationClass = AuthorityMutationClass::Rejected;
            return aContract;
    }
}

[[nodiscard]] inline AuthorityPilotVerification
makeAuthorityVerification(const AuthorityPilotContract& rContract)
{
    AuthorityPilotVerification aVerification;
    aVerification.meGraphMode = rContract.mbAllowsNormalizedGraph
                                    ? AuthorityVerificationMode::AllowNormalizedEquivalent
                                    : AuthorityVerificationMode::Exact;
    aVerification.meQueueMode = rContract.mbAllowsNormalizedQueue
                                    ? AuthorityVerificationMode::AllowNormalizedEquivalent
                                    : AuthorityVerificationMode::Exact;
    aVerification.meIrMode = rContract.mbAllowsNormalizedIr
                                 ? AuthorityVerificationMode::AllowNormalizedEquivalent
                                 : AuthorityVerificationMode::Exact;
    return aVerification;
}

} // namespace authoritydetail

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
