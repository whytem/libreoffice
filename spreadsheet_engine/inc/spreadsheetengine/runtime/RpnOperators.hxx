/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cmath>
#include <cstdint>

#include <spreadsheetengine/runtime/FloatingPoint.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

namespace spreadsheetengine::core::rpn
{

enum class UnaryNumericOperator : std::uint8_t
{
    Plus,
    Minus
};

enum class BinaryScalarOperator : std::uint8_t
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Power,
    Concat,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual
};

[[nodiscard]] constexpr bool isComparisonOperator(BinaryScalarOperator eOperator)
{
    switch (eOperator)
    {
        case BinaryScalarOperator::Equal:
        case BinaryScalarOperator::NotEqual:
        case BinaryScalarOperator::Less:
        case BinaryScalarOperator::LessEqual:
        case BinaryScalarOperator::Greater:
        case BinaryScalarOperator::GreaterEqual:
            return true;
        default:
            return false;
    }
}

[[nodiscard]] constexpr bool isConcatenationOperator(BinaryScalarOperator eOperator)
{
    return eOperator == BinaryScalarOperator::Concat;
}

[[nodiscard]] inline RpnCoercionResult<RpnValue> evaluateUnaryNumericOperator(
    UnaryNumericOperator eOperator, const RpnValue& rOperand)
{
    const auto aNumber = coerceToNumber(rOperand);
    if (!aNumber)
    {
        if (aNumber.meReadiness != RpnCoercionReadiness::Ready)
            return RpnCoercionResult<RpnValue>::deferred(aNumber.meReadiness);
        return RpnCoercionResult<RpnValue>::failure(aNumber.meError);
    }

    const double fValue
        = eOperator == UnaryNumericOperator::Minus ? -aNumber.maValue : aNumber.maValue;
    return RpnCoercionResult<RpnValue>::success(RpnValue::number(fValue));
}

[[nodiscard]] inline bool evaluateNumericComparison(
    double fLeft, double fRight, BinaryScalarOperator eOperator)
{
    switch (eOperator)
    {
        case BinaryScalarOperator::Equal:
            return fp::approxEqual(fLeft, fRight);
        case BinaryScalarOperator::NotEqual:
            return !fp::approxEqual(fLeft, fRight);
        case BinaryScalarOperator::Less:
            return fLeft < fRight;
        case BinaryScalarOperator::LessEqual:
            return fLeft < fRight || fp::approxEqual(fLeft, fRight);
        case BinaryScalarOperator::Greater:
            return fLeft > fRight;
        case BinaryScalarOperator::GreaterEqual:
            return fLeft > fRight || fp::approxEqual(fLeft, fRight);
        default:
            return false;
    }
}

[[nodiscard]] inline bool evaluateStringComparison(
    api::StringView rLeft, api::StringView rRight, BinaryScalarOperator eOperator)
{
    switch (eOperator)
    {
        case BinaryScalarOperator::Equal:
            return rLeft == rRight;
        case BinaryScalarOperator::NotEqual:
            return rLeft != rRight;
        case BinaryScalarOperator::Less:
            return rLeft < rRight;
        case BinaryScalarOperator::LessEqual:
            return rLeft <= rRight;
        case BinaryScalarOperator::Greater:
            return rLeft > rRight;
        case BinaryScalarOperator::GreaterEqual:
            return rLeft >= rRight;
        default:
            return false;
    }
}

[[nodiscard]] inline bool evaluateOrderedComparison(int nOrder, BinaryScalarOperator eOperator)
{
    switch (eOperator)
    {
        case BinaryScalarOperator::Equal:
            return nOrder == 0;
        case BinaryScalarOperator::NotEqual:
            return nOrder != 0;
        case BinaryScalarOperator::Less:
            return nOrder < 0;
        case BinaryScalarOperator::LessEqual:
            return nOrder <= 0;
        case BinaryScalarOperator::Greater:
            return nOrder > 0;
        case BinaryScalarOperator::GreaterEqual:
            return nOrder >= 0;
        default:
            return false;
    }
}

