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

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>

namespace spreadsheetengine::compat::libreoffice::infoinspectionexecution
{

enum class InfoKind : std::uint8_t
{
    Unsupported,
    System,
    OSVersion,
    Release,
    NumFile,
    Recalc,
    Directory,
    MemAvail,
    MemUsed,
    Origin,
    TotMem
};

[[nodiscard]] inline InfoKind classifyInfoType(const OUString& rInfoType)
{
    const auto aUpper = toApiString(rInfoType);
    if (aUpper == u"SYSTEM")
        return InfoKind::System;
    if (aUpper == u"OSVERSION")
        return InfoKind::OSVersion;
    if (aUpper == u"RELEASE")
        return InfoKind::Release;
    if (aUpper == u"NUMFILE")
        return InfoKind::NumFile;
    if (aUpper == u"RECALC")
        return InfoKind::Recalc;
    if (aUpper == u"DIRECTORY")
        return InfoKind::Directory;
    if (aUpper == u"MEMAVAIL")
        return InfoKind::MemAvail;
    if (aUpper == u"MEMUSED")
        return InfoKind::MemUsed;
    if (aUpper == u"ORIGIN")
        return InfoKind::Origin;
    if (aUpper == u"TOTMEM")
        return InfoKind::TotMem;
    return InfoKind::Unsupported;
}

[[nodiscard]] inline bool isUnavailableInfoKind(InfoKind eKind)
{
    switch (eKind)
    {
        case InfoKind::Directory:
        case InfoKind::MemAvail:
        case InfoKind::MemUsed:
        case InfoKind::Origin:
        case InfoKind::TotMem:
            return true;
        case InfoKind::Unsupported:
        case InfoKind::System:
        case InfoKind::OSVersion:
        case InfoKind::Release:
        case InfoKind::NumFile:
        case InfoKind::Recalc:
            return false;
    }
    return false;
}

[[nodiscard]] inline spreadsheetengine::api::CellValue makeStaticInfoValue(InfoKind eKind)
{
    switch (eKind)
    {
        case InfoKind::System:
            return spreadsheetengine::api::CellValue::text(u"" SC_INFO_OSVERSION "");
        case InfoKind::NumFile:
            return spreadsheetengine::api::CellValue::number(1.0);
        case InfoKind::Unsupported:
        case InfoKind::OSVersion:
        case InfoKind::Release:
        case InfoKind::Recalc:
        case InfoKind::Directory:
        case InfoKind::MemAvail:
        case InfoKind::MemUsed:
        case InfoKind::Origin:
        case InfoKind::TotMem:
            break;
    }
    return spreadsheetengine::api::CellValue::empty();
}

} // namespace spreadsheetengine::compat::libreoffice::infoinspectionexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
