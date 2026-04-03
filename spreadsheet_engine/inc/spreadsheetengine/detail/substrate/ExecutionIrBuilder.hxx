/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <vector>

#include <spreadsheetengine/detail/substrate/DependencyGraphShadowMapping.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrLowering.hxx>

namespace spreadsheetengine::detail::substrate
{

struct ExecutionIrCompileArtifacts
{
    std::optional<token::CompiledFormula> moFormula;
    sal_Int32 mnFailureIndex = -1;
    api::String maFailureMessage;

    [[nodiscard]] explicit operator bool() const
    {
        return moFormula.has_value() && maFailureMessage.empty();
    }
};

using ExecutionIrCompileCallback = std::function<ExecutionIrCompileArtifacts(const ShadowCellRecord&)>;

namespace irdetail
{

[[nodiscard]] inline ExecutionIrFormulaSource makeExecutionIrFormulaSource(
    const ShadowCellRecord& rCell)
{
    return { rCell.moFormula ? rCell.moFormula->maFormulaSource : api::String {}, {} };
}

[[nodiscard]] inline ExecutionIrLoweringContext makeExecutionIrLoweringContext(
    const ShadowCellRecord& rCell)
{
    return { rCell.maId, rCell.moFormulaGroup, makeExecutionIrFormulaSource(rCell),
        rCell.mbInFormulaTree, rCell.mbInFormulaTrack };
}

inline void normalizeFormulaGroups(std::vector<ShadowFormulaGroupRecord>& rGroups)
{
    for (auto& rGroup : rGroups)
        graphmapping::sortAndUnique(rGroup.maMembers, graphmapping::ShadowCellIdLess {});

    std::sort(rGroups.begin(), rGroups.end(),
        [](const ShadowFormulaGroupRecord& rLeft, const ShadowFormulaGroupRecord& rRight) {
            return graphmapping::ShadowFormulaGroupIdLess {}(rLeft.maId, rRight.maId);
        });
}

inline void normalizeFormulaRecords(std::vector<ExecutionIrFormulaRecord>& rRecords)
{
    std::sort(rRecords.begin(), rRecords.end(),
        [](const ExecutionIrFormulaRecord& rLeft, const ExecutionIrFormulaRecord& rRight) {
            return graphmapping::ShadowCellIdLess {}(rLeft.maId, rRight.maId);
        });
}

inline void normalizeBuildFailures(std::vector<ExecutionIrBuildFailure>& rFailures)
{
    std::sort(rFailures.begin(), rFailures.end(),
        [](const ExecutionIrBuildFailure& rLeft, const ExecutionIrBuildFailure& rRight) {
            if (!(rLeft.maId == rRight.maId))
                return graphmapping::ShadowCellIdLess {}(rLeft.maId, rRight.maId);
            if (rLeft.maFormulaSource != rRight.maFormulaSource)
                return rLeft.maFormulaSource < rRight.maFormulaSource;
            return rLeft.maFailureMessage < rRight.maFailureMessage;
        });
}

[[nodiscard]] inline api::String defaultFailureMessage(api::StringView rFallback)
{
    return rFallback.empty() ? api::String(u"execution IR build failed") : api::String(rFallback);
}

} // namespace irdetail

[[nodiscard]] inline ExecutionIrWorkbookShadow buildExecutionIrWorkbookShadow(
    const ComputationalWorkbookShadow& rShadow, const ExecutionIrCompileCallback& rCompile)
{
    ExecutionIrWorkbookShadow aIrShadow;
    aIrShadow.maSnapshot = rShadow.maSnapshot;
    aIrShadow.maGrammar = rShadow.maGrammar;
    aIrShadow.maFormulaGroups = rShadow.maFormulaGroups;
    irdetail::normalizeFormulaGroups(aIrShadow.maFormulaGroups);

    for (const auto& rSheet : rShadow.maSheets)
    {
        for (const auto& rCell : rSheet.maCells)
        {
            if (!rCell.hasFormula())
                continue;

            const auto aCompileArtifacts = rCompile(rCell);
            if (!aCompileArtifacts)
            {
                aIrShadow.maBuildFailures.push_back(
                    { rCell.maId, rCell.moFormula->maFormulaSource,
                        irdetail::defaultFailureMessage(aCompileArtifacts.maFailureMessage) });
                continue;
            }

            const auto aLowered = lowerCompiledFormulaToExecutionIr(
                *aCompileArtifacts.moFormula, irdetail::makeExecutionIrLoweringContext(rCell));
            if (!aLowered)
            {
                aIrShadow.maBuildFailures.push_back({ rCell.maId, rCell.moFormula->maFormulaSource,
                    irdetail::defaultFailureMessage(aLowered.maFailureMessage) });
                continue;
            }

            aIrShadow.maFormulaRecords.push_back(aLowered.maFormula);
        }
    }

    irdetail::normalizeFormulaRecords(aIrShadow.maFormulaRecords);
    irdetail::normalizeBuildFailures(aIrShadow.maBuildFailures);
    return aIrShadow;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
