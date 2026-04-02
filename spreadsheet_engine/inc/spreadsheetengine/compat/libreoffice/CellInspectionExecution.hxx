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
#include <comphelper/lok.hxx>
#include <document.hxx>
#include <docsh.hxx>
#include <editeng/justifyitem.hxx>
#include <externalrefmgr.hxx>
#include <formula/grammar.hxx>
#include <interpretercontext.hxx>
#include <patattr.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/printer.hxx>
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
using CellAddress = spreadsheetengine::api::CellAddress;
using CellValue = spreadsheetengine::api::CellValue;

struct BoundedCellInfoRequest
{
    CellAddress maAddress;
    CellValue maCellValue;
    formula::FormulaGrammar::AddressConvention meConvention
        = formula::FormulaGrammar::CONV_OOO;
    std::optional<spreadsheetengine::api::String> moSheetName;
};

struct DirectCellInfoEvaluation
{
    InfoKind meKind = InfoKind::Unsupported;
    spreadsheetengine::api::ValueResult<CellValue> maResult
        = spreadsheetengine::api::ValueResult<CellValue>::failure(
            spreadsheetengine::api::Error::IllegalArgument);
    bool mbHandled = false;
};

[[nodiscard]] inline InfoKind classifyInfoType(const OUString& rInfoType);

[[nodiscard]] inline spreadsheetengine::api::ValueResult<CellValue> evaluateBoundedCellInfo(
    InfoKind eKind, const BoundedCellInfoRequest& rRequest);

class DirectCellInspectionAdapter
{
    const ScDocument& mrDocument;
    ScAddress maFormulaPos;
    formula::FormulaGrammar::AddressConvention meConvention
        = formula::FormulaGrammar::CONV_OOO;

public:
    DirectCellInspectionAdapter(const ScDocument& rDocument, const ScAddress& rFormulaPos,
        formula::FormulaGrammar::AddressConvention eConvention)
        : mrDocument(rDocument)
        , maFormulaPos(rFormulaPos)
        , meConvention(eConvention)
    {
    }

    [[nodiscard]] DirectCellInfoEvaluation evaluateLocalInfo(
        const OUString& rInfoType, const ScAddress& rCellPos, const CellValue& rCellValue) const
    {
        DirectCellInfoEvaluation aEvaluation;
        aEvaluation.meKind = classifyInfoType(rInfoType);
        if (aEvaluation.meKind == InfoKind::Unsupported
            || aEvaluation.meKind == InfoKind::Coord
            || spreadsheetengine::runtime::cellinspection::isHostPropertyInfoKind(
                aEvaluation.meKind))
        {
            return aEvaluation;
        }

        std::optional<spreadsheetengine::api::String> oSheetName;
        if (aEvaluation.meKind == InfoKind::Address && rCellPos.Tab() != maFormulaPos.Tab())
        {
            OUString aSheetName;
            mrDocument.GetName(rCellPos.Tab(), aSheetName);
            oSheetName = toApiString(aSheetName);
        }

        BoundedCellInfoRequest aInfoRequest;
        aInfoRequest.maAddress = toApiCellAddress(rCellPos);
        aInfoRequest.maCellValue = rCellValue;
        aInfoRequest.meConvention = meConvention;
        aInfoRequest.moSheetName = oSheetName;
        aEvaluation.maResult = evaluateBoundedCellInfo(aEvaluation.meKind, aInfoRequest);
        aEvaluation.mbHandled = true;
        return aEvaluation;
    }
};

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

[[nodiscard]] inline OUString formatLocalFilenameInfo(
    const INetURLObject& rUrlObject, const OUString& rTabName,
    formula::FormulaGrammar::AddressConvention eConvention, bool bLibreOfficeKitActive);

[[nodiscard]] inline OUString formatExternalFilenameInfo(
    const OUString& rFileName, const OUString& rTabName,
    formula::FormulaGrammar::AddressConvention eConvention);

[[nodiscard]] inline spreadsheetengine::api::CellValue makeLocalFilenamePropertyValue(
    const ScDocument& rDoc, SCTAB nTab, formula::FormulaGrammar::AddressConvention eConvention)
{
    OUString aFuncResult;
    if (nTab >= rDoc.GetTableCount())
        return makeFilenameValue(aFuncResult);

    if (rDoc.GetLinkMode(nTab) == ScLinkMode::VALUE)
        rDoc.GetName(nTab, aFuncResult);
    else
    {
        ScDocShell* pShell = rDoc.GetDocumentShell();
        if (pShell && pShell->GetMedium())
        {
            const INetURLObject& rUrlObject = pShell->GetMedium()->GetURLObject();
            OUString aTabName;
            rDoc.GetName(nTab, aTabName);
            aFuncResult = formatLocalFilenameInfo(
                rUrlObject, aTabName, eConvention, comphelper::LibreOfficeKit::isActive());
        }
    }
    return makeFilenameValue(aFuncResult);
}

