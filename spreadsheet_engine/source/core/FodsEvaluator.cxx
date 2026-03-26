/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/detail/FodsEvaluator.hxx>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string>

namespace spreadsheetengine::core::fods
{
namespace
{

[[nodiscard]] std::tuple<api::SheetId, api::ColumnIndex, api::RowIndex> makeAddressKey(
    const api::CellAddress& rAddress)
{
    return { rAddress.mnSheet, rAddress.mnColumn, rAddress.mnRow };
}

[[nodiscard]] EvaluationResult makeScalarResult(
    const api::CellValue& rValue, bool bUsedCachedValue = false)
{
    EvaluationResult aResult;
    aResult.maValue = api::CellValueView::scalar(rValue);
    aResult.mbUsedCachedValue = bUsedCachedValue;
    return aResult;
}

[[nodiscard]] EvaluationResult makeReferenceResult(const api::ResolvedReference& rReference)
{
    EvaluationResult aResult;
    aResult.maValue = api::CellValueView::matrixReference(rReference);
    return aResult;
}

[[nodiscard]] EvaluationResult makeFailure(api::Error eError)
{
    EvaluationResult aResult;
    aResult.meError = eError;
    return aResult;
}

[[nodiscard]] bool hasCachedFallbackValue(const workbook::Cell& rCell)
{
    return rCell.maValue.isNumber() || rCell.maValue.isBoolean() || rCell.maValue.isText()
           || rCell.maValue.isError();
}

[[nodiscard]] api::String uppercaseAscii(api::StringView rValue)
{
    api::String aResult;
    aResult.reserve(rValue.size());
    for (const char16_t cChar : rValue)
    {
        if (cChar >= u'a' && cChar <= u'z')
            aResult.push_back(static_cast<char16_t>(cChar - u'a' + u'A'));
        else
            aResult.push_back(cChar);
    }
    return aResult;
}

[[nodiscard]] api::String normalizeDisplayFunctionName(api::StringView rName)
{
    const api::StringView aMicrosoftPrefix = u"COM.MICROSOFT.";
    const api::StringView aOpenOfficePrefix = u"ORG.OPENOFFICE.";
    if (rName.substr(0, aMicrosoftPrefix.size()) == aMicrosoftPrefix)
        return api::String(rName.substr(aMicrosoftPrefix.size()));
    if (rName.substr(0, aOpenOfficePrefix.size()) == aOpenOfficePrefix)
        return api::String(rName.substr(aOpenOfficePrefix.size()));
    return api::String(rName);
}

[[nodiscard]] api::Error mapErrorLiteral(api::StringView rText)
{
    if (rText == u"#N/A")
        return api::Error::NotAvailable;
    if (rText == u"#DIV/0!")
        return api::Error::DivisionByZero;
    if (rText == u"#VALUE!")
        return api::Error::NoValue;
    if (rText == u"#NUM!")
        return api::Error::NoConvergence;
    if (rText == u"#NAME?" || rText == u"#REF!" || rText == u"#NULL!")
        return api::Error::IllegalArgument;

    return api::Error::NoValue;
}

[[nodiscard]] std::optional<double> parseAsciiDouble(api::StringView rValue)
{
    if (rValue.empty())
        return std::nullopt;

    std::string aAscii;
    aAscii.reserve(rValue.size());
    for (const char16_t cChar : rValue)
    {
        if (cChar > 0x7f)
            return std::nullopt;
        aAscii.push_back(static_cast<char>(cChar));
    }

    char* pEnd = nullptr;
    const double fValue = std::strtod(aAscii.c_str(), &pEnd);
    if (!pEnd || *pEnd != '\0')
        return std::nullopt;

    return fValue;
}

[[nodiscard]] api::String formatNumber(double fValue)
{
    std::string aAscii = std::to_string(fValue);
    const std::size_t nDot = aAscii.find('.');
    if (nDot != std::string::npos)
    {
        while (!aAscii.empty() && aAscii.back() == '0')
            aAscii.pop_back();
        if (!aAscii.empty() && aAscii.back() == '.')
            aAscii.pop_back();
    }

    api::String aResult;
    aResult.reserve(aAscii.size());
    for (const char cChar : aAscii)
        aResult.push_back(static_cast<char16_t>(cChar));
    return aResult;
}

[[nodiscard]] api::String formatQuotedString(api::StringView rValue)
{
    api::String aResult;
    aResult.reserve(rValue.size() + 2);
    aResult.push_back(u'"');
    for (const char16_t cChar : rValue)
    {
        if (cChar == u'"')
            aResult.push_back(u'"');
        aResult.push_back(cChar);
    }
    aResult.push_back(u'"');
    return aResult;
}

[[nodiscard]] api::ValueResult<double> coerceToNumber(const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Empty:
            return api::ValueResult<double>::success(0.0);
        case api::CellValueKind::Number:
        case api::CellValueKind::Boolean:
            return api::ValueResult<double>::success(rValue.mfNumber);
        case api::CellValueKind::Text:
        {
            if (auto oValue = parseAsciiDouble(rValue.maString))
                return api::ValueResult<double>::success(*oValue);
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        }
        case api::CellValueKind::Error:
            return api::ValueResult<double>::failure(rValue.meError);
    }

    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

[[nodiscard]] api::ValueResult<bool> coerceToBoolean(const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Empty:
            return api::ValueResult<bool>::success(false);
        case api::CellValueKind::Number:
        case api::CellValueKind::Boolean:
            return api::ValueResult<bool>::success(rValue.mfNumber != 0.0);
        case api::CellValueKind::Text:
        {
            const api::String aUpper = uppercaseAscii(rValue.maString);
            if (aUpper == u"TRUE")
                return api::ValueResult<bool>::success(true);
            if (aUpper == u"FALSE")
                return api::ValueResult<bool>::success(false);
            if (auto oNumber = parseAsciiDouble(rValue.maString))
                return api::ValueResult<bool>::success(*oNumber != 0.0);
            return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
        }
        case api::CellValueKind::Error:
            return api::ValueResult<bool>::failure(rValue.meError);
    }

