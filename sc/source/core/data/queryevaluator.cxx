/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <rtl/math.hxx>
#include <queryevaluator.hxx>

#include <cellform.hxx>
#include <cellformtmpl.hxx>
#include <cellvalue.hxx>
#include <document.hxx>
#include <docoptio.hxx>
#include <queryparam.hxx>
#include <table.hxx>

#include <svl/numformat.hxx>
#include <svl/sharedstringpool.hxx>
#include <svl/zformat.hxx>
#include <unotools/collatorwrapper.hxx>

#include <spreadsheetengine/api/Query.hxx>

namespace sequery = spreadsheetengine::api::query;

namespace
{

sequery::Operator toApiQueryOperator(ScQueryOp eOp)
{
    switch (eOp)
    {
        case SC_EQUAL:
            return sequery::Operator::Equal;
        case SC_NOT_EQUAL:
            return sequery::Operator::NotEqual;
        case SC_LESS:
            return sequery::Operator::Less;
        case SC_GREATER:
            return sequery::Operator::Greater;
        case SC_LESS_EQUAL:
            return sequery::Operator::LessEqual;
        case SC_GREATER_EQUAL:
            return sequery::Operator::GreaterEqual;
        case SC_CONTAINS:
            return sequery::Operator::Contains;
        case SC_DOES_NOT_CONTAIN:
            return sequery::Operator::DoesNotContain;
        case SC_BEGINS_WITH:
            return sequery::Operator::BeginsWith;
        case SC_ENDS_WITH:
            return sequery::Operator::EndsWith;
        case SC_DOES_NOT_BEGIN_WITH:
            return sequery::Operator::DoesNotBeginWith;
        case SC_DOES_NOT_END_WITH:
            return sequery::Operator::DoesNotEndWith;
        case SC_TOPVAL:
        case SC_BOTVAL:
        case SC_TOPPERC:
        case SC_BOTPERC:
            break;
    }

    return sequery::Operator::Equal;
}

sequery::OperandKind toApiOperandKind(ScQueryEntry::QueryType eType)
{
    switch (eType)
    {
        case ScQueryEntry::ByValue:
            return sequery::OperandKind::Value;
        case ScQueryEntry::ByString:
            return sequery::OperandKind::Text;
        case ScQueryEntry::ByDate:
            return sequery::OperandKind::Date;
        case ScQueryEntry::ByEmpty:
            return sequery::OperandKind::Empty;
        case ScQueryEntry::ByTextColor:
            return sequery::OperandKind::TextColor;
        case ScQueryEntry::ByBackgroundColor:
            return sequery::OperandKind::BackgroundColor;
    }

    return sequery::OperandKind::Value;
}

sequery::SearchType toApiSearchType(utl::SearchParam::SearchType eType)
{
    switch (eType)
    {
        case utl::SearchParam::SearchType::Unknown:
            break;
        case utl::SearchParam::SearchType::Normal:
            return sequery::SearchType::Normal;
        case utl::SearchParam::SearchType::Wildcard:
            return sequery::SearchType::Wildcard;
        case utl::SearchParam::SearchType::Regexp:
            return sequery::SearchType::Regex;
    }

    return sequery::SearchType::Normal;
}

sequery::CellClass toApiCellClass(const ScRefCellValue& rCell)
{
    return { rCell.hasNumeric(), rCell.hasString(),
        rCell.getType() == CELLTYPE_FORMULA
            && rCell.getFormula()->GetErrCode() != FormulaError::NONE };
}

} // namespace

bool ScQueryEvaluator::isPartialTextMatchOp(ScQueryOp eOp)
{
    return sequery::isPartialTextMatchOp(toApiQueryOperator(eOp));
}

bool ScQueryEvaluator::isTextMatchOp(ScQueryOp eOp)
{
    return sequery::isTextMatchOp(toApiQueryOperator(eOp));
}

bool ScQueryEvaluator::isMatchWholeCellHelper(bool docMatchWholeCell, ScQueryOp eOp)
{
    return sequery::isMatchWholeCell(docMatchWholeCell, toApiQueryOperator(eOp));
}

bool ScQueryEvaluator::isMatchWholeCell(ScQueryOp eOp) const
{
    return isMatchWholeCellHelper(mbMatchWholeCell, eOp);
}

bool ScQueryEvaluator::isMatchWholeCell(const ScDocument& rDoc, ScQueryOp eOp)
{
    return isMatchWholeCellHelper(rDoc.GetDocOptions().IsMatchWholeCell(), eOp);
}

