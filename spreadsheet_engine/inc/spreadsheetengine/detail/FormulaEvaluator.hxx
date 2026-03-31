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
#include <optional>
#include <tuple>
#include <vector>

#include <spreadsheetengine/detail/OdfFormulaParser.hxx>
#include <spreadsheetengine/detail/TokenModel.hxx>
#include <spreadsheetengine/detail/WorkbookModel.hxx>
#include <spreadsheetengine/runtime/MathAggregate.hxx>

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

    enum class CacheState : std::uint8_t
    {
        Unseen,
        Active,
        Complete
    };

    enum class ExecutionMode : std::uint8_t
    {
        Ast = 0,
        CompiledToken
    };

    struct CacheEntry
    {
        CacheState meState = CacheState::Unseen;
        EvaluationResult maResult;
    };

    struct FunctionEvalContext;

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
    [[nodiscard]] EvaluationResult evaluateFunctionIfChainDispatch(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateAggregateCriteriaFamilyBody(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateInformationFamilyBody(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateStatisticalRuntimeFamilyBody(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateFinancialFamilyBody(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateDateTimeFamilyBody(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateLookupFamilyBody(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateTextFamilyBody(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateConversionFamilyBody(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateMathFamilyBody(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] EvaluationResult evaluateLogicalFamilyBody(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] std::optional<EvaluationResult> tryEvaluateAggregateFamily(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] std::optional<EvaluationResult> tryEvaluateInformationFamily(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] std::optional<EvaluationResult> tryEvaluateStatisticalRuntimeFamily(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] std::optional<EvaluationResult> tryEvaluateFinancialFamily(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] std::optional<EvaluationResult> tryEvaluateDateTimeFamily(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] std::optional<EvaluationResult> tryEvaluateLookupFamily(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] std::optional<EvaluationResult> tryEvaluateTextFamily(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] std::optional<EvaluationResult> tryEvaluateConversionFamily(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] std::optional<EvaluationResult> tryEvaluateMathFamily(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] std::optional<EvaluationResult> tryEvaluateLogicalFamily(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
    [[nodiscard]] api::ValueResult<bool> scanAggregateScanArgument(
        const formula::Node& rArgument, const api::CellAddress& rCurrentAddress,
        math::AggregateScan& rScan, const math::AggregateOptions& rOptions,
        sal_Int32 nFunction);
    [[nodiscard]] std::optional<EvaluationResult> tryEvaluateSpecialForm(
        api::StringView rFunctionName, const formula::Node& rNode,
        const api::CellAddress& rCurrentAddress);
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
