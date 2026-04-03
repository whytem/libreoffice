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
#include <formula/errorcodes.hxx>
#include <formula/grammar.hxx>
#include <interpretercontext.hxx>
#include <patattr.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/printer.hxx>
#include <svl/numformat.hxx>
#include <svl/zformat.hxx>
#include <tools/urlobj.hxx>

#include <global.hxx>
#include <tokenarray.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/CompileHost.hxx>
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

struct LocalHostCellInfoRequest
{
    ScAddress maCellPos;
    bool mbHasString = false;
    formula::FormulaGrammar::AddressConvention meConvention
        = formula::FormulaGrammar::CONV_OOO;
    sal_uInt32 mnFormat = 0;
};

struct DirectHostCellInfoEvaluation
{
    InfoKind meKind = InfoKind::Unsupported;
    CellValue maValue = CellValue::empty();
    bool mbHandled = false;
};

struct ExternalCellInfoRequest
{
    CellAddress maAddress;
    sal_uInt16 mnFileId = 0;
    OUString maTabName;
    ScSingleRefData maReference;
    ScExternalRefCache::TokenRef mxToken;
    ScExternalRefCache::CellFormat maFormat;
    formula::FormulaGrammar::AddressConvention meConvention
        = formula::FormulaGrammar::CONV_OOO;
};

struct DirectExternalCellInfoEvaluation
{
    InfoKind meKind = InfoKind::Unsupported;
    FormulaError meError = FormulaError::NONE;
    CellValue maValue = CellValue::empty();
    bool mbHandled = false;
};

struct ExternalHostCellInfoRequest
{
    sal_uInt32 mnFormat = 0;
};

struct DirectExternalHostCellInfoEvaluation
{
    InfoKind meKind = InfoKind::Unsupported;
    CellValue maValue = CellValue::empty();
    bool mbHandled = false;
};

[[nodiscard]] inline InfoKind classifyInfoType(const OUString& rInfoType);

[[nodiscard]] inline spreadsheetengine::api::ValueResult<CellValue> evaluateBoundedCellInfo(
    InfoKind eKind, const BoundedCellInfoRequest& rRequest);

[[nodiscard]] inline spreadsheetengine::api::CellValue makeExternalContentsValue(
    const formula::FormulaToken& rToken);

[[nodiscard]] inline spreadsheetengine::api::CellValue makeExternalTypeValue(
    const formula::FormulaToken& rToken);

[[nodiscard]] inline spreadsheetengine::api::CellValue makeExternalAddressValue(
    const ScDocument& rDoc, const ScAddress& rFormulaPos, sal_uInt16 nFileId,
    const OUString& rTabName, const ScSingleRefData& rReference);

[[nodiscard]] inline std::optional<spreadsheetengine::api::CellValue>
makeExternalFilenamePropertyValue(ScExternalRefManager& rRefMgr, sal_uInt16 nFileId,
    const OUString& rTabName, formula::FormulaGrammar::AddressConvention eConvention);

[[nodiscard]] inline spreadsheetengine::api::CellValue makeCoordPropertyValue(
    const ScDocument& rDoc, const ScAddress& rCellPos);

[[nodiscard]] inline spreadsheetengine::api::CellValue makeLocalFilenamePropertyValue(
    const ScDocument& rDoc, SCTAB nTab, formula::FormulaGrammar::AddressConvention eConvention);

[[nodiscard]] inline spreadsheetengine::api::CellValue makeWidthPropertyValue(
    ScDocument& rDoc, const ScAddress& rCellPos);

[[nodiscard]] inline spreadsheetengine::api::CellValue makePrefixPropertyValue(
    const ScDocument& rDoc, const ScAddress& rCellPos, bool bHasString);

[[nodiscard]] inline spreadsheetengine::api::CellValue makeProtectPropertyValue(
    const ScDocument& rDoc, const ScAddress& rCellPos);

[[nodiscard]] inline spreadsheetengine::api::CellValue makeFormatPropertyValue(
    const ScInterpreterContext& rContext, sal_uInt32 nFormat);

[[nodiscard]] inline spreadsheetengine::api::CellValue makeColorPropertyValue(
    const ScInterpreterContext& rContext, sal_uInt32 nFormat);

[[nodiscard]] inline spreadsheetengine::api::CellValue makeParenthesesPropertyValue(
    const ScInterpreterContext& rContext, sal_uInt32 nFormat);

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

