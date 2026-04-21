/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <map>
#include <random>
#include <tuple>
#include <utility>
#include <vector>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/Query.hxx>

namespace spreadsheetengine::core::host
{

class InMemoryEvaluationHost final : public api::EvaluationHost
{
    struct Sheet
    {
        api::String maName;
        std::map<std::pair<api::ColumnIndex, api::RowIndex>, api::CellValue> maCells;
    };

    std::vector<Sheet> maSheets;
    api::DateParts maNullDate { 1899, 12, 30 };
    api::String maLocaleTag;
    api::query::SearchType meSearchType = api::query::SearchType::Normal;
    std::map<std::pair<api::String, api::NumberParseMode>, api::NumberParseResult> maParsedNumbers;
    std::map<std::pair<double, api::FormatIndex>, api::String> maFormattedNumbers;
    mutable std::mt19937 maRandomGenerator;

    [[nodiscard]] const Sheet* getSheet(api::SheetId nSheet) const
    {
        if (nSheet < 0 || static_cast<std::size_t>(nSheet) >= maSheets.size())
            return nullptr;
        return &maSheets[static_cast<std::size_t>(nSheet)];
    }

    [[nodiscard]] Sheet* getSheet(api::SheetId nSheet)
    {
        if (nSheet < 0 || static_cast<std::size_t>(nSheet) >= maSheets.size())
            return nullptr;
        return &maSheets[static_cast<std::size_t>(nSheet)];
    }

public:
    [[nodiscard]] api::SheetId addSheet(api::StringView rName)
    {
        maSheets.push_back({ api::String(rName), {} });
        return static_cast<api::SheetId>(maSheets.size() - 1);
    }

    void setNullDate(const api::DateParts& rNullDate) { maNullDate = rNullDate; }

    void setLocaleTag(api::StringView rLocaleTag) { maLocaleTag = api::String(rLocaleTag); }

    void setSearchType(api::query::SearchType eSearchType) { meSearchType = eSearchType; }

    void seedRandomGenerator(std::uint32_t nSeed) { maRandomGenerator.seed(nSeed); }

    void setParsedNumber(api::StringView rText, double fValue, api::FormatIndex nFormat = 0,
        api::NumberParseResult::Kind eKind = api::NumberParseResult::Kind::Number,
        api::NumberParseMode eMode = api::NumberParseMode::General)
    {
        maParsedNumbers[{ api::String(rText), eMode }] = { fValue, nFormat, eKind };
    }

    void setFormattedNumber(
        double fValue, api::FormatIndex nFormat, api::StringView rFormattedValue)
    {
        maFormattedNumbers[{ fValue, nFormat }] = api::String(rFormattedValue);
    }

    [[nodiscard]] bool setCellValue(const api::CellAddress& rAddress, const api::CellValue& rValue)
    {
        Sheet* pSheet = getSheet(rAddress.mnSheet);
        if (!pSheet)
            return false;

        pSheet->maCells[{ rAddress.mnColumn, rAddress.mnRow }] = rValue;
        return true;
    }

    [[nodiscard]] bool hasSheet(api::SheetId nSheet) const override
    {
        return getSheet(nSheet) != nullptr;
    }

    [[nodiscard]] sal_Int32 sheetCount() const override
    {
        return static_cast<sal_Int32>(maSheets.size());
    }

    [[nodiscard]] api::ValueResult<api::String> getSheetName(api::SheetId nSheet) const override
    {
        const Sheet* pSheet = getSheet(nSheet);
        if (!pSheet)
            return api::ValueResult<api::String>::failure(api::Error::IllegalArgument);
        return api::ValueResult<api::String>::success(pSheet->maName);
    }