void ScQueryEvaluator::setupTransliteratorIfNeeded()
{
    if (!mpTransliteration)
        mpTransliteration = &ScGlobal::GetTransliteration(mrParam.bCaseSens);
}

void ScQueryEvaluator::setupCollatorIfNeeded()
{
    if (!mpCollator)
        mpCollator = &ScGlobal::GetCollator(mrParam.bCaseSens);
}

ScQueryEvaluator::ScQueryEvaluator(ScDocument& rDoc, const ScTable& rTab,
                                   const ScQueryParam& rParam, ScInterpreterContext* pContext,
                                   bool* pTestEqualCondition, bool bNewSearchFunction)
    : mrDoc(rDoc)
    , mrStrPool(rDoc.GetSharedStringPool())
    , mrTab(rTab)
    , mrParam(rParam)
    , mpTestEqualCondition(pTestEqualCondition)
    , mpTransliteration(nullptr)
    , mpCollator(nullptr)
    , mbMatchWholeCell(!bNewSearchFunction ? rDoc.GetDocOptions().IsMatchWholeCell() : true)
    , mbCaseSensitive(rParam.bCaseSens)
    , mpContext(pContext)
    , mnEntryCount(mrParam.GetEntryCount())
{
    if (mnEntryCount <= nFixedBools)
    {
        mpPasst = &maBool[0];
        mpTest = &maTest[0];
    }
    else
    {
        mpBoolDynamic.reset(new bool[mnEntryCount]);
        mpTestDynamic.reset(new bool[mnEntryCount]);
        mpPasst = mpBoolDynamic.get();
        mpTest = mpTestDynamic.get();
    }
}

bool ScQueryEvaluator::isRealWildOrRegExp(const ScQueryEntry& rEntry) const
{
    return sequery::isRealWildOrRegExp(
        toApiSearchType(mrParam.eSearchType), toApiQueryOperator(rEntry.eOp));
}

bool ScQueryEvaluator::isTestWildOrRegExp(const ScQueryEntry& rEntry) const
{
    return sequery::isTestWildOrRegExp(
        mpTestEqualCondition != nullptr, toApiSearchType(mrParam.eSearchType),
        toApiQueryOperator(rEntry.eOp));
}

bool ScQueryEvaluator::isQueryByValue(ScQueryOp eOp, ScQueryEntry::QueryType eType,
                                      const ScRefCellValue& rCell)
{
    return sequery::isQueryByValue(
        toApiQueryOperator(eOp), toApiOperandKind(eType), toApiCellClass(rCell));
}

bool ScQueryEvaluator::isQueryByValueForCell(const ScRefCellValue& rCell)
{
    return sequery::isQueryByValueForCell(toApiCellClass(rCell));
}

bool ScQueryEvaluator::isQueryByString(ScQueryOp eOp, ScQueryEntry::QueryType eType,
                                       const ScRefCellValue& rCell)
{
    return sequery::isQueryByString(
        toApiQueryOperator(eOp), toApiOperandKind(eType), toApiCellClass(rCell));
}

sal_uInt32 ScQueryEvaluator::getNumFmt(SCCOL nCol, SCROW nRow)
{
    sal_uInt32 nNumFmt
        = (mpContext ? mrTab.GetNumberFormat(*mpContext, ScAddress(nCol, nRow, mrTab.GetTab()))
                     : mrTab.GetNumberFormat(nCol, nRow));
    if (nNumFmt && (nNumFmt % SV_COUNTRY_LANGUAGE_OFFSET) == 0)
        // Any General of any locale is irrelevant for rounding.
        nNumFmt = 0;
    return nNumFmt;
}