    return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
}

[[nodiscard]] api::ValueResult<api::String> coerceToString(const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Empty:
            return api::ValueResult<api::String>::success({});
        case api::CellValueKind::Number:
            return api::ValueResult<api::String>::success(formatNumber(rValue.mfNumber));
        case api::CellValueKind::Boolean:
            return api::ValueResult<api::String>::success(
                rValue.mfNumber != 0.0 ? api::String(u"TRUE") : api::String(u"FALSE"));
        case api::CellValueKind::Text:
            return api::ValueResult<api::String>::success(rValue.maString);
        case api::CellValueKind::Error:
            return api::ValueResult<api::String>::failure(rValue.meError);
    }

    return api::ValueResult<api::String>::failure(api::Error::IllegalArgument);
}

[[nodiscard]] std::optional<api::ColumnIndex> parseColumnName(api::StringView rColumnName)
{
    if (rColumnName.empty())
        return std::nullopt;

    sal_Int64 nColumn = 0;
    for (const char16_t cChar : rColumnName)
    {
        char16_t cUpper = cChar;
        if (cUpper >= u'a' && cUpper <= u'z')
            cUpper = static_cast<char16_t>(cUpper - u'a' + u'A');
        if (cUpper < u'A' || cUpper > u'Z')
            return std::nullopt;
        nColumn = nColumn * 26 + (cUpper - u'A' + 1);
    }

    return static_cast<api::ColumnIndex>(nColumn - 1);
}

[[nodiscard]] api::String unquoteSheetName(api::StringView rSheetName)
{
    if (rSheetName.size() < 2 || rSheetName.front() != u'\'' || rSheetName.back() != u'\'')
        return api::String(rSheetName);

    api::String aResult;
    aResult.reserve(rSheetName.size() - 2);
    for (std::size_t nIndex = 1; nIndex + 1 < rSheetName.size(); ++nIndex)
    {
        if (rSheetName[nIndex] == u'\'' && nIndex + 1 < rSheetName.size() - 1
            && rSheetName[nIndex + 1] == u'\'')
        {
            aResult.push_back(u'\'');
            ++nIndex;
            continue;
        }

        aResult.push_back(rSheetName[nIndex]);
    }
    return aResult;
}

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
            return formatNumber(rNode.mfNumber);
        case formula::NodeKind::StringLiteral:
            return formatQuotedString(rNode.maPrimaryText);
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
            api::String aResult = normalizeDisplayFunctionName(rNode.maPrimaryText);
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

