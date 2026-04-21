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
#include <variant>

#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Matrix.hxx>
#include <spreadsheetengine/api/String.hxx>

#include <spreadsheetengine/api/Types.hxx>

namespace spreadsheetengine::api::query
{
enum class SearchType : std::uint8_t;
}

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
    enum class Kind : std::uint8_t
    {
        Number,
        Date,
        Time,
        DateTime
    };
    Kind meKind = Kind::Number;

    [[nodiscard]] constexpr bool operator==(const NumberParseResult& rOther) const = default;
};

enum class NumberParseMode : std::uint8_t
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

enum class CellValueViewKind : std::uint8_t
{
    Scalar,
    MatrixReference
};

enum class CellValueKind : std::uint8_t
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
    [[nodiscard]] virtual query::SearchType getSearchType() const = 0;
};

// Spill-range allocation contract.  See RpnSpill.hxx for the engine-side
// planners.  The allocator surfaces `#SPILL!` on collision — this is NEW
// behavior relative to legacy Calc, which never emits `#SPILL!` from
// member functions.  Phase 5A admissions do not yet reach the allocator
// (they PushMatrix directly like legacy ScFilter / ScSort), so only the
// contract is established here; the host implementation for spill lands
// with Phase 5B.
enum class SpillAllocationError : std::uint8_t
{
    Collision,
    OutOfBounds,
    InvalidShape,
    InvalidRequest
};

class SpillRangeAllocator
{
public:
    virtual ~SpillRangeAllocator() = default;

    // Ask the host to reserve a contiguous range of cells for a dynamic-
    // array result anchored at `rAnchor`.  The host checks the target
    // rectangle for collisions and returns either the allocated CellRange
    // or a SpillAllocationError describing why the request cannot be
    // satisfied.  On Collision the dispatch bridge translates the result
    // to `#SPILL!`; OutOfBounds / InvalidShape / InvalidRequest surface as
    // IllegalArgument so the caller can decline cleanly.
    [[nodiscard]] virtual std::variant<CellRange, SpillAllocationError> allocateSpillRange(
        const CellAddress& rAnchor, const MatrixDimensions& rDimensions) = 0;

    // Non-destructive collision probe.  Returns true when any cell inside
    // `rRange` (other than `rAnchor` itself) is non-empty.  Callers use
    // this as a planning prerequisite before asking the host to commit to
    // a particular shape.
    [[nodiscard]] virtual bool checkSpillCollision(const CellRange& rRange) const = 0;

    // Report the sheet position of the formula currently being
    // evaluated.  Matches `ScInterpreter::aPos` on the libreoffice side;
    // standalone hosts return whatever anchor the current driver
    // publishes.
    [[nodiscard]] virtual CellAddress getCurrentFormulaPosition() const = 0;

    // Record the bounds of the dynamic-array formula after a successful
    // allocation so the host's downstream machinery (dependency tracking,
    // draw-layer overlay, etc.) stays in sync with the spilled rectangle.
    virtual void markArrayFormulaBounds(const CellRange& rRange) = 0;
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
