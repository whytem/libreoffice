/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Matrix.hxx>
#include <spreadsheetengine/api/String.hxx>

#include <sal/types.h>

namespace spreadsheetengine::api
{

using SheetId = sal_Int32;
using ColumnIndex = sal_Int32;
using RowIndex = sal_Int32;
using FormatIndex = sal_uInt32;

struct CellAddress
{
    SheetId mnSheet = 0;
    ColumnIndex mnColumn = 0;
    RowIndex mnRow = 0;

    [[nodiscard]] constexpr bool operator==(const CellAddress& rOther) const = default;
};

struct CellRange
{
    CellAddress maStart;
    CellAddress maEnd;

    [[nodiscard]] constexpr bool operator==(const CellRange& rOther) const = default;

    [[nodiscard]] constexpr bool isSingleCell() const
    {
        return maStart == maEnd;
    }

    [[nodiscard]] constexpr bool isNormalized() const
    {
        return maStart.mnSheet == maEnd.mnSheet && maStart.mnColumn <= maEnd.mnColumn
               && maStart.mnRow <= maEnd.mnRow;
    }

    [[nodiscard]] constexpr ColumnIndex columnCount() const
    {
        if (!isNormalized())
            return 0;
        return maEnd.mnColumn - maStart.mnColumn + 1;
    }

    [[nodiscard]] constexpr RowIndex rowCount() const
    {
        if (!isNormalized())
            return 0;
        return maEnd.mnRow - maStart.mnRow + 1;
    }

    [[nodiscard]] constexpr bool containsOffset(ColumnIndex nColumnOffset, RowIndex nRowOffset) const
    {
        return nColumnOffset >= 0 && nRowOffset >= 0 && nColumnOffset < columnCount()
               && nRowOffset < rowCount();
    }
};

struct NumberParseResult
{
    double mfValue = 0.0;
    FormatIndex mnFormat = 0;
    enum class Kind : sal_uInt8
    {
        Number,
        Date,
        Time,
        DateTime
    };
    Kind meKind = Kind::Number;

    [[nodiscard]] constexpr bool operator==(const NumberParseResult& rOther) const = default;
};

enum class NumberParseMode : sal_uInt8
{
    General,
    LaxTime
};

struct ResolvedReference
{
    CellRange maRange;

    [[nodiscard]] constexpr bool isSingleCell() const { return maRange.isSingleCell(); }

    [[nodiscard]] constexpr bool isNormalized() const { return maRange.isNormalized(); }

    [[nodiscard]] constexpr MatrixDimensions matrixDimensions() const
    {
        return { maRange.columnCount(), maRange.rowCount() };
    }

    [[nodiscard]] constexpr bool containsOffset(
        ColumnIndex nColumnOffset, RowIndex nRowOffset) const
    {
        return maRange.containsOffset(nColumnOffset, nRowOffset);
    }

    [[nodiscard]] constexpr CellAddress addressAt(
        ColumnIndex nColumnOffset, RowIndex nRowOffset) const
    {
        return { maRange.maStart.mnSheet, maRange.maStart.mnColumn + nColumnOffset,
            maRange.maStart.mnRow + nRowOffset };
    }
};

enum class CellValueViewKind : sal_uInt8
{
    Scalar,
    MatrixReference
};

enum class CellValueKind : sal_uInt8
{
    Empty,
    Number,
    Boolean,
    Text,
    Error
};

struct CellValue
{
    double mfNumber = 0.0;
    String maString;
    Error meError = Error::None;
    CellValueKind meKind = CellValueKind::Empty;

    [[nodiscard]] constexpr bool operator==(const CellValue& rOther) const = default;

    [[nodiscard]] constexpr bool isEmpty() const { return meKind == CellValueKind::Empty; }
    [[nodiscard]] constexpr bool isNumber() const { return meKind == CellValueKind::Number; }
    [[nodiscard]] constexpr bool isBoolean() const { return meKind == CellValueKind::Boolean; }
    [[nodiscard]] constexpr bool isText() const { return meKind == CellValueKind::Text; }
    [[nodiscard]] constexpr bool isError() const { return meKind == CellValueKind::Error; }

