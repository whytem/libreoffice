/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FormulaEvaluatorInternals.hxx"

#include <spreadsheetengine/runtime/RpnControlFlow.hxx>

#include <algorithm>
#include <array>
#include <vector>

namespace spreadsheetengine::core::eval
{
namespace
{

template <std::size_t N>
[[nodiscard]] bool matchesFunctionRegistry(
    api::StringView rFunctionName, const std::array<api::StringView, N>& rRegistry)
{
    return std::find(rRegistry.begin(), rRegistry.end(), rFunctionName) != rRegistry.end();
}

} // namespace

std::optional<EvaluationResult> Evaluator::tryEvaluateLogicalFamily(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    static constexpr std::array kLogicalFunctions{
        api::StringView(u"TRUE"),
        api::StringView(u"FALSE"),
        api::StringView(u"NA"),
        api::StringView(u"AND"),
        api::StringView(u"OR"),
        api::StringView(u"XOR"),
        api::StringView(u"NOT"),
        api::StringView(u"IFS"),
        api::StringView(u"COM.MICROSOFT.IFS"),
        api::StringView(u"SWITCH"),
        api::StringView(u"COM.MICROSOFT.SWITCH"),
    };
    if (!matchesFunctionRegistry(rFunctionName, kLogicalFunctions))
        return std::nullopt;
    return evaluateLogicalFamilyBody(rFunctionName, rNode, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateLogicalFamilyBody(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    const api::StringView aFunctionName = rFunctionName;
    const auto makeCellError = [&](api::Error eError) -> EvaluationResult {
        return makeScalarResult(api::CellValue::error(eError));
    };
    const auto evaluateLogicalFold = [&](bool bInitial, auto aFold) -> EvaluationResult {
        if (rNode.maChildren.empty())
            return makeCellError(api::Error::IllegalArgument);

        bool bResult = bInitial;
        bool bSawValue = false;
        for (const auto& pChild : rNode.maChildren)
        {
            const bool bReferenceLike = pChild->meKind == formula::NodeKind::CellReference
                                        || pChild->meKind == formula::NodeKind::RangeReference
                                        || pChild->meKind == formula::NodeKind::NamedReference;

            EvaluationResult aArgument = bReferenceLike
                                             ? evaluateReferenceNode(*pChild, rCurrentAddress)
                                             : evaluateNode(*pChild, rCurrentAddress);
            if (!aArgument)
                return makeCellError(aArgument.meError);

            if (aArgument.maValue.isMatrixReference())
            {
                const auto& rReference = aArgument.maValue.maReference;
                for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
                {
                    for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                    {
                        EvaluationResult aCell = materializeReferenceValue(rReference, nCol, nRow);
                        if (!aCell)
                            return aCell;
                        if (aCell.maValue.maValue.isEmpty() || aCell.maValue.maValue.isText())
                            continue;
                        const auto aBool = coerceToBoolean(aCell.maValue.maValue);
                        if (!aBool)
                            return makeCellError(aBool.meError);
                        bResult = aFold(bResult, aBool.maValue);
                        bSawValue = true;
                    }
                }
                continue;
            }

            const auto aBool = coerceToBoolean(aArgument.maValue.maValue);
            if (!aBool)
                return makeCellError(aBool.meError);
            bResult = aFold(bResult, aBool.maValue);
            bSawValue = true;
        }

        if (!bSawValue)
            return makeCellError(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::boolean(bResult));
    };

    if (aFunctionName == u"TRUE")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::boolean(true));
    }

    if (aFunctionName == u"FALSE")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::boolean(false));
    }

    if (aFunctionName == u"NA")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
    }

    if (aFunctionName == u"AND")
        return evaluateLogicalFold(true, [](bool bLeft, bool bRight) { return bLeft && bRight; });

    if (aFunctionName == u"OR")
        return evaluateLogicalFold(false, [](bool bLeft, bool bRight) { return bLeft || bRight; });

    if (aFunctionName == u"XOR")
        return evaluateLogicalFold(false, [](bool bLeft, bool bRight) { return bLeft != bRight; });

    if (aFunctionName == u"NOT")
    {
        if (rNode.maChildren.size() != 1)
            return makeCellError(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return makeCellError(aArgument.meError);
        const auto aBool = coerceToBoolean(aArgument.maValue.maValue);
        if (!aBool)
            return makeCellError(aBool.meError);
        return makeScalarResult(api::CellValue::boolean(!aBool.maValue));
    }

    if (aFunctionName == u"IFS" || aFunctionName == u"COM.MICROSOFT.IFS")
    {
        namespace serpn = spreadsheetengine::core::rpn;

        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        std::size_t nPairIndex = 0;
        while (nPairIndex * 2 < rNode.maChildren.size())
        {
            const std::size_t nConditionIndex = nPairIndex * 2;
            EvaluationResult aCondition = ensureScalarValue(
                *this, evaluateNode(*rNode.maChildren[nConditionIndex], rCurrentAddress));
            const serpn::RpnValue aConditionRpn
                = aCondition ? serpn::RpnValue::fromCellValue(aCondition.maValue.maValue)
                             : serpn::RpnValue::error(aCondition.meError);

            const auto aPlan = serpn::planIfsBranch(
                aConditionRpn, nPairIndex,
                static_cast<std::int16_t>(rNode.maChildren.size() - nConditionIndex - 1));
            if (aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready)
                return makeFailure(api::Error::IllegalArgument);

            const auto& rBranch = aPlan.maValue;
            switch (rBranch.meDirective)
            {
                case serpn::BranchDirective::TakeSlot:
                    if (rBranch.mnSlot == nPairIndex)
                        return evaluateNode(*rNode.maChildren[nConditionIndex + 1], rCurrentAddress);
                    nPairIndex = rBranch.mnSlot;
                    continue;
                case serpn::BranchDirective::PropagateError:
                    return makeScalarResult(api::CellValue::error(rBranch.meError));
                case serpn::BranchDirective::ReturnParameterExpected:
                    return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
                case serpn::BranchDirective::ReturnNotAvailable:
                    return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
                default:
                    return makeFailure(api::Error::IllegalArgument);
            }
        }

        return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
    }

    if (aFunctionName == u"SWITCH" || aFunctionName == u"COM.MICROSOFT.SWITCH")
    {
        namespace serpn = spreadsheetengine::core::rpn;

        if (rNode.maChildren.size() < 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aReference
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aReference)
            return aReference;

        std::vector<serpn::RpnValue> aCaseLabels;
        aCaseLabels.reserve((rNode.maChildren.size() - 1) / 2);
        for (std::size_t nIndex = 1; nIndex + 1 < rNode.maChildren.size(); nIndex += 2)
        {
            EvaluationResult aCase
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[nIndex], rCurrentAddress));
            if (!aCase)
            {
                if (nIndex + 2 >= rNode.maChildren.size())
                    return aCase;
                aCaseLabels.push_back(serpn::RpnValue::error(aCase.meError));
                continue;
            }

            aCaseLabels.push_back(serpn::RpnValue::fromCellValue(aCase.maValue.maValue));
        }

        const std::span<const serpn::RpnValue> aCaseSpan(aCaseLabels.data(), aCaseLabels.size());
        const bool bHasDefault = (rNode.maChildren.size() % 2) == 0;
        const auto aPlan = serpn::planSwitchBranch(
            serpn::RpnValue::fromCellValue(aReference.maValue.maValue), aCaseSpan,
            bHasDefault ? std::optional(aCaseLabels.size()) : std::nullopt);
        if (aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready)
            return makeFailure(api::Error::IllegalArgument);

        const auto& rBranch = aPlan.maValue;
        switch (rBranch.meDirective)
        {
            case serpn::BranchDirective::TakeSlot:
                if (rBranch.mnSlot < aCaseLabels.size())
                    return evaluateNode(*rNode.maChildren[2 + (rBranch.mnSlot * 2)], rCurrentAddress);
                if (bHasDefault && rBranch.mnSlot == aCaseLabels.size())
                    return evaluateNode(*rNode.maChildren.back(), rCurrentAddress);
                return makeFailure(api::Error::IllegalArgument);
            case serpn::BranchDirective::PropagateError:
                return makeScalarResult(api::CellValue::error(rBranch.meError));
            default:
                return makeFailure(api::Error::IllegalArgument);
        }
    }

    return makeFailure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::eval
