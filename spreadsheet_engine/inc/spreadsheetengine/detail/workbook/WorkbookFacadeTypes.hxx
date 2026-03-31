/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>
#include <vector>

#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/Grammar.hxx>
#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/String.hxx>

namespace spreadsheetengine::detail::facade
{

// --- Identity carriers ---

/// Stable sheet identity within a facade session.
using SheetId = api::SheetId;

/// Stable identity for a formula-bearing cell within a facade session.
struct FormulaCellId
{
    api::CellAddress maAddress;

    [[nodiscard]] constexpr bool operator==(const FormulaCellId& rOther) const = default;
};

/// Stable identity for a named range within a facade session.
struct NamedRangeId
{
    sal_Int32 mnIndex = -1;
    std::optional<SheetId> moSheet; ///< nullopt = global scope

    [[nodiscard]] constexpr bool operator==(const NamedRangeId& rOther) const = default;

    [[nodiscard]] constexpr bool isValid() const { return mnIndex >= 0; }
    [[nodiscard]] constexpr bool isGlobal() const { return !moSheet.has_value(); }
};

// --- Descriptors ---

/// Read-only view of a sheet's identity and calculation-relevant metadata.
struct SheetDescriptor
{
    SheetId mnId = 0;
    api::String maName;
    bool mbHidden = false;

    [[nodiscard]] constexpr bool operator==(const SheetDescriptor& rOther) const = default;
};

/// Cell type classification for the facade.
enum class CellKind : sal_uInt8
{
    Empty,
    Scalar,
    Formula
};

/// Read-only view of a cell's address, type, and value.
struct CellDescriptor
{
    api::CellAddress maAddress;
    CellKind meKind = CellKind::Empty;
    api::CellValue maValue;
    bool mbHasFormula = false;

    [[nodiscard]] constexpr bool operator==(const CellDescriptor& rOther) const = default;
};

/// Formula cell classification.
enum class FormulaCellKind : sal_uInt8
{
    Ordinary,
    SharedGroupMember,
    MatrixOrigin,
    MatrixMember
};

/// Read-only view of a formula-bearing cell with metadata.
struct FormulaCellDescriptor
{
    FormulaCellId maId;
    api::CellValue maCachedValue;
    api::String maFormulaSource;
    FormulaCellKind meKind = FormulaCellKind::Ordinary;
    bool mbDirty = false;
    bool mbNeedsRecalc = false;

    [[nodiscard]] constexpr bool operator==(const FormulaCellDescriptor& rOther) const = default;
};

/// Named range scope.
enum class NamedRangeScope : sal_uInt8
{
    Global,
    SheetLocal
};

/// Read-only view of a named range.
struct NamedRangeDescriptor
{
    NamedRangeId maId;
    api::String maName;
    NamedRangeScope meScope = NamedRangeScope::Global;
    std::optional<SheetId> moScopeSheet;
    api::CellAddress maBaseAddress;
    api::String maTargetExpression; ///< normalized target range text

    [[nodiscard]] constexpr bool operator==(const NamedRangeDescriptor& rOther) const = default;
};

/// Read-only view of a shared-formula/group.
struct FormulaGroupDescriptor
{
    api::CellAddress maAnchor;
    sal_Int32 mnLength = 0; ///< number of rows in the group
    bool mbShareable = true;

    [[nodiscard]] constexpr bool operator==(const FormulaGroupDescriptor& rOther) const = default;

    [[nodiscard]] constexpr bool isValid() const { return mnLength > 0; }
};

/// Lightweight snapshot metadata for differential validation.
struct WorkbookSnapshotInfo
{
    sal_Int64 mnGeneration = 0;
    sal_Int32 mnSheetCount = 0;
    sal_Int32 mnFormulaCellCount = 0;

    [[nodiscard]] constexpr bool operator==(const WorkbookSnapshotInfo& rOther) const = default;
};

// --- Mutation events ---

/// Kind of document mutation for invalidation modeling.
enum class MutationKind : sal_uInt8
{
    SetScalarValue,
    SetFormula,
    ClearCell,
    ClearRange,
    InsertRows,
    DeleteRows,
    InsertColumns,
    DeleteColumns,
    MoveRange,
    CopyRange,
    RenameSheet,
    AddNamedRange,
    RemoveNamedRange,
    RenameNamedRange
};

/// Normalized mutation event description.
struct MutationEvent
{
    MutationKind meKind = MutationKind::SetScalarValue;

    /// Primary affected address (for single-cell operations).
    api::CellAddress maAddress;

    /// Affected range (for range operations).
    api::CellRange maRange;

    /// Secondary range (for move/copy destination).
    api::CellRange maDestination;

    /// Sheet identity before the operation (for rename).
    SheetId mnSheet = 0;