    [[nodiscard]] api::ValueResult<bool> iterateRangeCells(
        const api::CellRange& rRange,
        const std::function<bool(const api::CellAddress&, const api::CellValue&)>& rVisitor)
        const override
    {
        if (!rRange.isNormalized() || !hasSheet(rRange.maStart.mnSheet))
            return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

        for (api::RowIndex nRow = rRange.maStart.mnRow;; ++nRow)
        {
            for (api::ColumnIndex nColumn = rRange.maStart.mnColumn;; ++nColumn)
            {
                const api::CellAddress aAddress { rRange.maStart.mnSheet, nColumn, nRow };
                const auto aValue = getCellValue(aAddress);
                if (!aValue)
                    return api::ValueResult<bool>::failure(aValue.meError);
                if (!rVisitor(aAddress, aValue.maValue))
                    return api::ValueResult<bool>::success(false);
                if (nColumn == rRange.maEnd.mnColumn)
                    break;
            }

            if (nRow == rRange.maEnd.mnRow)
                break;
        }

        return api::ValueResult<bool>::success(true);
    }

    [[nodiscard]] api::DateParts getNullDate() const override { return maNullDate; }

    [[nodiscard]] api::String getLocaleTag() const override { return maLocaleTag; }

    [[nodiscard]] api::query::SearchType getSearchType() const override
    {
        return meSearchType;
    }

    [[nodiscard]] api::ValueResult<double> sampleUniformReal(
        double fLowerInclusive, double fUpperExclusive) const override
    {
        if (fUpperExclusive < fLowerInclusive)
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);

        std::uniform_real_distribution<double> aDistribution(fLowerInclusive, fUpperExclusive);
        return api::ValueResult<double>::success(aDistribution(maRandomGenerator));
    }

    [[nodiscard]] api::ValueResult<api::CellValue> getCellValue(
        const api::CellAddress& rAddress) const override
    {
        const Sheet* pSheet = getSheet(rAddress.mnSheet);
        if (!pSheet)
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);

        const auto aIt = pSheet->maCells.find({ rAddress.mnColumn, rAddress.mnRow });
        if (aIt == pSheet->maCells.end())
            return api::ValueResult<api::CellValue>::success(api::CellValue::empty());

        return api::ValueResult<api::CellValue>::success(aIt->second);
    }

    [[nodiscard]] api::ValueResult<api::CellValue> getRangeValue(const api::CellRange& rRange,
        api::ColumnIndex nColumnOffset, api::RowIndex nRowOffset) const override
    {
        if (!rRange.isNormalized() || !rRange.containsOffset(nColumnOffset, nRowOffset))
        {
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
        }

        return getCellValue({ rRange.maStart.mnSheet, rRange.maStart.mnColumn + nColumnOffset,
            rRange.maStart.mnRow + nRowOffset });
    }

    [[nodiscard]] api::ValueResult<api::ResolvedReference> resolveReference(
        const api::CellRange& rRange) const override
    {
        if (!rRange.isNormalized() || !hasSheet(rRange.maStart.mnSheet))
        {
            return api::ValueResult<api::ResolvedReference>::failure(api::Error::IllegalArgument);
        }

        return api::ValueResult<api::ResolvedReference>::success({ rRange });
    }

    [[nodiscard]] api::ValueResult<api::NumberParseResult> parseNumber(
        api::StringView rValue, api::NumberParseMode eMode = api::NumberParseMode::General) const override
    {
        const auto aKey = std::make_pair(api::String(rValue), eMode);
        auto aIt = maParsedNumbers.find(aKey);
        if (aIt == maParsedNumbers.end() && eMode != api::NumberParseMode::General)
            aIt = maParsedNumbers.find(
                std::make_pair(api::String(rValue), api::NumberParseMode::General));
        if (aIt == maParsedNumbers.end())
        {
            return api::ValueResult<api::NumberParseResult>::failure(api::Error::NoValue);
        }

        return api::ValueResult<api::NumberParseResult>::success(aIt->second);
    }

    [[nodiscard]] api::ValueResult<api::String> formatNumber(
        double fValue, api::FormatIndex nFormat = 0) const override
    {
        const auto aIt = maFormattedNumbers.find({ fValue, nFormat });
        if (aIt == maFormattedNumbers.end())
            return api::ValueResult<api::String>::failure(api::Error::IllegalArgument);

        return api::ValueResult<api::String>::success(aIt->second);
    }
};

} // namespace spreadsheetengine::core::host

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