[[nodiscard]] std::optional<api::CellAddress> parseCellAddressToken(
    api::StringView rToken, const workbook::Workbook& rWorkbook, api::SheetId nImplicitSheet)
{
    const std::size_t nDotPos = rToken.rfind(u'.');
    if (nDotPos == api::StringView::npos || nDotPos + 1 >= rToken.size())
        return std::nullopt;

    api::SheetId nSheet = nImplicitSheet;
    api::StringView aSheetToken = rToken.substr(0, nDotPos);
    if (!aSheetToken.empty())
    {
        while (!aSheetToken.empty() && aSheetToken.front() == u'$')
            aSheetToken.remove_prefix(1);

        if (!aSheetToken.empty())
        {
            const api::String aSheetName = unquoteSheetName(aSheetToken);
            const auto oSheetId = rWorkbook.findSheetId(aSheetName);
            if (!oSheetId)
                return std::nullopt;
            nSheet = *oSheetId;
        }
    }

    api::StringView aAddressToken = rToken.substr(nDotPos + 1);
    if (!aAddressToken.empty() && aAddressToken.front() == u'$')
        aAddressToken.remove_prefix(1);

    std::size_t nColumnEnd = 0;
    while (nColumnEnd < aAddressToken.size())
    {
        const char16_t cChar = aAddressToken[nColumnEnd];
        const bool bAlpha = (cChar >= u'A' && cChar <= u'Z') || (cChar >= u'a' && cChar <= u'z');
        if (!bAlpha)
            break;
        ++nColumnEnd;
    }

    if (nColumnEnd == 0)
        return std::nullopt;

    const auto oColumn = parseColumnName(aAddressToken.substr(0, nColumnEnd));
    if (!oColumn)
        return std::nullopt;

    aAddressToken.remove_prefix(nColumnEnd);
    if (!aAddressToken.empty() && aAddressToken.front() == u'$')
        aAddressToken.remove_prefix(1);
    if (aAddressToken.empty())
        return std::nullopt;

    sal_Int64 nRow = 0;
    for (const char16_t cChar : aAddressToken)
    {
        if (cChar < u'0' || cChar > u'9')
            return std::nullopt;
        nRow = nRow * 10 + (cChar - u'0');
    }

    if (nRow <= 0)
        return std::nullopt;

    return api::CellAddress { nSheet, *oColumn, static_cast<api::RowIndex>(nRow - 1) };
}

[[nodiscard]] EvaluationResult ensureScalarValue(Evaluator& rEvaluator, EvaluationResult aResult)
{
    if (!aResult)
        return aResult;
    if (aResult.maValue.isScalar())
        return aResult;
    if (!aResult.maValue.maReference.isSingleCell())
        return makeFailure(api::Error::IllegalArgument);
    return rEvaluator.materializeReferenceValue(aResult.maValue.maReference, 0, 0);
}

[[nodiscard]] bool evaluateNumericComparison(
    double fLeft, double fRight, formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case formula::BinaryOperator::Equal:
            return fLeft == fRight;
        case formula::BinaryOperator::NotEqual:
            return fLeft != fRight;
        case formula::BinaryOperator::Less:
            return fLeft < fRight;
        case formula::BinaryOperator::LessEqual:
            return fLeft <= fRight;
        case formula::BinaryOperator::Greater:
            return fLeft > fRight;
        case formula::BinaryOperator::GreaterEqual:
            return fLeft >= fRight;
        default:
            return false;
    }
}

