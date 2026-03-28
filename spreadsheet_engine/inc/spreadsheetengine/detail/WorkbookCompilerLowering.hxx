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

#include <spreadsheetengine/api/ReferenceData.hxx>
#include <spreadsheetengine/detail/CalcConfig.hxx>
#include <spreadsheetengine/detail/OdfFormulaParser.hxx>
#include <spreadsheetengine/detail/WorkbookCompileHost.hxx>

namespace spreadsheetengine::detail::compiler
{

enum class FormulaLoweringReason : sal_uInt8
{
    Ready = 0,
    ParseFailure,
    MissingNamedReference,
    UnsupportedArrayElement,
    UnsupportedReferenceText,
    UnsupportedNodeKind
};

[[nodiscard]] constexpr api::StringView loweringReasonName(FormulaLoweringReason eReason)
{
    switch (eReason)
    {
        case FormulaLoweringReason::Ready:
            return u"ready";
        case FormulaLoweringReason::ParseFailure:
            return u"parse_failure";
        case FormulaLoweringReason::MissingNamedReference:
            return u"missing_named_reference";
        case FormulaLoweringReason::UnsupportedArrayElement:
            return u"unsupported_array_element";
        case FormulaLoweringReason::UnsupportedReferenceText:
            return u"unsupported_reference_text";
        case FormulaLoweringReason::UnsupportedNodeKind:
            return u"unsupported_node_kind";
    }

    return u"unknown";
}

struct FormulaLoweringResult
{
    token::CompiledFormula maFormula;
    FormulaLoweringReason meReason = FormulaLoweringReason::Ready;
    api::String maDetail;
    std::size_t mnFailureOffset = 0;

