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

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/runtime/ScalarCoercion.hxx>

namespace spreadsheetengine::core::rpn
{

enum class RpnValueKind : std::uint8_t
{
    Empty,
    Number,
    Boolean,
    String,
    Error,
    Reference,
    Matrix
};

enum class RpnCoercionReadiness : std::uint8_t
{
    Ready,
    NeedsReferenceResolution,
    NeedsMatrixMaterialization
};

template <typename T> struct RpnCoercionResult
{
    T maValue {};
    api::Error meError = api::Error::None;
    RpnCoercionReadiness meReadiness = RpnCoercionReadiness::Ready;

    [[nodiscard]] constexpr bool ok() const
    {
        return meError == api::Error::None && meReadiness == RpnCoercionReadiness::Ready;
    }

    constexpr explicit operator bool() const { return ok(); }

    [[nodiscard]] static constexpr RpnCoercionResult success(const T& rValue)
    {
        return { rValue, api::Error::None, RpnCoercionReadiness::Ready };
    }

    [[nodiscard]] static constexpr RpnCoercionResult failure(api::Error eError)
    {
        return { T {}, eError, RpnCoercionReadiness::Ready };
    }

    [[nodiscard]] static constexpr RpnCoercionResult deferred(RpnCoercionReadiness eReadiness)
    {
        return { T {}, api::Error::None, eReadiness };
    }
};

struct RpnValue
{
    api::CellValue maScalar;
    api::ResolvedReference maReference;
    api::MatrixDimensions maMatrixDimensions;
    RpnValueKind meKind = RpnValueKind::Empty;

    [[nodiscard]] constexpr bool operator==(const RpnValue& rOther) const = default;

    [[nodiscard]] constexpr bool isScalar() const
    {
        return meKind != RpnValueKind::Reference && meKind != RpnValueKind::Matrix;
    }

    [[nodiscard]] constexpr bool isReference() const
    {
        return meKind == RpnValueKind::Reference;
    }

    [[nodiscard]] constexpr bool isMatrix() const
    {
        return meKind == RpnValueKind::Matrix;
    }

    [[nodiscard]] constexpr bool isError() const
    {
        return meKind == RpnValueKind::Error;
    }

    [[nodiscard]] static constexpr RpnValue empty()
    {
        return {};
    }

    [[nodiscard]] static constexpr RpnValue number(double fValue)
    {
        return fromCellValue(api::CellValue::number(fValue));
    }

    [[nodiscard]] static constexpr RpnValue boolean(bool bValue)
    {
        return fromCellValue(api::CellValue::boolean(bValue));
    }

    [[nodiscard]] static RpnValue text(api::StringView rValue)
    {
        return fromCellValue(api::CellValue::text(rValue));
    }

    [[nodiscard]] static constexpr RpnValue error(api::Error eError)
    {
        return fromCellValue(api::CellValue::error(eError));
    }

    [[nodiscard]] static constexpr RpnValue fromCellValue(const api::CellValue& rValue)
    {
        RpnValue aValue;
        aValue.maScalar = rValue;
        switch (rValue.meKind)
        {
            case api::CellValueKind::Empty:
                aValue.meKind = RpnValueKind::Empty;
                break;
            case api::CellValueKind::Number:
                aValue.meKind = RpnValueKind::Number;
                break;
            case api::CellValueKind::Boolean:
                aValue.meKind = RpnValueKind::Boolean;
                break;
            case api::CellValueKind::Text:
                aValue.meKind = RpnValueKind::String;
                break;
            case api::CellValueKind::Error:
                aValue.meKind = RpnValueKind::Error;
                break;
        }
        return aValue;
    }

    [[nodiscard]] static constexpr RpnValue reference(const api::ResolvedReference& rReference)
    {
        RpnValue aValue;
        aValue.maReference = rReference;
        aValue.meKind = RpnValueKind::Reference;
        return aValue;
    }

    [[nodiscard]] static constexpr RpnValue matrix(const api::MatrixDimensions& rDimensions)
    {
        RpnValue aValue;
        aValue.maMatrixDimensions = rDimensions;
        aValue.meKind = RpnValueKind::Matrix;
        return aValue;
    }
};

[[nodiscard]] constexpr RpnCoercionReadiness classifyCoercionReadiness(const RpnValue& rValue)
{
    if (rValue.isMatrix())
        return RpnCoercionReadiness::NeedsMatrixMaterialization;
    if (rValue.isReference())
        return RpnCoercionReadiness::NeedsReferenceResolution;
    return RpnCoercionReadiness::Ready;
}

[[nodiscard]] constexpr RpnCoercionReadiness combineCoercionReadiness(
    RpnCoercionReadiness eLeft, RpnCoercionReadiness eRight)
{
    if (eLeft == RpnCoercionReadiness::NeedsMatrixMaterialization
        || eRight == RpnCoercionReadiness::NeedsMatrixMaterialization)
    {
        return RpnCoercionReadiness::NeedsMatrixMaterialization;
    }

    if (eLeft == RpnCoercionReadiness::NeedsReferenceResolution
        || eRight == RpnCoercionReadiness::NeedsReferenceResolution)
    {
        return RpnCoercionReadiness::NeedsReferenceResolution;
    }

    return RpnCoercionReadiness::Ready;
}

[[nodiscard]] constexpr RpnCoercionReadiness classifyUnaryScalarOperatorReadiness(
    const RpnValue& rOperand)
{
    return classifyCoercionReadiness(rOperand);
}

[[nodiscard]] constexpr RpnCoercionReadiness classifyBinaryScalarOperatorReadiness(
    const RpnValue& rLeft, const RpnValue& rRight)
{
    return combineCoercionReadiness(
        classifyCoercionReadiness(rLeft), classifyCoercionReadiness(rRight));
}

[[nodiscard]] inline RpnCoercionResult<double> coerceToNumber(const RpnValue& rValue)
{
    const auto eReadiness = classifyCoercionReadiness(rValue);
    if (eReadiness != RpnCoercionReadiness::Ready)
        return RpnCoercionResult<double>::deferred(eReadiness);

    const auto aScalar = spreadsheetengine::core::coercion::coerceToNumber(rValue.maScalar);
    if (!aScalar)
        return RpnCoercionResult<double>::failure(aScalar.meError);
    return RpnCoercionResult<double>::success(aScalar.maValue);
}

[[nodiscard]] inline RpnCoercionResult<bool> coerceToBoolean(const RpnValue& rValue)
{
    const auto eReadiness = classifyCoercionReadiness(rValue);
    if (eReadiness != RpnCoercionReadiness::Ready)
        return RpnCoercionResult<bool>::deferred(eReadiness);

    const auto aScalar = spreadsheetengine::core::coercion::coerceToBoolean(rValue.maScalar);
    if (!aScalar)
        return RpnCoercionResult<bool>::failure(aScalar.meError);
    return RpnCoercionResult<bool>::success(aScalar.maValue);
}

[[nodiscard]] inline RpnCoercionResult<api::String> coerceToString(const RpnValue& rValue)
{
    const auto eReadiness = classifyCoercionReadiness(rValue);
    if (eReadiness != RpnCoercionReadiness::Ready)
        return RpnCoercionResult<api::String>::deferred(eReadiness);

    const auto aScalar = spreadsheetengine::core::coercion::coerceToString(rValue.maScalar);
    if (!aScalar)
        return RpnCoercionResult<api::String>::failure(aScalar.meError);
    return RpnCoercionResult<api::String>::success(aScalar.maValue);
}

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