[[nodiscard]] bool evaluateStringComparison(
    api::StringView rLeft, api::StringView rRight, formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case formula::BinaryOperator::Equal:
            return rLeft == rRight;
        case formula::BinaryOperator::NotEqual:
            return rLeft != rRight;
        case formula::BinaryOperator::Less:
            return rLeft < rRight;
        case formula::BinaryOperator::LessEqual:
            return rLeft <= rRight;
        case formula::BinaryOperator::Greater:
            return rLeft > rRight;
        case formula::BinaryOperator::GreaterEqual:
            return rLeft >= rRight;
        default:
            return false;
    }
}

} // namespace

const workbook::Sheet* Evaluator::getSheet(api::SheetId nSheet) const
{
    if (nSheet < 0 || static_cast<std::size_t>(nSheet) >= mrWorkbook.maSheets.size())
        return nullptr;
    return &mrWorkbook.maSheets[static_cast<std::size_t>(nSheet)];
}

const workbook::Cell* Evaluator::getCell(const api::CellAddress& rAddress) const
{
    const workbook::Sheet* pSheet = getSheet(rAddress.mnSheet);
    return pSheet ? pSheet->findCell(rAddress.mnColumn, rAddress.mnRow) : nullptr;
}

EvaluationResult Evaluator::materializeReferenceValue(
    const api::ResolvedReference& rReference, api::ColumnIndex nColumnOffset,
    api::RowIndex nRowOffset)
{
    if (!rReference.isNormalized() || !rReference.containsOffset(nColumnOffset, nRowOffset))
        return makeFailure(api::Error::IllegalArgument);

    return evaluateCell(rReference.addressAt(nColumnOffset, nRowOffset));
}

api::ValueResult<api::ResolvedReference> Evaluator::resolveReferenceText(
    api::StringView rReference, api::SheetId nCurrentSheet) const
{
    const std::size_t nColonPos = rReference.find(u':');
    if (nColonPos == api::StringView::npos)
    {
        const auto oAddress = parseCellAddressToken(rReference, mrWorkbook, nCurrentSheet);
        if (!oAddress)
            return api::ValueResult<api::ResolvedReference>::failure(api::Error::IllegalArgument);

        return api::ValueResult<api::ResolvedReference>::success({ { *oAddress, *oAddress } });
    }

    const auto oStart
        = parseCellAddressToken(rReference.substr(0, nColonPos), mrWorkbook, nCurrentSheet);
    if (!oStart)
        return api::ValueResult<api::ResolvedReference>::failure(api::Error::IllegalArgument);

    const auto oEnd = parseCellAddressToken(
        rReference.substr(nColonPos + 1), mrWorkbook, oStart->mnSheet);
    if (!oEnd || oStart->mnSheet != oEnd->mnSheet)
        return api::ValueResult<api::ResolvedReference>::failure(api::Error::IllegalArgument);

    api::CellRange aRange { *oStart, *oEnd };
    if (aRange.maStart.mnColumn > aRange.maEnd.mnColumn)
        std::swap(aRange.maStart.mnColumn, aRange.maEnd.mnColumn);
    if (aRange.maStart.mnRow > aRange.maEnd.mnRow)
        std::swap(aRange.maStart.mnRow, aRange.maEnd.mnRow);

    return api::ValueResult<api::ResolvedReference>::success({ aRange });
}

api::ValueResult<api::ResolvedReference> Evaluator::resolveNamedRange(
    api::StringView rName, api::SheetId nScopeSheet) const
{
    if (nScopeSheet >= 0 && static_cast<std::size_t>(nScopeSheet) < mrWorkbook.maSheets.size())
    {
        const auto& rSheet = mrWorkbook.maSheets[static_cast<std::size_t>(nScopeSheet)];
        if (const auto* pLocal = mrWorkbook.findNamedRange(rName, rSheet.maName))
            return resolveReferenceText(pLocal->maCellRangeAddress, nScopeSheet);
    }

    if (const auto* pGlobal = mrWorkbook.findNamedRange(rName))
        return resolveReferenceText(pGlobal->maCellRangeAddress, nScopeSheet);

    return api::ValueResult<api::ResolvedReference>::failure(api::Error::NotAvailable);
}