    constexpr explicit operator bool() const
    {
        return meReason == FormulaLoweringReason::Ready;
    }
};

namespace detail
{

constexpr token::OpCodeValue kLoweredOpUnaryPlus = 0x8001;
constexpr token::OpCodeValue kLoweredOpFunctionCall = 0x800f;
constexpr token::OpCodeValue kLoweredOpRangeConstructor = 0x8010;
constexpr token::OpCodeValue kLoweredOpReferenceList = 0x8011;
constexpr token::OpCodeValue kLoweredOpArgumentCount = 0x8012;

inline void setFailure(
    FormulaLoweringResult& rResult, FormulaLoweringReason eReason, api::String aDetail = {},
    std::size_t nFailureOffset = 0)
{
    rResult.meReason = eReason;
    rResult.maDetail = std::move(aDetail);
    rResult.mnFailureOffset = nFailureOffset;
}

[[nodiscard]] constexpr bool isAsciiAlpha(const sal_Unicode cChar)
{
    return (cChar >= u'A' && cChar <= u'Z') || (cChar >= u'a' && cChar <= u'z');
}

[[nodiscard]] inline api::String foldAsciiCase(api::StringView rText)
{
    api::String aFolded;
    aFolded.reserve(rText.size());
    for (const sal_Unicode cChar : rText)
    {
        if (cChar >= u'A' && cChar <= u'Z')
            aFolded.push_back(static_cast<sal_Unicode>(cChar - u'A' + u'a'));
        else
            aFolded.push_back(cChar);
    }
    return aFolded;
}

[[nodiscard]] inline std::optional<token::OpCodeValue>
lookupLexicalFunctionOpcode(api::StringView rName)
{
    const api::String aNormalized = api::String(rName);

    if (aNormalized == u"TRUE")
        return token::kOpCodeTrue;
    if (aNormalized == u"FALSE")
        return token::kOpCodeFalse;
    if (aNormalized == u"PI")
        return token::kOpCodePi;
    if (aNormalized == u"XOR")
        return token::kOpCodeXor;
    if (aNormalized == u"DEGREES")
        return token::kOpCodeDegrees;
    if (aNormalized == u"TODAY")
        return token::kOpCodeGetActDate;
    if (aNormalized == u"NA")
        return token::kOpCodeNoValue;
    if (aNormalized == u"ACOT")
        return token::kOpCodeArcCot;
    if (aNormalized == u"IF")
        return token::kOpCodeIf;
    if (aNormalized == u"IFERROR")
        return token::kOpCodeIfError;
    if (aNormalized == u"IFNA")
        return token::kOpCodeIfNa;
    if (aNormalized == u"ISERROR")
        return token::kOpCodeIsError;
    if (aNormalized == u"ISNA")
        return token::kOpCodeIsNv;
    if (aNormalized == u"ISBLANK")
        return token::kOpCodeIsEmpty;
    if (aNormalized == u"ISEVEN")
        return token::kOpCodeIsEven;
    if (aNormalized == u"ISODD")
        return token::kOpCodeIsOdd;
    if (aNormalized == u"N")
        return token::kOpCodeN;
    if (aNormalized == u"ATANH")
        return token::kOpCodeAtanh;
    if (aNormalized == u"VALUE")
        return token::kOpCodeValue;
    if (aNormalized == u"DATEVALUE")
        return token::kOpCodeGetDateValue;
    if (aNormalized == u"TIMEVALUE")
        return token::kOpCodeGetTimeValue;
    if (aNormalized == u"CLEAN")
        return token::kOpCodeClean;
    if (aNormalized == u"CHAR")
        return token::kOpCodeChar;
    if (aNormalized == u"CODE")
        return token::kOpCodeCode;
    if (aNormalized == u"UPPER")
        return token::kOpCodeUpper;
    if (aNormalized == u"LOWER")
        return token::kOpCodeLower;
    if (aNormalized == u"LEN")
        return token::kOpCodeLen;
    if (aNormalized == u"T")
        return token::kOpCodeT;
    if (aNormalized == u"FORMULA")
        return token::kOpCodeFormula;
    if (aNormalized == u"JIS")
        return token::kOpCodeJis;
    if (aNormalized == u"ASC")
        return token::kOpCodeAsc;
    if (aNormalized == u"UNICHAR")
        return token::kOpCodeUnichar;
    if (aNormalized == u"ISOWEEKNUM")
        return token::kOpCodeIsoWeeknum;
    if (aNormalized == u"ROUND")
        return token::kOpCodeRound;
    if (aNormalized == u"ROUNDUP")
        return token::kOpCodeRoundUp;
    if (aNormalized == u"ROUNDDOWN")
        return token::kOpCodeRoundDown;
    if (aNormalized == u"CEILING")
        return token::kOpCodeCeil;
    if (aNormalized == u"FLOOR")
        return token::kOpCodeFloor;
    if (aNormalized == u"GCD")
        return token::kOpCodeGcd;
    if (aNormalized == u"LCM")
        return token::kOpCodeLcm;
    if (aNormalized == u"LOG")
        return token::kOpCodeLog;
    if (aNormalized == u"DATE")
        return token::kOpCodeGetDate;
    if (aNormalized == u"TIME")
        return token::kOpCodeGetTime;
    if (aNormalized == u"DAYS360")
        return token::kOpCodeGetDiffDate360;
    if (aNormalized == u"NETWORKDAYS")
        return token::kOpCodeNetWorkdays;
    if (aNormalized == u"NETWORKDAYS.INTL" || aNormalized == u"COM.MICROSOFT.NETWORKDAYS.INTL")
        return token::kOpCodeNetWorkdaysMs;
    if (aNormalized == u"WEEKNUM")
        return token::kOpCodeWeek;
    if (aNormalized == u"WEEKDAY")
        return token::kOpCodeGetDayOfWeek;
    if (aNormalized == u"MOD")
        return token::kOpCodeMod;
    if (aNormalized == u"MIN")
        return token::kOpCodeMin;
    if (aNormalized == u"MAX")
        return token::kOpCodeMax;
    if (aNormalized == u"SUM")
        return token::kOpCodeSum;
    if (aNormalized == u"COLUMNS")
        return token::kOpCodeColumns;
    if (aNormalized == u"SUBTOTAL")
        return token::kOpCodeSubTotal;
    if (aNormalized == u"INDIRECT")
        return token::kOpCodeIndirect;
    if (aNormalized == u"ADDRESS")
        return token::kOpCodeAddress;
    if (aNormalized == u"MATCH")
        return token::kOpCodeMatch;
    if (aNormalized == u"SUMIF")
        return token::kOpCodeSumIf;
    if (aNormalized == u"LOOKUP")
        return token::kOpCodeLookup;
    if (aNormalized == u"VLOOKUP")
        return token::kOpCodeVLookup;
    if (aNormalized == u"HLOOKUP")
        return token::kOpCodeHLookup;
    if (aNormalized == u"OFFSET")
        return token::kOpCodeOffset;
    if (aNormalized == u"AREAS")
        return token::kOpCodeAreas;
    if (aNormalized == u"REPLACE")
        return token::kOpCodeReplace;
    if (aNormalized == u"EXACT")
        return token::kOpCodeExact;
    if (aNormalized == u"LEFT")
        return token::kOpCodeLeft;
    if (aNormalized == u"RIGHT")
        return token::kOpCodeRight;
    if (aNormalized == u"SEARCH")
        return token::kOpCodeSearch;
    if (aNormalized == u"MID")
        return token::kOpCodeMid;
    if (aNormalized == u"TEXT")
        return token::kOpCodeText;
    if (aNormalized == u"CONCATENATE")
        return token::kOpCodeConcat;
    if (aNormalized == u"MMULT")
        return token::kOpCodeMatMult;
    if (aNormalized == u"DECIMAL")
        return token::kOpCodeDecimal;
    if (aNormalized == u"HYPERLINK")
        return token::kOpCodeHyperLink;
    if (aNormalized == u"BASE")
        return token::kOpCodeBase;
    if (aNormalized == u"GETPIVOTDATA")
        return token::kOpCodeGetPivotData;
    if (aNormalized == u"EUROCONVERT")
        return token::kOpCodeEuroConvert;
    if (aNormalized == u"DATEDIF")
        return token::kOpCodeDateDif;
    if (aNormalized == u"AGGREGATE")
        return token::kOpCodeAggregate;
    if (aNormalized == u"RAWSUBTRACT")
        return token::kOpCodeRawSubtract;
    if (aNormalized == u"CONCAT")
        return token::kOpCodeConcatMs;
    if (aNormalized == u"TEXTJOIN")
        return token::kOpCodeTextJoinMs;
    if (aNormalized == u"REPLACEB")
        return token::kOpCodeReplaceB;
    if (aNormalized == u"LENB")
        return token::kOpCodeLenB;
    if (aNormalized == u"FINDB")
        return token::kOpCodeFindB;
    if (aNormalized == u"SEARCHB")
        return token::kOpCodeSearchB;
    if (aNormalized == u"AND")
        return token::kOpCodeAnd;

    if (const auto eSymbol = spreadsheetengine::core::findConfigOpCodeSymbol(aNormalized))
    {
        switch (*eSymbol)
        {
            case spreadsheetengine::api::ConfigOpCodeSymbol::Add:
                return token::kOpCodeAdd;
            case spreadsheetengine::api::ConfigOpCodeSymbol::Sub:
                return token::kOpCodeSub;
            case spreadsheetengine::api::ConfigOpCodeSymbol::Mul:
                return token::kOpCodeMul;
            case spreadsheetengine::api::ConfigOpCodeSymbol::Div:
                return token::kOpCodeDiv;
            case spreadsheetengine::api::ConfigOpCodeSymbol::Pow:
                return token::kOpCodePow;
            case spreadsheetengine::api::ConfigOpCodeSymbol::And:
                return token::kOpCodeAnd;
            case spreadsheetengine::api::ConfigOpCodeSymbol::Min:
                return token::kOpCodeMin;
            case spreadsheetengine::api::ConfigOpCodeSymbol::Max:
                return token::kOpCodeMax;
            case spreadsheetengine::api::ConfigOpCodeSymbol::Sum:
                return token::kOpCodeSum;
            case spreadsheetengine::api::ConfigOpCodeSymbol::Lookup:
                return token::kOpCodeLookup;
            case spreadsheetengine::api::ConfigOpCodeSymbol::VLookup:
                return token::kOpCodeVLookup;
            case spreadsheetengine::api::ConfigOpCodeSymbol::HLookup:
                return token::kOpCodeHLookup;
            case spreadsheetengine::api::ConfigOpCodeSymbol::Mod:
                return token::kOpCodeMod;
            case spreadsheetengine::api::ConfigOpCodeSymbol::Na:
                return token::kOpCodeNoValue;
            default:
                break;
        }
    }

    return std::nullopt;
}

[[nodiscard]] inline std::optional<api::ColumnIndex> parseColumnName(api::StringView rColumnName)
{
    if (rColumnName.empty())
        return std::nullopt;

    sal_Int64 nColumn = 0;
    for (sal_Unicode cChar : rColumnName)
    {
        if (cChar >= u'a' && cChar <= u'z')
            cChar = static_cast<sal_Unicode>(cChar - u'a' + u'A');
        if (cChar < u'A' || cChar > u'Z')
            return std::nullopt;
        nColumn = (nColumn * 26) + (cChar - u'A' + 1);
    }

    return static_cast<api::ColumnIndex>(nColumn - 1);
}

[[nodiscard]] inline api::String unquoteSheetName(api::StringView rSheetName)
{
    if (rSheetName.size() < 2 || rSheetName.front() != u'\'' || rSheetName.back() != u'\'')
        return api::String(rSheetName);

    api::String aResult;
    aResult.reserve(rSheetName.size() - 2);
    for (std::size_t nIndex = 1; nIndex + 1 < rSheetName.size(); ++nIndex)
    {
        if (rSheetName[nIndex] == u'\'' && nIndex + 1 < rSheetName.size() - 1
            && rSheetName[nIndex + 1] == u'\'')
        {
            aResult.push_back(u'\'');
            ++nIndex;
        }
        else
        {
            aResult.push_back(rSheetName[nIndex]);
        }
    }
    return aResult;
}

struct ParsedSingleReference
{
    enum class Shape : sal_uInt8
    {
        Cell = 0,
        WholeRow,
        WholeColumn
    };