    /// Textual data (new name for rename, formula text for SetFormula, etc.).
    api::String maText;

    /// Optional named-range state before a named-range mutation.
    std::optional<NamedRangeDescriptor> moNamedRangeBefore;

    /// Optional named-range state after a named-range mutation.
    std::optional<NamedRangeDescriptor> moNamedRangeAfter;

    /// Row/column count for insert/delete operations.
    sal_Int32 mnCount = 0;

    /// Whether this is a copy (true) or move (false) for MoveRange/CopyRange.
    bool mbCopy = false;

    [[nodiscard]] constexpr bool operator==(const MutationEvent& rOther) const = default;

    // --- Factory helpers ---

    [[nodiscard]] static MutationEvent setScalarValue(const api::CellAddress& rAddress)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::SetScalarValue;
        aEvent.maAddress = rAddress;
        return aEvent;
    }

    [[nodiscard]] static MutationEvent setFormula(
        const api::CellAddress& rAddress, api::StringView rFormula)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::SetFormula;
        aEvent.maAddress = rAddress;
        aEvent.maText = api::String(rFormula);
        return aEvent;
    }

    [[nodiscard]] static MutationEvent clearCell(const api::CellAddress& rAddress)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::ClearCell;
        aEvent.maAddress = rAddress;
        return aEvent;
    }

    [[nodiscard]] static MutationEvent clearRange(const api::CellRange& rRange)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::ClearRange;
        aEvent.maRange = rRange;
        return aEvent;
    }

    [[nodiscard]] static MutationEvent insertRows(
        SheetId nSheet, api::RowIndex nRow, sal_Int32 nCount)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::InsertRows;
        aEvent.mnSheet = nSheet;
        aEvent.maAddress = { nSheet, 0, nRow };
        aEvent.mnCount = nCount;
        return aEvent;
    }

    [[nodiscard]] static MutationEvent deleteRows(
        SheetId nSheet, api::RowIndex nRow, sal_Int32 nCount)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::DeleteRows;
        aEvent.mnSheet = nSheet;
        aEvent.maAddress = { nSheet, 0, nRow };
        aEvent.mnCount = nCount;
        return aEvent;
    }

    [[nodiscard]] static MutationEvent insertColumns(
        SheetId nSheet, api::ColumnIndex nColumn, sal_Int32 nCount)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::InsertColumns;
        aEvent.mnSheet = nSheet;
        aEvent.maAddress = { nSheet, nColumn, 0 };
        aEvent.mnCount = nCount;
        return aEvent;
    }

    [[nodiscard]] static MutationEvent deleteColumns(
        SheetId nSheet, api::ColumnIndex nColumn, sal_Int32 nCount)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::DeleteColumns;
        aEvent.mnSheet = nSheet;
        aEvent.maAddress = { nSheet, nColumn, 0 };
        aEvent.mnCount = nCount;
        return aEvent;
    }

    [[nodiscard]] static MutationEvent moveRange(
        const api::CellRange& rSource, const api::CellRange& rDestination)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::MoveRange;
        aEvent.maRange = rSource;
        aEvent.maDestination = rDestination;
        aEvent.mbCopy = false;
        return aEvent;
    }

    [[nodiscard]] static MutationEvent copyRange(
        const api::CellRange& rSource, const api::CellRange& rDestination)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::CopyRange;
        aEvent.maRange = rSource;
        aEvent.maDestination = rDestination;
        aEvent.mbCopy = true;
        return aEvent;
    }

    [[nodiscard]] static MutationEvent renameSheet(SheetId nSheet, api::StringView rNewName)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::RenameSheet;
        aEvent.mnSheet = nSheet;
        aEvent.maText = api::String(rNewName);
        return aEvent;
    }

    [[nodiscard]] static MutationEvent addNamedRange(const NamedRangeDescriptor& rAfter)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::AddNamedRange;
        aEvent.maText = rAfter.maName;
        aEvent.moNamedRangeAfter = rAfter;
        return aEvent;
    }

    [[nodiscard]] static MutationEvent removeNamedRange(const NamedRangeDescriptor& rBefore)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::RemoveNamedRange;
        aEvent.maText = rBefore.maName;
        aEvent.moNamedRangeBefore = rBefore;
        return aEvent;
    }

    [[nodiscard]] static MutationEvent renameNamedRange(
        const NamedRangeDescriptor& rBefore, const NamedRangeDescriptor& rAfter)
    {
        MutationEvent aEvent;
        aEvent.meKind = MutationKind::RenameNamedRange;
        aEvent.maText = rAfter.maName;
        aEvent.moNamedRangeBefore = rBefore;
        aEvent.moNamedRangeAfter = rAfter;
        return aEvent;
    }
};

} // namespace spreadsheetengine::detail::facade

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
