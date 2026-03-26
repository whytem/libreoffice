/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rtl/ustring.hxx>

#include <spreadsheetengine/api/String.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::String toApiString(const OUString& rValue)
{
    return spreadsheetengine::api::String(rValue.getStr(), rValue.getLength());
}

inline OUString toLibreOfficeString(const spreadsheetengine::api::String& rValue)
{
    return OUString(
        reinterpret_cast<const sal_Unicode*>(rValue.data()),
        static_cast<sal_Int32>(rValue.size()));
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
