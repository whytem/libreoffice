/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/MathBitwise.hxx>
#include <cstdint>

#include <cmath>
#include <optional>

#include <spreadsheetengine/runtime/FloatingPoint.hxx>
#include <spreadsheetengine/api/Types.hxx>

namespace spreadsheetengine::core::math
{

namespace
{

constexpr double BIT_OPERAND_LIMIT = 281474976710656.0; // 2^48

std::optional<std::uint64_t> normalizeBitOperand(double fValue)
{
    const double fNormalized = fp::approxFloor(fValue);
    if (fNormalized >= BIT_OPERAND_LIMIT || fNormalized < 0.0)
        return std::nullopt;

    return static_cast<std::uint64_t>(fNormalized);
}

}

std::optional<double> computeBitAnd(double fLeft, double fRight)
{
    const auto nLeft = normalizeBitOperand(fLeft);
    const auto nRight = normalizeBitOperand(fRight);
    if (!nLeft || !nRight)
        return std::nullopt;

    return static_cast<double>(*nLeft & *nRight);
}

std::optional<double> computeBitOr(double fLeft, double fRight)
{
    const auto nLeft = normalizeBitOperand(fLeft);
    const auto nRight = normalizeBitOperand(fRight);
    if (!nLeft || !nRight)
        return std::nullopt;

    return static_cast<double>(*nLeft | *nRight);
}

std::optional<double> computeBitXor(double fLeft, double fRight)
{
    const auto nLeft = normalizeBitOperand(fLeft);
    const auto nRight = normalizeBitOperand(fRight);
    if (!nLeft || !nRight)
        return std::nullopt;

    return static_cast<double>(*nLeft ^ *nRight);
}

std::optional<double> computeBitLeftShift(double fValue, double fShift)
{
    const auto nValue = normalizeBitOperand(fValue);
    if (!nValue)
        return std::nullopt;

    const double fNormalizedValue = static_cast<double>(*nValue);
    const double fNormalizedShift = fp::approxFloor(fShift);

    if (fNormalizedShift < 0.0)
        return fp::approxFloor(fNormalizedValue / std::pow(2.0, -fNormalizedShift));
    if (fNormalizedShift == 0.0)
        return fNormalizedValue;
    return fNormalizedValue * std::pow(2.0, fNormalizedShift);
}

std::optional<double> computeBitRightShift(double fValue, double fShift)
{
    const auto nValue = normalizeBitOperand(fValue);
    if (!nValue)
        return std::nullopt;

    const double fNormalizedValue = static_cast<double>(*nValue);
    const double fNormalizedShift = fp::approxFloor(fShift);

    if (fNormalizedShift < 0.0)
        return fNormalizedValue * std::pow(2.0, -fNormalizedShift);
    if (fNormalizedShift == 0.0)
        return fNormalizedValue;
    return fp::approxFloor(fNormalizedValue / std::pow(2.0, fNormalizedShift));
}

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
