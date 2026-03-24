/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <tools/date.hxx>

#include <spreadsheetengine/api/Date.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::DateParts toApiDateParts(const Date& rDate)
{
    return { rDate.GetYear(), static_cast<std::int16_t>(rDate.GetMonth()),
        static_cast<std::int16_t>(rDate.GetDay()) };
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