[[nodiscard]] inline RpnCoercionResult<RpnValue> evaluateBinaryScalarOperator(
    BinaryScalarOperator eOperator, const RpnValue& rLeft, const RpnValue& rRight)
{
    const auto eReadiness = classifyBinaryScalarOperatorReadiness(rLeft, rRight);
    if (eReadiness != RpnCoercionReadiness::Ready)
        return RpnCoercionResult<RpnValue>::deferred(eReadiness);

    if (isConcatenationOperator(eOperator))
    {
        const auto aLeftText = coerceToString(rLeft);
        if (!aLeftText)
            return RpnCoercionResult<RpnValue>::failure(aLeftText.meError);

        const auto aRightText = coerceToString(rRight);
        if (!aRightText)
            return RpnCoercionResult<RpnValue>::failure(aRightText.meError);

        api::String aResult = aLeftText.maValue;
        aResult += aRightText.maValue;
        return RpnCoercionResult<RpnValue>::success(RpnValue::text(aResult));
    }

    if (isComparisonOperator(eOperator))
    {
        if (rLeft.meKind == RpnValueKind::Error)
            return RpnCoercionResult<RpnValue>::failure(rLeft.maScalar.meError);
        if (rRight.meKind == RpnValueKind::Error)
            return RpnCoercionResult<RpnValue>::failure(rRight.maScalar.meError);

        const auto compareWithEmpty = [&](const RpnValue& rOther, bool bEmptyIsLeft)
            -> RpnCoercionResult<RpnValue> {
            int nOrder = 0;
            if (rOther.meKind == RpnValueKind::Empty)
            {
                nOrder = 0;
            }
            else if (rOther.meKind == RpnValueKind::String)
            {
                nOrder = rOther.maScalar.maString.empty() ? 0 : -1;
            }
            else
            {
                const auto aOtherNumber = coerceToNumber(rOther);
                if (!aOtherNumber)
                    return RpnCoercionResult<RpnValue>::failure(aOtherNumber.meError);
                if (!fp::approxEqual(aOtherNumber.maValue, 0.0))
                    nOrder = aOtherNumber.maValue < 0.0 ? 1 : -1;
            }

            if (!bEmptyIsLeft)
                nOrder = -nOrder;
            return RpnCoercionResult<RpnValue>::success(
                RpnValue::boolean(evaluateOrderedComparison(nOrder, eOperator)));
        };

        if (rLeft.meKind == RpnValueKind::Empty)
            return compareWithEmpty(rRight, true);
        if (rRight.meKind == RpnValueKind::Empty)
            return compareWithEmpty(rLeft, false);

        if (rLeft.meKind == RpnValueKind::String && rRight.meKind == RpnValueKind::String)
        {
            return RpnCoercionResult<RpnValue>::success(RpnValue::boolean(
                evaluateStringComparison(rLeft.maScalar.maString, rRight.maScalar.maString, eOperator)));
        }

        if (rLeft.meKind == RpnValueKind::String || rRight.meKind == RpnValueKind::String)
        {
            const int nOrder = rLeft.meKind == RpnValueKind::String ? 1 : -1;
            return RpnCoercionResult<RpnValue>::success(
                RpnValue::boolean(evaluateOrderedComparison(nOrder, eOperator)));
        }

        const auto aLeftNumber = coerceToNumber(rLeft);
        if (!aLeftNumber)
            return RpnCoercionResult<RpnValue>::failure(aLeftNumber.meError);

        const auto aRightNumber = coerceToNumber(rRight);
        if (!aRightNumber)
            return RpnCoercionResult<RpnValue>::failure(aRightNumber.meError);

        return RpnCoercionResult<RpnValue>::success(RpnValue::boolean(
            evaluateNumericComparison(aLeftNumber.maValue, aRightNumber.maValue, eOperator)));
    }

    const auto aLeftNumber = coerceToNumber(rLeft);
    if (!aLeftNumber)
        return RpnCoercionResult<RpnValue>::failure(aLeftNumber.meError);

    const auto aRightNumber = coerceToNumber(rRight);
    if (!aRightNumber)
        return RpnCoercionResult<RpnValue>::failure(aRightNumber.meError);

    switch (eOperator)
    {
        case BinaryScalarOperator::Add:
            return RpnCoercionResult<RpnValue>::success(
                RpnValue::number(fp::approxAdd(aLeftNumber.maValue, aRightNumber.maValue)));
        case BinaryScalarOperator::Subtract:
            return RpnCoercionResult<RpnValue>::success(
                RpnValue::number(fp::approxSub(aLeftNumber.maValue, aRightNumber.maValue)));
        case BinaryScalarOperator::Multiply:
            return RpnCoercionResult<RpnValue>::success(
                RpnValue::number(aLeftNumber.maValue * aRightNumber.maValue));
        case BinaryScalarOperator::Divide:
            if (aRightNumber.maValue == 0.0)
                return RpnCoercionResult<RpnValue>::failure(api::Error::DivisionByZero);
            return RpnCoercionResult<RpnValue>::success(
                RpnValue::number(aLeftNumber.maValue / aRightNumber.maValue));
        case BinaryScalarOperator::Power:
            return RpnCoercionResult<RpnValue>::success(
                RpnValue::number(std::pow(aLeftNumber.maValue, aRightNumber.maValue)));
        default:
            return RpnCoercionResult<RpnValue>::failure(api::Error::IllegalArgument);
    }
}

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
