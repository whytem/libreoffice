/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0/. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>

#include <spreadsheetengine/api/ReferenceUpdate.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrLowering.hxx>
#include <spreadsheetengine/detail/workbook/WorkbookFacadeTypes.hxx>

namespace spreadsheetengine::detail::substrate
{

struct ExecutionIrStructuralUpdatePlan
{
    api::refupdate::UpdateMode meMode = api::refupdate::UpdateMode::InsertDelete;
    api::CellRange maWhere;
    api::ColumnIndex mnDx = 0;
    api::RowIndex mnDy = 0;
    api::SheetId mnDz = 0;
    bool mbExpandRefs = false;
    bool mbSupported = false;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrStructuralUpdatePlan& rOther) const
        = default;
    [[nodiscard]] constexpr explicit operator bool() const { return mbSupported; }
};

struct ExecutionIrReferenceUpdateSummary
{
    api::refupdate::UpdateResult meResult = api::refupdate::UpdateResult::Nothing;
    sal_Int32 mnUpdatedInstructionCount = 0;
    bool mbChanged = false;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrReferenceUpdateSummary& rOther) const
        = default;
};

namespace irrefdetail
{

inline void foldUpdateResult(
    ExecutionIrReferenceUpdateSummary& rSummary, api::refupdate::UpdateResult eResult, bool bChanged)
{
    if (!bChanged)
        return;

    rSummary.mbChanged = true;
    ++rSummary.mnUpdatedInstructionCount;

    if (eResult == api::refupdate::UpdateResult::Invalid)
        rSummary.meResult = eResult;
    else if (eResult == api::refupdate::UpdateResult::Sticky
             && rSummary.meResult != api::refupdate::UpdateResult::Invalid)
    {
        rSummary.meResult = eResult;
    }
    else if (eResult == api::refupdate::UpdateResult::Updated
             && rSummary.meResult == api::refupdate::UpdateResult::Nothing)
    {
        rSummary.meResult = eResult;
    }
}

[[nodiscard]] inline bool updateSingleReference(api::refdata::SingleRefData& rReference,
    const api::refdata::SheetLimits& rLimits, const api::CellAddress& rCurrentPosition,
    const ExecutionIrStructuralUpdatePlan& rPlan, api::refupdate::UpdateResult& reResult)
{
    const auto aBefore = rReference;
    const api::CellAddress aAbsolute
        = api::refdata::toAbsoluteAddress(rReference, rLimits, rCurrentPosition);
    api::CellRange aRange { aAbsolute, aAbsolute };
    reResult = api::refupdate::updateReference(rPlan.meMode, rPlan.maWhere, rPlan.mnDx, rPlan.mnDy,
        rPlan.mnDz, rLimits.mnMaxColumn, rLimits.mnMaxRow, rLimits.mnMaxSheet, rPlan.mbExpandRefs,
        aRange);
    api::refdata::setAddress(rReference, rLimits, aRange.maStart, rCurrentPosition);
    return rReference != aBefore;
}

[[nodiscard]] inline bool updateRangeReference(api::refdata::ComplexRefData& rReference,
    const api::refdata::SheetLimits& rLimits, const api::CellAddress& rCurrentPosition,
    const ExecutionIrStructuralUpdatePlan& rPlan, api::refupdate::UpdateResult& reResult)
{
    const auto aBefore = rReference;
    api::CellRange aRange = api::refdata::toAbsoluteRange(rReference, rLimits, rCurrentPosition);
    reResult = api::refupdate::updateReference(rPlan.meMode, rPlan.maWhere, rPlan.mnDx, rPlan.mnDy,
        rPlan.mnDz, rLimits.mnMaxColumn, rLimits.mnMaxRow, rLimits.mnMaxSheet, rPlan.mbExpandRefs,
        aRange);
    api::refdata::setRange(rReference, rLimits, aRange, rCurrentPosition);
    return rReference != aBefore;
}

} // namespace irrefdetail

