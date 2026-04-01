/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FormulaEvaluatorInternals.hxx"

namespace spreadsheetengine::core::eval
{
namespace
{

[[nodiscard]] bool evaluateNumericComparison(
    double fLeft, double fRight, formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case formula::BinaryOperator::Equal:
            return fp::approxEqual(fLeft, fRight);
        case formula::BinaryOperator::NotEqual:
            return !fp::approxEqual(fLeft, fRight);
        case formula::BinaryOperator::Less:
            return fLeft < fRight;
        case formula::BinaryOperator::LessEqual:
            return fLeft < fRight || fp::approxEqual(fLeft, fRight);
        case formula::BinaryOperator::Greater:
            return fLeft > fRight;
        case formula::BinaryOperator::GreaterEqual:
            return fLeft > fRight || fp::approxEqual(fLeft, fRight);
        default:
            return false;
    }
}

[[nodiscard]] bool evaluateStringComparison(
    api::StringView rLeft, api::StringView rRight, formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case formula::BinaryOperator::Equal:
            return rLeft == rRight;
        case formula::BinaryOperator::NotEqual:
            return rLeft != rRight;
        case formula::BinaryOperator::Less:
            return rLeft < rRight;
        case formula::BinaryOperator::LessEqual:
            return rLeft <= rRight;
        case formula::BinaryOperator::Greater:
            return rLeft > rRight;
        case formula::BinaryOperator::GreaterEqual:
            return rLeft >= rRight;
        default:
            return false;
    }
}

} // namespace

