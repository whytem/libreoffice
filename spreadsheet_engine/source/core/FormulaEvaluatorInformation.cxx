/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FormulaEvaluatorInternals.hxx"

#include <algorithm>
#include <array>

namespace spreadsheetengine::core::eval
{
namespace
{

template <std::size_t N>
[[nodiscard]] bool matchesFunctionRegistry(
    api::StringView rFunctionName, const std::array<api::StringView, N>& rRegistry)
{
    return std::find(rRegistry.begin(), rRegistry.end(), rFunctionName) != rRegistry.end();
}

} // namespace

std::optional<EvaluationResult> Evaluator::tryEvaluateInformationFamily(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    static constexpr std::array kInformationFunctions{
        api::StringView(u"ISERROR"),
        api::StringView(u"ISERR"),
        api::StringView(u"ISNUMBER"),
        api::StringView(u"ISNA"),
        api::StringView(u"ISTEXT"),
        api::StringView(u"ISNONTEXT"),
        api::StringView(u"ISBLANK"),
        api::StringView(u"ISEVEN"),
        api::StringView(u"ISODD"),
        api::StringView(u"ISREF"),
        api::StringView(u"N"),
        api::StringView(u"TYPE"),
        api::StringView(u"ROW"),
        api::StringView(u"COLUMN"),
        api::StringView(u"ERROR.TYPE"),
        api::StringView(u"ERRORTYPE"),
    };
    if (!matchesFunctionRegistry(rFunctionName, kInformationFunctions))
        return std::nullopt;
    return evaluateInformationFamilyBody(rFunctionName, rNode, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateInformationFamilyBody(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    const api::StringView aFunctionName = rFunctionName;

    if (aFunctionName == u"ISERROR")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return makeScalarResult(api::CellValue::boolean(true));
        return makeScalarResult(api::CellValue::boolean(aArgument.maValue.maValue.isError()));
    }

    if (aFunctionName == u"ISERR")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return makeScalarResult(
                api::CellValue::boolean(aArgument.meError != api::Error::NotAvailable));
        return makeScalarResult(api::CellValue::boolean(
            aArgument.maValue.maValue.isError()
            && aArgument.maValue.maValue.meError != api::Error::NotAvailable));
    }

    if (aFunctionName == u"ISNUMBER")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (aArgument && !aArgument.maValue.isScalar())
            aArgument = materializeReferenceValue(aArgument.maValue.maReference, 0, 0);
        if (!aArgument)
            return makeScalarResult(api::CellValue::boolean(false));
        return makeScalarResult(api::CellValue::boolean(
            aArgument.maValue.maValue.isNumber() || aArgument.maValue.maValue.isBoolean()));
    }

