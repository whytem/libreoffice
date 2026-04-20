/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/detail/FormulaEvaluator.hxx>
#include <cstdint>

#include <spreadsheetengine/api/Logic.hxx>
#include <spreadsheetengine/runtime/ReferenceText.hxx>
#include <spreadsheetengine/runtime/RpnControlFlow.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

#include "FormulaEvaluatorUtils.hxx"

#include <cmath>

namespace spreadsheetengine::core::eval
{
namespace
{

[[nodiscard]] api::String formatReferenceTokenForDisplay(api::StringView rToken)
{
    if (rToken.empty())
        return {};

    std::size_t nDotPos = rToken.rfind(u'.');
    if (nDotPos == api::StringView::npos)
        return api::String(rToken);

    api::String aResult;
    api::StringView aSheet = rToken.substr(0, nDotPos);
    api::StringView aAddress = rToken.substr(nDotPos + 1);
    while (!aSheet.empty() && aSheet.front() == u'$')
        aSheet.remove_prefix(1);
    while (!aAddress.empty() && aAddress.front() == u'.')
        aAddress.remove_prefix(1);

    if (!aSheet.empty())
    {
        aResult += aSheet;
        aResult.push_back(u'.');
    }
    aResult += aAddress;
    return aResult;
}

[[nodiscard]] int binaryPrecedence(formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case formula::BinaryOperator::Equal:
        case formula::BinaryOperator::NotEqual:
        case formula::BinaryOperator::Less:
        case formula::BinaryOperator::LessEqual:
        case formula::BinaryOperator::Greater:
        case formula::BinaryOperator::GreaterEqual:
            return 1;
        case formula::BinaryOperator::Concat:
            return 2;
        case formula::BinaryOperator::Add:
        case formula::BinaryOperator::Subtract:
            return 3;
        case formula::BinaryOperator::Multiply:
        case formula::BinaryOperator::Divide:
            return 4;
        case formula::BinaryOperator::Power:
            return 5;
    }
    return 0;
}

[[nodiscard]] api::String binaryOperatorToken(formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case formula::BinaryOperator::Add:
            return u"+";
        case formula::BinaryOperator::Subtract:
            return u"-";
        case formula::BinaryOperator::Multiply:
            return u"*";
        case formula::BinaryOperator::Divide:
            return u"/";
        case formula::BinaryOperator::Power:
            return u"^";
        case formula::BinaryOperator::Concat:
            return u"&";
        case formula::BinaryOperator::Equal:
            return u"=";
        case formula::BinaryOperator::NotEqual:
            return u"<>";
        case formula::BinaryOperator::Less:
            return u"<";
        case formula::BinaryOperator::LessEqual:
            return u"<=";
        case formula::BinaryOperator::Greater:
            return u">";
        case formula::BinaryOperator::GreaterEqual:
            return u">=";
    }
    return {};
}

[[nodiscard]] std::optional<api::String> formatFormulaNodeForDisplay(
    const formula::Node& rNode, int nParentPrecedence = 0);

[[nodiscard]] std::optional<api::String> formatChildForDisplay(
    const formula::Node& rNode, int nParentPrecedence)
{
    return formatFormulaNodeForDisplay(rNode, nParentPrecedence);
}

