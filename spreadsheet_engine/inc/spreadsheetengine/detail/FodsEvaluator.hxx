/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <map>
#include <tuple>
#include <vector>

#include <spreadsheetengine/detail/OdfFormulaParser.hxx>
#include <spreadsheetengine/detail/WorkbookModel.hxx>

namespace spreadsheetengine::core::fods
{

struct EvaluationResult
{
    api::CellValueView maValue = api::CellValueView::scalar(api::CellValue::empty());
    api::Error meError = api::Error::None;
    std::vector<api::CellAddress> maCyclePath;
    bool mbUsedCachedValue = false;

    [[nodiscard]] constexpr bool ok() const { return meError == api::Error::None; }

    constexpr explicit operator bool() const { return ok(); }
};

class Evaluator
{
    using AddressKey = std::tuple<api::SheetId, api::ColumnIndex, api::RowIndex>;

    enum class CacheState : sal_uInt8
    {
        Unseen,
        Active,
        Complete
    };

    struct CacheEntry
    {
        CacheState meState = CacheState::Unseen;
        EvaluationResult maResult;
    };

    const workbook::Workbook& mrWorkbook;
    std::map<AddressKey, CacheEntry> maCellCache;
    std::vector<api::CellAddress> maEvaluationStack;

    [[nodiscard]] const workbook::Sheet* getSheet(api::SheetId nSheet) const;
    [[nodiscard]] const workbook::Cell* getCell(const api::CellAddress& rAddress) const;

    [[nodiscard]] EvaluationResult evaluateNode(
        const formula::Node& rNode, const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateReferenceNode(
        const formula::Node& rNode, const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateFunction(
        const formula::Node& rNode, const api::CellAddress& rCurrentAddress);

public:
    explicit Evaluator(const workbook::Workbook& rWorkbook)
        : mrWorkbook(rWorkbook)
    {
    }

    [[nodiscard]] EvaluationResult evaluateCell(const api::CellAddress& rAddress);

    [[nodiscard]] EvaluationResult evaluateFormula(
        api::StringView rFormula, const api::CellAddress& rCurrentAddress);

    [[nodiscard]] api::ValueResult<api::ResolvedReference> resolveReferenceText(
        api::StringView rReference, api::SheetId nCurrentSheet) const;

    [[nodiscard]] api::ValueResult<api::ResolvedReference> resolveNamedRange(
        api::StringView rName, api::SheetId nScopeSheet) const;

    [[nodiscard]] EvaluationResult materializeReferenceValue(
        const api::ResolvedReference& rReference, api::ColumnIndex nColumnOffset,
        api::RowIndex nRowOffset);
};

} // namespace spreadsheetengine::core::fods

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
