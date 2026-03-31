/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FormulaEvaluatorInternals.hxx"

#include <algorithm>
#include <array>

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
    const auto evaluateLogicalFold = [&](bool bInitial, auto aFold) -> EvaluationResult {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

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
                return aArgument;

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
                            return makeFailure(aBool.meError);
                        bResult = aFold(bResult, aBool.maValue);
                        bSawValue = true;
                    }
                }
                continue;
            }

            const auto aBool = coerceToBoolean(aArgument.maValue.maValue);
            if (!aBool)
                return makeFailure(aBool.meError);
            bResult = aFold(bResult, aBool.maValue);
            bSawValue = true;
        }

        if (!bSawValue)
            return makeFailure(api::Error::IllegalArgument);

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
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;
        const auto aBool = coerceToBoolean(aArgument.maValue.maValue);
        if (!aBool)
            return makeFailure(aBool.meError);
        return makeScalarResult(api::CellValue::boolean(!aBool.maValue));
    }

    if (aFunctionName == u"IFS" || aFunctionName == u"COM.MICROSOFT.IFS")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        for (std::size_t nIndex = 0; nIndex < rNode.maChildren.size(); nIndex += 2)
        {
            if (nIndex >= rNode.maChildren.size())
                break;

            EvaluationResult aCondition
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[nIndex], rCurrentAddress));
            const std::int16_t nRemaining = static_cast<std::int16_t>(rNode.maChildren.size() - nIndex - 1);

            bool bCondition = false;
            bool bConditionError = false;
            if (!aCondition)
                bConditionError = true;
            else
            {
                const auto aBool = coerceToBoolean(aCondition.maValue.maValue);
                if (!aBool)
                    bConditionError = true;
                else
                    bCondition = aBool.maValue;
            }

            switch (api::logic::evaluateIfsCondition(bCondition, bConditionError, nRemaining))
            {
                case api::logic::IfsAction::SelectCurrentResult:
                    return evaluateNode(*rNode.maChildren[nIndex + 1], rCurrentAddress);
                case api::logic::IfsAction::SkipCurrentResult:
                    break;
                case api::logic::IfsAction::ReturnParameterExpected:
                    return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
                case api::logic::IfsAction::ReturnNotAvailable:
                    return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
                case api::logic::IfsAction::ReturnNoValue:
                    return makeScalarResult(api::CellValue::error(api::Error::NoValue));
            }
        }

        return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
    }

    if (aFunctionName == u"SWITCH" || aFunctionName == u"COM.MICROSOFT.SWITCH")
    {
        if (rNode.maChildren.size() < 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aReference
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aReference)
            return aReference;

        const api::CellValue aReferenceValue = aReference.maValue.maValue;
        const bool bReferenceIsText = aReferenceValue.isText() || aReferenceValue.isEmpty();
        std::size_t nIndex = 1;
        while (nIndex + 1 < rNode.maChildren.size())
        {
            EvaluationResult aCase
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[nIndex], rCurrentAddress));
            if (!aCase)
            {
                if (nIndex + 2 >= rNode.maChildren.size())
                    return aCase;
                nIndex += 2;
                continue;
            }

            bool bMatched = false;
            if (bReferenceIsText)
            {
                const auto aReferenceText = coerceToString(aReferenceValue);
                const auto aCaseText = coerceToString(aCase.maValue.maValue);
                if (!aReferenceText || !aCaseText)
                    return makeScalarResult(api::CellValue::error(api::Error::NoValue));
                bMatched = sequery::compareFoldedText(aReferenceText.maValue, aCaseText.maValue) == 0;
            }
            else
            {
                const auto aReferenceNumber = coerceToNumber(aReferenceValue);
                const auto aCaseNumber = coerceToNumber(aCase.maValue.maValue);
                if (!aReferenceNumber || !aCaseNumber)
                {
                    if (nIndex + 2 >= rNode.maChildren.size())
                        return makeScalarResult(api::CellValue::error(api::Error::NoValue));
                    nIndex += 2;
                    continue;
                }
                bMatched = fp::approxEqual(aReferenceNumber.maValue, aCaseNumber.maValue);
            }

            if (bMatched)
                return evaluateNode(*rNode.maChildren[nIndex + 1], rCurrentAddress);

            nIndex += 2;
        }

        if (nIndex < rNode.maChildren.size())
            return evaluateNode(*rNode.maChildren[nIndex], rCurrentAddress);

        return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
    }

    return makeFailure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::eval