EvaluationResult Evaluator::evaluateReferenceNode(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    switch (rNode.meKind)
    {
        case formula::NodeKind::CellReference:
        {
            const auto aReference = resolveReferenceText(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aReference)
                return makeFailure(aReference.meError);
            return makeReferenceResult(aReference.maValue);
        }
        case formula::NodeKind::RangeReference:
        {
            api::String aReference = rNode.maPrimaryText;
            aReference.push_back(u':');
            aReference += rNode.maSecondaryText;
            const auto aRange = resolveReferenceText(aReference, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeFailure(aRange.meError);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::NamedReference:
        {
            const auto aRange = resolveNamedRange(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeFailure(aRange.meError);
            return makeReferenceResult(aRange.maValue);
        }
        default:
        {
            EvaluationResult aValue = evaluateNode(rNode, rCurrentAddress);
            if (!aValue)
                return aValue;
            if (!aValue.maValue.isMatrixReference())
                return makeFailure(api::Error::IllegalArgument);
            return aValue;
        }
    }
}

EvaluationResult Evaluator::evaluateFunction(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);

    if (aFunctionName == u"TRUE")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::boolean(true));
    }

    if (aFunctionName == u"FALSE")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::boolean(false));
    }

    if (aFunctionName == u"NA")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
    }

    if (aFunctionName == u"FORMULA")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aReference = evaluateReferenceNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aReference)
            return aReference;
        if (!aReference.maValue.isMatrixReference() || !aReference.maValue.maReference.isSingleCell())
            return makeFailure(api::Error::IllegalArgument);

        const workbook::Cell* pCell = getCell(aReference.maValue.maReference.maRange.maStart);
        if (!pCell || !pCell->hasFormula())
            return makeScalarResult(api::CellValue::text({}));

        const formula::ParseResult aParsed = formula::parseFormula(pCell->maFormula);
        if (!aParsed)
            return makeFailure(api::Error::IllegalArgument);

        const auto oDisplay = formatFormulaNodeForDisplay(*aParsed.mpRoot);
        if (!oDisplay)
            return makeFailure(api::Error::IllegalArgument);

        api::String aFormula = u"=";
        aFormula += *oDisplay;
        return makeScalarResult(api::CellValue::text(aFormula));
    }

    if (aFunctionName == u"ISERROR")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return makeScalarResult(api::CellValue::boolean(true));
        return makeScalarResult(api::CellValue::boolean(aArgument.maValue.maValue.isError()));
    }

    if (aFunctionName == u"ISNA")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return makeScalarResult(api::CellValue::boolean(
                aArgument.meError == api::Error::NotAvailable));
        return makeScalarResult(api::CellValue::boolean(
            aArgument.maValue.maValue.isError()
            && aArgument.maValue.maValue.meError == api::Error::NotAvailable));
    }

    if (aFunctionName == u"AND")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        bool bResult = true;
        bool bSawValue = false;
        for (const auto& pChild : rNode.maChildren)
        {
            const bool bReferenceLike = pChild->meKind == formula::NodeKind::CellReference
                                        || pChild->meKind == formula::NodeKind::RangeReference
                                        || pChild->meKind == formula::NodeKind::NamedReference;

            EvaluationResult aArgument = bReferenceLike
                                             ? evaluateReferenceNode(*pChild, rCurrentAddress)
                                             : evaluateNode(*pChild, rCurrentAddress);
            if (!aArgument)
                return aArgument;

            if (aArgument.maValue.isMatrixReference())
            {
                const auto& rReference = aArgument.maValue.maReference;
                for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
                {
                    for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                    {
                        EvaluationResult aCell = materializeReferenceValue(rReference, nCol, nRow);
                        if (!aCell)
                            return aCell;
                        if (aCell.maValue.maValue.isEmpty() || aCell.maValue.maValue.isText())
                            continue;
                        const auto aBool = coerceToBoolean(aCell.maValue.maValue);
                        if (!aBool)
                            return makeFailure(aBool.meError);
                        bResult = bResult && aBool.maValue;
                        bSawValue = true;
                    }
                }
                continue;
            }

            const auto aBool = coerceToBoolean(aArgument.maValue.maValue);
            if (!aBool)
                return makeFailure(aBool.meError);
            bResult = bResult && aBool.maValue;
            bSawValue = true;
        }

        if (!bSawValue)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::boolean(bResult));
    }

    return makeFailure(api::Error::IllegalArgument);
}

