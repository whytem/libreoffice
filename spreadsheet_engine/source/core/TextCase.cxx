/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/TextCase.hxx>

namespace spreadsheetengine::core::text
{

spreadsheetengine::api::String uppercase(
    const CaseMappingService& rCaseService, spreadsheetengine::api::StringView rInput)
{
    return rCaseService.uppercase(rInput);
}

spreadsheetengine::api::String lowercase(
    const CaseMappingService& rCaseService, spreadsheetengine::api::StringView rInput)
{
    return rCaseService.lowercase(rInput);
}

spreadsheetengine::api::String propercase(
    const CaseMappingService& rCaseService, spreadsheetengine::api::StringView rInput)
{
    spreadsheetengine::api::String aBuffer(rInput);
    if (aBuffer.empty())
        return aBuffer;

    const auto aUpper = rCaseService.uppercase(aBuffer);
    const auto aLower = rCaseService.lowercase(aBuffer);
    aBuffer[0] = aUpper[0];

    for (std::size_t nPos = 1; nPos < aBuffer.size(); ++nPos)
    {
        if (!rCaseService.isLetter(aBuffer[nPos - 1]))
            aBuffer[nPos] = aUpper[nPos];
        else
            aBuffer[nPos] = aLower[nPos];
    }

    return aBuffer;
}

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
