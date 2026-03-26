/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/String.hxx>

namespace spreadsheetengine::api::formulavalue
{

enum class ValueType : sal_uInt8
{
    Invalid,
    Value,
    String,
    Error
};

struct FormulaResultValue
{
    double mfValue = 0.0;
    String maString;
    bool mbMultiLine = false;
    ValueType meType = ValueType::Invalid;
    Error meError = Error::None;

    [[nodiscard]] constexpr bool operator==(const FormulaResultValue& rOther) const = default;
};

enum class CarrierType : sal_uInt8
{
    Unknown,
    EmptyCell,
    Double,
    Error,
    String,
    HybridCell,
    MatrixCell
};

[[nodiscard]] constexpr FormulaResultValue makeInvalidResult()
{
    return {};
}

[[nodiscard]] constexpr FormulaResultValue makeValueResult(double fValue)
{
    return { fValue, {}, false, ValueType::Value, Error::None };
}

[[nodiscard]] inline FormulaResultValue makeStringResult(StringView rValue, bool bMultiLine)
{
    return { 0.0, String(rValue), bMultiLine, ValueType::String, Error::None };
}

[[nodiscard]] constexpr FormulaResultValue makeErrorResult(Error eError)
{
    return { 0.0, {}, false, ValueType::Error, eError };
}

[[nodiscard]] constexpr bool isValueCarrierType(CarrierType eType, bool bEmptyDisplayedAsString)
{
    if (bEmptyDisplayedAsString)
        return true;

    switch (eType)
    {
        case CarrierType::Unknown:
        case CarrierType::EmptyCell:
        case CarrierType::Double:
        case CarrierType::Error:
            return true;
        default:
            return false;
    }
}

[[nodiscard]] constexpr bool isValueCarrierTypeNoError(CarrierType eType)
{
    switch (eType)
    {
        case CarrierType::Double:
        case CarrierType::EmptyCell:
            return true;
        default:
            return false;
    }
}

[[nodiscard]] constexpr bool isStringCarrierType(CarrierType eType)
{
    return eType == CarrierType::String || eType == CarrierType::HybridCell;
}

} // namespace spreadsheetengine::api::formulavalue

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
