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
        api::StringView(u"ISNUMBER"),
        api::StringView(u"ISNA"),
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
