/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/String.hxx>

#include <spreadsheetengine/api/Types.hxx>

namespace spreadsheetengine::api
{

using MatrixSize = sal_Int32;

enum class MatrixValueType : std::uint8_t
{
    Value = 0x00,
    Boolean = 0x01,
    Text = 0x02,
    Empty = Text | 0x04,
    EmptyPath = Empty | 0x08,
    NonvalueMask = EmptyPath
};

[[nodiscard]] constexpr bool isValueType(MatrixValueType eType)
{
    return static_cast<std::uint8_t>(eType) <= static_cast<std::uint8_t>(MatrixValueType::Boolean);
}

[[nodiscard]] constexpr bool isBooleanType(MatrixValueType eType)
{
    return eType == MatrixValueType::Boolean;
}

[[nodiscard]] constexpr bool isNonValueType(MatrixValueType eType)
{
    return (static_cast<std::uint8_t>(eType) & static_cast<std::uint8_t>(MatrixValueType::NonvalueMask))
           != 0;
}

[[nodiscard]] constexpr bool isRealStringType(MatrixValueType eType)
{
    return (static_cast<std::uint8_t>(eType) & static_cast<std::uint8_t>(MatrixValueType::NonvalueMask))
           == static_cast<std::uint8_t>(MatrixValueType::Text);
}

[[nodiscard]] constexpr bool isEmptyType(MatrixValueType eType)
{
    return (static_cast<std::uint8_t>(eType) & static_cast<std::uint8_t>(MatrixValueType::NonvalueMask))
           == static_cast<std::uint8_t>(MatrixValueType::Empty);
}

[[nodiscard]] constexpr bool isEmptyPathType(MatrixValueType eType)
{
    return (static_cast<std::uint8_t>(eType) & static_cast<std::uint8_t>(MatrixValueType::NonvalueMask))
           == static_cast<std::uint8_t>(MatrixValueType::EmptyPath);
}

struct MatrixDimensions
{
    MatrixSize mnColumns = 0;
    MatrixSize mnRows = 0;

    [[nodiscard]] constexpr bool operator==(const MatrixDimensions& rOther) const = default;

    [[nodiscard]] constexpr bool isAllocated() const { return mnColumns >= 0 && mnRows >= 0; }

    [[nodiscard]] constexpr bool isEmpty() const { return mnColumns == 0 || mnRows == 0; }

    [[nodiscard]] constexpr std::uint64_t elementCount() const
    {
        if (mnColumns <= 0 || mnRows <= 0)
            return 0;

        return static_cast<std::uint64_t>(mnColumns) * static_cast<std::uint64_t>(mnRows);
    }
};

struct MatrixCoordinate
{
    MatrixSize mnColumn = 0;
    MatrixSize mnRow = 0;

    [[nodiscard]] constexpr bool operator==(const MatrixCoordinate& rOther) const = default;
};

[[nodiscard]] constexpr bool isValidCoordinate(
    const MatrixDimensions& rDimensions, const MatrixCoordinate& rCoordinate)
{
    return rCoordinate.mnColumn >= 0 && rCoordinate.mnRow >= 0
           && rCoordinate.mnColumn < rDimensions.mnColumns
           && rCoordinate.mnRow < rDimensions.mnRows;
}

[[nodiscard]] constexpr bool isRowVector(const MatrixDimensions& rDimensions)
{
    return rDimensions.mnColumns > 0 && rDimensions.mnRows == 1;
}

[[nodiscard]] constexpr bool isColumnVector(const MatrixDimensions& rDimensions)
{
    return rDimensions.mnColumns == 1 && rDimensions.mnRows > 0;
}

[[nodiscard]] constexpr bool normalizeReplicatedCoordinate(
    const MatrixDimensions& rDimensions, MatrixCoordinate& rCoordinate)
{
    if (isValidCoordinate(rDimensions, rCoordinate))
        return true;

    if (isRowVector(rDimensions) && rCoordinate.mnColumn >= 0
        && rCoordinate.mnColumn < rDimensions.mnColumns)
    {
        rCoordinate.mnRow = 0;
        return true;
    }

    if (isColumnVector(rDimensions) && rCoordinate.mnRow >= 0
        && rCoordinate.mnRow < rDimensions.mnRows)
    {
        rCoordinate.mnColumn = 0;
        return true;
    }

    return false;
}

struct MatrixValue
{
    double mfValue = 0.0;
    String maString;
    MatrixValueType meType = MatrixValueType::Empty;

    [[nodiscard]] bool operator==(const MatrixValue& rOther) const
    {
        return mfValue == rOther.mfValue && maString == rOther.maString && meType == rOther.meType;
    }

    [[nodiscard]] bool operator!=(const MatrixValue& rOther) const
    {
        return !(*this == rOther);
    }
};

} // namespace spreadsheetengine::api

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
