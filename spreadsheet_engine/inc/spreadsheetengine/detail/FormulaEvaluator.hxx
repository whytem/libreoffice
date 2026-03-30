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
#include <spreadsheetengine/detail/TokenModel.hxx>
#include <spreadsheetengine/detail/WorkbookModel.hxx>

namespace spreadsheetengine::core::eval
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
    using LocalBindingMap = std::map<api::String, EvaluationResult>;

    enum class CacheState : sal_uInt8
    {
        Unseen,
        Active,
        Complete
    };

    enum class ExecutionMode : sal_uInt8
    {
        Ast = 0,
        CompiledToken
    };

    struct CacheEntry
    {
        CacheState meState = CacheState::Unseen;
        EvaluationResult maResult;
    };

    const workbook::Workbook& mrWorkbook;
    std::map<AddressKey, CacheEntry> maAstCellCache;
    std::map<AddressKey, CacheEntry> maCompiledCellCache;
    std::vector<api::CellAddress> maEvaluationStack;
    std::vector<LocalBindingMap> maLocalBindings;
    ExecutionMode meActiveExecutionMode = ExecutionMode::Ast;

    [[nodiscard]] const workbook::Sheet* getSheet(api::SheetId nSheet) const;
    [[nodiscard]] const workbook::Cell* getCell(const api::CellAddress& rAddress) const;
    [[nodiscard]] std::map<AddressKey, CacheEntry>& cacheForMode(ExecutionMode eMode);
    [[nodiscard]] EvaluationResult evaluateCellInternal(
        const api::CellAddress& rAddress, ExecutionMode eMode);

    [[nodiscard]] EvaluationResult evaluateNode(
        const formula::Node& rNode, const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateReferenceNode(
        const formula::Node& rNode, const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateFunction(
        const formula::Node& rNode, const api::CellAddress& rCurrentAddress);
    [[nodiscard]] const EvaluationResult* lookupLocalBinding(api::StringView rName) const;

public:
    explicit Evaluator(const workbook::Workbook& rWorkbook)
        : mrWorkbook(rWorkbook)
    {
    }

    [[nodiscard]] EvaluationResult evaluateCell(const api::CellAddress& rAddress);

    [[nodiscard]] EvaluationResult evaluateFormula(
        api::StringView rFormula, const api::CellAddress& rCurrentAddress);

    [[nodiscard]] EvaluationResult evaluateCompiledFormula(
        const detail::token::CompiledFormula& rFormula, const api::CellAddress& rCurrentAddress);

    [[nodiscard]] EvaluationResult evaluateFormulaViaCompiledTokens(
        api::StringView rFormula, const api::CellAddress& rCurrentAddress);

    [[nodiscard]] api::ValueResult<api::ResolvedReference> resolveReferenceText(
        api::StringView rReference, api::SheetId nCurrentSheet) const;

    [[nodiscard]] api::ValueResult<api::ResolvedReference> resolveNamedRange(
        api::StringView rName, api::SheetId nScopeSheet) const;

    [[nodiscard]] EvaluationResult materializeReferenceValue(
        const api::ResolvedReference& rReference, api::ColumnIndex nColumnOffset,
        api::RowIndex nRowOffset);

    [[nodiscard]] EvaluationResult evaluateCellViaCompiledTokens(
        const api::CellAddress& rAddress);
};

} // namespace spreadsheetengine::core::eval

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
