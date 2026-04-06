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

#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateLiveApply.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrComparison.hxx>

namespace spreadsheetengine::compat::libreoffice::substratefinalverification
{

enum class FinalVerificationObservationKind : sal_uInt8
{
    Exact,
    NormalizedEquivalent,
    OrderingOnly,
    HiddenHostVerificationOrchestration,
    MissingVerificationInputs,
    QueueOrStateMismatch,
    OutOfContract
};

struct FinalVerificationObservation
{
    FinalVerificationObservationKind meKind = FinalVerificationObservationKind::OutOfContract;
    api::String maReason;
    bool mbVerificationExecuted = false;
    bool mbRolledBack = false;
    bool mbQueueExact = false;
    bool mbComputationalFullMatch = false;
    bool mbGraphFullMatch = false;
    bool mbGraphNormalizedEquivalent = false;
    bool mbIrExact = false;
    bool mbIrNormalizedEquivalent = false;
    bool mbBroadcasterExact = false;
    bool mbBroadcasterOrderingEquivalent = false;
    bool mbLiveApplyExact = false;
    bool mbPrimitiveRealizationExact = false;
    bool mbPrimitiveRollbackExact = false;

    [[nodiscard]] constexpr bool operator==(const FinalVerificationObservation& rOther) const
        = default;
};

namespace detail
{

[[nodiscard]] inline bool isNormalizedEquivalent(
    const spreadsheetengine::detail::substrate::DependencyGraphShadowComparison& rGraph,
    const std::optional<spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison>& oIr)
{
    return rGraph.meKind
               == spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind::
                   NormalizedEquivalent
           || (oIr && oIr->meKind
                         == spreadsheetengine::detail::substrate::ExecutionIrComparisonKind::
                             NormalizedEquivalent);
}

[[nodiscard]] inline bool isOrderingOnly(
    const recalcshadow::ShadowComparison& rQueue,
    const std::optional<spreadsheetengine::detail::substrate::BroadcasterCanonicalizationComparison>&
        oBroadcasters)
{
    if (rQueue.meKind == recalcshadow::ShadowComparisonKind::OrderMismatch)
        return true;

    return oBroadcasters
           && (oBroadcasters->meKind
                       == spreadsheetengine::detail::substrate::BroadcasterCanonicalizationKind::
                           OrderingOnly
               || oBroadcasters->mbOrderingEquivalent);
}

[[nodiscard]] inline bool isHiddenHostLiveApply(
    substrateliveapply::LiveApplyObservationKind eKind)
{
    return eKind == substrateliveapply::LiveApplyObservationKind::HiddenHostApplyOrchestration;
}

[[nodiscard]] inline bool isMissingLiveApplyInput(
    substrateliveapply::LiveApplyObservationKind eKind)
{
    return eKind
           == substrateliveapply::LiveApplyObservationKind::MissingRealizedOrRolledBackObjects;
}

[[nodiscard]] inline bool isHiddenHostPrimitiveRealization(
    substrateobjectrealization::PrimitiveRealizationObservationKind eKind)
{
    return eKind
           == substrateobjectrealization::PrimitiveRealizationObservationKind::
               HiddenHostRealizationOrchestration;
}

[[nodiscard]] inline bool isMissingPrimitiveRealizationInput(
    substrateobjectrealization::PrimitiveRealizationObservationKind eKind)
{
    return eKind
           == substrateobjectrealization::PrimitiveRealizationObservationKind::
               MissingRealizedObjects;
}

[[nodiscard]] inline bool isHiddenHostPrimitiveRollback(
    substraterollback::PrimitiveRollbackObservationKind eKind)
{
    return eKind
           == substraterollback::PrimitiveRollbackObservationKind::
               HiddenHostRollbackOrchestration;
}

[[nodiscard]] inline bool isMissingPrimitiveRollbackInput(
    substraterollback::PrimitiveRollbackObservationKind eKind)
{
    return eKind
           == substraterollback::PrimitiveRollbackObservationKind::MissingRestoredObjects;
}

} // namespace detail

[[nodiscard]] inline FinalVerificationObservation classifyFinalVerificationObservation(
    bool bVerificationExecuted, const recalcshadow::ShadowComparison& rQueue,
    const spreadsheetengine::detail::substrate::ComputationalShadowComparison& rComputational,
    const spreadsheetengine::detail::substrate::DependencyGraphShadowComparison& rGraph,
    const std::optional<spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison>& oIr,
    const std::optional<spreadsheetengine::detail::substrate::BroadcasterCanonicalizationComparison>&
        oBroadcasters,
    const std::optional<substrateliveapply::LiveApplyObservation>& oLiveApply,
    const std::optional<substrateobjectrealization::PrimitiveRealizationObservation>&
        oPrimitiveRealization,
    const std::optional<substraterollback::PrimitiveRollbackObservation>& oPrimitiveRollback,
    api::StringView rReasonIfOutOfContract = {})
{
    FinalVerificationObservation aObservation;
    aObservation.mbVerificationExecuted = bVerificationExecuted;
    aObservation.mbRolledBack = oPrimitiveRollback.has_value();
    aObservation.mbQueueExact = rQueue.meKind == recalcshadow::ShadowComparisonKind::Exact;
    aObservation.mbComputationalFullMatch = rComputational.mbFullMatch;
    aObservation.mbGraphFullMatch = rGraph.mbFullMatch;
    aObservation.mbGraphNormalizedEquivalent
        = rGraph.meKind
          == spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind::
              NormalizedEquivalent;
    aObservation.mbIrExact
        = !oIr || oIr->meKind == spreadsheetengine::detail::substrate::ExecutionIrComparisonKind::Exact;
    aObservation.mbIrNormalizedEquivalent
        = oIr && oIr->meKind
                      == spreadsheetengine::detail::substrate::ExecutionIrComparisonKind::
                          NormalizedEquivalent;
    aObservation.mbBroadcasterExact = !oBroadcasters || oBroadcasters->mbExactMatch;
    aObservation.mbBroadcasterOrderingEquivalent
        = oBroadcasters
          && (oBroadcasters->meKind
                      == spreadsheetengine::detail::substrate::BroadcasterCanonicalizationKind::
                          OrderingOnly
              || oBroadcasters->mbOrderingEquivalent);
    aObservation.mbLiveApplyExact
        = oLiveApply && oLiveApply->meKind == substrateliveapply::LiveApplyObservationKind::Exact;
    aObservation.mbPrimitiveRealizationExact
        = oPrimitiveRealization
          && oPrimitiveRealization->meKind
                 == substrateobjectrealization::PrimitiveRealizationObservationKind::Exact;
    aObservation.mbPrimitiveRollbackExact
        = oPrimitiveRollback
          && oPrimitiveRollback->meKind
                 == substraterollback::PrimitiveRollbackObservationKind::Exact;

    if (!bVerificationExecuted)
    {
        aObservation.meKind = FinalVerificationObservationKind::OutOfContract;
        aObservation.maReason = rReasonIfOutOfContract;
        return aObservation;
    }

    if (!oLiveApply)
    {
        aObservation.meKind = FinalVerificationObservationKind::OutOfContract;
        aObservation.maReason = u"missing_live_apply_observation";
        return aObservation;
    }

    if (oPrimitiveRealization && oPrimitiveRollback)
    {
        aObservation.meKind = FinalVerificationObservationKind::OutOfContract;
        aObservation.maReason = u"conflicting_primitive_verification_paths";
        return aObservation;
    }

    if (!oPrimitiveRealization && !oPrimitiveRollback)
    {
        aObservation.meKind = FinalVerificationObservationKind::OutOfContract;
        aObservation.maReason = u"missing_primitive_verification_observation";
        return aObservation;
    }

    if (oLiveApply->meKind == substrateliveapply::LiveApplyObservationKind::OutOfContract)
    {
        aObservation.meKind = FinalVerificationObservationKind::OutOfContract;
        aObservation.maReason = oLiveApply->maReason;
        return aObservation;
    }

    if (oPrimitiveRealization
        && oPrimitiveRealization->meKind
               == substrateobjectrealization::PrimitiveRealizationObservationKind::OutOfContract)
    {
        aObservation.meKind = FinalVerificationObservationKind::OutOfContract;
        aObservation.maReason = oPrimitiveRealization->maReason;
        return aObservation;
    }

    if (oPrimitiveRollback
        && oPrimitiveRollback->meKind
               == substraterollback::PrimitiveRollbackObservationKind::OutOfContract)
    {
        aObservation.meKind = FinalVerificationObservationKind::OutOfContract;
        aObservation.maReason = oPrimitiveRollback->maReason;
        return aObservation;
    }

    if (detail::isHiddenHostLiveApply(oLiveApply->meKind)
        || (oPrimitiveRealization
            && detail::isHiddenHostPrimitiveRealization(oPrimitiveRealization->meKind))
        || (oPrimitiveRollback
            && detail::isHiddenHostPrimitiveRollback(oPrimitiveRollback->meKind)))
    {
        aObservation.meKind = FinalVerificationObservationKind::HiddenHostVerificationOrchestration;
        aObservation.maReason = u"host_only_final_verification_orchestration";
        return aObservation;
    }

    if (detail::isMissingLiveApplyInput(oLiveApply->meKind)
        || (oPrimitiveRealization
            && detail::isMissingPrimitiveRealizationInput(oPrimitiveRealization->meKind))
        || (oPrimitiveRollback
            && detail::isMissingPrimitiveRollbackInput(oPrimitiveRollback->meKind)))
    {
        aObservation.meKind = FinalVerificationObservationKind::MissingVerificationInputs;
        aObservation.maReason = u"missing_final_verification_inputs";
        return aObservation;
    }

    if (oLiveApply->meKind == substrateliveapply::LiveApplyObservationKind::QueueOrStateMismatch
        || (oPrimitiveRealization
            && oPrimitiveRealization->meKind
                   == substrateobjectrealization::PrimitiveRealizationObservationKind::
                       QueueOrStateMismatch)
        || (oPrimitiveRollback
            && oPrimitiveRollback->meKind
                   == substraterollback::PrimitiveRollbackObservationKind::QueueOrStateMismatch))
    {
        aObservation.meKind = FinalVerificationObservationKind::QueueOrStateMismatch;
        aObservation.maReason = u"primitive_verification_queue_or_state_mismatch";
        return aObservation;
    }

    if (aObservation.mbQueueExact && aObservation.mbComputationalFullMatch
        && aObservation.mbGraphFullMatch && aObservation.mbLiveApplyExact
        && (aObservation.mbPrimitiveRealizationExact || aObservation.mbPrimitiveRollbackExact)
        && aObservation.mbBroadcasterExact
        && !detail::isNormalizedEquivalent(rGraph, oIr) && aObservation.mbIrExact)
    {
        aObservation.meKind = FinalVerificationObservationKind::Exact;
        return aObservation;
    }

    if (aObservation.mbQueueExact && aObservation.mbComputationalFullMatch
        && aObservation.mbGraphFullMatch && aObservation.mbLiveApplyExact
        && (aObservation.mbPrimitiveRealizationExact || aObservation.mbPrimitiveRollbackExact)
        && aObservation.mbBroadcasterExact
        && detail::isNormalizedEquivalent(rGraph, oIr))
    {
        aObservation.meKind = FinalVerificationObservationKind::NormalizedEquivalent;
        aObservation.maReason = u"final_verification_normalized_equivalent";
        return aObservation;
    }

    if (detail::isOrderingOnly(rQueue, oBroadcasters)
        && aObservation.mbComputationalFullMatch && aObservation.mbGraphFullMatch
        && aObservation.mbLiveApplyExact
        && (aObservation.mbPrimitiveRealizationExact || aObservation.mbPrimitiveRollbackExact)
        && (!oIr || oIr->meKind != spreadsheetengine::detail::substrate::ExecutionIrComparisonKind::Mismatch))
    {
        aObservation.meKind = FinalVerificationObservationKind::OrderingOnly;
        aObservation.maReason = u"final_verification_ordering_only";
        return aObservation;
    }

    aObservation.meKind = FinalVerificationObservationKind::QueueOrStateMismatch;
    if (rQueue.meKind != recalcshadow::ShadowComparisonKind::Exact)
        aObservation.maReason = u"final_verification_queue_mismatch";
    else if (!rComputational.mbFullMatch)
        aObservation.maReason = u"final_verification_computational_mismatch";
    else if (!rGraph.mbFullMatch)
        aObservation.maReason = u"final_verification_graph_mismatch";
    else if (oIr && oIr->meKind == spreadsheetengine::detail::substrate::ExecutionIrComparisonKind::Mismatch)
        aObservation.maReason = u"final_verification_ir_mismatch";
    else
        aObservation.maReason = u"final_verification_state_mismatch";
    return aObservation;
}

[[nodiscard]] inline const char* toString(FinalVerificationObservationKind eKind)
{
    switch (eKind)
    {
        case FinalVerificationObservationKind::Exact:
            return "exact";
        case FinalVerificationObservationKind::NormalizedEquivalent:
            return "normalized_equivalent";
        case FinalVerificationObservationKind::OrderingOnly:
            return "ordering_only";
        case FinalVerificationObservationKind::HiddenHostVerificationOrchestration:
            return "hidden_host_verification_orchestration";
        case FinalVerificationObservationKind::MissingVerificationInputs:
            return "missing_verification_inputs";
        case FinalVerificationObservationKind::QueueOrStateMismatch:
            return "queue_or_state_mismatch";
        case FinalVerificationObservationKind::OutOfContract:
            return "out_of_contract";
    }

    return "unknown";
}

} // namespace spreadsheetengine::compat::libreoffice::substratefinalverification

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