std::pair<bool, bool> ScQueryEvaluator::compareByValue(const ScRefCellValue& rCell, SCCOL nCol,
                                                       SCROW nRow, const ScQueryEntry& rEntry,
                                                       const ScQueryEntry::Item& rItem)
{
    bool bOk = false;
    bool bTestEqual = false;
    double nCellVal;
    double fQueryVal = rItem.mfVal;
    // Defer all number format detection to as late as possible as it's a
    // bottle neck, even if that complicates the code. Also do not
    // unnecessarily call ScDocument::RoundValueAsShown() for the same
    // reason.
    sal_uInt32 nNumFmt = NUMBERFORMAT_ENTRY_NOT_FOUND;

    switch (rCell.getType())
    {
        case CELLTYPE_VALUE:
            nCellVal = rCell.getDouble();
            break;
        case CELLTYPE_FORMULA:
            nCellVal = rCell.getFormula()->GetValue();
            break;
        default:
            nCellVal = 0.0;
    }
    if (rItem.mbRoundForFilter && nCellVal != 0.0)
    {
        nNumFmt = getNumFmt(nCol, nRow);
        if (nNumFmt)
        {
            switch (rCell.getType())
            {
                case CELLTYPE_VALUE:
                case CELLTYPE_FORMULA:
                    nCellVal = mrDoc.RoundValueAsShown(nCellVal, nNumFmt, mpContext);
                    break;
                default:
                    assert(!"can't be");
            }
        }
    }

    /* NOTE: lcl_PrepareQuery() prepares a filter query such that if a
     * date+time format was queried rEntry.bQueryByDate is not set. In
     * case other queries wanted to use this mechanism they should do
     * the same, in other words only if rEntry.nVal is an integer value
     * rEntry.bQueryByDate should be true and the time fraction be
     * stripped here. */

    if (rItem.meType == ScQueryEntry::ByDate)
    {
        if (nNumFmt == NUMBERFORMAT_ENTRY_NOT_FOUND)
            nNumFmt = getNumFmt(nCol, nRow);
        if (nNumFmt)
        {
            const SvNumberFormatter* pFormatter
                = mpContext ? mpContext->GetFormatTable() : mrDoc.GetFormatTable();
            const SvNumberformat* pEntry = pFormatter->GetEntry(nNumFmt);
            if (pEntry)
            {
                SvNumFormatType nNumFmtType = pEntry->GetType();
                /* NOTE: Omitting the check for absence of
                 * css::util::NumberFormat::TIME would include also date+time formatted
                 * values of the same day. That may be desired in some
                 * cases, querying all time values of a day, but confusing
                 * in other cases. A user can always setup a standard
                 * filter query for x >= date AND x < date+1 */
                if ((nNumFmtType & SvNumFormatType::DATE) && !(nNumFmtType & SvNumFormatType::TIME))
                {
                    // The format is of date type.  Strip off the time
                    // element.
                    nCellVal = ::rtl::math::approxFloor(nCellVal);
                }
            }
        }
    }

    switch (rEntry.eOp)
    {
        case SC_EQUAL:
            bOk = ::rtl::math::approxEqual(nCellVal, fQueryVal);
            break;
        case SC_LESS:
            bOk = (nCellVal < fQueryVal) && !::rtl::math::approxEqual(nCellVal, fQueryVal);
            break;
        case SC_GREATER:
            bOk = (nCellVal > fQueryVal) && !::rtl::math::approxEqual(nCellVal, fQueryVal);
            break;
        case SC_LESS_EQUAL:
            bOk = (nCellVal < fQueryVal) || ::rtl::math::approxEqual(nCellVal, fQueryVal);
            if (bOk && mpTestEqualCondition)
                bTestEqual = ::rtl::math::approxEqual(nCellVal, fQueryVal);
            break;
        case SC_GREATER_EQUAL:
            bOk = (nCellVal > fQueryVal) || ::rtl::math::approxEqual(nCellVal, fQueryVal);
            if (bOk && mpTestEqualCondition)
                bTestEqual = ::rtl::math::approxEqual(nCellVal, fQueryVal);
            break;
        case SC_NOT_EQUAL:
            bOk = !::rtl::math::approxEqual(nCellVal, fQueryVal);
            break;
        default:
            assert(false);
            break;
    }

    return std::pair<bool, bool>(bOk, bTestEqual);
}

OUString ScQueryEvaluator::getCellString(const ScRefCellValue& rCell, SCROW nRow, SCCOL nCol)
{
    if (rCell.getType() == CELLTYPE_FORMULA
        && rCell.getFormula()->GetErrCode() != FormulaError::NONE)
    {
        // Error cell is evaluated as string (for now).
        const FormulaError error = rCell.getFormula()->GetErrCode();
        auto it = mCachedSharedErrorStrings.find(error);
        if (it == mCachedSharedErrorStrings.end())
        {
            svl::SharedString str = mrStrPool.intern(ScGlobal::GetErrorString(error));
            auto pos = mCachedSharedErrorStrings.insert({ error, str });
            assert(pos.second); // inserted
            it = pos.first;
        }
        return it->second.getString();
    }
    else if (rCell.getType() == CELLTYPE_STRING)
    {
        return rCell.getSharedString()->getString();
    }
    else
    {
        sal_uInt32 nFormat
            = mpContext ? mrTab.GetNumberFormat(*mpContext, ScAddress(nCol, nRow, mrTab.GetTab()))
                        : mrTab.GetNumberFormat(nCol, nRow);
        return ScCellFormat::GetInputString(rCell, nFormat, mpContext, mrDoc, true);
    }
}