EvaluationResult Evaluator::evaluateNode(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    switch (rNode.meKind)
    {
        case formula::NodeKind::NumberLiteral:
            return makeScalarResult(api::CellValue::number(rNode.mfNumber));
        case formula::NodeKind::StringLiteral:
            return makeScalarResult(api::CellValue::text(rNode.maPrimaryText));
        case formula::NodeKind::BooleanLiteral:
            return makeScalarResult(api::CellValue::boolean(rNode.mbBoolean));
        case formula::NodeKind::ErrorLiteral:
            return makeScalarResult(api::CellValue::error(mapErrorLiteral(rNode.maPrimaryText)));
        case formula::NodeKind::EmptyArgument:
            return makeScalarResult(api::CellValue::empty());
        case formula::NodeKind::CellReference:
        {
            const auto aReference = resolveReferenceText(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aReference)
                return makeFailure(aReference.meError);
            return materializeReferenceValue(aReference.maValue, 0, 0);
        }
        case formula::NodeKind::RangeReference:
        {
            api::String aReference = rNode.maPrimaryText;
            aReference.push_back(u':');
            aReference += rNode.maSecondaryText;
            const auto aRange = resolveReferenceText(aReference, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeFailure(aRange.meError);
            if (aRange.maValue.isSingleCell())
                return materializeReferenceValue(aRange.maValue, 0, 0);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::NamedReference:
        {
            const auto aRange = resolveNamedRange(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeFailure(aRange.meError);
            if (aRange.maValue.isSingleCell())
                return materializeReferenceValue(aRange.maValue, 0, 0);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::UnaryOperation:
        {
            EvaluationResult aChild = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
            if (!aChild)
                return aChild;
            const auto aNumber = coerceToNumber(aChild.maValue.maValue);
            if (!aNumber)
                return makeFailure(aNumber.meError);
            const double fValue = rNode.meUnaryOperator == formula::UnaryOperator::Minus
                                      ? -aNumber.maValue
                                      : aNumber.maValue;
            return makeScalarResult(api::CellValue::number(fValue));
        }
        case formula::NodeKind::BinaryOperation:
        {
            EvaluationResult aLeft = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
            if (!aLeft)
                return aLeft;
            EvaluationResult aRight = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aRight)
                return aRight;

            if (rNode.meBinaryOperator == formula::BinaryOperator::Concat)
            {
                const auto aLeftText = coerceToString(aLeft.maValue.maValue);
                if (!aLeftText)
                    return makeFailure(aLeftText.meError);
                const auto aRightText = coerceToString(aRight.maValue.maValue);
                if (!aRightText)
                    return makeFailure(aRightText.meError);
                api::String aValue = aLeftText.maValue;
                aValue += aRightText.maValue;
                return makeScalarResult(api::CellValue::text(aValue));
            }

            if (rNode.meBinaryOperator == formula::BinaryOperator::Equal
                || rNode.meBinaryOperator == formula::BinaryOperator::NotEqual
                || rNode.meBinaryOperator == formula::BinaryOperator::Less
                || rNode.meBinaryOperator == formula::BinaryOperator::LessEqual
                || rNode.meBinaryOperator == formula::BinaryOperator::Greater
                || rNode.meBinaryOperator == formula::BinaryOperator::GreaterEqual)
            {
                if (aLeft.maValue.maValue.isText() && aRight.maValue.maValue.isText())
                {
                    return makeScalarResult(api::CellValue::boolean(evaluateStringComparison(
                        aLeft.maValue.maValue.maString, aRight.maValue.maValue.maString,
                        rNode.meBinaryOperator)));
                }

                const auto aLeftNumber = coerceToNumber(aLeft.maValue.maValue);
                if (!aLeftNumber)
                    return makeFailure(aLeftNumber.meError);
                const auto aRightNumber = coerceToNumber(aRight.maValue.maValue);
                if (!aRightNumber)
                    return makeFailure(aRightNumber.meError);
                return makeScalarResult(api::CellValue::boolean(evaluateNumericComparison(
                    aLeftNumber.maValue, aRightNumber.maValue, rNode.meBinaryOperator)));
            }

            const auto aLeftNumber = coerceToNumber(aLeft.maValue.maValue);
            if (!aLeftNumber)
                return makeFailure(aLeftNumber.meError);
            const auto aRightNumber = coerceToNumber(aRight.maValue.maValue);
            if (!aRightNumber)
                return makeFailure(aRightNumber.meError);

            switch (rNode.meBinaryOperator)
            {
                case formula::BinaryOperator::Add:
                    return makeScalarResult(
                        api::CellValue::number(aLeftNumber.maValue + aRightNumber.maValue));
                case formula::BinaryOperator::Subtract:
                    return makeScalarResult(
                        api::CellValue::number(aLeftNumber.maValue - aRightNumber.maValue));
                case formula::BinaryOperator::Multiply:
                    return makeScalarResult(
                        api::CellValue::number(aLeftNumber.maValue * aRightNumber.maValue));
                case formula::BinaryOperator::Divide:
                    if (aRightNumber.maValue == 0.0)
                        return makeFailure(api::Error::DivisionByZero);
                    return makeScalarResult(
                        api::CellValue::number(aLeftNumber.maValue / aRightNumber.maValue));
                case formula::BinaryOperator::Power:
                    return makeScalarResult(api::CellValue::number(
                        std::pow(aLeftNumber.maValue, aRightNumber.maValue)));
                default:
                    return makeFailure(api::Error::IllegalArgument);
            }
        }
        case formula::NodeKind::FunctionCall:
            return evaluateFunction(rNode, rCurrentAddress);
    }

    return makeFailure(api::Error::IllegalArgument);
}

EvaluationResult Evaluator::evaluateFormula(
    api::StringView rFormula, const api::CellAddress& rCurrentAddress)
{
    const formula::ParseResult aParse = formula::parseFormula(rFormula);
    if (!aParse)
        return makeFailure(api::Error::IllegalArgument);
    return evaluateNode(*aParse.mpRoot, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateCell(const api::CellAddress& rAddress)
{
    if (!getSheet(rAddress.mnSheet))
        return makeFailure(api::Error::IllegalArgument);

    const workbook::Cell* pCell = getCell(rAddress);
    if (!pCell)
        return makeScalarResult(api::CellValue::empty());
    if (!pCell->hasFormula())
        return makeScalarResult(pCell->maValue);

    CacheEntry& rEntry = maCellCache[makeAddressKey(rAddress)];
    if (rEntry.meState == CacheState::Complete)
        return rEntry.maResult;

    if (rEntry.meState == CacheState::Active)
    {
        EvaluationResult aCycle = makeFailure(api::Error::IllegalArgument);
        auto aIt = std::find(maEvaluationStack.begin(), maEvaluationStack.end(), rAddress);
        if (aIt != maEvaluationStack.end())
            aCycle.maCyclePath.assign(aIt, maEvaluationStack.end());
        aCycle.maCyclePath.push_back(rAddress);
        return aCycle;
    }

    rEntry.meState = CacheState::Active;
    maEvaluationStack.push_back(rAddress);

    const auto finalize = [&](EvaluationResult aResult) -> EvaluationResult {
        maEvaluationStack.pop_back();
        rEntry.meState = CacheState::Complete;
        rEntry.maResult = aResult;
        return aResult;
    };

    EvaluationResult aResult = evaluateFormula(pCell->maFormula, rAddress);
    if (aResult && aResult.maValue.isMatrixReference())
    {
        if (aResult.maValue.maReference.isSingleCell())
            aResult = materializeReferenceValue(aResult.maValue.maReference, 0, 0);
        else
            aResult = makeFailure(api::Error::IllegalArgument);
    }

    if (!aResult && aResult.maCyclePath.empty() && hasCachedFallbackValue(*pCell))
        return finalize(makeScalarResult(pCell->maValue, true));

    return finalize(aResult);
}

} // namespace spreadsheetengine::core::fods

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