[[nodiscard]] std::optional<api::String> formatFormulaNodeForDisplay(
    const formula::Node& rNode, int nParentPrecedence)
{
    switch (rNode.meKind)
    {
        case formula::NodeKind::NumberLiteral:
            return detail::formatNumber(rNode.mfNumber);
        case formula::NodeKind::StringLiteral:
            return detail::formatQuotedString(rNode.maPrimaryText);
        case formula::NodeKind::BooleanLiteral:
            return api::String(rNode.mbBoolean ? u"TRUE()" : u"FALSE()");
        case formula::NodeKind::ErrorLiteral:
            return api::String(rNode.maPrimaryText);
        case formula::NodeKind::EmptyArgument:
            return api::String {};
        case formula::NodeKind::CellReference:
            return formatReferenceTokenForDisplay(rNode.maPrimaryText);
        case formula::NodeKind::RangeReference:
        {
            api::String aResult = formatReferenceTokenForDisplay(rNode.maPrimaryText);
            aResult.push_back(u':');
            aResult += formatReferenceTokenForDisplay(rNode.maSecondaryText);
            return aResult;
        }
        case formula::NodeKind::NamedReference:
            return api::String(rNode.maPrimaryText);
        case formula::NodeKind::RangeConstructor:
        {
            const auto oLeft = formatChildForDisplay(*rNode.maChildren[0], 0);
            const auto oRight = formatChildForDisplay(*rNode.maChildren[1], 0);
            if (!oLeft || !oRight)
                return std::nullopt;
            api::String aResult = *oLeft;
            aResult.push_back(u':');
            aResult += *oRight;
            return aResult;
        }
        case formula::NodeKind::ReferenceList:
        {
            api::String aResult;
            for (std::size_t nIndex = 0; nIndex < rNode.maChildren.size(); ++nIndex)
            {
                const auto oChild = formatChildForDisplay(*rNode.maChildren[nIndex], 0);
                if (!oChild)
                    return std::nullopt;
                aResult += *oChild;
                if (nIndex + 1 < rNode.maChildren.size())
                    aResult.push_back(u'~');
            }
            return aResult;
        }
        case formula::NodeKind::ArrayConstant:
        {
            api::String aResult = u"{";
            for (std::int32_t nRow = 0; nRow < rNode.mnArrayRows; ++nRow)
            {
                for (std::int32_t nColumn = 0; nColumn < rNode.mnArrayColumns; ++nColumn)
                {
                    const std::size_t nIndex
                        = static_cast<std::size_t>(nRow * rNode.mnArrayColumns + nColumn);
                    const auto oElement = formatChildForDisplay(*rNode.maChildren[nIndex], 0);
                    if (!oElement)
                        return std::nullopt;
                    aResult += *oElement;
                    if (nColumn + 1 < rNode.mnArrayColumns)
                        aResult.push_back(u',');
                }
                if (nRow + 1 < rNode.mnArrayRows)
                    aResult.push_back(u';');
            }
            aResult.push_back(u'}');
            return aResult;
        }
        case formula::NodeKind::UnaryOperation:
        {
            const auto oChild = formatChildForDisplay(*rNode.maChildren[0], 6);
            if (!oChild)
                return std::nullopt;
            api::String aResult = rNode.meUnaryOperator == formula::UnaryOperator::Minus
                                      ? api::String(u"-")
                                      : api::String(u"+");
            aResult += *oChild;
            return aResult;
        }
        case formula::NodeKind::BinaryOperation:
        {
            const int nPrecedence = binaryPrecedence(rNode.meBinaryOperator);
            const auto oLeft = formatChildForDisplay(*rNode.maChildren[0], nPrecedence);
            const auto oRight = formatChildForDisplay(*rNode.maChildren[1], nPrecedence + 1);
            if (!oLeft || !oRight)
                return std::nullopt;

            api::String aResult = *oLeft;
            aResult += binaryOperatorToken(rNode.meBinaryOperator);
            aResult += *oRight;
            if (nPrecedence < nParentPrecedence)
            {
                api::String aWrapped;
                aWrapped.push_back(u'(');
                aWrapped += aResult;
                aWrapped.push_back(u')');
                return aWrapped;
            }
            return aResult;
        }
        case formula::NodeKind::FunctionCall:
        {
            api::String aResult = detail::normalizeDisplayFunctionName(rNode.maPrimaryText);
            aResult.push_back(u'(');
            for (std::size_t nIndex = 0; nIndex < rNode.maChildren.size(); ++nIndex)
            {
                const auto oArgument = formatChildForDisplay(*rNode.maChildren[nIndex], 0);
                if (!oArgument)
                    return std::nullopt;
                aResult += *oArgument;
                if (nIndex + 1 < rNode.maChildren.size())
                    aResult.push_back(u',');
            }
            aResult.push_back(u')');
            return aResult;
        }
    }

    return std::nullopt;
}

} // namespace

