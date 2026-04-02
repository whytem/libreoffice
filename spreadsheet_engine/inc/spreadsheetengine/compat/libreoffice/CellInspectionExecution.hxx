/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>

#include <address.hxx>
#include <formula/grammar.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/runtime/CellInspection.hxx>

namespace spreadsheetengine::compat::libreoffice::cellinspectionexecution
{

using InfoKind = spreadsheetengine::runtime::cellinspection::InfoKind;

[[nodiscard]] inline InfoKind classifyInfoType(const OUString& rInfoType)
{
    return spreadsheetengine::runtime::cellinspection::classifyInfoType(toApiString(rInfoType));
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>
evaluateBoundedCellInfo(InfoKind eKind, const ScAddress& rAddress,
    const spreadsheetengine::api::CellValue& rCellValue, formula::FormulaGrammar::AddressConvention eConvention,
    const std::optional<OUString>& roSheetName = std::nullopt)
{
    using spreadsheetengine::api::CellValue;

    switch (eKind)
    {
        case InfoKind::Column:
            return spreadsheetengine::api::ValueResult<CellValue>::success(
                spreadsheetengine::runtime::cellinspection::columnValue(
                    { rAddress.Tab(), rAddress.Col(), rAddress.Row() }));
        case InfoKind::Row:
            return spreadsheetengine::api::ValueResult<CellValue>::success(
                spreadsheetengine::runtime::cellinspection::rowValue(
                    { rAddress.Tab(), rAddress.Col(), rAddress.Row() }));
        case InfoKind::Sheet:
            return spreadsheetengine::api::ValueResult<CellValue>::success(
                spreadsheetengine::runtime::cellinspection::sheetValue(
                    { rAddress.Tab(), rAddress.Col(), rAddress.Row() }));
        case InfoKind::Address:
        {
            referenceexecution::AddressFunctionRequest aRequest;
            aRequest.mnRow = rAddress.Row();
            aRequest.mnColumn = rAddress.Col();
            aRequest.mnAbsMode = 1;
            aRequest.mbA1Style = eConvention != formula::FormulaGrammar::CONV_XL_R1C1;
            aRequest.meConvention = eConvention;
            if (roSheetName)
                aRequest.maSheetToken = *roSheetName;

            const auto aAddress = referenceexecution::formatAddressFunctionResult(aRequest);
            if (!aAddress)
            {
                return spreadsheetengine::api::ValueResult<CellValue>::failure(aAddress.meError);
            }
            return spreadsheetengine::api::ValueResult<CellValue>::success(
                CellValue::text(toApiString(aAddress.maValue)));
        }
        case InfoKind::Contents:
            return spreadsheetengine::api::ValueResult<CellValue>::success(
                spreadsheetengine::runtime::cellinspection::contentsValue(rCellValue));
        case InfoKind::Type:
            return spreadsheetengine::api::ValueResult<CellValue>::success(
                spreadsheetengine::runtime::cellinspection::typeValue(rCellValue));
        case InfoKind::Unsupported:
            break;
    }

    return spreadsheetengine::api::ValueResult<CellValue>::failure(
        spreadsheetengine::api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::compat::libreoffice::cellinspectionexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
