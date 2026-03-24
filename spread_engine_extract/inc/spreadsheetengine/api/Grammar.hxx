/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace spreadsheetengine::api
{

enum class FormulaLanguage
{
    Unknown,
    Native,
    English,
    XlEnglish,
    Api,
    Odf11,
    Odff,
    Ooxml,
    External
};

enum class AddressConvention
{
    Unknown,
    OooA1,
    OdfA1,
    XlA1,
    XlR1C1,
    XlOox
};

struct Grammar
{
    FormulaLanguage meLanguage = FormulaLanguage::Unknown;
    AddressConvention meAddressConvention = AddressConvention::Unknown;
    bool mbEnglish = false;
};

} // namespace spreadsheetengine::api

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