template <typename TFunctor>
auto ScQueryEvaluator::visitCellSharedString(const ScRefCellValue& rCell, SCROW nRow, SCCOL nCol,
                                             const TFunctor& rOper)
{
    if (rCell.getType() == CELLTYPE_FORMULA
        && rCell.getFormula()->GetErrCode() != FormulaError::NONE)
    {
        // Error cell is evaluated as string (for now).
        const FormulaError error = rCell.getFormula()->GetErrCode();
        auto it = mCachedSharedErrorStrings.find(error);
        if (it == mCachedSharedErrorStrings.end())
        {
            svl::SharedString str = mrStrPool.intern(ScGlobal::GetErrorString(error));
            auto pos = mCachedSharedErrorStrings.insert({ error, std::move(str) });
            assert(pos.second); // inserted
            it = pos.first;
        }
        return rOper(it->second);
    }
    else if (rCell.getType() == CELLTYPE_STRING)
    {
        return rOper(*rCell.getSharedString());
    }
    else
    {
        sal_uInt32 nFormat
            = mpContext ? mrTab.GetNumberFormat(*mpContext, ScAddress(nCol, nRow, mrTab.GetTab()))
                        : mrTab.GetNumberFormat(nCol, nRow);
        return ScCellFormat::visitInputSharedString(rCell, nFormat, mpContext, mrDoc, mrStrPool,
                                                    true, false, rOper);
    }
}

svl::SharedString ScQueryEvaluator::getCellSharedString(const ScRefCellValue& rCell, SCROW nRow,
                                                        SCCOL nCol)
{
    return visitCellSharedString(rCell, nRow, nCol,
                                 [](const svl::SharedString& arg) { return arg; });
}

static bool equalCellSharedString(const svl::SharedString& rValueSource,
                                  const svl::SharedString& rString, bool bCaseSens)
{
    // Fast string equality check by comparing string identifiers.
    // This is the bFast path, all conditions should lead here on bFast == true.
    if (bCaseSens)
        return rValueSource.getData() == rString.getData();
    return rValueSource.getDataIgnoreCase() == rString.getDataIgnoreCase();
}

bool ScQueryEvaluator::equalCellSharedString(const ScRefCellValue& rCell, SCROW nRow,
                                             SCCOLROW nField, bool bCaseSens,
                                             const svl::SharedString& rString)
{
    return visitCellSharedString(rCell, nRow, nField,
                                 [&rString, bCaseSens](const svl::SharedString& arg) {
                                     return ::equalCellSharedString(arg, rString, bCaseSens);
                                 });
}

bool ScQueryEvaluator::isFastCompareByString(const ScQueryEntry& rEntry) const
{
    // If this is true, then there's a fast path in compareByString() which
    // can be selected using the template argument to get fast code
    // that will not check the same conditions every time. This makes a difference
    // in fast lookups that search for an exact value (case sensitive or not).
    const bool bRealWildOrRegExp = isRealWildOrRegExp(rEntry);
    const bool bTestWildOrRegExp = isTestWildOrRegExp(rEntry);
    return sequery::shouldUseFastStringEqualityPath(toApiQueryOperator(rEntry.eOp),
        bRealWildOrRegExp, bTestWildOrRegExp, isMatchWholeCell(rEntry.eOp));
}