    api::refdata::SingleRefData maReference;
    api::SheetId mnResolvedSheet = 0;
    api::ColumnIndex mnResolvedColumn = 0;
    api::RowIndex mnResolvedRow = 0;
    Shape meShape = Shape::Cell;
    bool mbExternal = false;
    sal_uInt16 mnFileId = 0;
    api::String maExternalTabName;
};

struct ExternalReferenceContext
{
    sal_uInt16 mnFileId = 0;
    api::String maTabName;
};

[[nodiscard]] inline token::ErrorCode mapErrorLiteral(api::StringView rError)
{
    if (rError == u"#N/A")
        return 2042;
    if (rError == u"#DIV/0!")
        return 2007;
    if (rError == u"#VALUE!")
        return 2015;
    if (rError == u"#REF!")
        return 2023;
    if (rError == u"#NAME?")
        return 2029;
    if (rError == u"#NUM!")
        return 2036;
    if (rError == u"#NULL!")
        return 2000;

    constexpr api::StringView aPrefix = u"#ERR";
    if (rError.starts_with(aPrefix) && rError.size() > aPrefix.size() + 1 && rError.back() == u'!')
    {
        sal_Int64 nError = 0;
        for (std::size_t nIndex = aPrefix.size(); nIndex + 1 < rError.size(); ++nIndex)
        {
            const sal_Unicode cChar = rError[nIndex];
            if (cChar < u'0' || cChar > u'9')
                return 0;
            nError = (nError * 10) + (cChar - u'0');
        }
        return static_cast<token::ErrorCode>(nError);
    }

    return 0;
}

[[nodiscard]] inline sal_uInt16 hashExternalLink(api::StringView rExternalLink)
{
    sal_uInt32 nHash = 2166136261u;
    for (const sal_Unicode cChar : rExternalLink)
        nHash = (nHash ^ cChar) * 16777619u;
    const sal_uInt16 nValue = static_cast<sal_uInt16>((nHash & 0xffffu) ? (nHash & 0xffffu) : 1u);
    return nValue;
}

[[nodiscard]] inline std::optional<std::size_t> findExternalReferenceMarker(api::StringView rToken)
{
    bool bInQuotes = false;
    for (std::size_t nIndex = 0; nIndex < rToken.size(); ++nIndex)
    {
        if (rToken[nIndex] == u'\'')
        {
            if (bInQuotes && nIndex + 1 < rToken.size() && rToken[nIndex + 1] == u'\'')
            {
                ++nIndex;
                continue;
            }

            bInQuotes = !bInQuotes;
            continue;
        }

        if (!bInQuotes && rToken[nIndex] == u'#' && nIndex + 1 < rToken.size()
            && rToken[nIndex + 1] == u'$')
        {
            return nIndex;
        }
    }

    return std::nullopt;
}

[[nodiscard]] inline std::optional<ParsedSingleReference> parseSingleReference(
    api::StringView rToken, const WorkbookCompileHost& rHost, const CompileContext& rContext,
    api::SheetId nImplicitSheet, std::optional<ExternalReferenceContext> oImplicitExternal = {})
{
    api::StringView aToken = rToken;
    if (!aToken.empty() && aToken.front() == u'.')
        aToken.remove_prefix(1);

    std::optional<ExternalReferenceContext> oExternal = oImplicitExternal;
    if (const auto oHashPos = findExternalReferenceMarker(aToken))
    {
        api::StringView aExternalToken = aToken.substr(0, *oHashPos);
        aToken = aToken.substr(*oHashPos + 1);
        oExternal = ExternalReferenceContext { hashExternalLink(aExternalToken), {} };
    }

    api::StringView aSheetToken;
    api::StringView aAddressToken = aToken;
    bool bHasExplicitSheet = false;
    const std::size_t nDotPos = aToken.rfind(u'.');
    if (nDotPos != api::StringView::npos && nDotPos + 1 < aToken.size()
        && (aToken[nDotPos + 1] == u'$' || isAsciiAlpha(aToken[nDotPos + 1])))
    {
        aSheetToken = aToken.substr(0, nDotPos);
        aAddressToken = aToken.substr(nDotPos + 1);
        bHasExplicitSheet = !aSheetToken.empty();
    }

    if (aAddressToken.empty())
        return std::nullopt;

    const auto isAllDigits = [](api::StringView rText) {
        if (rText.empty())
            return false;
        for (const sal_Unicode cChar : rText)
        {
            if (cChar < u'0' || cChar > u'9')
                return false;
        }
        return true;
    };

    bool bLeadingAbsolute = false;
    if (aAddressToken.front() == u'$')
    {
        bLeadingAbsolute = true;
        aAddressToken.remove_prefix(1);
    }
    if (aAddressToken.empty())
        return std::nullopt;

    bool bColumnAbsolute = bLeadingAbsolute;
    bool bRowAbsolute = false;
    bool bWholeRow = false;
    bool bWholeColumn = false;
    std::optional<api::ColumnIndex> oColumn;
    api::RowIndex nAbsoluteRow = 0;

    if (isAllDigits(aAddressToken))
    {
        sal_Int64 nRow = 0;
        for (const sal_Unicode cChar : aAddressToken)
            nRow = (nRow * 10) + (cChar - u'0');
        if (nRow <= 0)
            return std::nullopt;

        bWholeRow = true;
        bRowAbsolute = bLeadingAbsolute;
        nAbsoluteRow = static_cast<api::RowIndex>(nRow - 1);
    }
    else
    {
        std::size_t nColumnEnd = 0;
        while (nColumnEnd < aAddressToken.size() && isAsciiAlpha(aAddressToken[nColumnEnd]))
            ++nColumnEnd;
        if (nColumnEnd == 0)
            return std::nullopt;

        oColumn = parseColumnName(aAddressToken.substr(0, nColumnEnd));
        if (!oColumn)
            return std::nullopt;

        aAddressToken.remove_prefix(nColumnEnd);
        if (aAddressToken.empty())
        {
            bWholeColumn = true;
            bColumnAbsolute = bLeadingAbsolute;
        }
        else
        {
            if (aAddressToken.front() == u'$')
            {
                bRowAbsolute = true;
                aAddressToken.remove_prefix(1);
            }
            if (!isAllDigits(aAddressToken))
                return std::nullopt;

            sal_Int64 nRow = 0;
            for (const sal_Unicode cChar : aAddressToken)
                nRow = (nRow * 10) + (cChar - u'0');
            if (nRow <= 0)
                return std::nullopt;

            nAbsoluteRow = static_cast<api::RowIndex>(nRow - 1);
        }
    }

    ParsedSingleReference aParsed;
    bool bSheetRelative = true;
    api::SheetId nAbsoluteSheet = nImplicitSheet;
    if (bHasExplicitSheet)
    {
        if (!aSheetToken.empty() && aSheetToken.front() == u'$')
            aSheetToken.remove_prefix(1);
        if (oExternal)
        {
            nAbsoluteSheet = 0;
            bSheetRelative = false;
        }
        else
        {
            const api::String aSheetName = unquoteSheetName(aSheetToken);
            const auto oSheetId = rHost.workbook().findSheetId(aSheetName);
            if (!oSheetId)
            {
                nAbsoluteSheet = 0;
            }
            else
            {
                nAbsoluteSheet = *oSheetId;
            }
            bSheetRelative = false;
        }
    }
    else if (nImplicitSheet != rContext.maBaseAddress.mnSheet)
    {
        bSheetRelative = false;
        nAbsoluteSheet = nImplicitSheet;
    }

    const api::ColumnIndex nAbsoluteColumn = oColumn ? *oColumn : 0;
    aParsed.maReference.maFlags.mbColumnRelative = !bColumnAbsolute && !bWholeRow;
    aParsed.maReference.maFlags.mbRowRelative = !bRowAbsolute && !bWholeColumn;
    aParsed.maReference.maFlags.mbSheetRelative = bSheetRelative;
    aParsed.maReference.maFlags.mbFlag3D = !bSheetRelative;
    aParsed.maReference.mnColumn = aParsed.maReference.maFlags.mbColumnRelative
                                       ? (nAbsoluteColumn - rContext.maBaseAddress.mnColumn)
                                       : nAbsoluteColumn;
    aParsed.maReference.mnRow = aParsed.maReference.maFlags.mbRowRelative
                                    ? (nAbsoluteRow - rContext.maBaseAddress.mnRow)
                                    : nAbsoluteRow;
    aParsed.maReference.mnSheet
        = bSheetRelative ? (nAbsoluteSheet - rContext.maBaseAddress.mnSheet) : nAbsoluteSheet;
    aParsed.mnResolvedSheet = nAbsoluteSheet;
    aParsed.mnResolvedColumn = nAbsoluteColumn;
    aParsed.mnResolvedRow = nAbsoluteRow;
    aParsed.meShape = bWholeRow ? ParsedSingleReference::Shape::WholeRow
                                : (bWholeColumn ? ParsedSingleReference::Shape::WholeColumn
                                                : ParsedSingleReference::Shape::Cell);
    if (oExternal)
    {
        aParsed.mbExternal = true;
        aParsed.mnFileId = oExternal->mnFileId;
        aParsed.maExternalTabName = bHasExplicitSheet ? unquoteSheetName(aSheetToken) : oExternal->maTabName;
        if (aParsed.maExternalTabName.empty() && bHasExplicitSheet)
            aParsed.maExternalTabName = unquoteSheetName(aSheetToken);
    }
    else if (bHasExplicitSheet)
    {
        aParsed.maExternalTabName.clear();
    }
    return aParsed;
}

constexpr api::ColumnIndex kSmokeMaxColumn = 16383;
constexpr api::RowIndex kSmokeMaxRow = 1048575;

[[nodiscard]] inline api::refdata::ComplexRefData expandReferenceToRange(
    const ParsedSingleReference& rReference)
{
    api::refdata::ComplexRefData aRange;
    aRange.maRef1.maFlags.mbSheetRelative = false;
    aRange.maRef2.maFlags.mbSheetRelative = false;
    aRange.maRef1.maFlags.mbFlag3D = true;
    aRange.maRef2.maFlags.mbFlag3D = true;
    aRange.maRef1.mnSheet = rReference.mnResolvedSheet;
    aRange.maRef2.mnSheet = rReference.mnResolvedSheet;

    switch (rReference.meShape)
    {
        case ParsedSingleReference::Shape::Cell:
            aRange.maRef1.mnColumn = rReference.mnResolvedColumn;
            aRange.maRef1.mnRow = rReference.mnResolvedRow;
            aRange.maRef2 = aRange.maRef1;
            break;
        case ParsedSingleReference::Shape::WholeRow:
            aRange.maRef1.mnColumn = 0;
            aRange.maRef1.mnRow = rReference.mnResolvedRow;
            aRange.maRef2.mnColumn = kSmokeMaxColumn;
            aRange.maRef2.mnRow = rReference.mnResolvedRow;
            break;
        case ParsedSingleReference::Shape::WholeColumn:
            aRange.maRef1.mnColumn = rReference.mnResolvedColumn;
            aRange.maRef1.mnRow = 0;
            aRange.maRef2.mnColumn = rReference.mnResolvedColumn;
            aRange.maRef2.mnRow = kSmokeMaxRow;
            break;
    }

    return aRange;
}

[[nodiscard]] inline api::refdata::ComplexRefData mergeExpandedRanges(
    const ParsedSingleReference& rStart, const ParsedSingleReference& rEnd)
{
    const auto aStartRange = expandReferenceToRange(rStart);
    const auto aEndRange = expandReferenceToRange(rEnd);

    api::refdata::ComplexRefData aMerged;
    aMerged.maRef1.maFlags.mbSheetRelative = false;
    aMerged.maRef2.maFlags.mbSheetRelative = false;
    aMerged.maRef1.maFlags.mbFlag3D = true;
    aMerged.maRef2.maFlags.mbFlag3D = true;
    aMerged.maRef1.mnColumn = std::min(aStartRange.maRef1.mnColumn, aEndRange.maRef1.mnColumn);
    aMerged.maRef1.mnRow = std::min(aStartRange.maRef1.mnRow, aEndRange.maRef1.mnRow);
    aMerged.maRef1.mnSheet = std::min(aStartRange.maRef1.mnSheet, aEndRange.maRef1.mnSheet);
    aMerged.maRef2.mnColumn = std::max(aStartRange.maRef2.mnColumn, aEndRange.maRef2.mnColumn);
    aMerged.maRef2.mnRow = std::max(aStartRange.maRef2.mnRow, aEndRange.maRef2.mnRow);
    aMerged.maRef2.mnSheet = std::max(aStartRange.maRef2.mnSheet, aEndRange.maRef2.mnSheet);
    return aMerged;
}

[[nodiscard]] inline bool pushNodeTokens(
    const core::formula::Node& rNode, const WorkbookCompileHost& rHost,
    const CompileContext& rContext, FormulaLoweringResult& rResult);

[[nodiscard]] inline bool pushScalarArrayElement(
    const core::formula::Node& rNode, token::MatrixScalar& rValue)
{
    using core::formula::NodeKind;
    switch (rNode.meKind)
    {
        case NodeKind::NumberLiteral:
            rValue = rNode.mfNumber;
            return true;
        case NodeKind::StringLiteral:
            rValue = api::String(rNode.maPrimaryText);
            return true;
        case NodeKind::BooleanLiteral:
            rValue = rNode.mbBoolean ? 1.0 : 0.0;
            return true;
        case NodeKind::ErrorLiteral:
            rValue = mapErrorLiteral(rNode.maPrimaryText);
            return true;
        case NodeKind::EmptyArgument:
            rValue = api::String {};
            return true;
        case NodeKind::UnaryOperation:
            if (rNode.maChildren.size() != 1 || !rNode.maChildren.front())
                return false;
            if (!pushScalarArrayElement(*rNode.maChildren.front(), rValue))
                return false;
            if (rNode.meUnaryOperator == core::formula::UnaryOperator::Minus)
            {
                if (auto* pNumber = std::get_if<double>(&rValue))
                    *pNumber = -*pNumber;
                else
                    return false;
            }
            return true;
        default:
            return false;
    }
}

[[nodiscard]] constexpr token::OpCodeValue unaryOpcode(core::formula::UnaryOperator eOperator)
{
    switch (eOperator)
    {
        case core::formula::UnaryOperator::Plus:
            return kLoweredOpUnaryPlus;
        case core::formula::UnaryOperator::Minus:
            return token::kOpCodeNegSub;
    }
    return kLoweredOpUnaryPlus;
}

[[nodiscard]] constexpr token::OpCodeValue binaryOpcode(core::formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case core::formula::BinaryOperator::Add:
            return token::kOpCodeAdd;
        case core::formula::BinaryOperator::Subtract:
            return token::kOpCodeSub;
        case core::formula::BinaryOperator::Multiply:
            return token::kOpCodeMul;
        case core::formula::BinaryOperator::Divide:
            return token::kOpCodeDiv;
        case core::formula::BinaryOperator::Power:
            return token::kOpCodePow;
        case core::formula::BinaryOperator::Concat:
            return token::kOpCodeAmpersand;
        case core::formula::BinaryOperator::Equal:
            return token::kOpCodeEqual;
        case core::formula::BinaryOperator::NotEqual:
            return token::kOpCodeNotEqual;
        case core::formula::BinaryOperator::Less:
            return token::kOpCodeLess;
        case core::formula::BinaryOperator::LessEqual:
            return token::kOpCodeLessEqual;
        case core::formula::BinaryOperator::Greater:
            return token::kOpCodeGreater;
        case core::formula::BinaryOperator::GreaterEqual:
            return token::kOpCodeGreaterEqual;
    }
    return token::kOpCodeAdd;
}

inline void pushToken(
    FormulaLoweringResult& rResult, token::Kind eKind, token::OpCodeValue nOpCode,
    token::Payload aPayload = {})
{
    rResult.maFormula.maTokens.push_back({ eKind, nOpCode, std::move(aPayload) });
}

inline void pushOperatorByteToken(FormulaLoweringResult& rResult, token::OpCodeValue nOpCode)
{
    pushToken(rResult, token::Kind::Byte, nOpCode,
        token::ByteData { 0, token::kParamClassUnknown });
}

inline void pushJumpToken(
    FormulaLoweringResult& rResult, token::OpCodeValue nOpCode, std::initializer_list<short> aJumps)
{
    token::JumpData aData;
    aData.maJumps.assign(aJumps.begin(), aJumps.end());
    aData.mnInForceArray = token::kParamClassUnknown;
    pushToken(rResult, token::Kind::Jump, nOpCode, std::move(aData));
}

[[nodiscard]] inline bool pushNodeTokens(
    const core::formula::Node& rNode, const WorkbookCompileHost& rHost,
    const CompileContext& rContext, FormulaLoweringResult& rResult)
{
    using core::formula::NodeKind;
    using spreadsheetengine::detail::token::ByteData;
    using spreadsheetengine::detail::token::NameData;
    using spreadsheetengine::detail::token::StringData;

    switch (rNode.meKind)
    {
        case NodeKind::NumberLiteral:
            pushToken(rResult, token::Kind::Value, token::kOpCodePush, rNode.mfNumber);
            return true;

        case NodeKind::StringLiteral:
            pushToken(rResult, token::Kind::String, token::kOpCodePush,
                StringData { api::String(rNode.maPrimaryText), api::String(rNode.maPrimaryText) });
            return true;

        case NodeKind::BooleanLiteral:
            pushToken(rResult, token::Kind::Value, token::kOpCodePush, rNode.mbBoolean ? 1.0 : 0.0);
            return true;

        case NodeKind::ErrorLiteral:
            pushToken(rResult, token::Kind::Error, token::kOpCodePush,
                mapErrorLiteral(rNode.maPrimaryText));
            return true;

        case NodeKind::EmptyArgument:
            pushToken(rResult, token::Kind::Missing, token::kOpCodeMissing, {});
            return true;

        case NodeKind::CellReference:
        {
            const auto oReference = parseSingleReference(
                rNode.maPrimaryText, rHost, rContext, rContext.maBaseAddress.mnSheet);
            if (!oReference)
            {
                setFailure(rResult, FormulaLoweringReason::UnsupportedReferenceText,
                    api::String(rNode.maPrimaryText));
                return false;
            }
            if (oReference->mbExternal)
            {
                if (oReference->meShape == ParsedSingleReference::Shape::Cell)
                {
                    pushToken(rResult, token::Kind::ExternalSingleRef, token::kOpCodePush,
                        token::ExternalSingleRefData { oReference->mnFileId,
                            api::String(oReference->maExternalTabName), oReference->maReference });
                }
                else
                {
                    pushToken(rResult, token::Kind::ExternalDoubleRef, token::kOpCodePush,
                        token::ExternalDoubleRefData { oReference->mnFileId,
                            api::String(oReference->maExternalTabName),
                            expandReferenceToRange(*oReference) });
                }
            }
            else
            {
                if (oReference->meShape == ParsedSingleReference::Shape::Cell)
                {
                    pushToken(rResult, token::Kind::SingleRef, token::kOpCodePush,
                        oReference->maReference);
                }
                else
                {
                    pushToken(rResult, token::Kind::DoubleRef, token::kOpCodePush,
                        expandReferenceToRange(*oReference));
                }
            }
            return true;
        }

        case NodeKind::RangeReference:
        {
            const auto oStart = parseSingleReference(
                rNode.maPrimaryText, rHost, rContext, rContext.maBaseAddress.mnSheet);
            if (!oStart)
            {
                setFailure(rResult, FormulaLoweringReason::UnsupportedReferenceText,
                    api::String(rNode.maPrimaryText));
                return false;
            }
            std::optional<ExternalReferenceContext> oExternal;
            if (oStart->mbExternal)
                oExternal = ExternalReferenceContext { oStart->mnFileId, oStart->maExternalTabName };
            const auto oEnd = parseSingleReference(
                rNode.maSecondaryText, rHost, rContext, oStart->mnResolvedSheet, oExternal);
            if (!oEnd)
            {
                setFailure(rResult, FormulaLoweringReason::UnsupportedReferenceText,
                    api::String(rNode.maSecondaryText));
                return false;
            }
            const auto aReference = [&]() {
                if (oStart->meShape == ParsedSingleReference::Shape::Cell
                    && oEnd->meShape == ParsedSingleReference::Shape::Cell)
                {
                    api::refdata::ComplexRefData aCellRange;
                    aCellRange.maRef1 = oStart->maReference;
                    aCellRange.maRef2 = oEnd->maReference;
                    api::refdata::putInOrder(aCellRange, rContext.maBaseAddress);
                    return aCellRange;
                }

                return mergeExpandedRanges(*oStart, *oEnd);
            }();
            if (oStart->mbExternal || oEnd->mbExternal)
            {
                if (!oStart->mbExternal || !oEnd->mbExternal || oStart->mnFileId != oEnd->mnFileId
                    || oStart->maExternalTabName != oEnd->maExternalTabName)
                {
                    setFailure(rResult, FormulaLoweringReason::UnsupportedReferenceText,
                        api::String(rNode.maPrimaryText));
                    return false;
                }
                pushToken(rResult, token::Kind::ExternalDoubleRef, token::kOpCodePush,
                    token::ExternalDoubleRefData { oStart->mnFileId,
                        api::String(oStart->maExternalTabName), aReference });
            }
            else
            {
                pushToken(rResult, token::Kind::DoubleRef, token::kOpCodePush, aReference);
            }
            return true;
        }

        case NodeKind::NamedReference:
        {
            std::optional<api::SheetId> oScopeSheet;
            if (rContext.maBaseAddress.mnSheet >= 0)
                oScopeSheet = rContext.maBaseAddress.mnSheet;
            const auto oName = rHost.lookupRangeName(rNode.maPrimaryText, oScopeSheet, rContext);
            if (!oName)
            {
                setFailure(rResult, FormulaLoweringReason::MissingNamedReference,
                    api::String(rNode.maPrimaryText));
                return false;
            }
            pushToken(rResult, token::Kind::RangeName, token::kOpCodeName, *oName);
            return true;
        }

        case NodeKind::ArrayConstant:
        {
            token::MatrixData aMatrix;
            aMatrix.mnRows = rNode.mnArrayRows;
            aMatrix.mnColumns = rNode.mnArrayColumns;
            aMatrix.maValues.reserve(rNode.maChildren.size());
            for (const auto& pChild : rNode.maChildren)
            {
                if (!pChild)
                {
                    setFailure(rResult, FormulaLoweringReason::UnsupportedArrayElement);
                    return false;
                }

                token::MatrixScalar aValue;
                if (!pushScalarArrayElement(*pChild, aValue))
                {
                    setFailure(rResult, FormulaLoweringReason::UnsupportedArrayElement,
                        api::String(u"array_element"));
                    return false;
                }
                aMatrix.maValues.push_back(std::move(aValue));
            }
            pushToken(rResult, token::Kind::Matrix, token::kOpCodePush, std::move(aMatrix));
            return true;
        }

        case NodeKind::UnaryOperation:
            if (rNode.maChildren.size() != 1 || !rNode.maChildren.front())
            {
                setFailure(rResult, FormulaLoweringReason::UnsupportedNodeKind, u"UnaryOperation");
                return false;
            }
            if (!pushNodeTokens(*rNode.maChildren.front(), rHost, rContext, rResult))
                return false;
            pushToken(rResult, token::Kind::PlainOpcode, unaryOpcode(rNode.meUnaryOperator), {});
            return true;

        case NodeKind::BinaryOperation:
            if (rNode.maChildren.size() != 2 || !rNode.maChildren[0] || !rNode.maChildren[1])
            {
                setFailure(rResult, FormulaLoweringReason::UnsupportedNodeKind, u"BinaryOperation");
                return false;
            }
            if (!pushNodeTokens(*rNode.maChildren[0], rHost, rContext, rResult)
                || !pushNodeTokens(*rNode.maChildren[1], rHost, rContext, rResult))
            {
                return false;
            }
            pushToken(rResult, token::Kind::PlainOpcode, binaryOpcode(rNode.meBinaryOperator), {});
            return true;

        case NodeKind::FunctionCall:
            for (const auto& pChild : rNode.maChildren)
            {
                if (!pChild || !pushNodeTokens(*pChild, rHost, rContext, rResult))
                    return false;
            }
            if (const auto oExternal = rHost.lookupExternalName(rNode.maPrimaryText, rContext))
            {
                pushToken(rResult, token::Kind::ExternalName, token::kOpCodePush, *oExternal);
            }
            else
            {
                pushToken(rResult, token::Kind::StringName, token::kOpCodePush,
                    StringData { api::String(rNode.maPrimaryText), foldAsciiCase(rNode.maPrimaryText) });
            }
            pushToken(rResult, token::Kind::Byte, kLoweredOpArgumentCount,
                ByteData { static_cast<sal_uInt8>(rNode.maChildren.size()),
                    token::kParamClassUnknown });
            pushToken(rResult, token::Kind::PlainOpcode, kLoweredOpFunctionCall, {});
            return true;

        case NodeKind::RangeConstructor:
            if (rNode.maChildren.size() != 2 || !rNode.maChildren[0] || !rNode.maChildren[1])
            {
                setFailure(rResult, FormulaLoweringReason::UnsupportedNodeKind, u"RangeConstructor");
                return false;
            }
            if (!pushNodeTokens(*rNode.maChildren[0], rHost, rContext, rResult)
                || !pushNodeTokens(*rNode.maChildren[1], rHost, rContext, rResult))
            {
                return false;
            }
            pushToken(rResult, token::Kind::PlainOpcode, token::kOpCodeRange, {});
            return true;

        case NodeKind::ReferenceList:
            if (rNode.maChildren.size() < 2)
            {
                setFailure(rResult, FormulaLoweringReason::UnsupportedNodeKind, u"ReferenceList");
                return false;
            }
            if (!rNode.maChildren[0] || !pushNodeTokens(*rNode.maChildren[0], rHost, rContext, rResult))
                return false;
            for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); ++nIndex)
            {
                if (!rNode.maChildren[nIndex]
                    || !pushNodeTokens(*rNode.maChildren[nIndex], rHost, rContext, rResult))
                {
                    return false;
                }
                pushToken(rResult, token::Kind::PlainOpcode, token::kOpCodeUnion, {});
            }
            return true;
    }

    setFailure(rResult, FormulaLoweringReason::UnsupportedNodeKind);
    return false;
}