class DirectHostCellInspectionAdapter
{
    const ScDocument& mrDocument;
    const ScInterpreterContext& mrContext;

public:
    DirectHostCellInspectionAdapter(const ScDocument& rDocument,
        const ScInterpreterContext& rContext)
        : mrDocument(rDocument)
        , mrContext(rContext)
    {
    }

    [[nodiscard]] DirectHostCellInfoEvaluation evaluateLocalInfo(
        const OUString& rInfoType, const LocalHostCellInfoRequest& rRequest) const
    {
        DirectHostCellInfoEvaluation aEvaluation;
        aEvaluation.meKind = classifyInfoType(rInfoType);

        switch (aEvaluation.meKind)
        {
            case InfoKind::Filename:
                aEvaluation.maValue = makeLocalFilenamePropertyValue(
                    mrDocument, rRequest.maCellPos.Tab(), rRequest.meConvention);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Coord:
                aEvaluation.maValue = makeCoordPropertyValue(mrDocument, rRequest.maCellPos);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Width:
                aEvaluation.maValue = makeWidthPropertyValue(
                    const_cast<ScDocument&>(mrDocument), rRequest.maCellPos);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Prefix:
                aEvaluation.maValue = makePrefixPropertyValue(
                    mrDocument, rRequest.maCellPos, rRequest.mbHasString);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Protect:
                aEvaluation.maValue = makeProtectPropertyValue(mrDocument, rRequest.maCellPos);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Format:
                aEvaluation.maValue = makeFormatPropertyValue(mrContext, rRequest.mnFormat);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Color:
                aEvaluation.maValue = makeColorPropertyValue(mrContext, rRequest.mnFormat);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Parentheses:
                aEvaluation.maValue = makeParenthesesPropertyValue(mrContext, rRequest.mnFormat);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Unsupported:
            case InfoKind::Column:
            case InfoKind::Row:
            case InfoKind::Sheet:
            case InfoKind::Address:
            case InfoKind::Contents:
            case InfoKind::Type:
                return aEvaluation;
        }

        return aEvaluation;
    }
};

class DirectExternalCellInspectionAdapter
{
    const ScDocument& mrDocument;
    ScAddress maFormulaPos;

public:
    DirectExternalCellInspectionAdapter(const ScDocument& rDocument, const ScAddress& rFormulaPos)
        : mrDocument(rDocument)
        , maFormulaPos(rFormulaPos)
    {
    }

    [[nodiscard]] DirectExternalCellInfoEvaluation evaluateInfo(
        const OUString& rInfoType, const ExternalCellInfoRequest& rRequest) const
    {
        DirectExternalCellInfoEvaluation aEvaluation;
        aEvaluation.meKind = classifyInfoType(rInfoType);

        switch (aEvaluation.meKind)
        {
            case InfoKind::Column:
                aEvaluation.maValue = spreadsheetengine::runtime::cellinspection::columnValue(
                    rRequest.maAddress);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Row:
                aEvaluation.maValue = spreadsheetengine::runtime::cellinspection::rowValue(
                    rRequest.maAddress);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Sheet:
            {
                ScExternalRefManager* pRefMgr = mrDocument.GetExternalRefManager();
                aEvaluation.mbHandled = true;
                if (pRefMgr && pRefMgr->getCacheTable(rRequest.mnFileId, rRequest.maTabName, false))
                    aEvaluation.maValue
                        = spreadsheetengine::runtime::cellinspection::sheetValue(rRequest.maAddress);
                else
                    aEvaluation.meError = FormulaError::NoName;
                return aEvaluation;
            }
            case InfoKind::Address:
            {
                aEvaluation.maValue = makeExternalAddressValue(
                    mrDocument, maFormulaPos, rRequest.mnFileId, rRequest.maTabName,
                    rRequest.maReference);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            }
            case InfoKind::Filename:
            {
                ScExternalRefManager* pRefMgr = mrDocument.GetExternalRefManager();
                aEvaluation.mbHandled = true;
                if (!pRefMgr)
                    aEvaluation.meError = FormulaError::NoName;
                else
                {
                    const auto aValue = makeExternalFilenamePropertyValue(
                        *pRefMgr, rRequest.mnFileId, rRequest.maTabName, rRequest.meConvention);
                    if (!aValue)
                        aEvaluation.meError = FormulaError::NoName;
                    else
                        aEvaluation.maValue = *aValue;
                }
                return aEvaluation;
            }
            case InfoKind::Contents:
                aEvaluation.maValue = makeExternalContentsValue(*rRequest.mxToken);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Type:
                aEvaluation.maValue = makeExternalTypeValue(*rRequest.mxToken);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Coord:
            case InfoKind::Width:
            case InfoKind::Prefix:
            case InfoKind::Protect:
            case InfoKind::Format:
            case InfoKind::Color:
            case InfoKind::Parentheses:
            case InfoKind::Unsupported:
                return aEvaluation;
        }

        return aEvaluation;
    }
};