// The value is placed inside parameter rValueSource.
// For the template argument see isFastCompareByString().
template <bool bFast>
std::pair<bool, bool> ScQueryEvaluator::compareByString(const ScQueryEntry& rEntry,
                                                        const ScQueryEntry::Item& rItem,
                                                        const ScRefCellValue& rCell, SCROW nRow)
{
    bool bOk = false;
    bool bTestEqual = false;
    bool bMatchWholeCell;
    if (bFast)
        bMatchWholeCell = true;
    else
        bMatchWholeCell = isMatchWholeCell(rEntry.eOp);
    const bool bRealWildOrRegExp = !bFast && isRealWildOrRegExp(rEntry);
    const bool bTestWildOrRegExp = !bFast && isTestWildOrRegExp(rEntry);

    if (!bFast && (bRealWildOrRegExp || bTestWildOrRegExp))
    {
        svl::SharedString rValueSource = getCellSharedString(rCell, nRow, rEntry.nField);
        const OUString& rValue = rValueSource.getString();

        sal_Int32 nStart = 0;
        sal_Int32 nEnd = rValue.getLength();

        // from 614 on, nEnd is behind the found text
        bool bMatch = false;
        if (sequery::isEndsWithOp(toApiQueryOperator(rEntry.eOp)))
        {
            nEnd = 0;
            nStart = rValue.getLength();
            bMatch
                = rEntry.GetSearchTextPtr(mrParam.eSearchType, mrParam.bCaseSens, bMatchWholeCell)
                      ->SearchBackward(rValue, &nStart, &nEnd);
        }
        else
        {
            bMatch
                = rEntry.GetSearchTextPtr(mrParam.eSearchType, mrParam.bCaseSens, bMatchWholeCell)
                      ->SearchForward(rValue, &nStart, &nEnd);
        }
        bMatch = sequery::isWholeCellSearchMatch(bMatchWholeCell, bMatch, nStart, nEnd,
            rValue.getLength());
        if (bRealWildOrRegExp)
        {
            bOk = sequery::evaluatePatternSearchMatch(
                toApiQueryOperator(rEntry.eOp), bMatch, nStart, nEnd, rValue.getLength());
        }
        else
            bTestEqual = bMatch;
    }
    if (bFast || !bRealWildOrRegExp)
    {
        // Simple string matching i.e. no regexp match.
        if (bFast || isTextMatchOp(rEntry.eOp))
        {
            // Check this even with bFast.
            if (sequery::shouldRejectAssignedEmptyStringQuery(
                    toApiOperandKind(rItem.meType), rItem.maString.isEmpty()))
            {
                // #i18374# When used from functions (match, countif, sumif, vlookup, hlookup, lookup),
                // the query value is assigned directly, and the string is empty. In that case,
                // don't find any string (isEqual would find empty string results in formula cells).
                bOk = false;
                if (rEntry.eOp == SC_NOT_EQUAL)
                    bOk = !bOk;
            }
            else
            {
                if (sequery::shouldUseExactStringEqualityPath(bFast, bMatchWholeCell))
                {
                    bOk = equalCellSharedString(rCell, nRow, rEntry.nField, mrParam.bCaseSens,
                                                rItem.maString);

                    if (!bFast)
                        bOk = sequery::evaluateEqualityMatch(
                            toApiQueryOperator(rEntry.eOp), bOk);
                }
                else
                {
                    svl::SharedString rValueSource
                        = getCellSharedString(rCell, nRow, rEntry.nField);

                    // Where do we find a match (if at all)
                    sal_Int32 nStrPos;

                    if (!mbCaseSensitive)
                    { // Common case for vlookup etc.
                        const rtl_uString* pQuer = rItem.maString.getDataIgnoreCase();
                        const rtl_uString* pCellStr = rValueSource.getDataIgnoreCase();

                        assert(pCellStr != nullptr);
                        if (pQuer == nullptr)
                            pQuer = svl::SharedString::getEmptyString().getDataIgnoreCase();

                        const sal_Int32 nIndex = sequery::computeSubstringSearchStart(
                            toApiQueryOperator(rEntry.eOp), pCellStr->length, pQuer->length);

                        if (nIndex < 0)
                            nStrPos = -1;
                        else
                        { // OUString::indexOf
                            nStrPos = rtl_ustr_indexOfStr_WithLength(pCellStr->buffer + nIndex,
                                                                     pCellStr->length - nIndex,
                                                                     pQuer->buffer, pQuer->length);

                            if (nStrPos >= 0)
                                nStrPos += nIndex;
                        }
                    }
                    else
                    {
                        const OUString& rValue = rValueSource.getString();
                        const OUString aQueryStr = rItem.maString.getString();
                        const LanguageType nLang
                            = ScGlobal::oSysLocale->GetLanguageTag().getLanguageType();
                        setupTransliteratorIfNeeded();
                        const OUString aCell(mpTransliteration->transliterate(
                            rValue, nLang, 0, rValue.getLength(), nullptr));

                        const OUString aQuer(mpTransliteration->transliterate(
                            aQueryStr, nLang, 0, aQueryStr.getLength(), nullptr));

                        const sal_Int32 nIndex = sequery::computeSubstringSearchStart(
                            toApiQueryOperator(rEntry.eOp), aCell.getLength(),
                            aQuer.getLength());
                        nStrPos = ((nIndex < 0) ? -1 : aCell.indexOf(aQuer, nIndex));
                    }
                    bOk = sequery::evaluateSubstringMatch(
                        toApiQueryOperator(rEntry.eOp), nStrPos);
                }
            }
        }
        else
        { // use collator here because data was probably sorted
            svl::SharedString rValueSource = getCellSharedString(rCell, nRow, rEntry.nField);
            const OUString& rValue = rValueSource.getString();
            setupCollatorIfNeeded();
            sal_Int32 nCompare = mpCollator->compareString(rValue, rItem.maString.getString());
            const sequery::OrderedCompareResult aCompare
                = sequery::evaluateOrderedStringCompare(
                    toApiQueryOperator(rEntry.eOp), nCompare);
            bOk = aCompare.mbMatch;
            if (bOk && mpTestEqualCondition && !bTestEqual)
                bTestEqual = aCompare.mbEqual;
        }
    }

    return std::pair<bool, bool>(bOk, bTestEqual);
}

