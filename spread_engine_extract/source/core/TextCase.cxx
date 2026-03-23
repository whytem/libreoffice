/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/core/TextCase.hxx>

#include <rtl/ustrbuf.hxx>

namespace spreadsheetengine::core::text
{

OUString uppercase(const CharClass& rCharClass, const OUString& rInput)
{
    return rCharClass.uppercase(rInput);
}

OUString lowercase(const CharClass& rCharClass, const OUString& rInput)
{
    return rCharClass.lowercase(rInput);
}

OUString propercase(const CharClass& rCharClass, const OUString& rInput)
{
    OUStringBuffer aBuffer(rInput);
    const sal_Int32 nLength = aBuffer.getLength();
    if (nLength == 0)
        return aBuffer.makeStringAndClear();

    const OUString aUpper(rCharClass.uppercase(aBuffer.toString()));
    const OUString aLower(rCharClass.lowercase(aBuffer.toString()));
    aBuffer[0] = aUpper[0];

    for (sal_Int32 nPos = 1; nPos < nLength; ++nPos)
    {
        if (!rCharClass.isLetter(OUString(aBuffer[nPos - 1]), 0))
            aBuffer[nPos] = aUpper[nPos];
        else
            aBuffer[nPos] = aLower[nPos];
    }

    return aBuffer.makeStringAndClear();
}

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