[[nodiscard]] inline std::optional<ExecutionIrStructuralUpdatePlan>
makeExecutionIrStructuralUpdatePlan(
    const spreadsheetengine::detail::facade::MutationEvent& rMutation,
    const api::refdata::SheetLimits& rLimits)
{
    ExecutionIrStructuralUpdatePlan aPlan;
    aPlan.meMode = api::refupdate::UpdateMode::InsertDelete;
    switch (rMutation.meKind)
    {
        case spreadsheetengine::detail::facade::MutationKind::InsertRows:
            aPlan.maWhere = { { rMutation.mnSheet, 0, rMutation.maAddress.mnRow },
                { rMutation.mnSheet, rLimits.mnMaxColumn, rLimits.mnMaxRow } };
            aPlan.mnDy = rMutation.mnCount;
            aPlan.mbSupported = true;
            return aPlan;
        case spreadsheetengine::detail::facade::MutationKind::DeleteRows:
            aPlan.maWhere = { { rMutation.mnSheet, 0, rMutation.maAddress.mnRow },
                { rMutation.mnSheet, rLimits.mnMaxColumn, rLimits.mnMaxRow } };
            aPlan.mnDy = -rMutation.mnCount;
            aPlan.mbSupported = true;
            return aPlan;
        case spreadsheetengine::detail::facade::MutationKind::InsertColumns:
            aPlan.maWhere = { { rMutation.mnSheet, rMutation.maAddress.mnColumn, 0 },
                { rMutation.mnSheet, rLimits.mnMaxColumn, rLimits.mnMaxRow } };
            aPlan.mnDx = rMutation.mnCount;
            aPlan.mbSupported = true;
            return aPlan;
        case spreadsheetengine::detail::facade::MutationKind::DeleteColumns:
            aPlan.maWhere = { { rMutation.mnSheet, rMutation.maAddress.mnColumn, 0 },
                { rMutation.mnSheet, rLimits.mnMaxColumn, rLimits.mnMaxRow } };
            aPlan.mnDx = -rMutation.mnCount;
            aPlan.mbSupported = true;
            return aPlan;
        default:
            return std::nullopt;
    }
}

[[nodiscard]] inline ExecutionIrReferenceUpdateSummary updateExecutionIrFormulaReferences(
    ExecutionIrFormulaRecord& rFormula, const api::CellAddress& rCurrentPosition,
    const api::refdata::SheetLimits& rLimits, const ExecutionIrStructuralUpdatePlan& rPlan)
{
    ExecutionIrReferenceUpdateSummary aSummary;
    if (!rPlan)
        return aSummary;

    for (auto& rInstruction : rFormula.maInstructions)
    {
        api::refupdate::UpdateResult eResult = api::refupdate::UpdateResult::Nothing;
        bool bChanged = false;

        switch (rInstruction.meKind)
        {
            case ExecutionIrInstructionKind::SingleReference:
            case ExecutionIrInstructionKind::ColumnRowNameReference:
                if (auto* pReference = std::get_if<api::refdata::SingleRefData>(&rInstruction.maPayload))
                {
                    bChanged = irrefdetail::updateSingleReference(
                        *pReference, rLimits, rCurrentPosition, rPlan, eResult);
                }
                break;
            case ExecutionIrInstructionKind::RangeReference:
                if (auto* pReference = std::get_if<api::refdata::ComplexRefData>(&rInstruction.maPayload))
                {
                    bChanged = irrefdetail::updateRangeReference(
                        *pReference, rLimits, rCurrentPosition, rPlan, eResult);
                }
                break;
            case ExecutionIrInstructionKind::ExternalSingleReference:
                if (auto* pReference = std::get_if<ExecutionIrExternalSingleRefData>(&rInstruction.maPayload))
                {
                    bChanged = irrefdetail::updateSingleReference(
                        pReference->maReference, rLimits, rCurrentPosition, rPlan, eResult);
                }
                break;
            case ExecutionIrInstructionKind::ExternalRangeReference:
                if (auto* pReference = std::get_if<ExecutionIrExternalDoubleRefData>(&rInstruction.maPayload))
                {
                    bChanged = irrefdetail::updateRangeReference(
                        pReference->maReference, rLimits, rCurrentPosition, rPlan, eResult);
                }
                break;
            default:
                break;
        }

        irrefdetail::foldUpdateResult(aSummary, eResult, bChanged);
    }

    return aSummary;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