std::pair<bool, bool> ScQueryEvaluator::compareByTextColor(SCCOL nCol, SCROW nRow,
                                                           const ScQueryEntry::Item& rItem)
{
    ScAddress aPos(nCol, nRow, mrTab.GetTab());
    Color color = mrTab.GetCellTextColor(aPos);

    bool bMatch = rItem.maColor == color;
    return std::pair<bool, bool>(bMatch, false);
}

std::pair<bool, bool> ScQueryEvaluator::compareByBackgroundColor(SCCOL nCol, SCROW nRow,
                                                                 const ScQueryEntry::Item& rItem)
{
    ScAddress aPos(nCol, nRow, mrTab.GetTab());
    Color color = mrTab.GetCellBackgroundColor(aPos);

    bool bMatch = rItem.maColor == color;
    return std::pair<bool, bool>(bMatch, false);
}

// To be called only if both isQueryByValue() and isQueryByString()
// returned false and range lookup is wanted! In range lookup comparison
// numbers are less than strings. Nothing else is compared.
std::pair<bool, bool> ScQueryEvaluator::compareByRangeLookup(const ScRefCellValue& rCell,
                                                             const ScQueryEntry& rEntry,
                                                             const ScQueryEntry::Item& rItem)
{
    bool bTestEqual = false;
    return std::pair<bool, bool>(
        sequery::evaluateRangeLookupMatch(toApiQueryOperator(rEntry.eOp),
            toApiOperandKind(rItem.meType), toApiCellClass(rCell)),
        bTestEqual);
}

