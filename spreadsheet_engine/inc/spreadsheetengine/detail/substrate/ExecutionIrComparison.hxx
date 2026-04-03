/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/detail/substrate/ExecutionIrBuilder.hxx>

namespace spreadsheetengine::detail::substrate
{

enum class ExecutionIrComparisonKind : std::uint8_t
{
    Exact,
    NormalizedEquivalent,
    Mismatch
};

struct ExecutionIrWorkbookComparison
{
    ExecutionIrComparisonKind meKind = ExecutionIrComparisonKind::Mismatch;
    bool mbSnapshotMatch = false;
    bool mbGrammarMatch = false;
    bool mbFormulaRecordExactMatch = false;
    bool mbFormulaRecordNormalizedMatch = false;
    bool mbFormulaGroupExactMatch = false;
    bool mbFormulaGroupNormalizedMatch = false;
    bool mbBuildFailureExactMatch = false;
    bool mbBuildFailureNormalizedMatch = false;
    bool mbFullMatch = false;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrWorkbookComparison& rOther) const
        = default;
};

namespace ircmpdetail
{

[[nodiscard]] inline std::vector<ExecutionIrFormulaRecord> normalizeFormulaRecords(
    std::vector<ExecutionIrFormulaRecord> aRecords)
{
    irdetail::normalizeFormulaRecords(aRecords);
    return aRecords;
}

[[nodiscard]] inline std::vector<ShadowFormulaGroupRecord> normalizeFormulaGroups(
    std::vector<ShadowFormulaGroupRecord> aGroups)
{
    irdetail::normalizeFormulaGroups(aGroups);
    return aGroups;
}

[[nodiscard]] inline std::vector<ExecutionIrBuildFailure> normalizeBuildFailures(
    std::vector<ExecutionIrBuildFailure> aFailures)
{
    irdetail::normalizeBuildFailures(aFailures);
    return aFailures;
}

} // namespace ircmpdetail

[[nodiscard]] inline ExecutionIrWorkbookComparison compareExecutionIrWorkbookShadow(
    const ExecutionIrWorkbookShadow& rActual, const ExecutionIrWorkbookShadow& rExpected)
{
    ExecutionIrWorkbookComparison aComparison;
    aComparison.mbSnapshotMatch = rActual.maSnapshot == rExpected.maSnapshot;
    aComparison.mbGrammarMatch = rActual.maGrammar.meLanguage == rExpected.maGrammar.meLanguage
        && rActual.maGrammar.meAddressConvention == rExpected.maGrammar.meAddressConvention
        && rActual.maGrammar.mbEnglish == rExpected.maGrammar.mbEnglish;
    aComparison.mbFormulaRecordExactMatch = rActual.maFormulaRecords == rExpected.maFormulaRecords;
    aComparison.mbFormulaRecordNormalizedMatch
        = ircmpdetail::normalizeFormulaRecords(rActual.maFormulaRecords)
        == ircmpdetail::normalizeFormulaRecords(rExpected.maFormulaRecords);
    aComparison.mbFormulaGroupExactMatch = rActual.maFormulaGroups == rExpected.maFormulaGroups;
    aComparison.mbFormulaGroupNormalizedMatch
        = ircmpdetail::normalizeFormulaGroups(rActual.maFormulaGroups)
        == ircmpdetail::normalizeFormulaGroups(rExpected.maFormulaGroups);
    aComparison.mbBuildFailureExactMatch = rActual.maBuildFailures == rExpected.maBuildFailures;
    aComparison.mbBuildFailureNormalizedMatch
        = ircmpdetail::normalizeBuildFailures(rActual.maBuildFailures)
        == ircmpdetail::normalizeBuildFailures(rExpected.maBuildFailures);

    aComparison.mbFullMatch = aComparison.mbSnapshotMatch && aComparison.mbGrammarMatch
        && aComparison.mbFormulaRecordNormalizedMatch && aComparison.mbFormulaGroupNormalizedMatch
        && aComparison.mbBuildFailureNormalizedMatch;

    if (!aComparison.mbFullMatch)
        aComparison.meKind = ExecutionIrComparisonKind::Mismatch;
    else if (aComparison.mbFormulaRecordExactMatch && aComparison.mbFormulaGroupExactMatch
        && aComparison.mbBuildFailureExactMatch)
    {
        aComparison.meKind = ExecutionIrComparisonKind::Exact;
    }
    else
    {
        aComparison.meKind = ExecutionIrComparisonKind::NormalizedEquivalent;
    }

    return aComparison;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
