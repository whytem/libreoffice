/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/core/TextWidth.hxx>

#include <comphelper/processfactory.hxx>
#include <i18nutil/transliteration.hxx>
#include <unotools/transliterationwrapper.hxx>

namespace spreadsheetengine::core::text
{

OUString convertIntoHalfWidth(const OUString& rInput)
{
    auto init = []() -> utl::TransliterationWrapper&
    {
        static utl::TransliterationWrapper trans(
            comphelper::getProcessComponentContext(), static_cast<TransliterationFlags>(0));
        trans.loadModuleByImplName(u"FULLWIDTH_HALFWIDTH_LIKE_ASC"_ustr, LANGUAGE_SYSTEM);
        return trans;
    };
    static utl::TransliterationWrapper& rTrans(init());
    return rTrans.transliterate(rInput, 0, sal_uInt16(rInput.getLength()));
}

OUString convertIntoFullWidth(const OUString& rInput)
{
    auto init = []() -> utl::TransliterationWrapper&
    {
        static utl::TransliterationWrapper trans(
            comphelper::getProcessComponentContext(), static_cast<TransliterationFlags>(0));
        trans.loadModuleByImplName(u"HALFWIDTH_FULLWIDTH_LIKE_JIS"_ustr, LANGUAGE_SYSTEM);
        return trans;
    };
    static utl::TransliterationWrapper& rTrans(init());
    return rTrans.transliterate(rInput, 0, sal_uInt16(rInput.getLength()));
}

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