std::pair<bool, bool> ScQueryEvaluator::processEntry(SCROW nRow, SCCOL nCol,
                                                     const ScRefCellValue& aCell,
                                                     const ScQueryEntry& rEntry, size_t nEntryIndex)
{
    std::pair<bool, bool> aRes(false, false);
    const ScQueryEntry::QueryItemsType& rItems = rEntry.GetQueryItems();
    if (rItems.size() == 1 && rItems.front().meType == ScQueryEntry::ByEmpty)
    {
        if (rEntry.IsQueryByEmpty())
            aRes.first = aCell.isEmpty();
        else
        {
            assert(rEntry.IsQueryByNonEmpty());
            aRes.first = !aCell.isEmpty();
        }
        return aRes;
    }
    if (sequery::shouldTryMultiEqualityFastPath(toApiQueryOperator(rEntry.eOp), rItems.size()))
    {
        // If there are many items to query for (autofilter does this), then try to search
        // efficiently in those items. So first search all the items of the relevant type,
        // If that does not find anything, fall back to the generic code.
        double value = 0;
        bool valid = true;
        // For ScQueryEntry::ByValue check that the cell either is a value or is a formula
        // that has a value and is not an error (those are compared as strings). This
        // is basically simplified isQueryByValue().
        if (aCell.getType() == CELLTYPE_VALUE)
            value = aCell.getDouble();
        else if (aCell.getType() == CELLTYPE_FORMULA
                 && aCell.getFormula()->GetErrCode() != FormulaError::NONE
                 && aCell.getFormula()->IsValue())
        {
            value = aCell.getFormula()->GetValue();
        }
        else
            valid = false;
        if (valid)
        {
            if (sequery::shouldUseSortedItemCache(rItems.size()))
            {
                // Sort, cache and binary search for the value in items.
                // Don't bother comparing approximately.
                if (mCachedSortedItemValues.size() <= nEntryIndex)
                {
                    mCachedSortedItemValues.resize(nEntryIndex + 1);
                    mCachedSortedItemValues[nEntryIndex]
                        = sequery::collectSortedNumericValues(
                            rItems.begin(), rItems.end(),
                            [](const ScQueryEntry::Item& rItem) {
                                return rItem.meType == ScQueryEntry::ByValue;
                            },
                            [](const ScQueryEntry::Item& rItem) { return rItem.mfVal; });
                }
                auto& values = mCachedSortedItemValues[nEntryIndex];
                if (sequery::containsSortedNumericValue(values, value))
                    return std::make_pair(true, true);
            }
            else
            {
                // For speed don't bother comparing approximately here, usually there either
                // will be an exact match or it wouldn't match anyway.
                if (sequery::containsLinearNumericValue(
                        rItems.begin(), rItems.end(), value,
                        [](const ScQueryEntry::Item& rItem) {
                            return rItem.meType == ScQueryEntry::ByValue;
                        },
                        [](const ScQueryEntry::Item& rItem) { return rItem.mfVal; }))
                    return std::make_pair(true, true);
            }
        }
    }
    const bool bFastCompareByString = isFastCompareByString(rEntry);
    if (sequery::shouldUseStringIdentityMultiEqualityFastPath(
            bFastCompareByString, toApiQueryOperator(rEntry.eOp), rItems.size()))
    {
        // The same as above but for strings. Try to optimize the case when
        // it's a svl::SharedString comparison. That happens when SC_EQUAL is used
        // and simple matching is used, see compareByString()
        svl::SharedString cellSharedString = getCellSharedString(aCell, nRow, rEntry.nField);
        // Allow also checking ScQueryEntry::ByValue if the cell is not numeric,
        // as in that case isQueryByNumeric() would be false and isQueryByString() would
        // be true because of SC_EQUAL making isTextMatchOp() true.
        const bool bCompareValueOperandAsString
            = sequery::shouldCompareValueOperandAsString(toApiCellClass(aCell));
        // For ScQueryEntry::ByString check that the cell is represented by a shared string,
        // which means it's either a string cell or a formula error. This is not as
        // generous as isQueryByString() but it should be enough and better be safe.
        if (sequery::shouldUseSortedItemCache(rItems.size()))
        {
            // Sort, cache and binary search for the string in items.
            // Since each SharedString is identified by pointer value,
            // sorting by pointer value is enough.
            if (mCachedSortedItemStrings.size() <= nEntryIndex)
            {
                mCachedSortedItemStrings.resize(nEntryIndex + 1);
                mCachedSortedItemStrings[nEntryIndex]
                    = sequery::collectSortedStringIdentities(
                        rItems.begin(), rItems.end(),
                        [bCompareValueOperandAsString](const ScQueryEntry::Item& rItem) {
                            return sequery::shouldIncludeOperandInStringIdentityCache(
                                toApiOperandKind(rItem.meType), bCompareValueOperandAsString);
                        },
                        [this](const ScQueryEntry::Item& rItem) {
                            return mrParam.bCaseSens
                                       ? rItem.maString.getData()
                                       : rItem.maString.getDataIgnoreCase();
                        });
            }
            auto& values = mCachedSortedItemStrings[nEntryIndex];
            const rtl_uString* pStringIdentity
                = mrParam.bCaseSens ? cellSharedString.getData()
                                    : cellSharedString.getDataIgnoreCase();
            if (sequery::containsSortedStringIdentity(values, pStringIdentity))
                return std::make_pair(true, true);
        }
        else
        {
            const rtl_uString* pStringIdentity
                = mrParam.bCaseSens ? cellSharedString.getData()
                                    : cellSharedString.getDataIgnoreCase();
            if (sequery::containsLinearStringIdentity(
                    rItems.begin(), rItems.end(), pStringIdentity,
                    [bCompareValueOperandAsString](const ScQueryEntry::Item& rItem) {
                        return sequery::shouldIncludeOperandInStringIdentityCache(
                            toApiOperandKind(rItem.meType), bCompareValueOperandAsString);
                    },
                    [this](const ScQueryEntry::Item& rItem) {
                        return mrParam.bCaseSens
                                   ? rItem.maString.getData()
                                   : rItem.maString.getDataIgnoreCase();
                    }))
                return std::make_pair(true, true);
        }
    }
    // Generic handling.
    for (const auto& rItem : rItems)
    {
        if (rItem.meType == ScQueryEntry::ByTextColor)
        {
            std::pair<bool, bool> aThisRes = compareByTextColor(nCol, nRow, rItem);
            aRes.first |= aThisRes.first;
            aRes.second |= aThisRes.second;
        }
        else if (rItem.meType == ScQueryEntry::ByBackgroundColor)
        {
            std::pair<bool, bool> aThisRes = compareByBackgroundColor(nCol, nRow, rItem);
            aRes.first |= aThisRes.first;
            aRes.second |= aThisRes.second;
        }
        else if (isQueryByValue(rEntry.eOp, rItem.meType, aCell))
        {
            std::pair<bool, bool> aThisRes = compareByValue(aCell, nCol, nRow, rEntry, rItem);
            aRes.first |= aThisRes.first;
            aRes.second |= aThisRes.second;
        }
        else if (isQueryByString(rEntry.eOp, rItem.meType, aCell))
        {
            std::pair<bool, bool> aThisRes;
            if (bFastCompareByString) // fast
                aThisRes = compareByString<true>(rEntry, rItem, aCell, nRow);
            else
                aThisRes = compareByString(rEntry, rItem, aCell, nRow);
            aRes.first |= aThisRes.first;
            aRes.second |= aThisRes.second;
        }
        else if (mrParam.mbRangeLookup)
        {
            std::pair<bool, bool> aThisRes = compareByRangeLookup(aCell, rEntry, rItem);
            aRes.first |= aThisRes.first;
            aRes.second |= aThisRes.second;
        }

        if (aRes.first && (aRes.second || mpTestEqualCondition == nullptr))
            break;
    }
    return aRes;
}

