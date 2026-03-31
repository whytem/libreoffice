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
#include <vector>

#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/Query.hxx>
#include <spreadsheetengine/detail/OdfFormulaParser.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::query
{

struct CriteriaPredicate
{
    enum class OperandKind : sal_uInt8
    {
        Empty = 0,
        Number,
        Text
    };

    formula::BinaryOperator meOperator = formula::BinaryOperator::Equal;
    OperandKind meOperandKind = OperandKind::Empty;
    double mfNumber = 0.0;
    api::String maText;
    api::String maNumericText;
    bool mbNumberOriginatedFromText = false;
    bool mbOperatorOnlyTextCriterion = false;
};

struct CriteriaAggregateInput
{
    bool mbScalar = true;
    api::CellValue maScalar = api::CellValue::empty();
    api::ResolvedReference maReference;
    api::MatrixSize mnColumns = 1;
    api::MatrixSize mnRows = 1;
};

enum class CriteriaAggregateKind : sal_uInt8
{
    Count = 0,
    Sum,
    Average,
    Max,
    Min
};

class CriteriaAggregateMaterializer
{
public:
    virtual ~CriteriaAggregateMaterializer() = default;

    [[nodiscard]] virtual api::ValueResult<api::CellValue> materialize(
        const CriteriaAggregateInput& rInput, api::MatrixCoordinate aCoordinate) const = 0;
};

using CriteriaNumberTextParser = std::optional<api::NumberParseResult> (*)(api::StringView);
using CriteriaAsciiDoubleParser = std::optional<double> (*)(api::StringView);

SPREADSHEETENGINE_DLLPUBLIC sal_Int32 compareFoldedText(
    api::StringView rLeft, api::StringView rRight);

SPREADSHEETENGINE_DLLPUBLIC bool matchesWholeCellLookupText(api::StringView rLookupText,
    api::StringView rCandidateText, api::query::SearchType eSearchType);

SPREADSHEETENGINE_DLLPUBLIC bool matchesQueryText(api::StringView rLookupText,
    api::StringView rCandidateText, api::query::SearchType eSearchType, bool bMatchWholeCell);

SPREADSHEETENGINE_DLLPUBLIC std::optional<CriteriaPredicate> makeCriteriaPredicate(
    const api::CellValue& rCriteriaValue, CriteriaNumberTextParser pParseNumberText,
    CriteriaAsciiDoubleParser pParseAsciiDouble);

SPREADSHEETENGINE_DLLPUBLIC bool matchesCriteriaPredicate(const CriteriaPredicate& rPredicate,
    const api::CellValue& rCandidate, api::query::SearchType eSearchType,
    bool bMatchWholeCell);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<api::CellValue> evaluateCriteriaAggregate(
    const CriteriaAggregateMaterializer& rMaterializer,
    const std::vector<CriteriaAggregateInput>& rCriteriaRanges,
    const std::vector<CriteriaPredicate>& rCriteria, const CriteriaAggregateInput* pTargetRange,
    CriteriaAggregateKind eAggregateKind, api::query::SearchType eSearchType,
    bool bMatchWholeCell);

} // namespace spreadsheetengine::core::query

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
