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
#include <editeng/justifyitem.hxx>
#include <formula/grammar.hxx>
#include <svl/numformat.hxx>
#include <svl/zformat.hxx>
#include <tools/urlobj.hxx>

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

[[nodiscard]] inline bool formatHasNegativeColor(const SvNumberformat* pFormat)
{
    return pFormat && pFormat->GetColor(1);
}

[[nodiscard]] inline bool formatHasOpenParenthesis(const SvNumberformat* pFormat)
{
    return pFormat && (pFormat->GetFormatstring().indexOf('(') != -1);
}

[[nodiscard]] inline spreadsheetengine::runtime::cellinspection::PrefixStyle toPrefixStyle(
    SvxCellHorJustify eJustify)
{
    switch (eJustify)
    {
        case SvxCellHorJustify::Standard:
        case SvxCellHorJustify::Left:
        case SvxCellHorJustify::Block:
            return spreadsheetengine::runtime::cellinspection::PrefixStyle::Left;
        case SvxCellHorJustify::Center:
            return spreadsheetengine::runtime::cellinspection::PrefixStyle::Center;
        case SvxCellHorJustify::Right:
            return spreadsheetengine::runtime::cellinspection::PrefixStyle::Right;
        case SvxCellHorJustify::Repeat:
            return spreadsheetengine::runtime::cellinspection::PrefixStyle::Repeat;
    }
    return spreadsheetengine::runtime::cellinspection::PrefixStyle::None;
}

[[nodiscard]] inline spreadsheetengine::api::CellValue makeFilenameValue(const OUString& rValue)
{
    return spreadsheetengine::runtime::cellinspection::textPropertyValue(toApiString(rValue));
}

[[nodiscard]] inline spreadsheetengine::api::CellValue makeFormatValue(const OUString& rValue)
{
    return spreadsheetengine::runtime::cellinspection::textPropertyValue(toApiString(rValue));
}

[[nodiscard]] inline spreadsheetengine::api::CellValue makeWidthValue(sal_Int32 nZeroCount)
{
    return spreadsheetengine::runtime::cellinspection::numericPropertyValue(
        static_cast<double>(nZeroCount));
}

[[nodiscard]] inline spreadsheetengine::api::CellValue makeFlagValue(bool bFlag)
{
    return spreadsheetengine::runtime::cellinspection::numericPropertyValue(bFlag ? 1.0 : 0.0);
}

[[nodiscard]] inline spreadsheetengine::api::CellValue makePrefixValue(
    bool bHasString, SvxCellHorJustify eJustify)
{
    if (!bHasString)
        return spreadsheetengine::runtime::cellinspection::textPropertyValue(u"");
    return spreadsheetengine::runtime::cellinspection::prefixValue(toPrefixStyle(eJustify));
}

[[nodiscard]] inline OUString formatLocalFilenameInfo(
    const INetURLObject& rUrlObject, const OUString& rTabName,
    formula::FormulaGrammar::AddressConvention eConvention, bool bLibreOfficeKitActive)
{
    if (eConvention == formula::FormulaGrammar::CONV_XL_A1
        || eConvention == formula::FormulaGrammar::CONV_XL_R1C1
        || eConvention == formula::FormulaGrammar::CONV_XL_OOX)
    {
        OUString aResult;
        if (!bLibreOfficeKitActive)
            aResult = rUrlObject.GetPartBeforeLastName();
        aResult += "["
                   + rUrlObject.GetLastName(INetURLObject::DecodeMechanism::Unambiguous) + "]"
                   + rTabName;
        return aResult;
    }

    OUString aResult = "'";
    if (!bLibreOfficeKitActive)
        aResult += rUrlObject.GetMainURL(INetURLObject::DecodeMechanism::Unambiguous);
    else
        aResult += rUrlObject.GetLastName(INetURLObject::DecodeMechanism::Unambiguous);
    aResult += "'#$" + rTabName;
    return aResult;
}

[[nodiscard]] inline OUString formatExternalFilenameInfo(
    const OUString& rFileName, const OUString& rTabName,
    formula::FormulaGrammar::AddressConvention eConvention)
{
    if (eConvention == formula::FormulaGrammar::CONV_XL_A1
        || eConvention == formula::FormulaGrammar::CONV_XL_R1C1
        || eConvention == formula::FormulaGrammar::CONV_XL_OOX)
    {
        const sal_Int32 nPos = rFileName.lastIndexOf('/');
        return OUString::Concat(rFileName.subView(0, nPos + 1)) + "["
               + rFileName.subView(nPos + 1) + "]" + rTabName;
    }

    return "'" + rFileName + "'#$" + rTabName;
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
        case InfoKind::Coord:
        case InfoKind::Filename:
        case InfoKind::Width:
        case InfoKind::Prefix:
        case InfoKind::Protect:
        case InfoKind::Format:
        case InfoKind::Color:
        case InfoKind::Parentheses:
        case InfoKind::Unsupported:
            break;
    }

    return spreadsheetengine::api::ValueResult<CellValue>::failure(
        spreadsheetengine::api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::compat::libreoffice::cellinspectionexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