bool ScQueryEvaluator::ValidQuery(SCROW nRow, const ScRefCellValue* pCell,
                                  sc::TableColumnBlockPositionSet* pBlockPos)
{
    if (!mrParam.GetEntry(0).bDoQuery)
        return true;

    tools::Long nPos = -1;
    ScQueryParam::const_iterator it, itBeg = mrParam.begin(), itEnd = mrParam.end();
    for (it = itBeg; it != itEnd && it->bDoQuery; ++it)
    {
        const ScQueryEntry& rEntry = *it;

        // Short-circuit the test at the end of the loop - if this is SC_AND
        // and the previous value is false, this value will not be needed.
        // Disable this if pbTestEqualCondition is present as that one may get set
        // even if the result is false (that also means pTest doesn't need to be
        // handled here).
        if (rEntry.eConnect == SC_AND && mpTestEqualCondition == nullptr && nPos != -1
            && !mpPasst[nPos])
        {
            continue;
        }

        SCCOL nCol = static_cast<SCCOL>(rEntry.nField);

        // We can only handle one single direct query passed as a known pCell,
        // subsequent queries have to obtain the cell.
        ScRefCellValue aCell;
        if (pCell && it == itBeg)
            aCell = *pCell;
        else if (pBlockPos)
        { // hinted mdds access
            aCell = const_cast<ScTable&>(mrTab).GetCellValue(
                nCol, *pBlockPos->getBlockPosition(nCol), nRow);
        }
        else
            aCell = mrTab.GetCellValue(nCol, nRow);

        std::pair<bool, bool> aRes = processEntry(nRow, nCol, aCell, rEntry, it - itBeg);

        if (nPos == -1)
        {
            nPos++;
            mpPasst[nPos] = aRes.first;
            mpTest[nPos] = aRes.second;
        }
        else
        {
            if (rEntry.eConnect == SC_AND)
            {
                mpPasst[nPos] = mpPasst[nPos] && aRes.first;
                mpTest[nPos] = mpTest[nPos] && aRes.second;
            }
            else
            {
                nPos++;
                mpPasst[nPos] = aRes.first;
                mpTest[nPos] = aRes.second;
            }
        }
    }

    for (tools::Long j = 1; j <= nPos; j++)
    {
        mpPasst[0] = mpPasst[0] || mpPasst[j];
        mpTest[0] = mpTest[0] || mpTest[j];
    }

    bool bRet = mpPasst[0];
    if (mpTestEqualCondition)
        *mpTestEqualCondition = mpTest[0];

    return bRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