    [[nodiscard]] static constexpr CellValue empty()
    {
        return {};
    }

    [[nodiscard]] static constexpr CellValue number(double fValue)
    {
        CellValue aValue;
        aValue.mfNumber = fValue;
        aValue.meKind = CellValueKind::Number;
        return aValue;
    }

    [[nodiscard]] static constexpr CellValue boolean(bool bValue)
    {
        CellValue aValue;
        aValue.mfNumber = bValue ? 1.0 : 0.0;
        aValue.meKind = CellValueKind::Boolean;
        return aValue;
    }

    [[nodiscard]] static CellValue text(StringView rValue)
    {
        CellValue aValue;
        aValue.maString = String(rValue);
        aValue.meKind = CellValueKind::Text;
        return aValue;
    }

    [[nodiscard]] static constexpr CellValue error(Error eError)
    {
        CellValue aValue;
        aValue.meError = eError;
        aValue.meKind = CellValueKind::Error;
        return aValue;
    }
};

struct CellValueView
{
    CellValue maValue;
    ResolvedReference maReference;
    CellValueViewKind meKind = CellValueViewKind::Scalar;

    [[nodiscard]] constexpr bool operator==(const CellValueView& rOther) const = default;

    [[nodiscard]] constexpr bool isScalar() const
    {
        return meKind == CellValueViewKind::Scalar;
    }

    [[nodiscard]] constexpr bool isMatrixReference() const
    {
        return meKind == CellValueViewKind::MatrixReference;
    }

    [[nodiscard]] static constexpr CellValueView scalar(const CellValue& rValue)
    {
        CellValueView aView;
        aView.maValue = rValue;
        return aView;
    }

    [[nodiscard]] static constexpr CellValueView matrixReference(
        const ResolvedReference& rReference)
    {
        CellValueView aView;
        aView.maReference = rReference;
        aView.meKind = CellValueViewKind::MatrixReference;
        return aView;
    }
};

class CellReader
{
public:
    virtual ~CellReader() = default;

    [[nodiscard]] virtual ValueResult<CellValue> getCellValue(const CellAddress& rAddress) const = 0;

    [[nodiscard]] virtual ValueResult<CellValue> getRangeValue(
        const CellRange& rRange, ColumnIndex nColumnOffset, RowIndex nRowOffset) const = 0;
};

class ReferenceResolver
{
public:
    virtual ~ReferenceResolver() = default;

    [[nodiscard]] virtual ValueResult<ResolvedReference> resolveReference(
        const CellRange& rRange) const = 0;
};

class TextCoercion
{
public:
    virtual ~TextCoercion() = default;

    [[nodiscard]] virtual ValueResult<NumberParseResult> parseNumber(
        StringView rValue, NumberParseMode eMode = NumberParseMode::General) const = 0;
};

class ValueFormatting
{
public:
    virtual ~ValueFormatting() = default;

    [[nodiscard]] virtual ValueResult<String> formatNumber(
        double fValue, FormatIndex nFormat = 0) const = 0;
};

class WorkbookInfo
{
public:
    virtual ~WorkbookInfo() = default;

    [[nodiscard]] virtual bool hasSheet(SheetId nSheet) const = 0;
    [[nodiscard]] virtual sal_Int32 sheetCount() const = 0;
    [[nodiscard]] virtual ValueResult<String> getSheetName(SheetId nSheet) const = 0;
};

class RuntimeEnvironment
{
public:
    virtual ~RuntimeEnvironment() = default;

    [[nodiscard]] virtual DateParts getNullDate() const = 0;
    [[nodiscard]] virtual String getLocaleTag() const = 0;
};

class EvaluationHost : public CellReader,
                       public ReferenceResolver,
                       public TextCoercion,
                       public ValueFormatting,
                       public WorkbookInfo,
                       public RuntimeEnvironment
{
public:
    ~EvaluationHost() override = default;
};

} // namespace spreadsheetengine::api

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
