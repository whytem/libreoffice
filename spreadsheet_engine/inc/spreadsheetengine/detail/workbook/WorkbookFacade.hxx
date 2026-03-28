/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <functional>
#include <optional>
#include <vector>

#include <spreadsheetengine/detail/workbook/WorkbookFacadeTypes.hxx>

namespace spreadsheetengine::detail::facade
{

/// Callback type for formula-cell iteration.
using FormulaCellVisitor = std::function<bool(const FormulaCellDescriptor&)>;

/// Engine-owned calculation-facing workbook contract.
///
/// This is the stable runtime boundary that compiler, dependency,
/// invalidation, and scheduling logic target. The first implementation
/// is backed by Calc storage through adapters; the contract itself
/// belongs to spreadsheet_engine.
///
/// Version 1 is read-only. Mutation events are modeled as descriptions
/// for later invalidation shadow work, not as direct mutation commands.
class WorkbookFacade
{
public:
    virtual ~WorkbookFacade() = default;

    // --- Workbook-level queries ---

    /// Number of sheets in the workbook.
    [[nodiscard]] virtual sal_Int32 getSheetCount() const = 0;

    /// Look up sheet id by name. Returns nullopt if not found.
    [[nodiscard]] virtual std::optional<SheetId> findSheetId(
        api::StringView rName) const = 0;

    /// Get sheet descriptor by id. Returns nullopt if id is out of range.
    [[nodiscard]] virtual std::optional<SheetDescriptor> getSheetDescriptor(
        SheetId nSheet) const = 0;

    /// Get all sheet descriptors in order.
    [[nodiscard]] virtual std::vector<SheetDescriptor> getSheetDescriptors() const = 0;

    /// Current grammar/options relevant to calculation.
    [[nodiscard]] virtual api::Grammar getGrammar() const = 0;

    /// Snapshot metadata for differential validation.
    [[nodiscard]] virtual WorkbookSnapshotInfo getSnapshotInfo() const = 0;

    // --- Cell-level queries ---

    /// Check whether a cell exists (has content) at the given address.
    [[nodiscard]] virtual bool hasCell(const api::CellAddress& rAddress) const = 0;

    /// Get a cell descriptor. Returns an empty-kind descriptor if the cell
    /// does not exist.
    [[nodiscard]] virtual CellDescriptor getCellDescriptor(
        const api::CellAddress& rAddress) const = 0;

    /// Get a formula cell descriptor. Returns nullopt if the cell does not
    /// contain a formula.
    [[nodiscard]] virtual std::optional<FormulaCellDescriptor> getFormulaCellDescriptor(
        const api::CellAddress& rAddress) const = 0;

    // --- Formula-cell iteration ---

    /// Iterate over all formula-bearing cells on a sheet. The visitor
    /// returns true to continue, false to stop early.
    virtual void visitFormulaCells(SheetId nSheet,
        const FormulaCellVisitor& rVisitor) const = 0;

    /// Iterate over all formula-bearing cells in the workbook.
    virtual void visitAllFormulaCells(const FormulaCellVisitor& rVisitor) const = 0;

    // --- Named-range queries ---

    /// Get the number of named ranges (global + sheet-local).
    [[nodiscard]] virtual sal_Int32 getNamedRangeCount() const = 0;

    /// Look up a named range by name and optional scope sheet.
    [[nodiscard]] virtual std::optional<NamedRangeDescriptor> findNamedRange(
        api::StringView rName,
        std::optional<SheetId> oScopeSheet = std::nullopt) const = 0;

    /// Get all named range descriptors.
    [[nodiscard]] virtual std::vector<NamedRangeDescriptor> getNamedRangeDescriptors() const = 0;

    // --- Shared-formula/group queries ---

    /// Get group descriptor for a formula cell. Returns nullopt if the cell
    /// is not part of a shared-formula group.
    [[nodiscard]] virtual std::optional<FormulaGroupDescriptor> getFormulaGroupDescriptor(
        const api::CellAddress& rAddress) const = 0;
};

} // namespace spreadsheetengine::detail::facade

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