[[nodiscard]] inline std::optional<spreadsheetengine::api::CellValue>
makeExternalFilenamePropertyValue(ScExternalRefManager& rRefMgr, sal_uInt16 nFileId,
    const OUString& rTabName, formula::FormulaGrammar::AddressConvention eConvention)
{
    const OUString* pFileName = rRefMgr.getExternalFileName(nFileId);
    if (!pFileName)
        return std::nullopt;
    return makeFilenameValue(formatExternalFilenameInfo(*pFileName, rTabName, eConvention));
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

[[nodiscard]] inline spreadsheetengine::api::ValueResult<CellValue> evaluateBoundedCellInfo(
    InfoKind eKind, const BoundedCellInfoRequest& rRequest)
{
    switch (eKind)
    {
        case InfoKind::Column:
            return spreadsheetengine::api::ValueResult<CellValue>::success(
                spreadsheetengine::runtime::cellinspection::columnValue(
                    rRequest.maAddress));
        case InfoKind::Row:
            return spreadsheetengine::api::ValueResult<CellValue>::success(
                spreadsheetengine::runtime::cellinspection::rowValue(rRequest.maAddress));
        case InfoKind::Sheet:
            return spreadsheetengine::api::ValueResult<CellValue>::success(
                spreadsheetengine::runtime::cellinspection::sheetValue(rRequest.maAddress));
        case InfoKind::Address:
        {
            referenceexecution::AddressFunctionRequest aRequest;
            aRequest.mnRow = rRequest.maAddress.mnRow;
            aRequest.mnColumn = rRequest.maAddress.mnColumn;
            aRequest.mnAbsMode = 1;
            aRequest.mbA1Style = rRequest.meConvention != formula::FormulaGrammar::CONV_XL_R1C1;
            aRequest.meConvention = rRequest.meConvention;
            if (rRequest.moSheetName)
                aRequest.maSheetToken = toLibreOfficeString(*rRequest.moSheetName);

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
                spreadsheetengine::runtime::cellinspection::contentsValue(rRequest.maCellValue));
        case InfoKind::Type:
            return spreadsheetengine::api::ValueResult<CellValue>::success(
                spreadsheetengine::runtime::cellinspection::typeValue(rRequest.maCellValue));
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

[[nodiscard]] inline spreadsheetengine::api::CellValue makeWidthPropertyValue(
    ScDocument& rDoc, const ScAddress& rCellPos)
{
    Printer* pPrinter = rDoc.GetPrinter();
    MapMode aOldMode(pPrinter->GetMapMode());
    vcl::Font aOldFont(pPrinter->GetFont());
    vcl::Font aDefFont;

    pPrinter->SetMapMode(MapMode(MapUnit::MapTwip));
    rDoc.getCellAttributeHelper().getDefaultCellAttribute().fillFontOnly(aDefFont, pPrinter);
    pPrinter->SetFont(aDefFont);
    const tools::Long nZeroWidth = pPrinter->GetTextWidth(OUString('0'));
    assert(nZeroWidth != 0);
    pPrinter->SetFont(aOldFont);
    pPrinter->SetMapMode(aOldMode);
    const int nZeroCount
        = static_cast<int>(rDoc.GetColWidth(rCellPos.Col(), rCellPos.Tab()) / nZeroWidth);
    return makeWidthValue(nZeroCount);
}

[[nodiscard]] inline spreadsheetengine::api::CellValue makePrefixPropertyValue(
    const ScDocument& rDoc, const ScAddress& rCellPos, bool bHasString)
{
    const SvxHorJustifyItem& rJustAttr = rDoc.GetAttr(rCellPos, ATTR_HOR_JUSTIFY);
    return makePrefixValue(bHasString, rJustAttr.GetValue());
}

[[nodiscard]] inline spreadsheetengine::api::CellValue makeProtectPropertyValue(
    const ScDocument& rDoc, const ScAddress& rCellPos)
{
    const ScProtectionAttr& rProtAttr = rDoc.GetAttr(rCellPos, ATTR_PROTECTION);
    return makeFlagValue(rProtAttr.GetProtection());
}

[[nodiscard]] inline spreadsheetengine::api::CellValue makeFormatPropertyValue(
    const ScInterpreterContext& rContext, sal_uInt32 nFormat)
{
    return makeFormatValue(rContext.NFGetCalcCellReturn(nFormat));
}

[[nodiscard]] inline spreadsheetengine::api::CellValue makeColorPropertyValue(
    const ScInterpreterContext& rContext, sal_uInt32 nFormat)
{
    const SvNumberformat* pFormat = rContext.NFGetFormatEntry(nFormat);
    return makeFlagValue(formatHasNegativeColor(pFormat));
}

[[nodiscard]] inline spreadsheetengine::api::CellValue makeParenthesesPropertyValue(
    const ScInterpreterContext& rContext, sal_uInt32 nFormat)
{
    const SvNumberformat* pFormat = rContext.NFGetFormatEntry(nFormat);
    return makeFlagValue(formatHasOpenParenthesis(pFormat));
}

} // namespace spreadsheetengine::compat::libreoffice::cellinspectionexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
