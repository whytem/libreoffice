/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <unotools/bootstrap.hxx>
#include <vcl/svapp.hxx>

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

struct DirectInfoInspectionRequest
{
    spreadsheetengine::api::String maAutoRecalcLabel;
    spreadsheetengine::api::String maManualRecalcLabel;
    bool mbAutoCalc = false;
};

struct DirectInfoInspectionEvaluation
{
    InfoKind meKind = InfoKind::Unsupported;
    spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue> maResult
        = spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::failure(
            spreadsheetengine::api::Error::IllegalArgument);
    bool mbHandled = false;
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

class DirectInfoInspectionAdapter
{
public:
    [[nodiscard]] DirectInfoInspectionEvaluation evaluateInfo(
        const OUString& rInfoType, const DirectInfoInspectionRequest& rRequest) const
    {
        DirectInfoInspectionEvaluation aEvaluation;
        aEvaluation.meKind = classifyInfoType(rInfoType);

        switch (aEvaluation.meKind)
        {
            case InfoKind::System:
            case InfoKind::NumFile:
                aEvaluation.maResult
                    = spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::success(
                        makeStaticInfoValue(aEvaluation.meKind));
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::OSVersion:
#if (defined LINUX || defined __FreeBSD__)
                aEvaluation.maResult
                    = spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::success(
                        spreadsheetengine::api::CellValue::text(
                            toApiString(Application::GetOSVersion())));
#elif defined MACOSX
                aEvaluation.maResult
                    = spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::success(
                        spreadsheetengine::api::CellValue::text(
                            u"Windows (32-bit) NT 5.01"));
#else
                aEvaluation.maResult
                    = spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::success(
                        spreadsheetengine::api::CellValue::text(
                            u"Windows (32-bit) NT 5.01"));
#endif
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Release:
                aEvaluation.maResult
                    = spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::success(
                        spreadsheetengine::api::CellValue::text(
                            toApiString(::utl::Bootstrap::getBuildIdData(OUString()))));
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Recalc:
                aEvaluation.maResult
                    = spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::success(
                        spreadsheetengine::api::CellValue::text(
                            rRequest.mbAutoCalc ? rRequest.maAutoRecalcLabel
                                                : rRequest.maManualRecalcLabel));
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Directory:
            case InfoKind::MemAvail:
            case InfoKind::MemUsed:
            case InfoKind::Origin:
            case InfoKind::TotMem:
                aEvaluation.maResult
                    = spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::failure(
                        spreadsheetengine::api::Error::NotAvailable);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Unsupported:
                return aEvaluation;
        }

        return aEvaluation;
    }
};

} // namespace spreadsheetengine::compat::libreoffice::infoinspectionexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
