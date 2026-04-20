/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <spreadsheetengine/api/Logic.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

// Batch 1 substrate: engine-native control-flow decision layer.
//
// This header defines *pure* planners: given an RpnValue condition/selector
// plus a jump descriptor, they return a BranchPlan describing which path the
// caller should take. They never mutate PC state, never touch FormulaToken,
// and never read Calc state.
//
// This layer now backs the admitted scalar control-flow paths for
// ocIf / ocChoose / ocIfs_MS / ocSwitch_MS / ocIfError / ocIfNA. Matrix
// conditions and nested-interpreter LET semantics still defer to Calc-host
// logic, but the decision substrate itself is no longer substrate-only.

namespace spreadsheetengine::core::rpn
{

enum class BranchDirective : std::uint8_t
{
    // The caller should evaluate the selected subtree (index in mnSlot) and
    // then advance to mnEndpointSkipHint so siblings are skipped.
    TakeSlot,
    // The caller should push api::Error (meError) as the result and advance
    // directly to the endpoint.
    PropagateError,
    // The caller should push a synthetic boolean (mbSyntheticBool) as the
    // result and advance directly to the endpoint. Used for bare IF(cond)
    // without explicit TRUE/FALSE paths.
    ReturnSyntheticBoolean,
    // The caller should push api::Error::NotAvailable as the result.
    ReturnNotAvailable,
    // The caller should push api::Error::ParameterExpected / NoValue.
    ReturnParameterExpected,
    // Caller should keep the primary value that it already has on the stack
    // (IFERROR / IFNA non-matching path).
    KeepPrimaryValue,
    // Caller should evaluate the alternate subtree (IFERROR / IFNA matching
    // path). The alternate slot index is in mnSlot.
    EvaluateAlternate
};

struct BranchPlan
{
    BranchDirective meDirective = BranchDirective::PropagateError;
    // Slot selected by the planner (1-based for CHOOSE, 0-based for IFS pairs,
    // unused for single-branch directives).
    std::size_t mnSlot = 0;
    // Synthesized result to push when the directive is a return-* directive.
    bool mbSyntheticBool = false;
    // Error to push when the directive is PropagateError.
    api::Error meError = api::Error::None;
};

[[nodiscard]] inline constexpr BranchPlan makeErrorBranchPlan(api::Error eError)
{
    BranchPlan aPlan;
    aPlan.meDirective = BranchDirective::PropagateError;
    aPlan.meError = eError;
    return aPlan;
}

[[nodiscard]] inline constexpr BranchPlan makeSyntheticBooleanBranchPlan(bool bValue)
{
    BranchPlan aPlan;
    aPlan.meDirective = BranchDirective::ReturnSyntheticBoolean;
    aPlan.mbSyntheticBool = bValue;
    return aPlan;
}

[[nodiscard]] inline constexpr BranchPlan makeTakeSlotBranchPlan(std::size_t nSlot)
{
    BranchPlan aPlan;
    aPlan.meDirective = BranchDirective::TakeSlot;
    aPlan.mnSlot = nSlot;
    return aPlan;
}

// Convert an RpnValue condition into a boolean using the same scalar-bool
// coercion rules the evaluator already uses for IF / IFS / SWITCH.
// Matrix conditions defer via NeedsMatrixMaterialization; reference
// conditions defer via NeedsReferenceResolution.
[[nodiscard]] inline RpnCoercionResult<bool> coerceConditionToBoolean(const RpnValue& rCondition)
{
    if (rCondition.meKind == RpnValueKind::Reference)
        return RpnCoercionResult<bool>::deferred(RpnCoercionReadiness::NeedsReferenceResolution);
    if (rCondition.meKind == RpnValueKind::Matrix)
        return RpnCoercionResult<bool>::deferred(RpnCoercionReadiness::NeedsMatrixMaterialization);

    return coerceToBoolean(rCondition);
}

// IF(condition, then?, else?).
// nThenSlot / nElseSlot are the 0-based slot indices in the caller's token
// layout; pass std::nullopt for a missing slot. The caller is responsible for
// mapping slots to jump offsets.
[[nodiscard]] inline RpnCoercionResult<BranchPlan> planIfBranch(
    const RpnValue& rCondition,
    std::optional<std::size_t> oThenSlot,
    std::optional<std::size_t> oElseSlot)
{
    const auto aBool = coerceConditionToBoolean(rCondition);
    if (aBool.meReadiness != RpnCoercionReadiness::Ready)
        return RpnCoercionResult<BranchPlan>::deferred(aBool.meReadiness);

    const bool bConditionError = !aBool;
    const bool bCondition = aBool ? aBool.maValue : false;

    using api::logic::IfBranchAction;
    const IfBranchAction eAction = api::logic::selectIfBranch(
        bCondition, bConditionError, oThenSlot.has_value(), oElseSlot.has_value());

    switch (eAction)
    {
        case IfBranchAction::PropagateError:
            return RpnCoercionResult<BranchPlan>::success(makeErrorBranchPlan(aBool.meError));
        case IfBranchAction::ThenPath:
            return RpnCoercionResult<BranchPlan>::success(makeTakeSlotBranchPlan(*oThenSlot));
        case IfBranchAction::ElsePath:
            return RpnCoercionResult<BranchPlan>::success(makeTakeSlotBranchPlan(*oElseSlot));
        case IfBranchAction::ReturnTrue:
            return RpnCoercionResult<BranchPlan>::success(makeSyntheticBooleanBranchPlan(true));
        case IfBranchAction::ReturnFalse:
            return RpnCoercionResult<BranchPlan>::success(makeSyntheticBooleanBranchPlan(false));
    }
    return RpnCoercionResult<BranchPlan>::failure(api::Error::IllegalArgument);
}

// CHOOSE(index, val1, val2, ...).
// nBranchCount is the total number of value slots (excluding the selector).
[[nodiscard]] inline RpnCoercionResult<BranchPlan> planChooseBranch(
    const RpnValue& rSelector, std::int16_t nBranchCount)
{
    if (rSelector.meKind == RpnValueKind::Reference)
        return RpnCoercionResult<BranchPlan>::deferred(
            RpnCoercionReadiness::NeedsReferenceResolution);
    if (rSelector.meKind == RpnValueKind::Matrix)
        return RpnCoercionResult<BranchPlan>::deferred(
            RpnCoercionReadiness::NeedsMatrixMaterialization);

    const auto aNumber = coerceToNumber(rSelector);
    if (!aNumber)
    {
        if (aNumber.meReadiness != RpnCoercionReadiness::Ready)
            return RpnCoercionResult<BranchPlan>::deferred(aNumber.meReadiness);
        return RpnCoercionResult<BranchPlan>::success(makeErrorBranchPlan(aNumber.meError));
    }

    const auto oNormalized = api::logic::normalizeChooseIndex(
        aNumber.maValue, static_cast<std::int16_t>(nBranchCount + 1));
    if (!oNormalized)
        return RpnCoercionResult<BranchPlan>::success(
            makeErrorBranchPlan(api::Error::IllegalArgument));

    const auto aValidated = api::logic::chooseJumpIndex(
        *oNormalized, static_cast<std::int16_t>(nBranchCount + 1));
    if (!aValidated)
        return RpnCoercionResult<BranchPlan>::success(
            makeErrorBranchPlan(aValidated.meError));

    // Slot numbering matches CHOOSE's 1-based value-slot convention.
    return RpnCoercionResult<BranchPlan>::success(
        makeTakeSlotBranchPlan(static_cast<std::size_t>(aValidated.maValue)));
}

// IFS(c1, v1, c2, v2, ...).
// nConditionIndex is the 0-based pair index the caller is currently
// evaluating. nRemainingParamsAfterCondition is the count of args after the
// current condition (including the paired value and any later pairs).
// Returns a BranchPlan whose slot is the pair-index to emit when selected.
[[nodiscard]] inline RpnCoercionResult<BranchPlan> planIfsBranch(
    const RpnValue& rCondition,
    std::size_t nConditionIndex,
    std::int16_t nRemainingParamsAfterCondition)
{
    const auto aBool = coerceConditionToBoolean(rCondition);
    if (aBool.meReadiness != RpnCoercionReadiness::Ready)
        return RpnCoercionResult<BranchPlan>::deferred(aBool.meReadiness);

    const bool bError = !aBool;
    const bool bCondition = aBool ? aBool.maValue : false;

    using api::logic::IfsAction;
    const IfsAction eAction = api::logic::evaluateIfsCondition(
        bCondition, bError, nRemainingParamsAfterCondition);

    BranchPlan aPlan;
    switch (eAction)
    {
        case IfsAction::SelectCurrentResult:
            return RpnCoercionResult<BranchPlan>::success(
                makeTakeSlotBranchPlan(nConditionIndex));
        case IfsAction::SkipCurrentResult:
            aPlan.meDirective = BranchDirective::TakeSlot;
            aPlan.mnSlot = nConditionIndex + 1;
            return RpnCoercionResult<BranchPlan>::success(aPlan);
        case IfsAction::ReturnNotAvailable:
            aPlan.meDirective = BranchDirective::ReturnNotAvailable;
            return RpnCoercionResult<BranchPlan>::success(aPlan);
        case IfsAction::ReturnParameterExpected:
            aPlan.meDirective = BranchDirective::ReturnParameterExpected;
            return RpnCoercionResult<BranchPlan>::success(aPlan);
        case IfsAction::ReturnNoValue:
            return RpnCoercionResult<BranchPlan>::success(
                makeErrorBranchPlan(aBool.meError));
    }
    return RpnCoercionResult<BranchPlan>::failure(api::Error::IllegalArgument);
}

// SWITCH(selector, case1, result1, case2, result2, ..., [default]).
// The caller passes case labels already pushed. This planner returns
// TakeSlot with the 0-based result-slot index to emit, or PropagateError.
[[nodiscard]] inline RpnCoercionResult<BranchPlan> planSwitchBranch(
    const RpnValue& rSelector,
    std::span<const RpnValue> aCaseLabels,
    std::optional<std::size_t> oDefaultSlot)
{
    if (rSelector.meKind == RpnValueKind::Reference)
        return RpnCoercionResult<BranchPlan>::deferred(
            RpnCoercionReadiness::NeedsReferenceResolution);
    if (rSelector.meKind == RpnValueKind::Matrix)
        return RpnCoercionResult<BranchPlan>::deferred(
            RpnCoercionReadiness::NeedsMatrixMaterialization);

    for (std::size_t i = 0; i < aCaseLabels.size(); ++i)
    {
        const RpnValue& rLabel = aCaseLabels[i];
        if (rLabel.meKind == RpnValueKind::Reference)
            return RpnCoercionResult<BranchPlan>::deferred(
                RpnCoercionReadiness::NeedsReferenceResolution);
        if (rLabel.meKind == RpnValueKind::Matrix)
            return RpnCoercionResult<BranchPlan>::deferred(
                RpnCoercionReadiness::NeedsMatrixMaterialization);

        if (rSelector.meKind == RpnValueKind::String
            && rLabel.meKind == RpnValueKind::String)
        {
            if (rSelector.maScalar.maString == rLabel.maScalar.maString)
                return RpnCoercionResult<BranchPlan>::success(makeTakeSlotBranchPlan(i));
            continue;
        }

        const auto aSelNumber = coerceToNumber(rSelector);
        const auto aLabelNumber = coerceToNumber(rLabel);
        if (!aSelNumber || !aLabelNumber)
            continue;

        if (fp::approxEqual(aSelNumber.maValue, aLabelNumber.maValue))
            return RpnCoercionResult<BranchPlan>::success(makeTakeSlotBranchPlan(i));
    }

    if (oDefaultSlot)
        return RpnCoercionResult<BranchPlan>::success(makeTakeSlotBranchPlan(*oDefaultSlot));

    return RpnCoercionResult<BranchPlan>::success(
        makeErrorBranchPlan(api::Error::NotAvailable));
}

// IFERROR(primary, alternate) / IFNA(primary, alternate).
// The caller evaluates primary first, then calls this with its error state.
[[nodiscard]] inline BranchPlan planIfErrorBranch(
    api::Error ePrimaryError,
    bool bNAOnly,
    std::size_t nAlternateSlot)
{
    using api::logic::IfErrorAction;
    const IfErrorAction eAction = api::logic::selectIfErrorAction(ePrimaryError, bNAOnly);

    BranchPlan aPlan;
    switch (eAction)
    {
        case IfErrorAction::KeepPrimary:
            aPlan.meDirective = BranchDirective::KeepPrimaryValue;
            return aPlan;
        case IfErrorAction::EvaluateAlternate:
            aPlan.meDirective = BranchDirective::EvaluateAlternate;
            aPlan.mnSlot = nAlternateSlot;
            return aPlan;
    }
    return makeErrorBranchPlan(api::Error::IllegalArgument);
}

// LET(name1, value1, name2, value2, ..., expression).
// Lightweight scalar binding resolver. The first admission is intentionally
// scalar-only: no shadowing, no forward references, no matrix-bound names.
struct LetBinding
{
    api::String msName;
    RpnValue maValue;
};

struct LetScope
{
    std::vector<LetBinding> maBindings;

    [[nodiscard]] std::optional<RpnValue> lookup(api::StringView aName) const
    {
        for (auto it = maBindings.rbegin(); it != maBindings.rend(); ++it)
        {
            if (it->msName == aName)
                return it->maValue;
        }
        return std::nullopt;
    }

    void bind(api::String sName, RpnValue aValue)
    {
        maBindings.push_back(LetBinding { std::move(sName), std::move(aValue) });
    }
};

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