class DirectExternalHostCellInspectionAdapter
{
    const ScInterpreterContext& mrContext;

public:
    explicit DirectExternalHostCellInspectionAdapter(const ScInterpreterContext& rContext)
        : mrContext(rContext)
    {
    }

    [[nodiscard]] DirectExternalHostCellInfoEvaluation evaluateInfo(
        const OUString& rInfoType, const ExternalHostCellInfoRequest& rRequest) const
    {
        DirectExternalHostCellInfoEvaluation aEvaluation;
        aEvaluation.meKind = classifyInfoType(rInfoType);

        switch (aEvaluation.meKind)
        {
            case InfoKind::Format:
                aEvaluation.maValue = makeFormatPropertyValue(mrContext, rRequest.mnFormat);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Color:
                aEvaluation.maValue = makeColorPropertyValue(mrContext, rRequest.mnFormat);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Parentheses:
                aEvaluation.maValue = makeParenthesesPropertyValue(mrContext, rRequest.mnFormat);
                aEvaluation.mbHandled = true;
                return aEvaluation;
            case InfoKind::Unsupported:
            case InfoKind::Column:
            case InfoKind::Row:
            case InfoKind::Sheet:
            case InfoKind::Address:
            case InfoKind::Filename:
            case InfoKind::Contents:
            case InfoKind::Type:
            case InfoKind::Coord:
            case InfoKind::Width:
            case InfoKind::Prefix:
            case InfoKind::Protect:
                return aEvaluation;
        }

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

[[nodiscard]] inline spreadsheetengine::api::CellValue makeExternalContentsValue(
    const formula::FormulaToken& rToken)
{
    switch (rToken.GetType())
    {
        case formula::svString:
            return spreadsheetengine::runtime::cellinspection::textPropertyValue(
                toApiString(rToken.GetString().getString()));
        case formula::svDouble:
            return spreadsheetengine::runtime::cellinspection::textPropertyValue(
                toApiString(OUString::number(rToken.GetDouble())));
        case formula::svError:
            return spreadsheetengine::runtime::cellinspection::textPropertyValue(
                toApiString(ScGlobal::GetErrorString(rToken.GetError())));
        default:
            return spreadsheetengine::runtime::cellinspection::textPropertyValue(u"");
    }
}

[[nodiscard]] inline spreadsheetengine::api::CellValue makeExternalTypeValue(
    const formula::FormulaToken& rToken)
{
    sal_Unicode c = 'v';
    if (rToken.GetType() == formula::svString)
        c = 'l';
    else if (rToken.GetType() == formula::svEmptyCell)
        c = 'b';
    return spreadsheetengine::runtime::cellinspection::textPropertyValue(
        toApiString(OUString(c)));
}

[[nodiscard]] inline spreadsheetengine::api::CellValue makeExternalAddressValue(
    const ScDocument& rDoc, const ScAddress& rFormulaPos, sal_uInt16 nFileId,
    const OUString& rTabName, const ScSingleRefData& rReference)
{
    ScTokenArray aArray(rDoc);
    aArray.AddExternalSingleReference(nFileId, svl::SharedString(rTabName), rReference);
    const OUString aString = compilehost::createFormulaStringFromTokenArray(
        rDoc, rFormulaPos, aArray, formula::FormulaGrammar::GRAM_ODFF_A1);
    return spreadsheetengine::runtime::cellinspection::textPropertyValue(toApiString(aString));
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

[[nodiscard]] inline spreadsheetengine::api::CellValue makeCoordPropertyValue(
    const ScDocument& rDoc, const ScAddress& rCellPos)
{
    OUString aCellStr1 = ScAddress(static_cast<SCCOL>(rCellPos.Tab()), 0, 0)
                             .Format((ScRefFlags::COL_ABS | ScRefFlags::COL_VALID), nullptr,
                                 rDoc.GetAddressConvention());
    OUString aCellStr2 = rCellPos.Format(
        (ScRefFlags::COL_ABS | ScRefFlags::COL_VALID | ScRefFlags::ROW_ABS
         | ScRefFlags::ROW_VALID),
        nullptr, rDoc.GetAddressConvention());
    return spreadsheetengine::runtime::cellinspection::textPropertyValue(
        toApiString(aCellStr1 + ":" + aCellStr2));
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