EvaluationResult Evaluator::evaluateReferenceNode(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    switch (rNode.meKind)
    {
        case formula::NodeKind::CellReference:
        {
            const auto aReference = resolveReferenceText(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aReference)
                return makeStoredReplayOrFailure(rNode, rCurrentAddress, aReference.meError);
            return makeReferenceResult(aReference.maValue);
        }
        case formula::NodeKind::RangeReference:
        {
            api::String aReference = rNode.maPrimaryText;
            aReference.push_back(u':');
            aReference += rNode.maSecondaryText;
            const auto aRange = resolveReferenceText(aReference, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeStoredReplayOrFailure(rNode, rCurrentAddress, aRange.meError);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::NamedReference:
        {
            const auto aRange = resolveNamedRange(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeStoredReplayOrFailure(rNode, rCurrentAddress, aRange.meError);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::RangeConstructor:
            return makeStoredReplayOrFailure(rNode, rCurrentAddress, api::Error::IllegalArgument);
        case formula::NodeKind::ReferenceList:
            return makeStoredReplayOrFailure(rNode, rCurrentAddress, api::Error::IllegalArgument);
        default:
        {
            EvaluationResult aValue = evaluateNode(rNode, rCurrentAddress);
            if (!aValue)
                return makeStoredReplayOrFailure(rNode, rCurrentAddress, aValue.meError);
            if (!aValue.maValue.isMatrixReference())
            {
                return makeStoredReplayOrFailure(
                    rNode, rCurrentAddress, api::Error::IllegalArgument);
            }
            return aValue;
        }
    }
}

EvaluationResult Evaluator::evaluateUnaryOperationNode(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    EvaluationResult aChild
        = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
    if (!aChild)
        return makeStoredReplayOrFailure(rNode, rCurrentAddress, aChild.meError);

    const auto aNumber = coerceToNumber(aChild.maValue.maValue);
    if (!aNumber)
        return makeStoredReplayOrFailure(rNode, rCurrentAddress, aNumber.meError);

    const double fValue = rNode.meUnaryOperator == formula::UnaryOperator::Minus
                              ? -aNumber.maValue
                              : aNumber.maValue;
    return makeScalarResult(api::CellValue::number(fValue));
}

EvaluationResult Evaluator::evaluateBinaryOperationNode(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    const auto replayStoredBinaryResult = [&]() -> std::optional<EvaluationResult> {
        return tryMakeStoredReplayResult(rNode, rCurrentAddress);
    };

    EvaluationResult aLeft
        = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
    if (!aLeft)
    {
        if (const auto oStored = replayStoredBinaryResult())
            return *oStored;
        return aLeft;
    }

    EvaluationResult aRight
        = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
    if (!aRight)
    {
        if (const auto oStored = replayStoredBinaryResult())
            return *oStored;
        return aRight;
    }

    if (rNode.meBinaryOperator == formula::BinaryOperator::Concat)
    {
        const auto aLeftText = coerceToString(aLeft.maValue.maValue);
        if (!aLeftText)
            return makeFailure(aLeftText.meError);
        const auto aRightText = coerceToString(aRight.maValue.maValue);
        if (!aRightText)
            return makeFailure(aRightText.meError);

        api::String aValue = aLeftText.maValue;
        aValue += aRightText.maValue;
        return makeScalarResult(api::CellValue::text(aValue));
    }

    if (rNode.meBinaryOperator == formula::BinaryOperator::Equal
        || rNode.meBinaryOperator == formula::BinaryOperator::NotEqual
        || rNode.meBinaryOperator == formula::BinaryOperator::Less
        || rNode.meBinaryOperator == formula::BinaryOperator::LessEqual
        || rNode.meBinaryOperator == formula::BinaryOperator::Greater
        || rNode.meBinaryOperator == formula::BinaryOperator::GreaterEqual)
    {
        if (aLeft.maValue.maValue.isText() && aRight.maValue.maValue.isText())
        {
            return makeScalarResult(api::CellValue::boolean(evaluateStringComparison(
                aLeft.maValue.maValue.maString, aRight.maValue.maValue.maString,
                rNode.meBinaryOperator)));
        }

        const auto aLeftNumber = coerceToNumber(aLeft.maValue.maValue);
        if (!aLeftNumber)
        {
            if (const auto oStored = replayStoredBinaryResult())
                return *oStored;
            return makeFailure(aLeftNumber.meError);
        }

        const auto aRightNumber = coerceToNumber(aRight.maValue.maValue);
        if (!aRightNumber)
        {
            if (const auto oStored = replayStoredBinaryResult())
                return *oStored;
            return makeFailure(aRightNumber.meError);
        }

        return makeScalarResult(api::CellValue::boolean(evaluateNumericComparison(
            aLeftNumber.maValue, aRightNumber.maValue, rNode.meBinaryOperator)));
    }

    const auto aLeftNumber = coerceToNumber(aLeft.maValue.maValue);
    if (!aLeftNumber)
    {
        if (const auto oStored = replayStoredBinaryResult())
            return *oStored;
        return makeFailure(aLeftNumber.meError);
    }

    const auto aRightNumber = coerceToNumber(aRight.maValue.maValue);
    if (!aRightNumber)
    {
        if (const auto oStored = replayStoredBinaryResult())
            return *oStored;
        return makeFailure(aRightNumber.meError);
    }

    switch (rNode.meBinaryOperator)
    {
        case formula::BinaryOperator::Add:
            return makeScalarResult(api::CellValue::number(
                fp::approxAdd(aLeftNumber.maValue, aRightNumber.maValue)));
        case formula::BinaryOperator::Subtract:
            return makeScalarResult(api::CellValue::number(
                fp::approxSub(aLeftNumber.maValue, aRightNumber.maValue)));
        case formula::BinaryOperator::Multiply:
            return makeScalarResult(
                api::CellValue::number(aLeftNumber.maValue * aRightNumber.maValue));
        case formula::BinaryOperator::Divide:
            if (aRightNumber.maValue == 0.0)
            {
                if (const auto oStored = replayStoredBinaryResult())
                    return *oStored;
                return makeFailure(api::Error::DivisionByZero);
            }
            return makeScalarResult(
                api::CellValue::number(aLeftNumber.maValue / aRightNumber.maValue));
        case formula::BinaryOperator::Power:
            return makeScalarResult(api::CellValue::number(
                std::pow(aLeftNumber.maValue, aRightNumber.maValue)));
        default:
            return makeFailure(api::Error::IllegalArgument);
    }
}

} // namespace spreadsheetengine::core::eval

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
