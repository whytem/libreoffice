/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <address.hxx>
#include <document.hxx>
#include <rangenam.hxx>

#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/compat/libreoffice/WorkbookFacade.hxx>
#include <spreadsheetengine/detail/workbook/WorkbookFacadeTypes.hxx>

namespace spreadsheetengine::compat::libreoffice
{

namespace detail
{
namespace facade = spreadsheetengine::detail::facade;
}

/// Translate Calc document operations into engine-owned MutationEvent
/// descriptions. These are normalized event descriptions for the
/// dependency/invalidation shadow work — they do not themselves cause
/// mutations.
namespace mutation
{

[[nodiscard]] inline detail::facade::MutationEvent
translateSetScalarValue(const ScAddress& rAddress)
{
    return detail::facade::MutationEvent::setScalarValue(
        toApiCellAddress(rAddress));
}

[[nodiscard]] inline detail::facade::MutationEvent
translateSetFormula(const ScAddress& rAddress, const OUString& rFormula)
{
    return detail::facade::MutationEvent::setFormula(
        toApiCellAddress(rAddress), toApiString(rFormula));
}

[[nodiscard]] inline detail::facade::MutationEvent
translateClearCell(const ScAddress& rAddress)
{
    return detail::facade::MutationEvent::clearCell(toApiCellAddress(rAddress));
}

[[nodiscard]] inline detail::facade::MutationEvent
translateClearRange(const ScRange& rRange)
{
    return detail::facade::MutationEvent::clearRange(toApiCellRange(rRange));
}

[[nodiscard]] inline detail::facade::MutationEvent
translateInsertRows(SCTAB nTab, SCROW nRow, sal_Int32 nCount)
{
    return detail::facade::MutationEvent::insertRows(
        static_cast<detail::facade::SheetId>(nTab),
        static_cast<api::RowIndex>(nRow), nCount);
}

[[nodiscard]] inline detail::facade::MutationEvent
translateDeleteRows(SCTAB nTab, SCROW nRow, sal_Int32 nCount)
{
    return detail::facade::MutationEvent::deleteRows(
        static_cast<detail::facade::SheetId>(nTab),
        static_cast<api::RowIndex>(nRow), nCount);
}

[[nodiscard]] inline detail::facade::MutationEvent
translateInsertColumns(SCTAB nTab, SCCOL nCol, sal_Int32 nCount)
{
    return detail::facade::MutationEvent::insertColumns(
        static_cast<detail::facade::SheetId>(nTab),
        static_cast<api::ColumnIndex>(nCol), nCount);
}

[[nodiscard]] inline detail::facade::MutationEvent
translateDeleteColumns(SCTAB nTab, SCCOL nCol, sal_Int32 nCount)
{
    return detail::facade::MutationEvent::deleteColumns(
        static_cast<detail::facade::SheetId>(nTab),
        static_cast<api::ColumnIndex>(nCol), nCount);
}

[[nodiscard]] inline detail::facade::MutationEvent
translateMoveRange(const ScRange& rSource, const ScRange& rDestination)
{
    return detail::facade::MutationEvent::moveRange(
        toApiCellRange(rSource), toApiCellRange(rDestination));
}

[[nodiscard]] inline detail::facade::MutationEvent
translateCopyRange(const ScRange& rSource, const ScRange& rDestination)
{
    return detail::facade::MutationEvent::copyRange(
        toApiCellRange(rSource), toApiCellRange(rDestination));
}

[[nodiscard]] inline detail::facade::MutationEvent
translateRenameSheet(SCTAB nTab, const OUString& rNewName)
{
    return detail::facade::MutationEvent::renameSheet(
        static_cast<detail::facade::SheetId>(nTab), toApiString(rNewName));
}

[[nodiscard]] inline detail::facade::MutationEvent
translateAddNamedRange(const ScDocument& rDoc, const ScRangeData& rData,
    std::optional<SCTAB> oScopeTab = std::nullopt)
{
    return detail::facade::MutationEvent::addNamedRange(
        makeNamedRangeDescriptor(rDoc, rData, oScopeTab.has_value()
                ? std::optional<detail::facade::SheetId>(*oScopeTab)
                : std::nullopt));
}

[[nodiscard]] inline detail::facade::MutationEvent
translateRemoveNamedRange(const ScDocument& rDoc, const ScRangeData& rData,
    std::optional<SCTAB> oScopeTab = std::nullopt)
{
    return detail::facade::MutationEvent::removeNamedRange(
        makeNamedRangeDescriptor(rDoc, rData, oScopeTab.has_value()
                ? std::optional<detail::facade::SheetId>(*oScopeTab)
                : std::nullopt));
}

[[nodiscard]] inline detail::facade::MutationEvent
translateRenameNamedRange(const ScDocument& rDoc, const ScRangeData& rData,
    std::optional<SCTAB> oScopeTab, const OUString& rOldName)
{
    const auto oScopeSheet = oScopeTab.has_value()
        ? std::optional<detail::facade::SheetId>(*oScopeTab)
        : std::nullopt;

    auto aAfter = makeNamedRangeDescriptor(rDoc, rData, oScopeSheet);
    auto aBefore = aAfter;
    aBefore.maName = toApiString(rOldName);

    return detail::facade::MutationEvent::renameNamedRange(aBefore, aAfter);
}

} // namespace mutation

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