std::optional<EvaluationResult> Evaluator::tryEvaluateSpecialForm(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    if (!(rFunctionName == u"FORMULA" || rFunctionName == u"IF" || rFunctionName == u"CHOOSE"
            || rFunctionName == u"LET"
            || rFunctionName == u"IFERROR" || rFunctionName == u"COM.MICROSOFT.IFERROR"
            || rFunctionName == u"IFNA" || rFunctionName == u"COM.MICROSOFT.IFNA"
            || rFunctionName == u"INDIRECT" || rFunctionName == u"HYPERLINK"
            || rFunctionName == u"OFFSET"))
    {
        return std::nullopt;
    }

    auto evaluateScalarArgumentValue = [&](const formula::Node& rArgument)
        -> api::ValueResult<api::CellValue> {
        EvaluationResult aValue
            = detail::ensureScalarValue(*this, evaluateNode(rArgument, rCurrentAddress));
        if (!aValue)
            return api::ValueResult<api::CellValue>::failure(aValue.meError);
        return api::ValueResult<api::CellValue>::success(aValue.maValue.maValue);
    };

    auto evaluateNumericArgument = [&](const formula::Node& rArgument,
                                      std::optional<double> oDefaultForEmpty)
        -> api::ValueResult<double> {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument && oDefaultForEmpty)
            return api::ValueResult<double>::success(*oDefaultForEmpty);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<double>::failure(aValue.meError);

        const auto aNumber = detail::coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<double>::failure(aNumber.meError);
        return aNumber;
    };

    if (rFunctionName == u"FORMULA")
    {
        if (rNode.maChildren.size() != 1)
            return detail::makeFailure(api::Error::IllegalArgument);

        EvaluationResult aReference = evaluateReferenceNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aReference)
            return aReference;
        if (!aReference.maValue.isMatrixReference() || !aReference.maValue.maReference.isSingleCell())
            return detail::makeFailure(api::Error::IllegalArgument);

        const workbook::Cell* pCell = getCell(aReference.maValue.maReference.maRange.maStart);
        if (!pCell || !pCell->hasFormula())
            return detail::makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        const formula::ParseResult aParsed = formula::parseFormula(pCell->maFormula);
        if (!aParsed)
            return detail::makeFailure(api::Error::IllegalArgument);

        const auto oDisplay = formatFormulaNodeForDisplay(*aParsed.mpRoot);
        if (!oDisplay)
            return detail::makeFailure(api::Error::IllegalArgument);

        api::String aFormula = u"=";
        aFormula += *oDisplay;
        return detail::makeScalarResult(api::CellValue::text(aFormula));
    }

    if (rFunctionName == u"IF")
    {
        namespace serpn = spreadsheetengine::core::rpn;

        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
            return detail::makeFailure(api::Error::IllegalArgument);

        EvaluationResult aCondition
            = detail::ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));

        const serpn::RpnValue aConditionRpn
            = aCondition ? serpn::RpnValue::fromCellValue(aCondition.maValue.maValue)
                         : serpn::RpnValue::error(aCondition.meError);

        const auto aPlan = serpn::planIfBranch(
            aConditionRpn,
            rNode.maChildren.size() >= 2 ? std::optional(std::size_t(1)) : std::nullopt,
            rNode.maChildren.size() >= 3 ? std::optional(std::size_t(2)) : std::nullopt);

        if (aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready)
            return detail::makeFailure(api::Error::IllegalArgument);

        const auto& rBranch = aPlan.maValue;
        switch (rBranch.meDirective)
        {
            case serpn::BranchDirective::PropagateError:
                return detail::makeFailure(rBranch.meError);
            case serpn::BranchDirective::TakeSlot:
                return evaluateNode(*rNode.maChildren[rBranch.mnSlot], rCurrentAddress);
            case serpn::BranchDirective::ReturnSyntheticBoolean:
                return detail::makeScalarResult(
                    api::CellValue::boolean(rBranch.mbSyntheticBool));
            default:
                return detail::makeFailure(api::Error::IllegalArgument);
        }
    }

    if (rFunctionName == u"CHOOSE")
    {
        namespace serpn = spreadsheetengine::core::rpn;

        if (rNode.maChildren.size() < 2)
            return detail::makeFailure(api::Error::IllegalArgument);

        EvaluationResult aIndex
            = detail::ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aIndex)
            return aIndex;

        const auto aPlan = serpn::planChooseBranch(
            serpn::RpnValue::fromCellValue(aIndex.maValue.maValue),
            static_cast<std::int16_t>(rNode.maChildren.size() - 1));
        if (aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready)
            return detail::makeFailure(api::Error::IllegalArgument);

        const auto& rBranch = aPlan.maValue;
        switch (rBranch.meDirective)
        {
            case serpn::BranchDirective::PropagateError:
                return detail::makeFailure(rBranch.meError);
            case serpn::BranchDirective::TakeSlot:
                return evaluateNode(*rNode.maChildren[rBranch.mnSlot], rCurrentAddress);
            default:
                return detail::makeFailure(api::Error::IllegalArgument);
        }
    }

    if (rFunctionName == u"LET")
    {
        namespace serpn = spreadsheetengine::core::rpn;

        if (rNode.maChildren.size() < 3 || (rNode.maChildren.size() % 2) == 0)
            return detail::makeFailure(api::Error::IllegalArgument);

        maLocalBindings.emplace_back();
        auto popBindings = [this]() { maLocalBindings.pop_back(); };

        for (std::size_t nIndex = 0; nIndex + 1 < rNode.maChildren.size() - 1; nIndex += 2)
        {
            const formula::Node& rNameNode = *rNode.maChildren[nIndex];
            if (rNameNode.meKind != formula::NodeKind::NamedReference)
            {
                popBindings();
                return detail::makeFailure(api::Error::IllegalArgument);
            }

            EvaluationResult aValue = evaluateNode(*rNode.maChildren[nIndex + 1], rCurrentAddress);
            if (!aValue)
            {
                popBindings();
                return aValue;
            }

            const serpn::RpnValue aBindingValue = aValue.maValue.isMatrixReference()
                                                      ? serpn::RpnValue::reference(
                                                            aValue.maValue.maReference)
                                                      : serpn::RpnValue::fromCellValue(
                                                            aValue.maValue.maValue);
            maLocalBindings.back().bind(
                detail::uppercaseAscii(rNameNode.maPrimaryText), aBindingValue);
        }

        EvaluationResult aResult = evaluateNode(*rNode.maChildren.back(), rCurrentAddress);
        popBindings();
        return aResult;
    }

    if (rFunctionName == u"IFERROR" || rFunctionName == u"COM.MICROSOFT.IFERROR"
        || rFunctionName == u"IFNA" || rFunctionName == u"COM.MICROSOFT.IFNA")
    {
        namespace serpn = spreadsheetengine::core::rpn;

        if (rNode.maChildren.size() != 2)
            return detail::makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        if (rNode.maChildren[0]->meKind == formula::NodeKind::EmptyArgument)
            return detail::makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const bool bNAOnly = rFunctionName == u"IFNA" || rFunctionName == u"COM.MICROSOFT.IFNA";
        EvaluationResult aPrimary = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        api::Error ePrimaryError = api::Error::None;
        if (!aPrimary)
        {
            ePrimaryError = aPrimary.meError;
        }
        else if (aPrimary.maValue.isScalar() && aPrimary.maValue.maValue.isError())
        {
            ePrimaryError = aPrimary.maValue.maValue.meError;
        }

        const auto aPlan = serpn::planIfErrorBranch(ePrimaryError, bNAOnly, std::size_t { 1 });
        switch (aPlan.meDirective)
        {
            case serpn::BranchDirective::KeepPrimaryValue:
                return aPrimary;
            case serpn::BranchDirective::EvaluateAlternate:
                return evaluateNode(*rNode.maChildren[1], rCurrentAddress);
            default:
                return detail::makeFailure(api::Error::IllegalArgument);
        }
    }

    if (rFunctionName == u"INDIRECT")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return detail::makeFailure(api::Error::IllegalArgument);

        const auto aReferenceTextValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aReferenceTextValue)
            return detail::makeFailure(aReferenceTextValue.meError);
        const auto aReferenceText = detail::coerceToString(aReferenceTextValue.maValue);
        if (!aReferenceText)
            return detail::makeFailure(aReferenceText.meError);

        bool bUseA1 = true;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aA1Argument = evaluateScalarArgumentValue(*rNode.maChildren[1]);
            if (!aA1Argument)
                return detail::makeFailure(aA1Argument.meError);
            const auto aA1Bool = detail::coerceToBoolean(aA1Argument.maValue);
            if (!aA1Bool)
                return detail::makeFailure(aA1Bool.meError);
            bUseA1 = aA1Bool.maValue;
        }

        std::optional<api::ResolvedReference> oReference;
        if (bUseA1)
        {
            if (const auto oNormalized
                = runtime::referencetext::normalizeIndirectA1ReferenceText(
                    aReferenceText.maValue))
            {
                const auto aResolved = resolveReferenceText(*oNormalized, rCurrentAddress.mnSheet);
                if (aResolved)
                    oReference = aResolved.maValue;
            }

            if (!oReference)
            {
                const auto aNamed
                    = resolveNamedRange(aReferenceText.maValue, rCurrentAddress.mnSheet);
                if (aNamed)
                    oReference = aNamed.maValue;
            }
        }
        else
            oReference = runtime::referencetext::parseIndirectR1C1ReferenceText(
                aReferenceText.maValue, mrWorkbook, rCurrentAddress.mnSheet);

        if (!oReference)
            return detail::makeFailure(api::Error::IllegalArgument);
        if (oReference->isSingleCell())
            return materializeReferenceValue(*oReference, 0, 0);
        return detail::makeReferenceResult(*oReference);
    }

    if (rFunctionName == u"HYPERLINK")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return detail::makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLinkTarget
            = detail::ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aLinkTarget)
            return aLinkTarget;
        if (aLinkTarget.maValue.maValue.isError())
            return detail::makeFailure(aLinkTarget.maValue.maValue.meError);

        if (rNode.maChildren.size() == 1)
            return detail::makeScalarResult(aLinkTarget.maValue.maValue);

        EvaluationResult aDisplayValue
            = detail::ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aDisplayValue)
            return aDisplayValue;
        return detail::makeScalarResult(aDisplayValue.maValue.maValue);
    }

    if (rFunctionName == u"OFFSET")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return detail::makeFailure(api::Error::IllegalArgument);

        EvaluationResult aReference = evaluateReferenceNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aReference)
            return aReference;
        if (!aReference.maValue.isMatrixReference())
            return detail::makeFailure(api::Error::IllegalArgument);

        const auto aRows = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aRows)
            return detail::makeFailure(aRows.meError);
        const auto aColumns = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aColumns)
            return detail::makeFailure(aColumns.meError);

        const std::int32_t nRowOffset = static_cast<std::int32_t>(std::trunc(aRows.maValue));
        const std::int32_t nColumnOffset = static_cast<std::int32_t>(std::trunc(aColumns.maValue));

        std::int32_t nHeight
            = static_cast<std::int32_t>(aReference.maValue.maReference.maRange.rowCount());
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aHeight = evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
            if (!aHeight)
                return detail::makeFailure(aHeight.meError);
            const auto oWholeHeight = detail::toWholeNumber(aHeight.maValue);
            if (!oWholeHeight || *oWholeHeight < 1)
                return detail::makeFailure(api::Error::IllegalArgument);
            nHeight = *oWholeHeight;
        }

        std::int32_t nWidth
            = static_cast<std::int32_t>(aReference.maValue.maReference.maRange.columnCount());
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aWidth = evaluateNumericArgument(*rNode.maChildren[4], std::nullopt);
            if (!aWidth)
                return detail::makeFailure(aWidth.meError);
            const auto oWholeWidth = detail::toWholeNumber(aWidth.maValue);
            if (!oWholeWidth || *oWholeWidth < 1)
                return detail::makeFailure(api::Error::IllegalArgument);
            nWidth = *oWholeWidth;
        }

        const auto& rSourceRange = aReference.maValue.maReference.maRange;
        const std::int64_t nStartColumn
            = static_cast<std::int64_t>(rSourceRange.maStart.mnColumn) + nColumnOffset;
        const std::int64_t nStartRow
            = static_cast<std::int64_t>(rSourceRange.maStart.mnRow) + nRowOffset;
        const std::int64_t nEndColumn = nStartColumn + nWidth - 1;
        const std::int64_t nEndRow = nStartRow + nHeight - 1;
        if (nStartColumn < 0 || nStartRow < 0 || nEndColumn < 0 || nEndRow < 0)
            return detail::makeFailure(api::Error::NoValue);

        api::ResolvedReference aOffsetReference = aReference.maValue.maReference;
        aOffsetReference.maRange.maStart.mnColumn = static_cast<api::ColumnIndex>(nStartColumn);
        aOffsetReference.maRange.maStart.mnRow = static_cast<api::RowIndex>(nStartRow);
        aOffsetReference.maRange.maEnd.mnColumn = static_cast<api::ColumnIndex>(nEndColumn);
        aOffsetReference.maRange.maEnd.mnRow = static_cast<api::RowIndex>(nEndRow);
        return detail::makeReferenceResult(aOffsetReference);
    }

    return std::nullopt;
}

} // namespace spreadsheetengine::core::eval

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