[[nodiscard]] inline bool pushNodeTokensLexical(
    const core::formula::Node& rNode, const WorkbookCompileHost& rHost,
    const CompileContext& rContext, FormulaLoweringResult& rResult)
{
    using core::formula::NodeKind;
    using spreadsheetengine::detail::token::StringData;

    switch (rNode.meKind)
    {
        case NodeKind::NumberLiteral:
            pushToken(rResult, token::Kind::Value, token::kOpCodePush, rNode.mfNumber);
            return true;

        case NodeKind::StringLiteral:
            pushToken(rResult, token::Kind::String, token::kOpCodePush,
                StringData { api::String(rNode.maPrimaryText), api::String(rNode.maPrimaryText) });
            return true;

        case NodeKind::BooleanLiteral:
            pushToken(rResult, token::Kind::Value, token::kOpCodePush, rNode.mbBoolean ? 1.0 : 0.0);
            return true;

        case NodeKind::ErrorLiteral:
            pushToken(rResult, token::Kind::Error, token::kOpCodePush,
                mapErrorLiteral(rNode.maPrimaryText));
            return true;

        case NodeKind::EmptyArgument:
            pushToken(rResult, token::Kind::Missing, token::kOpCodeMissing, {});
            return true;

        case NodeKind::CellReference:
        case NodeKind::RangeReference:
        case NodeKind::NamedReference:
        case NodeKind::ArrayConstant:
            return pushNodeTokens(rNode, rHost, rContext, rResult);

        case NodeKind::UnaryOperation:
            if (rNode.maChildren.size() != 1 || !rNode.maChildren.front())
            {
                setFailure(rResult, FormulaLoweringReason::UnsupportedNodeKind, u"UnaryOperation");
                return false;
            }
            if (rNode.meUnaryOperator == core::formula::UnaryOperator::Plus)
                pushOperatorByteToken(rResult, kLoweredOpUnaryPlus);
            else
                pushOperatorByteToken(rResult, unaryOpcode(rNode.meUnaryOperator));
            return pushNodeTokensLexical(*rNode.maChildren.front(), rHost, rContext, rResult);

        case NodeKind::BinaryOperation:
            if (rNode.maChildren.size() != 2 || !rNode.maChildren[0] || !rNode.maChildren[1])
            {
                setFailure(rResult, FormulaLoweringReason::UnsupportedNodeKind, u"BinaryOperation");
                return false;
            }
            if (!pushNodeTokensLexical(*rNode.maChildren[0], rHost, rContext, rResult))
                return false;
            pushOperatorByteToken(rResult, binaryOpcode(rNode.meBinaryOperator));
            return pushNodeTokensLexical(*rNode.maChildren[1], rHost, rContext, rResult);

        case NodeKind::FunctionCall:
        {
            const auto oOpcode = lookupLexicalFunctionOpcode(rNode.maPrimaryText);
            if (!oOpcode)
            {
                if (!rNode.maPrimaryText.empty())
                {
                    const api::String aFoldedName = foldAsciiCase(rNode.maPrimaryText);
                    pushToken(rResult, token::Kind::String, token::kOpCodeBad,
                        StringData { aFoldedName, aFoldedName });
                }
                else
                {
                    setFailure(rResult, FormulaLoweringReason::UnsupportedNodeKind,
                        api::String(rNode.maPrimaryText));
                    return false;
                }
            }
            else
            {
                switch (*oOpcode)
                {
                    case token::kOpCodeIf:
                        pushJumpToken(rResult, *oOpcode, { 3, 0, 0, 0 });
                        break;
                    case token::kOpCodeIfError:
                    case token::kOpCodeIfNa:
                        pushJumpToken(rResult, *oOpcode, { 2, 0, 0 });
                        break;
                    default:
                        pushOperatorByteToken(rResult, *oOpcode);
                        break;
                }
            }
            pushToken(rResult, token::Kind::PlainOpcode, token::kOpCodeOpen, {});
            for (std::size_t nIndex = 0; nIndex < rNode.maChildren.size(); ++nIndex)
            {
                if (nIndex != 0)
                    pushToken(rResult, token::Kind::PlainOpcode, token::kOpCodeSep, {});
                if (!rNode.maChildren[nIndex]
                    || !pushNodeTokensLexical(*rNode.maChildren[nIndex], rHost, rContext, rResult))
                {
                    return false;
                }
            }
            pushToken(rResult, token::Kind::PlainOpcode, token::kOpCodeClose, {});
            return true;
        }

        default:
            setFailure(rResult, FormulaLoweringReason::UnsupportedNodeKind, u"LexicalUnsupported");
            return false;
    }
}

} // namespace detail