    if (aFunctionName == u"ISTEXT" || aFunctionName == u"ISNONTEXT")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (aArgument && !aArgument.maValue.isScalar())
            aArgument = materializeReferenceValue(aArgument.maValue.maReference, 0, 0);
        if (!aArgument)
            return makeScalarResult(api::CellValue::boolean(false));
        const bool bIsText = aArgument.maValue.maValue.isText();
        return makeScalarResult(api::CellValue::boolean(
            aFunctionName == u"ISTEXT" ? bIsText : !bIsText));
    }

    if (aFunctionName == u"ISBLANK")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (aArgument && !aArgument.maValue.isScalar())
            aArgument = materializeReferenceValue(aArgument.maValue.maReference, 0, 0);
        if (!aArgument)
            return makeScalarResult(api::CellValue::boolean(false));
        return makeScalarResult(api::CellValue::boolean(aArgument.maValue.maValue.isEmpty()));
    }

    if (aFunctionName == u"ISEVEN" || aFunctionName == u"ISODD")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValue)
            return makeFailure(aValue.meError);

        const auto aNumber = coerceToNumber(aValue.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);

        const std::int64_t nValue = static_cast<std::int64_t>(std::floor(std::abs(aNumber.maValue)));
        const bool bEven = (nValue % 2) == 0;
        return makeScalarResult(api::CellValue::boolean(
            aFunctionName == u"ISEVEN" ? bEven : !bEven));
    }

    if (aFunctionName == u"ISNA")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
        {
            return makeScalarResult(api::CellValue::boolean(
                aArgument.meError == api::Error::NotAvailable));
        }
        return makeScalarResult(api::CellValue::boolean(
            aArgument.maValue.maValue.isError()
            && aArgument.maValue.maValue.meError == api::Error::NotAvailable));
    }

    if (aFunctionName == u"ISREF")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const formula::Node& rArgument = *rNode.maChildren[0];
        bool bIsReference = false;
        if (rArgument.meKind == formula::NodeKind::CellReference)
        {
            const auto aResolved = resolveReferenceText(rArgument.maPrimaryText, rCurrentAddress.mnSheet);
            bIsReference = aResolved.ok();
        }
        else if (rArgument.meKind == formula::NodeKind::RangeReference)
        {
            api::String aAddress = rArgument.maPrimaryText;
            aAddress.push_back(u':');
            aAddress += rArgument.maSecondaryText;
            const auto aResolved = resolveReferenceText(aAddress, rCurrentAddress.mnSheet);
            bIsReference = aResolved.ok();
        }
        else if (rArgument.meKind == formula::NodeKind::NamedReference)
        {
            const auto aResolved = resolveNamedRange(rArgument.maPrimaryText, rCurrentAddress.mnSheet);
            bIsReference = aResolved.ok();
        }

        return makeScalarResult(api::CellValue::boolean(bIsReference));
    }

    if (aFunctionName == u"N")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (aArgument && !aArgument.maValue.isScalar())
            aArgument = materializeReferenceValue(aArgument.maValue.maReference, 0, 0);
        if (!aArgument)
            return makeFailure(aArgument.meError);

        const api::CellValue& rValue = aArgument.maValue.maValue;
        if (rValue.isError())
            return makeScalarResult(api::CellValue::error(rValue.meError));
        if (rValue.isNumber() || rValue.isBoolean())
            return makeScalarResult(api::CellValue::number(rValue.mfNumber));
        return makeScalarResult(api::CellValue::number(0.0));
    }

    if (aFunctionName == u"TYPE")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aArgument)
            return makeScalarResult(api::CellValue::number(16.0));
        if (!aArgument.maValue.isScalar())
            return makeScalarResult(api::CellValue::number(64.0));

        const api::CellValue& rValue = aArgument.maValue.maValue;
        if (rValue.isNumber())
            return makeScalarResult(api::CellValue::number(1.0));
        if (rValue.isText())
            return makeScalarResult(api::CellValue::number(2.0));
        if (rValue.isBoolean())
            return makeScalarResult(api::CellValue::number(4.0));
        if (rValue.isError())
            return makeScalarResult(api::CellValue::number(16.0));
        return makeScalarResult(api::CellValue::number(1.0));
    }

    if (aFunctionName == u"ROW" || aFunctionName == u"COLUMN")
    {
        if (rNode.maChildren.size() > 1)
            return makeFailure(api::Error::IllegalArgument);

        const bool bRow = aFunctionName == u"ROW";
        if (rNode.maChildren.empty()
            || rNode.maChildren[0]->meKind == formula::NodeKind::EmptyArgument)
        {
            return makeScalarResult(api::CellValue::number(
                static_cast<double>(bRow ? rCurrentAddress.mnRow + 1 : rCurrentAddress.mnColumn + 1)));
        }

        const formula::Node& rArgument = *rNode.maChildren[0];
        EvaluationResult aArgument = evaluateNode(rArgument, rCurrentAddress);
        if (!aArgument)
            return makeFailure(aArgument.meError);

        if (aArgument.maValue.isMatrixReference())
        {
            const auto& rStart = aArgument.maValue.maReference.maRange.maStart;
            return makeScalarResult(api::CellValue::number(
                static_cast<double>(bRow ? rStart.mnRow + 1 : rStart.mnColumn + 1)));
        }

        if (rArgument.meKind == formula::NodeKind::CellReference)
        {
            const auto aResolved = resolveReferenceText(rArgument.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aResolved || !aResolved.maValue.isSingleCell())
                return makeFailure(aResolved.meError);
            const auto& rStart = aResolved.maValue.maRange.maStart;
            return makeScalarResult(api::CellValue::number(
                static_cast<double>(bRow ? rStart.mnRow + 1 : rStart.mnColumn + 1)));
        }

        if (rArgument.meKind == formula::NodeKind::RangeReference)
        {
            api::String aAddress = rArgument.maPrimaryText;
            aAddress.push_back(u':');
            aAddress += rArgument.maSecondaryText;
            const auto aResolved = resolveReferenceText(aAddress, rCurrentAddress.mnSheet);
            if (!aResolved)
                return makeFailure(aResolved.meError);
            const auto& rStart = aResolved.maValue.maRange.maStart;
            return makeScalarResult(api::CellValue::number(
                static_cast<double>(bRow ? rStart.mnRow + 1 : rStart.mnColumn + 1)));
        }

        if (rArgument.meKind == formula::NodeKind::NamedReference)
        {
            const auto aResolved = resolveNamedRange(rArgument.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aResolved)
                return makeFailure(aResolved.meError);
            const auto& rStart = aResolved.maValue.maRange.maStart;
            return makeScalarResult(api::CellValue::number(
                static_cast<double>(bRow ? rStart.mnRow + 1 : rStart.mnColumn + 1)));
        }

        return makeFailure(api::Error::IllegalArgument);
    }

    if (aFunctionName == u"ERROR.TYPE" || aFunctionName == u"ERRORTYPE")
    {
        const bool bLegacyErrorType = aFunctionName == u"ERRORTYPE";
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const formula::Node& rArgument = *rNode.maChildren[0];
        const auto classifyReferenceErrorType = [&](const formula::Node& rReferenceNode)
            -> std::optional<std::int32_t> {
            api::ValueResult<api::ResolvedReference> aReference
                = api::ValueResult<api::ResolvedReference>::failure(api::Error::IllegalArgument);

            if (rReferenceNode.meKind == formula::NodeKind::CellReference)
            {
                aReference
                    = resolveReferenceText(rReferenceNode.maPrimaryText, rCurrentAddress.mnSheet);
                if (!aReference && aReference.meError == api::Error::IllegalArgument)
                    return bLegacyErrorType ? 524 : 4;
            }
            else if (rReferenceNode.meKind == formula::NodeKind::RangeReference)
            {
                api::String aAddress = rReferenceNode.maPrimaryText;
                aAddress.push_back(u':');
                aAddress += rReferenceNode.maSecondaryText;
                aReference = resolveReferenceText(aAddress, rCurrentAddress.mnSheet);
                if (!aReference && aReference.meError == api::Error::IllegalArgument)
                    return bLegacyErrorType ? 524 : 4;
            }
            else if (rReferenceNode.meKind == formula::NodeKind::NamedReference)
            {
                aReference
                    = resolveNamedRange(rReferenceNode.maPrimaryText, rCurrentAddress.mnSheet);
                if (!aReference && aReference.meError == api::Error::NotAvailable)
                    return bLegacyErrorType ? 525 : 5;
            }
            else
            {
                return std::nullopt;
            }

            if (!aReference || !aReference.maValue.isSingleCell())
                return bLegacyErrorType ? std::optional<std::int32_t>(519) : std::nullopt;

            const workbook::Cell* pCell = getCell(aReference.maValue.maRange.maStart);
            if (!pCell)
                return std::nullopt;

            if (pCell->maRawValueType == u"error")
            {
                if (bLegacyErrorType)
                    return classifyLegacyErrorTypeLiteral(pCell->maRawValue);
                return classifyOdfErrorTypeLiteral(pCell->maRawValue);
            }

            if (pCell->maValue.isError())
            {
                if (bLegacyErrorType)
                    return classifyLegacyErrorType(pCell->maValue.meError);
                return classifyOdfErrorType(pCell->maValue.meError);
            }

            return std::nullopt;
        };

        if (rArgument.meKind == formula::NodeKind::ErrorLiteral)
        {
            const api::String aUpperLiteral = uppercaseAscii(rArgument.maPrimaryText);
            if (!bLegacyErrorType
                && (aUpperLiteral == u"#GETTING_DATA"
                    || (aUpperLiteral.starts_with(u"#ERR") && aUpperLiteral.size() > 5
                        && aUpperLiteral.back() == u'!')))
            {
                return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
            }
            const std::optional<std::int32_t> oErrorType = bLegacyErrorType
                                                            ? classifyLegacyErrorTypeLiteral(
                                                                  rArgument.maPrimaryText)
                                                            : std::optional<std::int32_t>(
                                                                  classifyOdfErrorTypeLiteral(
                                                                      rArgument.maPrimaryText));
            if (oErrorType)
                return makeScalarResult(api::CellValue::number(static_cast<double>(*oErrorType)));
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        if (const auto oReferenceErrorType = classifyReferenceErrorType(rArgument))
        {
            return makeScalarResult(api::CellValue::number(static_cast<double>(*oReferenceErrorType)));
        }

        EvaluationResult aArgument = evaluateNode(rArgument, rCurrentAddress);
        if (!aArgument)
        {
            if (rArgument.meKind == formula::NodeKind::NamedReference
                && aArgument.meError == api::Error::NotAvailable)
            {
                return makeScalarResult(
                    api::CellValue::number(bLegacyErrorType ? 525.0 : 5.0));
            }
            if ((rArgument.meKind == formula::NodeKind::CellReference
                    || rArgument.meKind == formula::NodeKind::RangeReference)
                && aArgument.meError == api::Error::IllegalArgument)
            {
                return makeScalarResult(
                    api::CellValue::number(bLegacyErrorType ? 524.0 : 4.0));
            }
            const std::optional<std::int32_t> oErrorType = bLegacyErrorType
                                                            ? classifyLegacyErrorType(
                                                                  aArgument.meError)
                                                            : std::optional<std::int32_t>(
                                                                  classifyOdfErrorType(
                                                                      aArgument.meError));
            if (oErrorType)
                return makeScalarResult(api::CellValue::number(static_cast<double>(*oErrorType)));
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        if (!aArgument.maValue.isScalar() || !aArgument.maValue.maValue.isError())
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        const std::optional<std::int32_t> oErrorType = bLegacyErrorType
                                                        ? classifyLegacyErrorType(
                                                              aArgument.maValue.maValue.meError)
                                                        : std::optional<std::int32_t>(
                                                              classifyOdfErrorType(
                                                                  aArgument.maValue.maValue.meError));
        if (oErrorType)
            return makeScalarResult(api::CellValue::number(static_cast<double>(*oErrorType)));
        return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
    }

    return makeFailure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::eval
