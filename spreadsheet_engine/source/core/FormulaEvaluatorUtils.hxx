/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>

#include <spreadsheetengine/detail/FormulaEvaluator.hxx>

#include "CoreRuntimeUtils.hxx"

namespace spreadsheetengine::core::eval::detail
{

using spreadsheetengine::core::util::uppercaseAscii;
using spreadsheetengine::core::util::parseAsciiDouble;
using spreadsheetengine::core::util::coerceToNumber;
using spreadsheetengine::core::util::toWholeNumber;

[[nodiscard]] inline EvaluationResult makeScalarResult(
    const api::CellValue& rValue, bool bUsedCachedValue = false)
{
    EvaluationResult aResult;
    aResult.maValue = api::CellValueView::scalar(rValue);
    aResult.mbUsedCachedValue = bUsedCachedValue;
    return aResult;
}

[[nodiscard]] inline EvaluationResult makeReferenceResult(const api::ResolvedReference& rReference)
{
    EvaluationResult aResult;
    aResult.maValue = api::CellValueView::matrixReference(rReference);
    return aResult;
}

[[nodiscard]] inline EvaluationResult makeFailure(api::Error eError)
{
    EvaluationResult aResult;
    aResult.meError = eError;
    return aResult;
}

[[nodiscard]] inline bool hasFunctionPrefix(api::StringView rName, api::StringView rPrefix)
{
    return rName.substr(0, rPrefix.size()) == rPrefix;
}

[[nodiscard]] inline api::String normalizeDisplayFunctionName(api::StringView rName)
{
    const api::StringView aMicrosoftPrefix = u"COM.MICROSOFT.";
    const api::StringView aLibreOfficePrefix = u"ORG.LIBREOFFICE.";
    const api::StringView aOpenOfficePrefix = u"ORG.OPENOFFICE.";
    if (rName.substr(0, aMicrosoftPrefix.size()) == aMicrosoftPrefix)
        return api::String(rName.substr(aMicrosoftPrefix.size()));
    if (rName.substr(0, aLibreOfficePrefix.size()) == aLibreOfficePrefix)
        return api::String(rName.substr(aLibreOfficePrefix.size()));
    if (rName.substr(0, aOpenOfficePrefix.size()) == aOpenOfficePrefix)
        return api::String(rName.substr(aOpenOfficePrefix.size()));
    return api::String(rName);
}

[[nodiscard]] inline api::String normalizeFunctionName(api::StringView rName)
{
    return uppercaseAscii(normalizeDisplayFunctionName(rName));
}

[[nodiscard]] inline api::String formatNumber(double fValue)
{
    char aBuffer[32];
    const int nLength = std::snprintf(aBuffer, sizeof(aBuffer), "%.17G", fValue);
    const std::string aAscii(aBuffer, static_cast<std::size_t>(std::max(nLength, 0)));

    api::String aResult;
    aResult.reserve(aAscii.size());
    for (const char cChar : aAscii)
        aResult.push_back(static_cast<char16_t>(cChar));
    return aResult;
}

[[nodiscard]] inline api::String formatQuotedString(api::StringView rValue)
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

using spreadsheetengine::core::coercion::coerceToBoolean;
using spreadsheetengine::core::coercion::coerceToString;
using spreadsheetengine::core::coercion::normalizeStringPositionArgument;
using spreadsheetengine::core::coercion::normalizeOneBasedStringPositionArgument;
using spreadsheetengine::core::coercion::normalizeNonNegativeLengthArgument;

[[nodiscard]] inline EvaluationResult ensureScalarValue(Evaluator& rEvaluator, EvaluationResult aResult)
{
    if (!aResult)
        return aResult;
    if (aResult.maValue.isScalar())
        return aResult;
    if (!aResult.maValue.maReference.isSingleCell())
        return makeFailure(api::Error::IllegalArgument);
    return rEvaluator.materializeReferenceValue(aResult.maValue.maReference, 0, 0);
}

} // namespace spreadsheetengine::core::eval::detail

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