[[nodiscard]] inline FormulaLoweringResult lowerFormulaSource(
    api::StringView rFormula, const WorkbookCompileHost& rHost, const CompileContext& rContext)
{
    FormulaLoweringResult aResult;
    const auto aParsed = core::formula::parseFormula(rFormula);
    if (!aParsed)
    {
        detail::setFailure(aResult, FormulaLoweringReason::ParseFailure,
            api::String(aParsed.maError.maMessage), aParsed.maError.mnOffset);
        return aResult;
    }

    if (!detail::pushNodeTokens(*aParsed.mpRoot, rHost, rContext, aResult))
        return aResult;

    return aResult;
}

[[nodiscard]] inline FormulaLoweringResult lowerFormulaSourceLexical(
    api::StringView rFormula, const WorkbookCompileHost& rHost, const CompileContext& rContext)
{
    FormulaLoweringResult aResult;
    const auto aParsed = core::formula::parseFormula(rFormula);
    if (!aParsed)
    {
        detail::setFailure(aResult, FormulaLoweringReason::ParseFailure,
            api::String(aParsed.maError.maMessage), aParsed.maError.mnOffset);
        return aResult;
    }

    if (!detail::pushNodeTokensLexical(*aParsed.mpRoot, rHost, rContext, aResult))
        return aResult;

    return aResult;
}

} // namespace spreadsheetengine::detail::compiler

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
