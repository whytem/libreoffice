/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "helper/qahelper.hxx"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>
#include <string>

#include <rtl/string.hxx>

#include <document.hxx>
#include <globalnames.hxx>
#include <listenercontext.hxx>
#include <rangenam.hxx>
#include <scopetools.hxx>
#include <table.hxx>
#include <spreadsheetengine/compat/libreoffice/DependencyShadow.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalShadowMutation.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateObservation.hxx>
#include <spreadsheetengine/compat/libreoffice/DependencyGraphShadowMutation.hxx>
#include <spreadsheetengine/compat/libreoffice/ExecutionIrBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/ExecutionIrMutation.hxx>
#include <spreadsheetengine/compat/libreoffice/MutationTranslator.hxx>
#include <spreadsheetengine/compat/libreoffice/MutableComputationalSubstrate.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateFinalVerification.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstratePrimitiveHostExecutor.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstratePrimitiveExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRawMutation.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateLiveApply.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateAuthority.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateLifecycle.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateStructural.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcAuthority.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>
#include <spreadsheetengine/compat/libreoffice/WorkbookFacade.hxx>
#include <spreadsheetengine/detail/workbook/FacadeConsumers.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/GraphWiringDelta.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrComparison.hxx>
#include <spreadsheetengine/detail/dependency/DependencySnapshot.hxx>
#include <spreadsheetengine/detail/dependency/InvalidationPlanner.hxx>

namespace
{

class TestDependencyShadow : public ScUcalcTestBase
{
public:
    void tearDown() override
    {
        if (m_pDoc)
            m_pDoc->DiscardFormulaGroupContext();
        ScUcalcTestBase::tearDown();
    }
};

using spreadsheetengine::api::CellAddress;
using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
using spreadsheetengine::compat::libreoffice::dependencyshadow::ScopedInvalidationShadow;
using spreadsheetengine::compat::libreoffice::dependencyshadow::ShadowComparisonKind;
using ComputationalPilotResultKind
    = spreadsheetengine::compat::libreoffice::substrateauthority::PilotResultKind;
using spreadsheetengine::compat::libreoffice::substrateauthority::ScopedComputationalAuthority;
using ComputationalLifecycleResultKind
    = spreadsheetengine::compat::libreoffice::substratelifecycle::LifecycleResultKind;
using spreadsheetengine::compat::libreoffice::substratelifecycle::ScopedComputationalLifecycle;
using ComputationalMutationEntryResultKind
    = spreadsheetengine::compat::libreoffice::substratemutationentry::MutationEntryResultKind;
using spreadsheetengine::compat::libreoffice::substratemutationentry::
    ScopedComputationalMutationEntry;
using BroadcasterCanonicalizationKind
    = spreadsheetengine::detail::substrate::BroadcasterCanonicalizationKind;
using ObjectRealizationObservationKind
    = spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        ObjectRealizationObservationKind;
using PrimitiveRealizationObservationKind
    = spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        PrimitiveRealizationObservationKind;
using RollbackObservationKind
    = spreadsheetengine::compat::libreoffice::substraterollback::RollbackObservationKind;
using PrimitiveRollbackObservationKind
    = spreadsheetengine::compat::libreoffice::substraterollback::
        PrimitiveRollbackObservationKind;
using RawMutationObservationKind
    = spreadsheetengine::compat::libreoffice::substraterawmutation::RawMutationObservationKind;
using RawDocumentMutationObservationKind
    = spreadsheetengine::compat::libreoffice::substraterawmutation::RawDocumentMutationObservationKind;
using LiveApplyObservationKind
    = spreadsheetengine::compat::libreoffice::substrateliveapply::LiveApplyObservationKind;
using FinalVerificationObservationKind
    = spreadsheetengine::compat::libreoffice::substratefinalverification::
        FinalVerificationObservationKind;
using PrimitiveExecutionObservationKind
    = spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::
        PrimitiveExecutionObservationKind;
using PrimitiveHostExecutorObservationKind
    = spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::
        PrimitiveHostExecutorObservationKind;
using ComputationalStructuralResultKind
    = spreadsheetengine::compat::libreoffice::substratestructural::StructuralResultKind;
using spreadsheetengine::compat::libreoffice::substratestructural::ScopedComputationalStructural;
using spreadsheetengine::compat::libreoffice::recalcauthority::PilotResultKind;
using spreadsheetengine::compat::libreoffice::recalcauthority::ScopedRecalcAuthority;
using spreadsheetengine::compat::libreoffice::recalcshadow::ScopedRecalcShadow;
using spreadsheetengine::compat::libreoffice::recalcqueue::FormulaStateSnapshot;
using RecalcShadowComparisonKind
    = spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparisonKind;
using spreadsheetengine::compat::libreoffice::substrateobs::BroadcasterStateSnapshot;
using spreadsheetengine::compat::libreoffice::substrateobs::CellBroadcasterSnapshot;
using spreadsheetengine::compat::libreoffice::substrateobs::ListenerKind;
using spreadsheetengine::compat::libreoffice::substrateobs::LiveComputationalStateSnapshot;
using spreadsheetengine::detail::dependency::DirtyFormulaCell;
using GraphComparisonKind = spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind;
using ExecutionIrComparisonKind
    = spreadsheetengine::detail::substrate::ExecutionIrComparisonKind;

class ScopedEnvironmentOverride
{
    std::string maName;
    std::optional<std::string> moOriginalValue;

public:
    ScopedEnvironmentOverride(const char* pName, const char* pValue)
        : maName(pName)
    {
        if (const char* pOriginal = std::getenv(pName))
            moOriginalValue = pOriginal;

        if (pValue)
            setenv(maName.c_str(), pValue, 1);
        else
            unsetenv(maName.c_str());
    }

    ~ScopedEnvironmentOverride()
    {
        if (moOriginalValue)
            setenv(maName.c_str(), moOriginalValue->c_str(), 1);
        else
            unsetenv(maName.c_str());
    }
};

struct AddressLess
{
    [[nodiscard]] bool operator()(const CellAddress& rLeft, const CellAddress& rRight) const
    {
        if (rLeft.mnSheet != rRight.mnSheet)
            return rLeft.mnSheet < rRight.mnSheet;
        if (rLeft.mnColumn != rRight.mnColumn)
            return rLeft.mnColumn < rRight.mnColumn;
        return rLeft.mnRow < rRight.mnRow;
    }
};

[[nodiscard]] std::set<CellAddress, AddressLess> collectDirtyFormulaAddresses(
    const CalcWorkbookFacade& rFacade)
{
    std::set<CellAddress, AddressLess> aResult;
    rFacade.visitAllFormulaCells([&aResult](const spreadsheetengine::detail::facade::FormulaCellDescriptor& rDesc) {
        if (rDesc.mbDirty || rDesc.mbNeedsRecalc)
            aResult.insert(rDesc.maId.maAddress);
        return true;
    });
    return aResult;
}

[[nodiscard]] std::set<CellAddress, AddressLess> collectDirtyFormulaAddresses(
    const std::vector<DirtyFormulaCell>& rEntries)
{
    std::set<CellAddress, AddressLess> aResult;
    for (const auto& rEntry : rEntries)
        aResult.insert(rEntry.maAddress);
    return aResult;
}

[[nodiscard]] bool isSubsetOf(const std::set<CellAddress, AddressLess>& rSubset,
    const std::set<CellAddress, AddressLess>& rSuperset)
{
    return std::includes(rSuperset.begin(), rSuperset.end(), rSubset.begin(), rSubset.end(),
        AddressLess {});
}

void assertConservativeSuperset(const std::set<CellAddress, AddressLess>& rActual,
    const std::set<CellAddress, AddressLess>& rPredicted)
{
    CPPUNIT_ASSERT(isSubsetOf(rActual, rPredicted));
}

void assertNotUnderInvalidation(
    const std::optional<spreadsheetengine::compat::libreoffice::dependencyshadow::ShadowComparison>&
        oComparison)
{
    CPPUNIT_ASSERT(oComparison.has_value());
    CPPUNIT_ASSERT(oComparison->meKind != ShadowComparisonKind::UnderInvalidation);
}

void assertExactRecalcShadow(
    const std::optional<spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison>&
        oComparison)
{
    CPPUNIT_ASSERT(oComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(RecalcShadowComparisonKind::Exact, oComparison->meKind);
}

void forceFormulaTreeOrder(ScDocument& rDoc, std::initializer_list<ScAddress> aOrder)
{
    for (const ScAddress& rAddress : aOrder)
    {
        if (ScFormulaCell* pCell = rDoc.GetFormulaCell(rAddress))
        {
            if (rDoc.IsInFormulaTrack(pCell))
                rDoc.RemoveFromFormulaTrack(pCell);
            if (rDoc.IsInFormulaTree(pCell))
                rDoc.RemoveFromFormulaTree(pCell);
        }
    }

    for (const ScAddress& rAddress : aOrder)
    {
        if (ScFormulaCell* pCell = rDoc.GetFormulaCell(rAddress))
        {
            pCell->SetDirtyVar();
            rDoc.PutInFormulaTree(pCell);
        }
    }
}

void reverseCurrentFormulaTreeOrder(ScDocument& rDoc)
{
    auto aCurrent
        = spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectFormulaTreeAddresses(
            rDoc);
    std::reverse(aCurrent.begin(), aCurrent.end());
    for (const auto& rAddress : aCurrent)
    {
        if (ScFormulaCell* pCell = rDoc.GetFormulaCell(
                ScAddress(rAddress.mnColumn, rAddress.mnRow, rAddress.mnSheet)))
        {
            if (rDoc.IsInFormulaTrack(pCell))
                rDoc.RemoveFromFormulaTrack(pCell);
            if (rDoc.IsInFormulaTree(pCell))
                rDoc.RemoveFromFormulaTree(pCell);
        }
    }

    for (const auto& rAddress : aCurrent)
    {
        if (ScFormulaCell* pCell = rDoc.GetFormulaCell(
                ScAddress(rAddress.mnColumn, rAddress.mnRow, rAddress.mnSheet)))
        {
            pCell->SetDirtyVar();
            rDoc.PutInFormulaTree(pCell);
        }
    }
}

void assertPilotAppliedWithExactQueue(
    const std::optional<spreadsheetengine::compat::libreoffice::recalcauthority::PilotResult>&
        oResult,
    RecalcShadowComparisonKind eExpectedBefore,
    const ScDocument& rDoc)
{
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(PilotResultKind::Applied, oResult->meKind);
    CPPUNIT_ASSERT(oResult->moComparisonBefore.has_value());
    CPPUNIT_ASSERT_EQUAL(eExpectedBefore, oResult->moComparisonBefore->meKind);
    CPPUNIT_ASSERT(oResult->moComparisonAfter.has_value());
    CPPUNIT_ASSERT_EQUAL(RecalcShadowComparisonKind::Exact, oResult->moComparisonAfter->meKind);

    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            oResult->maPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(rDoc));
}

void assertPilotAppliedAndExactAfter(
    const std::optional<spreadsheetengine::compat::libreoffice::recalcauthority::PilotResult>&
        oResult,
    const ScDocument& rDoc)
{
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(PilotResultKind::Applied, oResult->meKind);
    CPPUNIT_ASSERT(oResult->moComparisonBefore.has_value());
    CPPUNIT_ASSERT(oResult->moComparisonBefore->meKind
                   != RecalcShadowComparisonKind::UnderScheduling);
    CPPUNIT_ASSERT(oResult->moComparisonAfter.has_value());
    CPPUNIT_ASSERT_EQUAL(RecalcShadowComparisonKind::Exact, oResult->moComparisonAfter->meKind);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            oResult->maPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(rDoc));
}

void assertComputationalPilotApplied(
    const std::optional<
        spreadsheetengine::compat::libreoffice::substrateauthority::PilotResult>& oResult,
    const ScDocument& rDoc)
{
    CPPUNIT_ASSERT(oResult.has_value());
    const std::string aResultMessage
        = "unexpected computational pilot result kind="
          + std::to_string(static_cast<int>(oResult->meKind))
          + " queue="
          + (oResult->moQueueComparison
                 ? std::to_string(static_cast<int>(oResult->moQueueComparison->meKind))
                 : std::string("none"))
          + " graph="
          + (oResult->moGraphComparison
                 ? std::to_string(static_cast<int>(oResult->moGraphComparison->meKind))
                       + ":" + (oResult->moGraphComparison->mbFullMatch ? "1" : "0")
                 : std::string("none"))
          + " ir="
          + (oResult->moIrComparison
                 ? std::to_string(static_cast<int>(oResult->moIrComparison->meKind))
                       + ":" + (oResult->moIrComparison->mbFullMatch ? "1" : "0")
                 : std::string("none"));
    CPPUNIT_ASSERT_MESSAGE(
        aResultMessage,
        oResult->meKind == ComputationalPilotResultKind::Applied
            || oResult->meKind == ComputationalPilotResultKind::AppliedNormalizedEquivalent);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(RecalcShadowComparisonKind::Exact, oResult->moQueueComparison->meKind);
    CPPUNIT_ASSERT(oResult->moGraphComparison.has_value());
    CPPUNIT_ASSERT_MESSAGE(
        "graph comparison kind="
            + std::to_string(static_cast<int>(oResult->moGraphComparison->meKind)),
        oResult->moGraphComparison->mbFullMatch);
    CPPUNIT_ASSERT(oResult->moIrComparison.has_value());
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            oResult->maTransition.maRecalcPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(rDoc));
}

void assertComputationalLifecycleApplied(
    const std::optional<
        spreadsheetengine::compat::libreoffice::substratelifecycle::LifecycleResult>& oResult,
    const ScDocument& rDoc)
{
    CPPUNIT_ASSERT(oResult.has_value());
    const std::string aResultMessage
        = "unexpected computational lifecycle result kind="
          + std::to_string(static_cast<int>(oResult->meKind))
          + " queue="
          + (oResult->moQueueComparison
                 ? std::to_string(static_cast<int>(oResult->moQueueComparison->meKind))
                 : std::string("none"))
          + " computational="
          + (oResult->moComputationalComparison
                 ? std::string(oResult->moComputationalComparison->mbFullMatch ? "1" : "0")
                 : std::string("none"))
          + " graph="
          + (oResult->moGraphComparison
                 ? std::to_string(static_cast<int>(oResult->moGraphComparison->meKind))
                       + ":" + (oResult->moGraphComparison->mbFullMatch ? "1" : "0")
                 : std::string("none"))
          + " ir="
          + (oResult->moIrComparison
                 ? std::to_string(static_cast<int>(oResult->moIrComparison->meKind))
                       + ":" + (oResult->moIrComparison->mbFullMatch ? "1" : "0")
                 : std::string("none"));
    CPPUNIT_ASSERT_MESSAGE(
        aResultMessage,
        oResult->meKind == ComputationalLifecycleResultKind::Applied
        || oResult->meKind == ComputationalLifecycleResultKind::AppliedNormalizedEquivalent);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(RecalcShadowComparisonKind::Exact, oResult->moQueueComparison->meKind);
    CPPUNIT_ASSERT(oResult->moComputationalComparison.has_value());
    CPPUNIT_ASSERT(oResult->moComputationalComparison->mbFullMatch);
    CPPUNIT_ASSERT(oResult->moGraphComparison.has_value());
    CPPUNIT_ASSERT(oResult->moGraphComparison->mbFullMatch);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            oResult->maTransition.maRecalcPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(rDoc));
}

void assertComputationalStructuralApplied(
    const std::optional<
        spreadsheetengine::compat::libreoffice::substratestructural::StructuralResult>& oResult,
    const ScDocument& rDoc)
{
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_MESSAGE(
        "unexpected computational structural result kind="
            + std::to_string(static_cast<int>(oResult->meKind))
            + " verdict="
            + std::to_string(static_cast<int>(oResult->maTransition.meVerdict))
            + " reason="
            + OUStringToOString(
                  spreadsheetengine::compat::libreoffice::toLibreOfficeString(
                      oResult->maTransition.maReason),
                  RTL_TEXTENCODING_UTF8)
                  .getStr(),
        oResult->meKind == ComputationalStructuralResultKind::Applied
            || oResult->meKind == ComputationalStructuralResultKind::AppliedNormalizedEquivalent);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(RecalcShadowComparisonKind::Exact, oResult->moQueueComparison->meKind);
    CPPUNIT_ASSERT(oResult->moComputationalComparison.has_value());
    CPPUNIT_ASSERT(oResult->moComputationalComparison->mbFullMatch);
    CPPUNIT_ASSERT(oResult->moGraphComparison.has_value());
    CPPUNIT_ASSERT(oResult->moGraphComparison->mbFullMatch);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            oResult->maTransition.maRecalcPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(rDoc));
}

void assertComputationalStructuralAppliedExactly(
    const std::optional<
        spreadsheetengine::compat::libreoffice::substratestructural::StructuralResult>& oResult,
    const ScDocument& rDoc)
{
    assertComputationalStructuralApplied(oResult, rDoc);
    CPPUNIT_ASSERT_EQUAL(ComputationalStructuralResultKind::Applied, oResult->meKind);
}

[[nodiscard]] std::string describeObjectRealizationObservation(
    const spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        ObjectRealizationObservation& rObservation);
[[nodiscard]] std::string describePrimitiveRealizationObservation(
    const spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        PrimitiveRealizationObservation& rObservation);
[[nodiscard]] std::string describeRollbackObservation(
    const spreadsheetengine::compat::libreoffice::substraterollback::RollbackObservation& rObservation);
[[nodiscard]] std::string describePrimitiveRollbackObservation(
    const spreadsheetengine::compat::libreoffice::substraterollback::PrimitiveRollbackObservation&
        rObservation);
[[nodiscard]] std::string describeRawMutationObservation(
    const spreadsheetengine::compat::libreoffice::substraterawmutation::RawMutationObservation& rObservation);
[[nodiscard]] std::string describeRawDocumentMutationObservation(
    const spreadsheetengine::compat::libreoffice::substraterawmutation::
        RawDocumentMutationObservation& rObservation);
[[nodiscard]] std::string describeLiveApplyObservation(
    const spreadsheetengine::compat::libreoffice::substrateliveapply::LiveApplyObservation& rObservation);
[[nodiscard]] std::string describeFinalVerificationObservation(
    const spreadsheetengine::compat::libreoffice::substratefinalverification::
        FinalVerificationObservation& rObservation);
[[nodiscard]] std::string describePrimitiveExecutionObservation(
    const spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::
        PrimitiveExecutionObservation& rObservation);
[[nodiscard]] std::string describePrimitiveHostExecutorObservation(
    const spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::
        PrimitiveHostExecutorObservation& rObservation);

void assertComputationalMutationEntryApplied(
    const std::optional<
        spreadsheetengine::compat::libreoffice::substratemutationentry::MutationEntryResult>&
        oResult,
    const ScDocument& rDoc)
{
    CPPUNIT_ASSERT(oResult.has_value());
    const std::string aResultMessage
        = "unexpected computational mutation-entry result kind="
          + std::to_string(static_cast<int>(oResult->meKind))
          + " path="
          + std::to_string(static_cast<int>(oResult->maTransition.mePath))
          + " queue="
          + (oResult->moQueueComparison
                 ? std::to_string(static_cast<int>(oResult->moQueueComparison->meKind))
                 : std::string("none"))
          + " computational="
          + (oResult->moComputationalComparison
                 ? std::string(oResult->moComputationalComparison->mbFullMatch ? "1" : "0")
                       + ":" + (oResult->moComputationalComparison->mbCellPopulationMatch ? "1" : "0")
                       + ":" + (oResult->moComputationalComparison->mbFormulaTreeMatch ? "1" : "0")
                       + ":" + (oResult->moComputationalComparison->mbFormulaTrackMatch ? "1" : "0")
                       + ":" + (oResult->moComputationalComparison->mbBroadcasterMatch ? "1" : "0")
                       + ":" + (oResult->moComputationalComparison->mbGroupMatch ? "1" : "0")
                       + ":" + (oResult->moComputationalComparison->mbNamedRangeMatch ? "1" : "0")
                 : std::string("none"))
          + " graph="
          + (oResult->moGraphComparison
                 ? std::to_string(static_cast<int>(oResult->moGraphComparison->meKind))
                       + ":" + (oResult->moGraphComparison->mbFullMatch ? "1" : "0")
                 : std::string("none"));
    const std::string aBroadcasterMessage
        = oResult->moBroadcasterCanonicalization
              ? std::string(" broadcaster=")
                    + spreadsheetengine::detail::substrate::detail::toString(
                        oResult->moBroadcasterCanonicalization->meKind)
                    + ":" + (oResult->moBroadcasterCanonicalization->mbExactMatch ? "1" : "0")
                    + ":" + std::to_string(
                        oResult->moBroadcasterCanonicalization->mnLiveDuplicateBroadcasterCount)
                    + ":" + std::to_string(
                        oResult->moBroadcasterCanonicalization->mnLiveDuplicateListenerCount)
                    + ":" + std::to_string(
                        oResult->moBroadcasterCanonicalization->mnLiveEmptyBroadcasterCount)
                    + ":" + std::to_string(
                        oResult->moBroadcasterCanonicalization->mnLiveHostUnknownListenerCount)
              : std::string(" broadcaster=none");
    const std::string aObjectRealizationMessage
        = oResult->moObjectRealizationObservation
              ? std::string(" object_realization=")
                    + describeObjectRealizationObservation(*oResult->moObjectRealizationObservation)
              : std::string(" object_realization=none");
    const std::string aPrimitiveRealizationMessage
        = oResult->moPrimitiveRealizationObservation
              ? std::string(" primitive_realization=")
                    + describePrimitiveRealizationObservation(
                        *oResult->moPrimitiveRealizationObservation)
              : std::string(" primitive_realization=none");
    const std::string aPrimitiveRollbackMessage
        = oResult->moPrimitiveRollbackObservation
              ? std::string(" primitive_rollback=")
                    + describePrimitiveRollbackObservation(
                        *oResult->moPrimitiveRollbackObservation)
              : std::string(" primitive_rollback=none");
    const std::string aRawMutationMessage
        = oResult->moRawMutationObservation
              ? std::string(" raw_mutation=")
                    + describeRawMutationObservation(*oResult->moRawMutationObservation)
              : std::string(" raw_mutation=none");
    const std::string aRawDocumentMutationMessage
        = oResult->moRawDocumentMutationObservation
              ? std::string(" raw_document_mutation=")
                    + describeRawDocumentMutationObservation(
                        *oResult->moRawDocumentMutationObservation)
              : std::string(" raw_document_mutation=none");
    const std::string aLiveApplyMessage
        = oResult->moLiveApplyObservation
              ? std::string(" live_apply=")
                    + describeLiveApplyObservation(*oResult->moLiveApplyObservation)
              : std::string(" live_apply=none");
    const std::string aFinalVerificationMessage
        = oResult->moFinalVerificationObservation
              ? std::string(" final_verification=")
                    + describeFinalVerificationObservation(
                        *oResult->moFinalVerificationObservation)
              : std::string(" final_verification=none");
    const std::string aPrimitiveExecutionMessage
        = oResult->moPrimitiveExecutionObservation
              ? std::string(" primitive_execution=")
                    + describePrimitiveExecutionObservation(
                        *oResult->moPrimitiveExecutionObservation)
              : std::string(" primitive_execution=none");
    const std::string aPrimitiveHostExecutorMessage
        = oResult->moPrimitiveHostExecutorObservation
              ? std::string(" primitive_host_executor=")
                    + describePrimitiveHostExecutorObservation(
                        *oResult->moPrimitiveHostExecutorObservation)
              : std::string(" primitive_host_executor=none");
    const std::string aFullMessage
        = aResultMessage + aBroadcasterMessage + aObjectRealizationMessage
          + aPrimitiveRealizationMessage + aPrimitiveRollbackMessage + aRawMutationMessage
          + aRawDocumentMutationMessage + aLiveApplyMessage + aFinalVerificationMessage
          + aPrimitiveExecutionMessage + aPrimitiveHostExecutorMessage;
    CPPUNIT_ASSERT_MESSAGE(
        aFullMessage,
        oResult->meKind == ComputationalMutationEntryResultKind::Applied
            || oResult->meKind
                   == ComputationalMutationEntryResultKind::AppliedNormalizedEquivalent);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(RecalcShadowComparisonKind::Exact, oResult->moQueueComparison->meKind);
    CPPUNIT_ASSERT(oResult->moComputationalComparison.has_value());
    CPPUNIT_ASSERT(oResult->moGraphComparison.has_value());
    CPPUNIT_ASSERT_MESSAGE(aFullMessage, oResult->moGraphComparison->mbFullMatch);
    CPPUNIT_ASSERT(oResult->moObjectRealizationObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(aFullMessage, ObjectRealizationObservationKind::Exact,
        oResult->moObjectRealizationObservation->meKind);
    CPPUNIT_ASSERT(oResult->moPrimitiveRealizationRecord.has_value());
    CPPUNIT_ASSERT(oResult->moPrimitiveRealizationObservation.has_value());
    CPPUNIT_ASSERT_MESSAGE(aFullMessage,
        oResult->moPrimitiveRealizationRecord->maObjectRealization.getCellCount()
        == oResult->moPrimitiveRealizationRecord->maObjectRealization.maCellStorage.getCellCount());
    CPPUNIT_ASSERT_MESSAGE(aFullMessage,
        oResult->moPrimitiveRealizationRecord->mbUsesPrimitiveRealization);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(aFullMessage, PrimitiveRealizationObservationKind::Exact,
        oResult->moPrimitiveRealizationObservation->meKind);
    CPPUNIT_ASSERT(!oResult->moPrimitiveRollbackRecord.has_value());
    CPPUNIT_ASSERT(!oResult->moPrimitiveRollbackObservation.has_value());
    CPPUNIT_ASSERT(oResult->moRawMutationObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(aFullMessage, RawMutationObservationKind::Exact,
        oResult->moRawMutationObservation->meKind);
    CPPUNIT_ASSERT(oResult->moRawMutationRecord.has_value());
    CPPUNIT_ASSERT(oResult->moRawDocumentMutationRecord.has_value());
    CPPUNIT_ASSERT_MESSAGE(aFullMessage,
        oResult->moRawDocumentMutationRecord->maRawMutation == *oResult->moRawMutationRecord);
    CPPUNIT_ASSERT_MESSAGE(aFullMessage,
        oResult->moRawDocumentMutationRecord->mbUsesPrimitiveDocumentMutation);
    CPPUNIT_ASSERT(oResult->moRawDocumentMutationObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(aFullMessage, RawDocumentMutationObservationKind::Exact,
        oResult->moRawDocumentMutationObservation->meKind);
    CPPUNIT_ASSERT(oResult->moPrimitiveExecutionPlan.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(aFullMessage, static_cast<sal_uInt8>(3),
        oResult->moPrimitiveExecutionPlan->mnStageCount);
    CPPUNIT_ASSERT_MESSAGE(
        aFullMessage,
        spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::hasStage(
            *oResult->moPrimitiveExecutionPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::
                PrimitiveExecutionStageKind::RawDocumentMutation));
    CPPUNIT_ASSERT_MESSAGE(
        aFullMessage,
        spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::hasStage(
            *oResult->moPrimitiveExecutionPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::
                PrimitiveExecutionStageKind::PrimitiveRealization));
    CPPUNIT_ASSERT_MESSAGE(
        aFullMessage,
        spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::hasStage(
            *oResult->moPrimitiveExecutionPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::
                PrimitiveExecutionStageKind::FinalVerification));
    CPPUNIT_ASSERT_MESSAGE(
        aFullMessage,
        !spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::hasStage(
            *oResult->moPrimitiveExecutionPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::
                PrimitiveExecutionStageKind::PrimitiveRollback));
    CPPUNIT_ASSERT_MESSAGE(aFullMessage,
        oResult->moPrimitiveExecutionPlan->mbUsesPrimitiveExecution);
    CPPUNIT_ASSERT(oResult->moLiveApplyPlan.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(aFullMessage, static_cast<sal_uInt8>(3),
        oResult->moLiveApplyPlan->mnStageCount);
    CPPUNIT_ASSERT_MESSAGE(
        aFullMessage,
        spreadsheetengine::compat::libreoffice::substrateliveapply::hasStage(
            *oResult->moLiveApplyPlan,
            spreadsheetengine::compat::libreoffice::substrateliveapply::LiveApplyStageKind::
                RawMutation));
    CPPUNIT_ASSERT_MESSAGE(
        aFullMessage,
        spreadsheetengine::compat::libreoffice::substrateliveapply::hasStage(
            *oResult->moLiveApplyPlan,
            spreadsheetengine::compat::libreoffice::substrateliveapply::LiveApplyStageKind::
                Realization));
    CPPUNIT_ASSERT_MESSAGE(
        aFullMessage,
        spreadsheetengine::compat::libreoffice::substrateliveapply::hasStage(
            *oResult->moLiveApplyPlan,
            spreadsheetengine::compat::libreoffice::substrateliveapply::LiveApplyStageKind::
                Verification));
    CPPUNIT_ASSERT_MESSAGE(
        aFullMessage,
        !spreadsheetengine::compat::libreoffice::substrateliveapply::hasStage(
            *oResult->moLiveApplyPlan,
            spreadsheetengine::compat::libreoffice::substrateliveapply::LiveApplyStageKind::
                Rollback));
    CPPUNIT_ASSERT(oResult->moLiveApplyObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(aFullMessage, LiveApplyObservationKind::Exact,
        oResult->moLiveApplyObservation->meKind);
    CPPUNIT_ASSERT(oResult->moFinalVerificationRecord.has_value());
    CPPUNIT_ASSERT_MESSAGE(aFullMessage,
        oResult->moFinalVerificationRecord->maLiveApplyPlan.mnStageCount
        == oResult->moLiveApplyPlan->mnStageCount);
    CPPUNIT_ASSERT_MESSAGE(aFullMessage,
        oResult->moFinalVerificationRecord->maLiveApplyPlan.mbRolledBack
        == oResult->moLiveApplyPlan->mbRolledBack);
    CPPUNIT_ASSERT_MESSAGE(aFullMessage,
        oResult->moFinalVerificationRecord->mbUsesFinalVerification);
    CPPUNIT_ASSERT(oResult->moFinalVerificationObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(aFullMessage, FinalVerificationObservationKind::Exact,
        oResult->moFinalVerificationObservation->meKind);
    CPPUNIT_ASSERT(oResult->moPrimitiveExecutionObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(aFullMessage, PrimitiveExecutionObservationKind::Exact,
        oResult->moPrimitiveExecutionObservation->meKind);
    CPPUNIT_ASSERT(oResult->moPrimitiveHostExecutorPlan.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(aFullMessage, static_cast<sal_uInt8>(3),
        oResult->moPrimitiveHostExecutorPlan->mnStageCount);
    CPPUNIT_ASSERT_MESSAGE(
        aFullMessage,
        spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::hasStage(
            *oResult->moPrimitiveHostExecutorPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::
                PrimitiveHostExecutorStageKind::RawDocumentMutationCall));
    CPPUNIT_ASSERT_MESSAGE(
        aFullMessage,
        spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::hasStage(
            *oResult->moPrimitiveHostExecutorPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::
                PrimitiveHostExecutorStageKind::PrimitiveRealizationCall));
    CPPUNIT_ASSERT_MESSAGE(
        aFullMessage,
        spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::hasStage(
            *oResult->moPrimitiveHostExecutorPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::
                PrimitiveHostExecutorStageKind::FinalVerificationCall));
    CPPUNIT_ASSERT_MESSAGE(
        aFullMessage,
        !spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::hasStage(
            *oResult->moPrimitiveHostExecutorPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::
                PrimitiveHostExecutorStageKind::PrimitiveRollbackCall));
    CPPUNIT_ASSERT_MESSAGE(aFullMessage,
        oResult->moPrimitiveHostExecutorPlan->mbUsesPrimitiveHostExecutor);
    CPPUNIT_ASSERT(oResult->moPrimitiveHostExecutorObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(aFullMessage, PrimitiveHostExecutorObservationKind::Exact,
        oResult->moPrimitiveHostExecutorObservation->meKind);

    const auto* pPlan
        = spreadsheetengine::detail::substrate::findMutationEntryRecalcPlan(oResult->maTransition);
    CPPUNIT_ASSERT(pPlan);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            *pPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(rDoc));
}

[[nodiscard]] const CellBroadcasterSnapshot* findCellBroadcaster(
    const BroadcasterStateSnapshot& rSnapshot, const ScAddress& rAddress)
{
    const auto aTarget = spreadsheetengine::compat::libreoffice::toApiCellAddress(rAddress);
    auto it = std::find_if(rSnapshot.maCellBroadcasters.begin(), rSnapshot.maCellBroadcasters.end(),
        [&aTarget](const CellBroadcasterSnapshot& rEntry) {
            return rEntry.maBroadcaster == aTarget;
        });
    return it == rSnapshot.maCellBroadcasters.end() ? nullptr : &*it;
}

[[nodiscard]] bool hasCellFormulaListener(const BroadcasterStateSnapshot& rSnapshot,
    const ScAddress& rBroadcaster, const ScAddress& rFormula)
{
    const CellBroadcasterSnapshot* pEntry = findCellBroadcaster(rSnapshot, rBroadcaster);
    if (!pEntry)
        return false;

    const auto aFormula = spreadsheetengine::compat::libreoffice::toApiCellAddress(rFormula);
    return std::any_of(pEntry->maListeners.begin(), pEntry->maListeners.end(),
        [&aFormula](const auto& rListener) {
            return rListener.meKind == ListenerKind::FormulaCell && rListener.maAnchor == aFormula;
        });
}

void assertNoCellBroadcaster(const BroadcasterStateSnapshot& rSnapshot, const ScAddress& rAddress)
{
    CPPUNIT_ASSERT(!findCellBroadcaster(rSnapshot, rAddress));
}

void assertHasEmptyCellBroadcaster(const BroadcasterStateSnapshot& rSnapshot, const ScAddress& rAddress)
{
    const CellBroadcasterSnapshot* pEntry = findCellBroadcaster(rSnapshot, rAddress);
    CPPUNIT_ASSERT(pEntry);
    CPPUNIT_ASSERT(pEntry->maListeners.empty());
}

void assertComparableGraph(
    const spreadsheetengine::detail::substrate::DependencyGraphShadowComparison& rComparison)
{
    CPPUNIT_ASSERT(rComparison.mbFullMatch);
    CPPUNIT_ASSERT(rComparison.meKind != GraphComparisonKind::Mismatch);
}

void assertComparableExecutionIr(
    const spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison& rComparison)
{
    CPPUNIT_ASSERT(rComparison.mbFullMatch);
    CPPUNIT_ASSERT(rComparison.meKind != ExecutionIrComparisonKind::Mismatch);
}

[[nodiscard]] std::string describeBroadcasterCanonicalization(
    const spreadsheetengine::detail::substrate::BroadcasterCanonicalizationComparison& rComparison)
{
    return std::string(
               spreadsheetengine::detail::substrate::detail::toString(rComparison.meKind))
           + " exact=" + (rComparison.mbExactMatch ? "1" : "0")
           + " order=" + (rComparison.mbOrderingEquivalent ? "1" : "0")
           + " dedupe=" + (rComparison.mbDeduplicatedEquivalent ? "1" : "0")
           + " drop_empty=" + (rComparison.mbDropEmptyEquivalent ? "1" : "0")
           + " ignore_kind=" + (rComparison.mbIgnoreListenerKindEquivalent ? "1" : "0")
           + " diagnostic=" + (rComparison.mbDiagnosticCanonicalEquivalent ? "1" : "0")
           + " dup_broadcasters=" + std::to_string(rComparison.mnLiveDuplicateBroadcasterCount)
           + " dup_listeners=" + std::to_string(rComparison.mnLiveDuplicateListenerCount)
           + " empty=" + std::to_string(rComparison.mnLiveEmptyBroadcasterCount)
           + " host_unknown=" + std::to_string(rComparison.mnLiveHostUnknownListenerCount)
           + " expected_cells=" + std::to_string(rComparison.mnExpectedCellBroadcasters)
           + " live_cells=" + std::to_string(rComparison.mnLiveCellBroadcasters)
           + " expected_areas=" + std::to_string(rComparison.mnExpectedAreaBroadcasters)
           + " live_areas=" + std::to_string(rComparison.mnLiveAreaBroadcasters);
}

[[nodiscard]] std::string describeObjectRealizationObservation(
    const spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        ObjectRealizationObservation& rObservation)
{
    return std::string(spreadsheetengine::compat::libreoffice::substrateobjectrealization::toString(
               rObservation.meKind))
           + " reason="
           + OUStringToOString(rObservation.maReason, RTL_TEXTENCODING_UTF8).getStr()
           + " lifetime=" + (rObservation.mbFormulaCellLifetimeApplied ? "1" : "0")
           + " cell_storage=" + (rObservation.mbCellStorageApplied ? "1" : "0")
           + " wiring=" + (rObservation.mbWiringApplied ? "1" : "0")
           + " queue=" + (rObservation.mbQueueExact ? "1" : "0")
           + " computational=" + (rObservation.mbComputationalFullMatch ? "1" : "0")
           + " graph=" + (rObservation.mbGraphFullMatch ? "1" : "0")
           + " broadcaster=" + (rObservation.mbBroadcasterExact ? "1" : "0")
           + " expected_broadcasters=" + std::to_string(rObservation.mnExpectedBroadcasters)
           + " live_broadcasters=" + std::to_string(rObservation.mnLiveBroadcasters);
}

[[nodiscard]] std::string describePrimitiveRealizationObservation(
    const spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        PrimitiveRealizationObservation& rObservation)
{
    return std::string(spreadsheetengine::compat::libreoffice::substrateobjectrealization::toString(
               rObservation.meKind))
           + " reason="
           + OUStringToOString(rObservation.maReason, RTL_TEXTENCODING_UTF8).getStr()
           + " primitive_applied=" + (rObservation.mbPrimitiveRealizationApplied ? "1" : "0")
           + " realization_exact=" + (rObservation.mbObjectRealizationExact ? "1" : "0")
           + " queue=" + (rObservation.mbQueueExact ? "1" : "0")
           + " computational=" + (rObservation.mbComputationalFullMatch ? "1" : "0")
           + " graph=" + (rObservation.mbGraphFullMatch ? "1" : "0")
           + " broadcaster=" + (rObservation.mbBroadcasterExact ? "1" : "0");
}

[[nodiscard]] std::string describeRollbackObservation(
    const spreadsheetengine::compat::libreoffice::substraterollback::RollbackObservation& rObservation)
{
    return std::string(spreadsheetengine::compat::libreoffice::substraterollback::toString(
               rObservation.meKind))
           + " reason="
           + OUStringToOString(rObservation.maReason, RTL_TEXTENCODING_UTF8).getStr()
           + " realization=" + (rObservation.mbObjectRealizationApplied ? "1" : "0")
           + " queue=" + (rObservation.mbQueueExact ? "1" : "0")
           + " computational=" + (rObservation.mbComputationalFullMatch ? "1" : "0")
           + " graph=" + (rObservation.mbGraphFullMatch ? "1" : "0")
           + " broadcaster=" + (rObservation.mbBroadcasterExact ? "1" : "0")
           + " expected_broadcasters=" + std::to_string(rObservation.mnExpectedBroadcasters)
           + " live_broadcasters=" + std::to_string(rObservation.mnLiveBroadcasters);
}

[[nodiscard]] std::string describePrimitiveRollbackObservation(
    const spreadsheetengine::compat::libreoffice::substraterollback::PrimitiveRollbackObservation&
        rObservation)
{
    return std::string(spreadsheetengine::compat::libreoffice::substraterollback::toString(
               rObservation.meKind))
           + " reason="
           + OUStringToOString(rObservation.maReason, RTL_TEXTENCODING_UTF8).getStr()
           + " primitive_applied=" + (rObservation.mbPrimitiveRollbackApplied ? "1" : "0")
           + " rollback_exact=" + (rObservation.mbRollbackExact ? "1" : "0")
           + " queue=" + (rObservation.mbQueueExact ? "1" : "0")
           + " computational=" + (rObservation.mbComputationalFullMatch ? "1" : "0")
           + " graph=" + (rObservation.mbGraphFullMatch ? "1" : "0")
           + " broadcaster=" + (rObservation.mbBroadcasterExact ? "1" : "0");
}

[[nodiscard]] std::string describeRawMutationObservation(
    const spreadsheetengine::compat::libreoffice::substraterawmutation::RawMutationObservation& rObservation)
{
    return std::string(spreadsheetengine::compat::libreoffice::substraterawmutation::toString(
               rObservation.meKind))
           + " reason="
           + OUStringToOString(rObservation.maReason, RTL_TEXTENCODING_UTF8).getStr()
           + " applied=" + (rObservation.mbMutationApplied ? "1" : "0")
           + " rolled_back=" + (rObservation.mbRolledBack ? "1" : "0")
           + " queue=" + (rObservation.mbQueueExact ? "1" : "0")
           + " computational=" + (rObservation.mbComputationalFullMatch ? "1" : "0")
           + " graph=" + (rObservation.mbGraphFullMatch ? "1" : "0")
           + " object_realization=" + (rObservation.mbObjectRealizationExact ? "1" : "0")
           + " rollback=" + (rObservation.mbRollbackExact ? "1" : "0");
}

[[nodiscard]] std::string describeRawDocumentMutationObservation(
    const spreadsheetengine::compat::libreoffice::substraterawmutation::
        RawDocumentMutationObservation& rObservation)
{
    return std::string(spreadsheetengine::compat::libreoffice::substraterawmutation::toString(
               rObservation.meKind))
           + " reason="
           + OUStringToOString(rObservation.maReason, RTL_TEXTENCODING_UTF8).getStr()
           + " primitive_applied=" + (rObservation.mbPrimitiveMutationApplied ? "1" : "0")
           + " raw_exact=" + (rObservation.mbRawMutationExact ? "1" : "0")
           + " queue=" + (rObservation.mbQueueExact ? "1" : "0")
           + " computational=" + (rObservation.mbComputationalFullMatch ? "1" : "0")
           + " graph=" + (rObservation.mbGraphFullMatch ? "1" : "0")
           + " object_realization=" + (rObservation.mbObjectRealizationExact ? "1" : "0")
           + " rollback=" + (rObservation.mbRollbackExact ? "1" : "0");
}

[[nodiscard]] std::string describeLiveApplyObservation(
    const spreadsheetengine::compat::libreoffice::substrateliveapply::LiveApplyObservation& rObservation)
{
    return std::string(spreadsheetengine::compat::libreoffice::substrateliveapply::toString(
               rObservation.meKind))
           + " reason="
           + OUStringToOString(rObservation.maReason, RTL_TEXTENCODING_UTF8).getStr()
           + " rolled_back=" + (rObservation.mbRolledBack ? "1" : "0")
           + " raw_exact=" + (rObservation.mbRawMutationExact ? "1" : "0")
           + " realization_exact=" + (rObservation.mbObjectRealizationExact ? "1" : "0")
           + " rollback_exact=" + (rObservation.mbRollbackExact ? "1" : "0")
           + " queue=" + (rObservation.mbQueueExact ? "1" : "0")
           + " computational=" + (rObservation.mbComputationalFullMatch ? "1" : "0")
           + " graph=" + (rObservation.mbGraphFullMatch ? "1" : "0");
}

[[nodiscard]] std::string describeFinalVerificationObservation(
    const spreadsheetengine::compat::libreoffice::substratefinalverification::
        FinalVerificationObservation& rObservation)
{
    return std::string(spreadsheetengine::compat::libreoffice::substratefinalverification::toString(
               rObservation.meKind))
           + " reason="
           + OUStringToOString(rObservation.maReason, RTL_TEXTENCODING_UTF8).getStr()
           + " executed=" + (rObservation.mbVerificationExecuted ? "1" : "0")
           + " rolled_back=" + (rObservation.mbRolledBack ? "1" : "0")
           + " queue=" + (rObservation.mbQueueExact ? "1" : "0")
           + " computational=" + (rObservation.mbComputationalFullMatch ? "1" : "0")
           + " graph=" + (rObservation.mbGraphFullMatch ? "1" : "0")
           + " graph_normalized=" + (rObservation.mbGraphNormalizedEquivalent ? "1" : "0")
           + " ir_exact=" + (rObservation.mbIrExact ? "1" : "0")
           + " ir_normalized=" + (rObservation.mbIrNormalizedEquivalent ? "1" : "0")
           + " broadcasters_exact=" + (rObservation.mbBroadcasterExact ? "1" : "0")
           + " broadcasters_ordering="
           + (rObservation.mbBroadcasterOrderingEquivalent ? "1" : "0")
           + " live_apply=" + (rObservation.mbLiveApplyExact ? "1" : "0")
           + " primitive_realization="
           + (rObservation.mbPrimitiveRealizationExact ? "1" : "0")
           + " primitive_rollback=" + (rObservation.mbPrimitiveRollbackExact ? "1" : "0");
}

[[nodiscard]] std::string describePrimitiveExecutionObservation(
    const spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::
        PrimitiveExecutionObservation& rObservation)
{
    return std::string(spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::toString(
               rObservation.meKind))
           + " reason="
           + OUStringToOString(rObservation.maReason, RTL_TEXTENCODING_UTF8).getStr()
           + " applied=" + (rObservation.mbPrimitiveExecutionApplied ? "1" : "0")
           + " rolled_back=" + (rObservation.mbRolledBack ? "1" : "0")
           + " raw_document_mutation="
           + (rObservation.mbRawDocumentMutationExact ? "1" : "0")
           + " primitive_realization="
           + (rObservation.mbPrimitiveRealizationExact ? "1" : "0")
           + " primitive_rollback=" + (rObservation.mbPrimitiveRollbackExact ? "1" : "0")
           + " final_verification_exact="
           + (rObservation.mbFinalVerificationExact ? "1" : "0")
           + " final_verification_normalized="
           + (rObservation.mbFinalVerificationNormalizedEquivalent ? "1" : "0")
           + " queue=" + (rObservation.mbQueueExact ? "1" : "0")
           + " computational=" + (rObservation.mbComputationalFullMatch ? "1" : "0")
           + " graph=" + (rObservation.mbGraphFullMatch ? "1" : "0");
}

[[nodiscard]] std::string describePrimitiveHostExecutorObservation(
    const spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::
        PrimitiveHostExecutorObservation& rObservation)
{
    return std::string(spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::
                           toString(rObservation.meKind))
           + " reason="
           + OUStringToOString(rObservation.maReason, RTL_TEXTENCODING_UTF8).getStr()
           + " applied=" + (rObservation.mbPrimitiveHostExecutorApplied ? "1" : "0")
           + " rolled_back=" + (rObservation.mbRolledBack ? "1" : "0")
           + " raw_document_mutation="
           + (rObservation.mbRawDocumentMutationExact ? "1" : "0")
           + " primitive_execution=" + (rObservation.mbPrimitiveExecutionExact ? "1" : "0")
           + " primitive_realization="
           + (rObservation.mbPrimitiveRealizationExact ? "1" : "0")
           + " primitive_rollback=" + (rObservation.mbPrimitiveRollbackExact ? "1" : "0")
           + " final_verification_exact="
           + (rObservation.mbFinalVerificationExact ? "1" : "0")
           + " final_verification_normalized="
           + (rObservation.mbFinalVerificationNormalizedEquivalent ? "1" : "0")
           + " queue=" + (rObservation.mbQueueExact ? "1" : "0")
           + " computational=" + (rObservation.mbComputationalFullMatch ? "1" : "0")
           + " graph=" + (rObservation.mbGraphFullMatch ? "1" : "0");
}

void assertFormulaStateEqual(
    const FormulaStateSnapshot& rExpected, const FormulaStateSnapshot& rActual)
{
    CPPUNIT_ASSERT(rExpected.maTreeOrder == rActual.maTreeOrder);
    CPPUNIT_ASSERT(rExpected.maTrackOrder == rActual.maTrackOrder);
    CPPUNIT_ASSERT(rExpected.maDirtyOnly == rActual.maDirtyOnly);
}

[[nodiscard]] bool hasGraphEdge(
    const spreadsheetengine::detail::substrate::DependencyGraphShadow& rGraph,
    const spreadsheetengine::detail::substrate::BroadcasterNodeId& rBroadcaster,
    const spreadsheetengine::detail::substrate::ListenerAnchorId& rListener)
{
    return std::any_of(rGraph.maEdges.begin(), rGraph.maEdges.end(),
        [&rBroadcaster, &rListener](const auto& rEdge) {
            return rEdge.maBroadcaster == rBroadcaster && rEdge.maListenerAnchor == rListener;
        });
}

} // namespace

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDirectAndTransitiveScalarInvalidationShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::detail::dependency::buildDependencySnapshot;
    using spreadsheetengine::detail::dependency::planInvalidation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 0, 1, 2.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr); // C1
    m_pDoc->SetString(3, 0, 0, u"=SUM(A1:A2)"_ustr); // D1

    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 1);
    const auto aSnapshot = buildDependencySnapshot(aBeforeFacade);
    const auto aPlan = planInvalidation(aSnapshot, translateSetScalarValue(ScAddress(0, 0, 0)));

    m_pDoc->SetValue(0, 0, 0, 99.0);

    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 2);
    const auto aActual = collectDirtyFormulaAddresses(aAfterFacade);
    const auto aPredicted = collectDirtyFormulaAddresses(aPlan.maDirtyFormulaCells);

    CPPUNIT_ASSERT(aActual == aPredicted);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testNamedRangeInvalidationShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::detail::dependency::buildDependencySnapshot;
    using spreadsheetengine::detail::dependency::planInvalidation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 0, 1, 2.0); // A2

    auto* pGlobalName = new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pGlobalName));

    m_pDoc->SetString(1, 0, 0, u"=SUM(Metrics)"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr); // C1

    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 1);
    const auto aSnapshot = buildDependencySnapshot(aBeforeFacade);
    const auto aPlan = planInvalidation(aSnapshot, translateSetScalarValue(ScAddress(0, 1, 0)));

    m_pDoc->SetValue(0, 1, 0, 9.0);

    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 2);
    const auto aActual = collectDirtyFormulaAddresses(aAfterFacade);
    const auto aPredicted = collectDirtyFormulaAddresses(aPlan.maDirtyFormulaCells);

    CPPUNIT_ASSERT(aActual == aPredicted);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testStructuralRowInsertShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::detail::dependency::buildDependencySnapshot;
    using spreadsheetengine::detail::dependency::planInvalidation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    m_pDoc->InsertTab(1, u"Summary"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // Data.A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // Data.A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // Data.B1
    m_pDoc->SetString(2, 0, 0, u"=SUM(A1:A2)"_ustr); // Data.C1
    m_pDoc->SetString(0, 0, 1, u"=Data.B1"_ustr); // Summary.A1

    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 1);
    const auto aSnapshot = buildDependencySnapshot(aBeforeFacade);
    const auto aPlan = planInvalidation(aSnapshot, translateInsertRows(0, 1, 1));

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));

    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 2);
    const auto aActual = collectDirtyFormulaAddresses(aAfterFacade);
    const auto aPredicted = collectDirtyFormulaAddresses(aPlan.maDirtyFormulaCells);

    CPPUNIT_ASSERT(aPlan.mbRequiresSnapshotRebuild);
    CPPUNIT_ASSERT(!aPlan.maRebuildScopes.empty());
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::dependency::RebuildScopeKind::Workbook,
        aPlan.maRebuildScopes.front().meKind);
    assertConservativeSuperset(aActual, aPredicted);

    m_pDoc->DeleteTab(1);
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testStructuralDeleteColumnShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;
    using spreadsheetengine::detail::dependency::buildDependencySnapshot;
    using spreadsheetengine::detail::dependency::planInvalidation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(1, 0, 0, 2.0); // B1
    m_pDoc->SetString(2, 0, 0, u"=A1+B1"_ustr); // C1
    m_pDoc->SetString(3, 0, 0, u"=C1"_ustr); // D1

    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 1);
    const auto aSnapshot = buildDependencySnapshot(aBeforeFacade);
    const auto aPlan = planInvalidation(aSnapshot, translateDeleteColumns(0, 1, 1));

    m_pDoc->DeleteCol(0, 0, m_pDoc->MaxRow(), 0, 1, 1);

    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 2);
    const auto aActual = collectDirtyFormulaAddresses(aAfterFacade);

    CPPUNIT_ASSERT(aPlan.mbRequiresSnapshotRebuild);
    CPPUNIT_ASSERT(!aPlan.maRebuildScopes.empty());
    CPPUNIT_ASSERT(static_cast<sal_Int32>(aActual.size())
                   <= static_cast<sal_Int32>(aPlan.maDirtyFormulaCells.size()));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testWorkbookScaleShadowCorpus)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::detail::dependency::buildDependencySnapshot;
    using spreadsheetengine::detail::dependency::planInvalidation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    m_pDoc->InsertTab(1, u"Summary"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 5.0); // Data.A1
    m_pDoc->SetValue(0, 1, 0, 7.0); // Data.A2

    auto* pGlobalName = new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pGlobalName));

    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr); // Data.B1
    m_pDoc->SetString(1, 1, 0, u"=A2*2"_ustr); // Data.B2
    m_pDoc->SetString(2, 0, 0, u"=SUM(A1:A2)"_ustr); // Data.C1
    m_pDoc->SetString(0, 0, 1, u"=Data.C1"_ustr); // Summary.A1
    m_pDoc->SetString(1, 0, 1, u"=SUM(Metrics)"_ustr); // Summary.B1
    m_pDoc->SetString(2, 0, 1, u"=B1"_ustr); // Summary.C1

    m_pDoc->CalcAll();

    {
        const ScopedInvalidationShadow aShadow(*m_pDoc, true);

        m_pDoc->SetValue(0, 0, 0, 9.0);

        assertNotUnderInvalidation(
            aShadow.compare(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0))));
    }

    m_pDoc->CalcAll();

    {
        const ScopedInvalidationShadow aShadow(*m_pDoc, true);

        m_pDoc->SetString(2, 0, 1, u"=IF(B1>0;B1;0)"_ustr);

        assertNotUnderInvalidation(
            aShadow.compare(*m_pDoc,
                translateSetFormula(ScAddress(2, 0, 1), u"=IF(B1>0;B1;0)"_ustr)));
    }

    m_pDoc->DeleteTab(1);
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testRuntimeDependencyShadowAudit)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedInvalidationShadow aShadow(*m_pDoc, true);
    CPPUNIT_ASSERT(aShadow.isCaptured());

    m_pDoc->SetValue(0, 0, 0, 99.0);

    const auto oComparison
        = aShadow.compare(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0)));
    CPPUNIT_ASSERT(oComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(ShadowComparisonKind::Exact, oComparison->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testSetValueRecalcShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcShadow aShadow(*m_pDoc, true);
    CPPUNIT_ASSERT(aShadow.isCaptured());

    m_pDoc->SetValue(0, 0, 0, 9.0);
    assertExactRecalcShadow(
        aShadow.compare(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0))));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testSetStringScalarRecalcShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetString(0, 0, 0, u"a"_ustr);
    m_pDoc->SetString(1, 0, 0, u"=LEN(A1)"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcShadow aShadow(*m_pDoc, true);
    m_pDoc->SetString(0, 0, 0, u"alpha"_ustr);

    assertExactRecalcShadow(
        aShadow.compare(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0))));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testSetFormulaRecalcShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->SetString(3, 0, 0, u"=B1+2"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcShadow aShadow(*m_pDoc, true);
    m_pDoc->SetString(1, 0, 0, u"=A1*3"_ustr);
    forceFormulaTreeOrder(*m_pDoc,
        { ScAddress(1, 0, 0), ScAddress(2, 0, 0), ScAddress(3, 0, 0) });

    assertExactRecalcShadow(
        aShadow.compare(*m_pDoc, translateSetFormula(ScAddress(1, 0, 0), u"=A1*3"_ustr)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testClearCellRecalcShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearCell;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcShadow aShadow(*m_pDoc, true);
    m_pDoc->SetEmptyCell(ScAddress(0, 0, 0));

    assertExactRecalcShadow(
        aShadow.compare(*m_pDoc, translateClearCell(ScAddress(0, 0, 0))));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testClearRangeRecalcShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearRange;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(1, 0, 0, u"=SUM(A1:A2)"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcShadow aShadow(*m_pDoc, true);
    ScMarkData aMark(m_pDoc->GetSheetLimits());
    aMark.SelectOneTable(0);
    m_pDoc->DeleteArea(0, 0, 0, 1, aMark, InsertDeleteFlags::CONTENTS);

    assertExactRecalcShadow(
        aShadow.compare(*m_pDoc, translateClearRange(ScRange(0, 0, 0, 0, 1, 0))));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testSetValueRecalcAuthorityPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->SetString(3, 0, 0, u"=A1+5"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.isCaptured());
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->SetValue(0, 0, 0, 9.0);
    forceFormulaTreeOrder(*m_pDoc,
        { ScAddress(2, 0, 0), ScAddress(1, 0, 0), ScAddress(3, 0, 0) });

    assertPilotAppliedWithExactQueue(
        aAuthority.apply(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0))),
        RecalcShadowComparisonKind::OrderMismatch, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testSetFormulaRecalcAuthorityPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->SetString(3, 0, 0, u"=B1+2"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->SetString(1, 0, 0, u"=A1*3"_ustr);
    forceFormulaTreeOrder(*m_pDoc,
        { ScAddress(3, 0, 0), ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    assertPilotAppliedWithExactQueue(
        aAuthority.apply(*m_pDoc, translateSetFormula(ScAddress(1, 0, 0), u"=A1*3"_ustr)),
        RecalcShadowComparisonKind::OrderMismatch, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testClearRangeRecalcAuthorityPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearRange;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(1, 0, 0, u"=SUM(A1:A2)"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->SetString(3, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    ScMarkData aMark(m_pDoc->GetSheetLimits());
    aMark.SelectOneTable(0);
    m_pDoc->DeleteArea(0, 0, 0, 1, aMark, InsertDeleteFlags::CONTENTS);
    forceFormulaTreeOrder(*m_pDoc,
        { ScAddress(3, 0, 0), ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    assertPilotAppliedWithExactQueue(
        aAuthority.apply(*m_pDoc, translateClearRange(ScRange(0, 0, 0, 0, 1, 0))),
        RecalcShadowComparisonKind::OrderMismatch, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testRecalcAuthoritySkipsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.isCaptured());
    CPPUNIT_ASSERT(!aAuthority.canApplyAuthority());

    m_pDoc->SetValue(0, 0, 0, 5.0);
    const auto oResult = aAuthority.apply(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0)));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(PilotResultKind::SkippedDirtyBaseline, oResult->meKind);
    CPPUNIT_ASSERT(oResult->moComparisonBefore.has_value());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testRuntimeRecalcAuthoritySetValueHook)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    const ScopedEnvironmentOverride aEnv("SPREADSHEET_ENGINE_RECALC_AUTHORITY", "1");
    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcShadow aShadow(*m_pDoc, true);
    m_pDoc->SetValue(0, 0, 0, 12.0);

    assertExactRecalcShadow(
        aShadow.compare(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0))));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testInsertRowRecalcAuthorityPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(1, 0, 0, u"=SUM(A1:A2)"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->SetString(3, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    reverseCurrentFormulaTreeOrder(*m_pDoc);

    assertPilotAppliedAndExactAfter(
        aAuthority.apply(*m_pDoc, translateInsertRows(0, 1, 1)), *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDeleteColumnRecalcAuthorityPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(1, 0, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=A1+B1"_ustr);
    m_pDoc->SetString(3, 0, 0, u"=C1"_ustr);
    m_pDoc->SetString(4, 0, 0, u"=D1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->DeleteCol(ScRange(1, 0, 0, 1, m_pDoc->MaxRow(), 0));
    reverseCurrentFormulaTreeOrder(*m_pDoc);

    assertPilotAppliedAndExactAfter(
        aAuthority.apply(*m_pDoc, translateDeleteColumns(0, 1, 1)), *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testNamedRangeRenameRecalcAuthorityPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateRenameNamedRange;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    auto* pName = new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pName));
    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(1, 0, 0, u"=SUM(Metrics)"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    std::map<OUString, ScRangeName> aRangeMap;
    aRangeMap.emplace(STR_GLOBAL_RANGE_NAME, *m_pDoc->GetRangeName());
    ScRangeName& rUpdated = aRangeMap.find(STR_GLOBAL_RANGE_NAME)->second;
    ScRangeData* pUpdated = rUpdated.findByIndex(pName->GetIndex());
    CPPUNIT_ASSERT(pUpdated);
    pUpdated->SetNewName(u"RenamedMetrics"_ustr);

    m_pDoc->SetAllRangeNames(aRangeMap);
    reverseCurrentFormulaTreeOrder(*m_pDoc);

    assertPilotAppliedAndExactAfter(
        aAuthority.apply(*m_pDoc,
            translateRenameNamedRange(*m_pDoc, *m_pDoc->GetRangeName()->findByIndex(pName->GetIndex()),
                std::nullopt, u"Metrics"_ustr)),
        *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalAuthoritySetValuePilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.isCaptured());
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->SetValue(0, 0, 0, 9.0);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    assertComputationalPilotApplied(
        aAuthority.apply(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0))), *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalAuthoritySetFormulaPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.isCaptured());
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->SetString(1, 0, 0, u"=A1*3"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    assertComputationalPilotApplied(
        aAuthority.apply(*m_pDoc, translateSetFormula(ScAddress(1, 0, 0), u"=A1*3"_ustr)),
        *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalAuthorityRejectsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedComputationalAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.isCaptured());
    CPPUNIT_ASSERT(!aAuthority.canApplyAuthority());

    m_pDoc->SetValue(0, 0, 0, 5.0);
    const auto oResult = aAuthority.apply(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0)));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalPilotResultKind::RejectedDirtyBaseline, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalAuthorityRejectsValidationOnlyMutation)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::recalcqueue::captureFormulaState;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(1, 0, 0, u"=SUM(A1:A2)"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    const auto aBeforeApply = captureFormulaState(*m_pDoc);
    const auto oResult = aAuthority.apply(*m_pDoc, translateInsertRows(0, 1, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalPilotResultKind::RejectedOutOfContract, oResult->meKind);
    assertFormulaStateEqual(aBeforeApply, captureFormulaState(*m_pDoc));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalAuthorityRollsBackVerificationFailure)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::recalcqueue::captureFormulaState;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->SetValue(0, 0, 0, 9.0);
    m_pDoc->SetString(2, 0, 0, u"=A1+1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    const auto aBeforeApply = captureFormulaState(*m_pDoc);
    const auto oResult = aAuthority.apply(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0)));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalPilotResultKind::RolledBackVerificationFailure, oResult->meKind);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value());
    CPPUNIT_ASSERT(oResult->moGraphComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::Mismatch, oResult->moGraphComparison->meKind);
    assertFormulaStateEqual(aBeforeApply, captureFormulaState(*m_pDoc));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalAuthorityClassifiesNormalizedEquivalent)
{
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison;
    using spreadsheetengine::compat::libreoffice::substrateauthority::PilotResult;
    using spreadsheetengine::compat::libreoffice::substrateauthority::detail::
        classifyVerifiedPilotResult;

    PilotResult aResult;
    aResult.maTransition.meVerdict
        = spreadsheetengine::detail::substrate::AuthorityPilotVerdict::Applicable;
    aResult.maTransition.maVerification.meQueueMode
        = spreadsheetengine::detail::substrate::AuthorityVerificationMode::Exact;
    aResult.maTransition.maVerification.meGraphMode
        = spreadsheetengine::detail::substrate::AuthorityVerificationMode::Exact;
    aResult.maTransition.maVerification.meIrMode
        = spreadsheetengine::detail::substrate::AuthorityVerificationMode::AllowNormalizedEquivalent;

    ShadowComparison aQueueComparison;
    aQueueComparison.meKind = RecalcShadowComparisonKind::Exact;
    aResult.moQueueComparison = aQueueComparison;

    spreadsheetengine::detail::substrate::DependencyGraphShadowComparison aGraphComparison;
    aGraphComparison.meKind = GraphComparisonKind::Exact;
    aGraphComparison.mbFullMatch = true;
    aResult.moGraphComparison = aGraphComparison;

    spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison aIrComparison;
    aIrComparison.meKind = ExecutionIrComparisonKind::NormalizedEquivalent;
    aIrComparison.mbFullMatch = true;
    aResult.moIrComparison = aIrComparison;

    CPPUNIT_ASSERT_EQUAL(
        ComputationalPilotResultKind::AppliedNormalizedEquivalent,
        classifyVerifiedPilotResult(aResult));
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleInsertFormulaPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(1, 0, 0, 2.0);
    m_pDoc->CalcAll();

    const ScopedComputationalLifecycle aLifecycle(*m_pDoc, true);
    CPPUNIT_ASSERT(aLifecycle.isCaptured());
    CPPUNIT_ASSERT(aLifecycle.canApplyLifecycle());

    m_pDoc->SetString(2, 0, 0, u"=A1+B1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0) });

    const auto oResult
        = aLifecycle.apply(*m_pDoc, translateSetFormula(ScAddress(2, 0, 0), u"=A1+B1"_ustr));
    assertComputationalLifecycleApplied(oResult, *m_pDoc);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(2, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleReplaceFormulaPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalLifecycle aLifecycle(*m_pDoc, true);
    CPPUNIT_ASSERT(aLifecycle.canApplyLifecycle());

    m_pDoc->SetString(1, 0, 0, u"=A1*3"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    const auto oResult
        = aLifecycle.apply(*m_pDoc, translateSetFormula(ScAddress(1, 0, 0), u"=A1*3"_ustr));
    assertComputationalLifecycleApplied(oResult, *m_pDoc);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(1, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleRemoveFormulaPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearCell;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalLifecycle aLifecycle(*m_pDoc, true);
    CPPUNIT_ASSERT(aLifecycle.canApplyLifecycle());

    m_pDoc->SetEmptyCell(ScAddress(1, 0, 0));

    const auto oResult = aLifecycle.apply(*m_pDoc, translateClearCell(ScAddress(1, 0, 0)));
    assertComputationalLifecycleApplied(oResult, *m_pDoc);
    CPPUNIT_ASSERT(!m_pDoc->GetFormulaCell(ScAddress(1, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleRejectsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedComputationalLifecycle aLifecycle(*m_pDoc, true);
    CPPUNIT_ASSERT(aLifecycle.isCaptured());
    CPPUNIT_ASSERT(!aLifecycle.canApplyLifecycle());

    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    const auto oResult
        = aLifecycle.apply(*m_pDoc, translateSetFormula(ScAddress(1, 0, 0), u"=A1*2"_ustr));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalLifecycleResultKind::RejectedDirtyBaseline, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleRejectsOutOfContractMutation)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::recalcqueue::captureFormulaState;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalLifecycle aLifecycle(*m_pDoc, true);
    CPPUNIT_ASSERT(aLifecycle.canApplyLifecycle());

    m_pDoc->SetValue(0, 0, 0, 5.0);
    const auto aBeforeApply = captureFormulaState(*m_pDoc);
    const auto oResult = aLifecycle.apply(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0)));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalLifecycleResultKind::RejectedOutOfContract, oResult->meKind);
    assertFormulaStateEqual(aBeforeApply, captureFormulaState(*m_pDoc));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleRollsBackVerificationFailure)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::recalcqueue::captureFormulaState;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalLifecycle aLifecycle(*m_pDoc, true);
    CPPUNIT_ASSERT(aLifecycle.canApplyLifecycle());

    m_pDoc->SetString(1, 0, 0, u"=A1*3"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=A1+1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    const auto aBeforeApply = captureFormulaState(*m_pDoc);
    const auto oResult
        = aLifecycle.apply(*m_pDoc, translateSetFormula(ScAddress(1, 0, 0), u"=A1*3"_ustr));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalLifecycleResultKind::RolledBackVerificationFailure, oResult->meKind);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value());
    CPPUNIT_ASSERT(oResult->moGraphComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::Mismatch, oResult->moGraphComparison->meKind);
    assertFormulaStateEqual(aBeforeApply, captureFormulaState(*m_pDoc));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleClassifiesNormalizedEquivalent)
{
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison;
    using spreadsheetengine::compat::libreoffice::substratelifecycle::LifecycleResult;
    using spreadsheetengine::compat::libreoffice::substratelifecycle::detail::
        classifyVerifiedLifecycleResult;

    LifecycleResult aResult;
    aResult.maTransition.meVerdict
        = spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable;
    aResult.maTransition.maVerification.meComputationalMode
        = spreadsheetengine::detail::substrate::LifecycleVerificationMode::Exact;
    aResult.maTransition.maVerification.meGraphMode
        = spreadsheetengine::detail::substrate::LifecycleVerificationMode::
            AllowNormalizedEquivalent;
    aResult.maTransition.maVerification.mbObserveIrOnly = true;

    ShadowComparison aQueueComparison;
    aQueueComparison.meKind = RecalcShadowComparisonKind::Exact;
    aResult.moQueueComparison = aQueueComparison;

    spreadsheetengine::detail::substrate::ComputationalShadowComparison aComputationalComparison;
    aComputationalComparison.mbFullMatch = true;
    aResult.moComputationalComparison = aComputationalComparison;

    spreadsheetengine::detail::substrate::DependencyGraphShadowComparison aGraphComparison;
    aGraphComparison.meKind = GraphComparisonKind::NormalizedEquivalent;
    aGraphComparison.mbFullMatch = true;
    aResult.moGraphComparison = aGraphComparison;

    spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison aIrComparison;
    aIrComparison.meKind = ExecutionIrComparisonKind::Exact;
    aIrComparison.mbFullMatch = true;
    aResult.moIrComparison = aIrComparison;

    CPPUNIT_ASSERT_EQUAL(
        ComputationalLifecycleResultKind::AppliedNormalizedEquivalent,
        classifyVerifiedLifecycleResult(aResult));
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleClassifiesRepairDetected)
{
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison;
    using spreadsheetengine::compat::libreoffice::substratelifecycle::LifecycleResult;
    using spreadsheetengine::compat::libreoffice::substratelifecycle::detail::
        classifyVerifiedLifecycleResult;

    LifecycleResult aResult;
    aResult.maTransition.meVerdict
        = spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable;
    aResult.maTransition.maVerification.meComputationalMode
        = spreadsheetengine::detail::substrate::LifecycleVerificationMode::Exact;
    aResult.maTransition.maVerification.meGraphMode
        = spreadsheetengine::detail::substrate::LifecycleVerificationMode::Exact;

    ShadowComparison aQueueComparison;
    aQueueComparison.meKind = RecalcShadowComparisonKind::Exact;
    aResult.moQueueComparison = aQueueComparison;

    spreadsheetengine::detail::substrate::ComputationalShadowComparison aComputationalComparison;
    aComputationalComparison.mbFullMatch = false;
    aResult.moComputationalComparison = aComputationalComparison;

    spreadsheetengine::detail::substrate::DependencyGraphShadowComparison aGraphComparison;
    aGraphComparison.meKind = GraphComparisonKind::Exact;
    aGraphComparison.mbFullMatch = true;
    aResult.moGraphComparison = aGraphComparison;

    CPPUNIT_ASSERT_EQUAL(
        ComputationalLifecycleResultKind::RepairDetected,
        classifyVerifiedLifecycleResult(aResult));
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalMutationEntryRuntimeExplicitGate)
{
    {
        ScopedEnvironmentOverride aMutationEntry(
            "SPREADSHEET_ENGINE_COMPUTATIONAL_MUTATION_ENTRY", "0");

        m_pDoc->InsertTab(0, u"Data"_ustr);
        sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

        const auto aCapture
            = ScopedComputationalMutationEntry::captureIfRuntimeEnabled(*m_pDoc);
        CPPUNIT_ASSERT(!aCapture.isCaptured());

        m_pDoc->DeleteTab(0);
    }

    {
        ScopedEnvironmentOverride aMutationEntry(
            "SPREADSHEET_ENGINE_COMPUTATIONAL_MUTATION_ENTRY", "1");

        m_pDoc->InsertTab(0, u"Data"_ustr);
        sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

        const auto aCapture
            = ScopedComputationalMutationEntry::captureIfRuntimeEnabled(*m_pDoc);
        CPPUNIT_ASSERT(aCapture.isCaptured());

        m_pDoc->DeleteTab(0);
    }
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalMutationEntrySetValue)
{
    using spreadsheetengine::api::CellValue;
    using spreadsheetengine::detail::substrate::MutationEntryRequest;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalMutationEntry aEntry(*m_pDoc, true);
    CPPUNIT_ASSERT(aEntry.isCaptured());
    CPPUNIT_ASSERT(aEntry.canApplyMutationEntry());

    const auto oResult
        = aEntry.apply(*m_pDoc, MutationEntryRequest::setScalarValue({ 0, 0, 0 },
                                         CellValue::number(9.0)));
    assertComputationalMutationEntryApplied(oResult, *m_pDoc);
    CPPUNIT_ASSERT(oResult->moComputationalComparison.has_value());
    CPPUNIT_ASSERT(oResult->moComputationalComparison->mbCellPopulationMatch);
    CPPUNIT_ASSERT(oResult->moComputationalComparison->mbFormulaTreeMatch);
    CPPUNIT_ASSERT(oResult->moComputationalComparison->mbFormulaTrackMatch);
    CPPUNIT_ASSERT(oResult->moComputationalComparison->mbBroadcasterMatch);
    CPPUNIT_ASSERT(oResult->moComputationalComparison->mbGroupMatch);
    CPPUNIT_ASSERT(oResult->moComputationalComparison->mbNamedRangeMatch);
    CPPUNIT_ASSERT(oResult->moBroadcasterCanonicalization.has_value());
    CPPUNIT_ASSERT(oResult->moBroadcasterCanonicalization->mbExactMatch);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        describeBroadcasterCanonicalization(*oResult->moBroadcasterCanonicalization),
        BroadcasterCanonicalizationKind::Exact,
        oResult->moBroadcasterCanonicalization->meKind);
    CPPUNIT_ASSERT(oResult->moComputationalComparison->mbFullMatch);
    CPPUNIT_ASSERT_EQUAL(9.0, m_pDoc->GetValue(ScAddress(0, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalMutationEntrySetFormula)
{
    using spreadsheetengine::detail::substrate::MutationEntryRequest;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalMutationEntry aEntry(*m_pDoc, true);
    CPPUNIT_ASSERT(aEntry.canApplyMutationEntry());

    const auto oResult
        = aEntry.apply(*m_pDoc, MutationEntryRequest::setFormula({ 0, 1, 0 }, u"=A1*3"));
    assertComputationalMutationEntryApplied(oResult, *m_pDoc);
    CPPUNIT_ASSERT(oResult->moComputationalComparison.has_value());
    CPPUNIT_ASSERT(oResult->moComputationalComparison->mbFullMatch);

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);
    CPPUNIT_ASSERT_EQUAL(u"=A1*3"_ustr, pFormula->GetFormula());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalMutationEntryInsertRows)
{
    using spreadsheetengine::detail::substrate::MutationEntryRequest;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalMutationEntry aEntry(*m_pDoc, true);
    CPPUNIT_ASSERT(aEntry.canApplyMutationEntry());

    const auto oResult = aEntry.apply(*m_pDoc, MutationEntryRequest::insertRows(0, 1, 1));
    assertComputationalMutationEntryApplied(oResult, *m_pDoc);
    CPPUNIT_ASSERT(oResult->moComputationalComparison.has_value());
    CPPUNIT_ASSERT(oResult->moComputationalComparison->mbFullMatch);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(0, 3, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalMutationEntryRejectsDirtyBaseline)
{
    using spreadsheetengine::api::CellValue;
    using spreadsheetengine::compat::libreoffice::recalcqueue::captureFormulaState;
    using spreadsheetengine::detail::substrate::MutationEntryRequest;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const auto aBeforeApply = captureFormulaState(*m_pDoc);

    const ScopedComputationalMutationEntry aEntry(*m_pDoc, true);
    CPPUNIT_ASSERT(aEntry.isCaptured());
    CPPUNIT_ASSERT(!aEntry.canApplyMutationEntry());

    const auto oResult
        = aEntry.apply(*m_pDoc, MutationEntryRequest::setScalarValue({ 0, 0, 0 },
                                         CellValue::number(5.0)));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalMutationEntryResultKind::RejectedDirtyBaseline, oResult->meKind);
    CPPUNIT_ASSERT(oResult->moRawMutationObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        describeRawMutationObservation(*oResult->moRawMutationObservation),
        RawMutationObservationKind::Exact, oResult->moRawMutationObservation->meKind);
    CPPUNIT_ASSERT(oResult->moRawMutationRecord.has_value());
    CPPUNIT_ASSERT(oResult->moRawDocumentMutationRecord.has_value());
    CPPUNIT_ASSERT(
        oResult->moRawDocumentMutationRecord->maRawMutation == *oResult->moRawMutationRecord);
    CPPUNIT_ASSERT(oResult->moRawDocumentMutationObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        describeRawDocumentMutationObservation(*oResult->moRawDocumentMutationObservation),
        RawDocumentMutationObservationKind::Exact,
        oResult->moRawDocumentMutationObservation->meKind);
    CPPUNIT_ASSERT(oResult->moPrimitiveExecutionPlan.has_value());
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt8>(3), oResult->moPrimitiveExecutionPlan->mnStageCount);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::hasStage(
            *oResult->moPrimitiveExecutionPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::
                PrimitiveExecutionStageKind::RawDocumentMutation));
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::hasStage(
            *oResult->moPrimitiveExecutionPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::
                PrimitiveExecutionStageKind::PrimitiveRollback));
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::hasStage(
            *oResult->moPrimitiveExecutionPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::
                PrimitiveExecutionStageKind::FinalVerification));
    CPPUNIT_ASSERT(!spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::hasStage(
        *oResult->moPrimitiveExecutionPlan,
        spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::
            PrimitiveExecutionStageKind::PrimitiveRealization));
    CPPUNIT_ASSERT(oResult->moLiveApplyPlan.has_value());
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt8>(2), oResult->moLiveApplyPlan->mnStageCount);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::substrateliveapply::hasStage(
            *oResult->moLiveApplyPlan,
            spreadsheetengine::compat::libreoffice::substrateliveapply::LiveApplyStageKind::
                RawMutation));
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::substrateliveapply::hasStage(
            *oResult->moLiveApplyPlan,
            spreadsheetengine::compat::libreoffice::substrateliveapply::LiveApplyStageKind::
                Rollback));
    CPPUNIT_ASSERT(!spreadsheetengine::compat::libreoffice::substrateliveapply::hasStage(
        *oResult->moLiveApplyPlan,
        spreadsheetengine::compat::libreoffice::substrateliveapply::LiveApplyStageKind::
            Verification));
    CPPUNIT_ASSERT(oResult->moLiveApplyObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeLiveApplyObservation(*oResult->moLiveApplyObservation),
        LiveApplyObservationKind::Exact, oResult->moLiveApplyObservation->meKind);
    CPPUNIT_ASSERT(oResult->moFinalVerificationRecord.has_value());
    CPPUNIT_ASSERT(
        oResult->moFinalVerificationRecord->maLiveApplyPlan.mnStageCount
        == oResult->moLiveApplyPlan->mnStageCount);
    CPPUNIT_ASSERT(
        oResult->moFinalVerificationRecord->maLiveApplyPlan.mbRolledBack
        == oResult->moLiveApplyPlan->mbRolledBack);
    CPPUNIT_ASSERT(oResult->moFinalVerificationRecord->mbUsesFinalVerification);
    CPPUNIT_ASSERT(oResult->moPrimitiveExecutionObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        describePrimitiveExecutionObservation(*oResult->moPrimitiveExecutionObservation),
        PrimitiveExecutionObservationKind::Exact,
        oResult->moPrimitiveExecutionObservation->meKind);
    CPPUNIT_ASSERT(oResult->moPrimitiveHostExecutorPlan.has_value());
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt8>(3), oResult->moPrimitiveHostExecutorPlan->mnStageCount);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::hasStage(
            *oResult->moPrimitiveHostExecutorPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::
                PrimitiveHostExecutorStageKind::RawDocumentMutationCall));
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::hasStage(
            *oResult->moPrimitiveHostExecutorPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::
                PrimitiveHostExecutorStageKind::PrimitiveRollbackCall));
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::hasStage(
            *oResult->moPrimitiveHostExecutorPlan,
            spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::
                PrimitiveHostExecutorStageKind::FinalVerificationCall));
    CPPUNIT_ASSERT(!spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::hasStage(
        *oResult->moPrimitiveHostExecutorPlan,
        spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::
            PrimitiveHostExecutorStageKind::PrimitiveRealizationCall));
    CPPUNIT_ASSERT(oResult->moPrimitiveHostExecutorObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        describePrimitiveHostExecutorObservation(*oResult->moPrimitiveHostExecutorObservation),
        PrimitiveHostExecutorObservationKind::Exact,
        oResult->moPrimitiveHostExecutorObservation->meKind);
    CPPUNIT_ASSERT(oResult->moFinalVerificationObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        describeFinalVerificationObservation(*oResult->moFinalVerificationObservation),
        FinalVerificationObservationKind::Exact,
        oResult->moFinalVerificationObservation->meKind);
    CPPUNIT_ASSERT(!oResult->moPrimitiveRealizationRecord.has_value());
    CPPUNIT_ASSERT(!oResult->moPrimitiveRealizationObservation.has_value());
    CPPUNIT_ASSERT(oResult->moPrimitiveRollbackRecord.has_value());
    CPPUNIT_ASSERT(oResult->moPrimitiveRollbackRecord->mbUsesPrimitiveRollback);
    CPPUNIT_ASSERT(oResult->moPrimitiveRollbackObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        describePrimitiveRollbackObservation(*oResult->moPrimitiveRollbackObservation),
        PrimitiveRollbackObservationKind::Exact,
        oResult->moPrimitiveRollbackObservation->meKind);
    CPPUNIT_ASSERT(oResult->moRollbackObservation.has_value());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        describeRollbackObservation(*oResult->moRollbackObservation),
        RollbackObservationKind::Exact, oResult->moRollbackObservation->meKind);
    CPPUNIT_ASSERT_EQUAL(1.0, m_pDoc->GetValue(ScAddress(0, 0, 0)));
    assertFormulaStateEqual(aBeforeApply, captureFormulaState(*m_pDoc));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalMutationEntryClassifiesRepairDetected)
{
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison;
    using spreadsheetengine::compat::libreoffice::substratemutationentry::MutationEntryResult;
    using spreadsheetengine::compat::libreoffice::substratemutationentry::detail::
        classifyVerifiedMutationEntryResult;

    MutationEntryResult aResult;
    aResult.maTransition.mePath = spreadsheetengine::detail::substrate::MutationEntryPath::Lifecycle;
    aResult.maTransition.moLifecycleTransition.emplace();
    aResult.maTransition.moLifecycleTransition->meVerdict
        = spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable;
    aResult.maTransition.moLifecycleTransition->maVerification.meComputationalMode
        = spreadsheetengine::detail::substrate::LifecycleVerificationMode::Exact;
    aResult.maTransition.moLifecycleTransition->maVerification.meGraphMode
        = spreadsheetengine::detail::substrate::LifecycleVerificationMode::Exact;

    ShadowComparison aQueueComparison;
    aQueueComparison.meKind = RecalcShadowComparisonKind::Exact;
    aResult.moQueueComparison = aQueueComparison;

    spreadsheetengine::detail::substrate::ComputationalShadowComparison aComputationalComparison;
    aComputationalComparison.mbFullMatch = false;
    aResult.moComputationalComparison = aComputationalComparison;

    spreadsheetengine::detail::substrate::DependencyGraphShadowComparison aGraphComparison;
    aGraphComparison.meKind = GraphComparisonKind::Exact;
    aGraphComparison.mbFullMatch = true;
    aResult.moGraphComparison = aGraphComparison;

    spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison aIrComparison;
    aIrComparison.meKind = ExecutionIrComparisonKind::Exact;
    aIrComparison.mbFullMatch = true;
    aResult.moIrComparison = aIrComparison;

    CPPUNIT_ASSERT_EQUAL(
        ComputationalMutationEntryResultKind::RepairDetected,
        classifyVerifiedMutationEntryResult(aResult));
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalNarrowRolloutDisabledByDefault)
{
    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "0");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", "0");
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", "0");
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", "0");

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    const auto aAuthorityCapture
        = ScopedComputationalAuthority::captureIfRuntimeEnabled(*m_pDoc);
    const auto aLifecycleCapture
        = ScopedComputationalLifecycle::captureIfRuntimeEnabled(*m_pDoc);
    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);

    CPPUNIT_ASSERT(!aAuthorityCapture.isCaptured());
    CPPUNIT_ASSERT(!aLifecycleCapture.isCaptured());
    CPPUNIT_ASSERT(!aStructuralCapture.isCaptured());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalNarrowRolloutLifecycleEnabledByUmbrella)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(1, 0, 0, 2.0);
    m_pDoc->CalcAll();

    const auto aAuthorityCapture
        = ScopedComputationalAuthority::captureIfRuntimeEnabled(*m_pDoc);
    const auto aLifecycleCapture
        = ScopedComputationalLifecycle::captureIfRuntimeEnabled(*m_pDoc);
    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aAuthorityCapture.isCaptured());
    CPPUNIT_ASSERT(aLifecycleCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aLifecycleCapture.canApplyLifecycle());

    m_pDoc->SetString(2, 0, 0, u"=A1+B1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0) });

    const auto oResult = aLifecycleCapture.apply(
        *m_pDoc, translateSetFormula(ScAddress(2, 0, 0), u"=A1+B1"_ustr));
    assertComputationalLifecycleApplied(oResult, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralInsertRowPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.isCaptured());
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 3, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateInsertRows(0, 1, 1));
    assertComputationalStructuralApplied(oResult, *m_pDoc);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(0, 3, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalNarrowRolloutStructuralEnabledByUmbrella)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 3, 0) });

    const auto oResult = aStructuralCapture.apply(*m_pDoc, translateInsertRows(0, 1, 1));
    assertComputationalStructuralApplied(oResult, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralDeleteColumnPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(1, 0, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=$B$1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->DeleteCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(1, 0, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateDeleteColumns(0, 0, 1));
    assertComputationalStructuralApplied(oResult, *m_pDoc);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(1, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralDeleteRowPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->DeleteRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 1, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateDeleteRows(0, 0, 1));
    assertComputationalStructuralAppliedExactly(oResult, *m_pDoc);
    CPPUNIT_ASSERT(oResult->maTransition.maContract.isAdmitted());
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(0, 1, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralInsertColumnPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=$A$1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateInsertColumns(0, 0, 1));
    assertComputationalStructuralAppliedExactly(oResult, *m_pDoc);
    CPPUNIT_ASSERT(oResult->maTransition.maContract.isAdmitted());
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(2, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalNarrowRolloutDeleteRowEnabledByUmbrella)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteRows;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->DeleteRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 1, 0) });

    const auto oResult = aStructuralCapture.apply(*m_pDoc, translateDeleteRows(0, 0, 1));
    assertComputationalStructuralAppliedExactly(oResult, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalNarrowRolloutInsertColumnEnabledByUmbrella)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=$A$1+1"_ustr);
    m_pDoc->CalcAll();

    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0) });

    const auto oResult = aStructuralCapture.apply(*m_pDoc, translateInsertColumns(0, 0, 1));
    assertComputationalStructuralAppliedExactly(oResult, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalNarrowRolloutStructuralOverrideBeatsUmbrella)
{
    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", "0");

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    const auto aAuthorityCapture
        = ScopedComputationalAuthority::captureIfRuntimeEnabled(*m_pDoc);
    const auto aLifecycleCapture
        = ScopedComputationalLifecycle::captureIfRuntimeEnabled(*m_pDoc);
    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);

    CPPUNIT_ASSERT(aAuthorityCapture.isCaptured());
    CPPUNIT_ASSERT(aLifecycleCapture.isCaptured());
    CPPUNIT_ASSERT(!aStructuralCapture.isCaptured());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalNarrowRolloutGlobalNamedRangeStaysRejectedWithoutCandidateGate)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);
    ScopedEnvironmentOverride aGlobalNamedRange(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_GLOBAL_NAMED_RANGE", "0");

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructuralCapture.apply(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalNarrowRolloutGlobalNamedRangeEnabledByCandidateGate)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);
    ScopedEnvironmentOverride aGlobalNamedRange(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_GLOBAL_NAMED_RANGE", "1");

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(3, 0, 0) });

    const auto oResult = aStructuralCapture.apply(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::substrate::StructuralMutationClass::Admitted,
        oResult->maTransition.maContract.meMutationClass);
    CPPUNIT_ASSERT(oResult->meKind
        != ComputationalStructuralResultKind::RejectedOutOfContract);
    CPPUNIT_ASSERT(oResult->meKind
        != ComputationalStructuralResultKind::RejectedDirtyBaseline);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value());
    CPPUNIT_ASSERT(oResult->moComputationalComparison.has_value());
    CPPUNIT_ASSERT(oResult->moGraphComparison.has_value());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalNarrowRolloutGlobalNamedRangeOffSheetConsumerCandidate)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);
    ScopedEnvironmentOverride aGlobalNamedRange(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_GLOBAL_NAMED_RANGE", "1");

    m_pDoc->InsertTab(0, u"Data"_ustr);
    m_pDoc->InsertTab(1, u"Summary"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(0, 0, 1, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 0, 1) });

    const auto oResult = aStructuralCapture.apply(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(1);
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralRejectsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(0, 2, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.isCaptured());
    CPPUNIT_ASSERT(!aStructural.canApplyStructural());

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    const auto oResult = aStructural.apply(*m_pDoc, translateInsertRows(0, 1, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedDirtyBaseline, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralRejectsUnsupportedMutation)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateMoveRange;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    const auto oResult = aStructural.apply(*m_pDoc,
        translateMoveRange(ScRange(0, 0, 0, 0, 0, 0), ScRange(1, 0, 0, 1, 0, 0)));
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(0, 2, 0)));
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralDeleteRowRejectsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(0, 2, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.isCaptured());
    CPPUNIT_ASSERT(!aStructural.canApplyStructural());

    m_pDoc->DeleteRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    const auto oResult = aStructural.apply(*m_pDoc, translateDeleteRows(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedDirtyBaseline, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralInsertColumnRejectsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=$A$1+1"_ustr);
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.isCaptured());
    CPPUNIT_ASSERT(!aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructural.apply(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedDirtyBaseline, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralDeleteRowRejectsNamedRangeSlice)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->DeleteRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    const auto oResult = aStructural.apply(*m_pDoc, translateDeleteRows(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralInsertColumnRejectsNamedRangeSlice)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=$A$1+1"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$1"_ustr)));
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructural.apply(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralValidateGlobalNamedRangeSlice)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructural.validateCandidate(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::substrate::StructuralMutationClass::ValidationOnly,
        oResult->maTransition.maContract.meMutationClass);
    CPPUNIT_ASSERT(oResult->meKind != ComputationalStructuralResultKind::RejectedOutOfContract);
    CPPUNIT_ASSERT(oResult->meKind != ComputationalStructuralResultKind::RejectedDirtyBaseline);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value()
        || oResult->meKind == ComputationalStructuralResultKind::RepairDetected);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralValidateGlobalNamedRangeRejectsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(2, 0, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.isCaptured());
    CPPUNIT_ASSERT(!aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructural.validateCandidate(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedDirtyBaseline, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralValidateGlobalNamedRangeRepairDetected)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    m_pDoc->SetString(3, 0, 0, u"=SUM(Metrics)+1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(3, 0, 0) });

    const auto oResult = aStructural.validateCandidate(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RepairDetected, oResult->meKind);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(2, 0, 0)));
    CPPUNIT_ASSERT(!m_pDoc->GetFormulaCell(ScAddress(3, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralLocalNamedRangeStaysDeferred)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 4.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(LocalMetric)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName(0)->insert(
        new ScRangeData(*m_pDoc, u"LocalMetric"_ustr, u"$A$1:$A$1"_ustr)));
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructural.validateCandidate(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralValidateMultiAreaNamedRangeRejects)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$A$1:$A$1~$A$2:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructural.validateCandidate(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralSharedGroupStaysRejectedWithoutCandidateGate)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(1, 1, 0, u"=A2*2"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(1, 1, 0), ScAddress(1, 2, 0) });

    const auto oResult = aStructural.validateCandidate(*m_pDoc, translateInsertRows(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralValidateSharedGroupPreserveCandidate)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    namespace consumers = spreadsheetengine::detail::facade::consumers;

    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", "1");
    ScopedEnvironmentOverride aSharedGroup(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_SHARED_GROUP", "1");

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(1, 1, 0, u"=A2*2"_ustr);
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeGroups = consumers::collectFormulaGroupDescriptors(aBeforeFacade);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::size_t>(1), aBeforeGroups.size());

    const ScopedComputationalStructural aStructuralCapture(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->InsertRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(1, 1, 0), ScAddress(1, 2, 0) });

    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);
    const auto aAfterGroups = consumers::collectFormulaGroupDescriptors(aAfterFacade);
    const auto aGroupTransition = consumers::classifyFormulaGroupTransition(
        aBeforeGroups, aAfterGroups,
        spreadsheetengine::detail::facade::MutationEvent::insertRows(0, 0, 1));

    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::facade::consumers::SharedFormulaGroupTransitionKind::Preserve,
        aGroupTransition.meKind);

    const auto oResult
        = aStructuralCapture.validateCandidate(*m_pDoc, translateInsertRows(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::substrate::StructuralMutationClass::ValidationOnly,
        oResult->maTransition.maContract.meMutationClass);
    CPPUNIT_ASSERT(oResult->meKind
        != ComputationalStructuralResultKind::RejectedOutOfContract);
    CPPUNIT_ASSERT(oResult->meKind
        != ComputationalStructuralResultKind::RejectedDirtyBaseline);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value()
        || oResult->meKind == ComputationalStructuralResultKind::RepairDetected);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralValidateSharedGroupRepairDetected)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    namespace consumers = spreadsheetengine::detail::facade::consumers;

    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", "1");
    ScopedEnvironmentOverride aSharedGroup(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_SHARED_GROUP", "1");

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(1, 1, 0, u"=A2*2"_ustr);
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeGroups = consumers::collectFormulaGroupDescriptors(aBeforeFacade);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::size_t>(1), aBeforeGroups.size());

    const ScopedComputationalStructural aStructuralCapture(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->InsertRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    m_pDoc->SetString(1, 2, 0, u"=A3*3"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(1, 1, 0), ScAddress(1, 2, 0) });

    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);
    const auto aAfterGroups = consumers::collectFormulaGroupDescriptors(aAfterFacade);
    const auto aGroupTransition = consumers::classifyFormulaGroupTransition(
        aBeforeGroups, aAfterGroups,
        spreadsheetengine::detail::facade::MutationEvent::insertRows(0, 0, 1));

    CPPUNIT_ASSERT(aGroupTransition.meKind
        != spreadsheetengine::detail::facade::consumers::SharedFormulaGroupTransitionKind::Preserve);

    const auto oResult
        = aStructuralCapture.validateCandidate(*m_pDoc, translateInsertRows(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RepairDetected, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralSharedGroupNamedRangeCombinedStaysRejected)
{
    using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    namespace consumers = spreadsheetengine::detail::facade::consumers;

    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", "1");
    ScopedEnvironmentOverride aSharedGroup(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_SHARED_GROUP", "1");

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$A$1:$A$2"_ustr)));
    m_pDoc->SetString(1, 0, 0, u"=COUNTA(Metrics)+A1"_ustr);
    m_pDoc->SetString(1, 1, 0, u"=COUNTA(Metrics)+A2"_ustr);
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeGroups = consumers::collectFormulaGroupDescriptors(aBeforeFacade);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::size_t>(1), aBeforeGroups.size());

    const ScopedComputationalStructural aStructuralCapture(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->InsertRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(1, 1, 0), ScAddress(1, 2, 0) });

    const auto oResult
        = aStructuralCapture.validateCandidate(*m_pDoc, translateInsertRows(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralRepairDetectedRollback)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    m_pDoc->SetString(0, 3, 0, u"=$A$2*1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 3, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateInsertRows(0, 1, 1));
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RepairDetected, oResult->meKind);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(0, 2, 0)));
    CPPUNIT_ASSERT(!m_pDoc->GetFormulaCell(ScAddress(0, 3, 0)));
    CPPUNIT_ASSERT_EQUAL(20.0, m_pDoc->GetValue(ScAddress(0, 1, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralDeleteRowRepairDetectedRollback)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->DeleteRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    m_pDoc->SetString(0, 1, 0, u"=$A$2*1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 1, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateDeleteRows(0, 0, 1));
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RepairDetected, oResult->meKind);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(0, 2, 0)));
    CPPUNIT_ASSERT(!m_pDoc->GetFormulaCell(ScAddress(0, 1, 0)));
    CPPUNIT_ASSERT_EQUAL(10.0, m_pDoc->GetValue(ScAddress(0, 0, 0)));
    CPPUNIT_ASSERT_EQUAL(20.0, m_pDoc->GetValue(ScAddress(0, 1, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralInsertColumnRepairDetectedRollback)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=$A$1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    m_pDoc->SetString(2, 0, 0, u"=$A$1+1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateInsertColumns(0, 0, 1));
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RepairDetected, oResult->meKind);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(1, 0, 0)));
    CPPUNIT_ASSERT(!m_pDoc->GetFormulaCell(ScAddress(2, 0, 0)));
    CPPUNIT_ASSERT_EQUAL(1.0, m_pDoc->GetValue(ScAddress(0, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalSubstrateScalarEditCapture)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::dependency::buildDependencySnapshot;
    using spreadsheetengine::detail::dependency::buildRecalcPlan;
    using spreadsheetengine::detail::dependency::planInvalidation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr); // C1
    m_pDoc->SetString(3, 0, 0, u"=SUM(A1:A2)"_ustr); // D1
    m_pDoc->CalcAll();

    const LiveComputationalStateSnapshot aBefore = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(hasCellFormulaListener(aBefore.maBroadcasters, ScAddress(0, 0, 0),
        ScAddress(1, 0, 0)));
    CPPUNIT_ASSERT(hasCellFormulaListener(aBefore.maBroadcasters, ScAddress(1, 0, 0),
        ScAddress(2, 0, 0)));

    const CalcWorkbookFacade aFacade(*m_pDoc, 1);
    const auto aDependencySnapshot = buildDependencySnapshot(aFacade);
    const auto aInvalidationPlan
        = planInvalidation(aDependencySnapshot, translateSetScalarValue(ScAddress(0, 0, 0)));
    const auto aRecalcPlan = buildRecalcPlan(aDependencySnapshot, aInvalidationPlan);

    m_pDoc->SetValue(0, 0, 0, 99.0);

    const LiveComputationalStateSnapshot aAfter = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(aBefore.maBroadcasters == aAfter.maBroadcasters);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            aRecalcPlan)
        == aAfter.maFormulaTree);
    CPPUNIT_ASSERT(aAfter.maFormulaTrack.empty());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalSubstrateFormulaEditCapture)
{
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->CalcAll();

    const LiveComputationalStateSnapshot aBefore = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(hasCellFormulaListener(aBefore.maBroadcasters, ScAddress(0, 0, 0),
        ScAddress(1, 0, 0)));

    m_pDoc->SetString(1, 0, 0, u"=A2"_ustr);

    const LiveComputationalStateSnapshot aAfter = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(!hasCellFormulaListener(aAfter.maBroadcasters, ScAddress(0, 0, 0),
        ScAddress(1, 0, 0)));
    CPPUNIT_ASSERT(hasCellFormulaListener(aAfter.maBroadcasters, ScAddress(0, 1, 0),
        ScAddress(1, 0, 0)));
    CPPUNIT_ASSERT(aAfter.maFormulaTrack.empty());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalSubstrateDelayedStartListeningCapture)
{
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);

    {
        sc::EndListeningContext aEndCxt(*m_pDoc);
        pFormula->EndListeningTo(aEndCxt);
        aEndCxt.purgeEmptyBroadcasters();
    }

    const LiveComputationalStateSnapshot aDetached = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(!hasCellFormulaListener(aDetached.maBroadcasters, ScAddress(0, 0, 0),
        ScAddress(1, 0, 0)));

    ScTable* pTable = m_pDoc->FetchTable(0);
    CPPUNIT_ASSERT(pTable);
    ScColumn& rFormulaColumn = pTable->CreateColumnIfNotExists(1);

    m_pDoc->EnableDelayStartListeningFormulaCells(&rFormulaColumn, true);
    rFormulaColumn.StartListeningUnshared({ 0, 0 });

    const LiveComputationalStateSnapshot aDelayed = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(!hasCellFormulaListener(aDelayed.maBroadcasters, ScAddress(0, 0, 0),
        ScAddress(1, 0, 0)));

    m_pDoc->EnableDelayStartListeningFormulaCells(&rFormulaColumn, false);

    const LiveComputationalStateSnapshot aRestored = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(hasCellFormulaListener(aRestored.maBroadcasters, ScAddress(0, 0, 0),
        ScAddress(1, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalSubstrateDelayedBroadcasterDeletionCapture)
{
    using spreadsheetengine::compat::libreoffice::substrateobs::collectBroadcasterStateSnapshot;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);

    ScTable* pTable = m_pDoc->FetchTable(0);
    CPPUNIT_ASSERT(pTable);
    ScColumn& rSourceColumn = pTable->CreateColumnIfNotExists(0);

    {
        sc::DelayDeletingBroadcasters aDelay(*m_pDoc);
        rSourceColumn.EndListening(*pFormula, 0);

        const BroadcasterStateSnapshot aDelayed = collectBroadcasterStateSnapshot(*m_pDoc);
        assertHasEmptyCellBroadcaster(aDelayed, ScAddress(0, 0, 0));
    }

    const BroadcasterStateSnapshot aAfter = collectBroadcasterStateSnapshot(*m_pDoc);
    assertNoCellBroadcaster(aAfter, ScAddress(0, 0, 0));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalShadowRebuildAfterSafeMutations)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearCell;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::rebuildComputationalShadowAfterMutation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    auto* pName = new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pName));
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr); // C1
    m_pDoc->CalcAll();

    m_pDoc->SetValue(0, 0, 0, 5.0);
    auto aScalarState = rebuildComputationalShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 1), *m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0)));
    const auto* pA1 = aScalarState.maShadow.findCell({ 0, 0, 0 });
    CPPUNIT_ASSERT(pA1);
    CPPUNIT_ASSERT(pA1->maCell.maValue.isNumber());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, pA1->maCell.maValue.mfNumber, 1e-12);

    m_pDoc->SetString(1, 0, 0, u"=A1*4"_ustr);
    auto aFormulaEditState = rebuildComputationalShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 2), *m_pDoc,
        translateSetFormula(ScAddress(1, 0, 0), u"=A1*4"_ustr));
    const auto* pB1 = aFormulaEditState.maShadow.findCell({ 0, 1, 0 });
    CPPUNIT_ASSERT(pB1);
    CPPUNIT_ASSERT(pB1->moFormula.has_value());
    CPPUNIT_ASSERT(pB1->moFormula->maFormulaSource == spreadsheetengine::api::String(u"=A1*4"));

    m_pDoc->SetString(3, 0, 0, u"=B1+C1"_ustr);
    auto aFormulaInsertState = rebuildComputationalShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 3), *m_pDoc,
        translateSetFormula(ScAddress(3, 0, 0), u"=B1+C1"_ustr));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(3), aFormulaInsertState.maShadow.getFormulaCellCount());
    CPPUNIT_ASSERT(aFormulaInsertState.maShadow.findCell({ 0, 3, 0 }));

    m_pDoc->SetEmptyCell(ScAddress(0, 1, 0));
    auto aClearState = rebuildComputationalShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 4), *m_pDoc, translateClearCell(ScAddress(0, 1, 0)));
    CPPUNIT_ASSERT(!aClearState.maShadow.findCell({ 0, 0, 1 }));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalShadowRepresentativeStructuralWidening)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::rebuildComputationalShadowAfterMutation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(1, 1, 0, 2.0); // B2
    m_pDoc->SetString(2, 1, 0, u"=A1+B2"_ustr); // C2
    m_pDoc->CalcAll();

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    auto aRowInsertState = rebuildComputationalShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 1), *m_pDoc, translateInsertRows(0, 1, 1));
    CPPUNIT_ASSERT(aRowInsertState.maShadow.findCell({ 0, 0, 0 }));
    CPPUNIT_ASSERT(aRowInsertState.maShadow.findCell({ 0, 1, 2 }));
    CPPUNIT_ASSERT(aRowInsertState.maShadow.findCell({ 0, 2, 2 }));
    CPPUNIT_ASSERT(!aRowInsertState.maShadow.findCell({ 0, 1, 1 }));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aRowInsertState.maShadow.getFormulaCellCount());

    m_pDoc->DeleteCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    auto aDeleteColumnState = rebuildComputationalShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 2), *m_pDoc, translateDeleteColumns(0, 0, 1));
    CPPUNIT_ASSERT(aDeleteColumnState.maShadow.findCell({ 0, 0, 2 }));
    CPPUNIT_ASSERT(aDeleteColumnState.maShadow.findCell({ 0, 1, 2 }));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aDeleteColumnState.maShadow.getFormulaCellCount());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalShadowDifferentialValidation)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearCell;
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::rebuildComputationalShadowAfterMutation;
    using spreadsheetengine::detail::substrate::compareComputationalShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    auto assertFullMatch = [&](const spreadsheetengine::detail::facade::MutationEvent& rMutation,
                               sal_Int64 nGeneration) {
        const CalcWorkbookFacade aFacade(*m_pDoc, nGeneration);
        const auto aState = rebuildComputationalShadowAfterMutation(aFacade, *m_pDoc, rMutation);
        const auto aComparison = compareComputationalShadow(aState.maShadow, aFacade, aState.maObservation);
        CPPUNIT_ASSERT(aComparison.mbFullMatch);
    };

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->CalcAll();

    m_pDoc->SetValue(0, 0, 0, 7.0);
    assertFullMatch(translateSetScalarValue(ScAddress(0, 0, 0)), 1);

    m_pDoc->SetString(1, 0, 0, u"=A2*3"_ustr);
    assertFullMatch(translateSetFormula(ScAddress(1, 0, 0), u"=A2*3"_ustr), 2);

    m_pDoc->SetEmptyCell(ScAddress(0, 1, 0));
    assertFullMatch(translateClearCell(ScAddress(0, 1, 0)), 3);

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    assertFullMatch(translateInsertRows(0, 1, 1), 4);

    m_pDoc->DeleteCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    assertFullMatch(translateDeleteColumns(0, 0, 1), 5);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testMutableComputationalSubstrateTracksAdmittedSlice)
{
    using spreadsheetengine::compat::libreoffice::bootstrapMutableComputationalSubstrateState;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::applyMutableAuthorityTransition;
    using spreadsheetengine::detail::substrate::applyMutableLifecycleTransition;
    using spreadsheetengine::detail::substrate::applyMutableStructuralTransition;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildLifecyclePilotTransition;
    using spreadsheetengine::detail::substrate::buildStructuralPilotTransition;
    using spreadsheetengine::detail::substrate::compareAdmittedCellStorage;
    using spreadsheetengine::detail::substrate::compareComputationalShadow;
    using spreadsheetengine::detail::substrate::authoritybuilddetail::buildAuthorityExecutionIrShadow;
    using spreadsheetengine::detail::substrate::buildAuthorityPilotTransition;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(1, 0, 0, 2.0); // B1
    m_pDoc->SetString(2, 0, 0, u"=A1+B1"_ustr); // C1
    m_pDoc->CalcAll();

    auto aMutableState = bootstrapMutableComputationalSubstrateState(*m_pDoc, 0);
    CPPUNIT_ASSERT(aMutableState.mbBootstrapped);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aMutableState.maShadow.getFormulaCellCount());
    CPPUNIT_ASSERT(compareAdmittedCellStorage(aMutableState.maCellStorage, aMutableState.maShadow)
                       .mbFullMatch);

    m_pDoc->SetValue(0, 0, 0, 5.0);
    {
        spreadsheetengine::detail::substrate::AuthorityPilotInput aInput;
        aInput.maComputationalShadow = aMutableState.maShadow;
        aInput.maGraphShadow = buildDependencyGraphShadow(
            aMutableState.maShadow, aMutableState.maObservation);
        aInput.maIrShadow
            = buildAuthorityExecutionIrShadow(aMutableState.maShadow, aMutableState.maFacade);
        aInput.maMutation = translateSetScalarValue(ScAddress(0, 0, 0));
        aInput.moScalarValueAfter = spreadsheetengine::api::CellValue::number(5.0);
        aInput.mbCleanBaseline = true;

        const auto aTransition = buildAuthorityPilotTransition(aInput);
        CPPUNIT_ASSERT(applyMutableAuthorityTransition(aMutableState, aTransition));
        const auto* pA1 = aMutableState.maShadow.findCell({ 0, 0, 0 });
        CPPUNIT_ASSERT(pA1);
        CPPUNIT_ASSERT(pA1->maCell.maValue.isNumber());
        CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, pA1->maCell.maValue.mfNumber, 1e-12);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aMutableState.mnAppliedMutationCount);
        CPPUNIT_ASSERT(compareAdmittedCellStorage(aMutableState.maCellStorage,
                           aTransition.maComputationalAfter)
                           .mbFullMatch);
    }

    m_pDoc->SetString(1, 1, 0, u"=C1*2"_ustr); // B2
    {
        spreadsheetengine::detail::substrate::LifecyclePilotInput aInput;
        aInput.maComputationalShadow = aMutableState.maShadow;
        aInput.maGraphShadow = buildDependencyGraphShadow(
            aMutableState.maShadow, aMutableState.maObservation);
        aInput.maIrShadow
            = buildAuthorityExecutionIrShadow(aMutableState.maShadow, aMutableState.maFacade);
        aInput.maMutation = translateSetFormula(ScAddress(1, 1, 0), u"=C1*2"_ustr);
        aInput.mbCleanBaseline = true;

        const CalcWorkbookFacade aAfterFacade(*m_pDoc, 2);
        const auto oFormula = aAfterFacade.getFormulaCellDescriptor({ 0, 1, 1 });
        CPPUNIT_ASSERT(oFormula);
        aInput.moFormulaCachedValueAfter = oFormula->maCachedValue;

        const auto aTransition = buildLifecyclePilotTransition(aInput);
        CPPUNIT_ASSERT(applyMutableLifecycleTransition(aMutableState, aTransition));
        CPPUNIT_ASSERT(aMutableState.maShadow.findCell({ 0, 1, 1 }));
        CPPUNIT_ASSERT(aMutableState.maFacade.getFormulaCellDescriptor({ 0, 1, 1 }).has_value());
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(2), aMutableState.mnAppliedMutationCount);
        CPPUNIT_ASSERT(compareAdmittedCellStorage(aMutableState.maCellStorage,
                           aTransition.maComputationalAfter)
                           .mbFullMatch);
    }

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDependencyGraphShadowRebuildAfterSafeMutations)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearCell;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::rebuildDependencyGraphShadowAfterMutation;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=A1+A2"_ustr); // C1
    m_pDoc->CalcAll();

    auto assertGraphMatch = [&](const spreadsheetengine::detail::facade::MutationEvent& rMutation,
                                sal_Int64 nGeneration) {
        const CalcWorkbookFacade aFacade(*m_pDoc, nGeneration);
        const auto aState = rebuildDependencyGraphShadowAfterMutation(aFacade, *m_pDoc, rMutation);
        const auto aComparison = compareDependencyGraphShadow(
            aState.maGraphShadow, aState.maComputationalShadow, aState.maObservation);
        CPPUNIT_ASSERT(aComparison.mbFullMatch);
        CPPUNIT_ASSERT(aComparison.meKind != GraphComparisonKind::Mismatch);
    };

    m_pDoc->SetValue(0, 0, 0, 7.0);
    assertGraphMatch(translateSetScalarValue(ScAddress(0, 0, 0)), 1);

    m_pDoc->SetString(1, 0, 0, u"=A2*3"_ustr);
    assertGraphMatch(translateSetFormula(ScAddress(1, 0, 0), u"=A2*3"_ustr), 2);

    m_pDoc->SetString(3, 0, 0, u"=B1+C1"_ustr);
    assertGraphMatch(translateSetFormula(ScAddress(3, 0, 0), u"=B1+C1"_ustr), 3);

    m_pDoc->SetEmptyCell(ScAddress(0, 1, 0));
    assertGraphMatch(translateClearCell(ScAddress(0, 1, 0)), 4);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testGraphWiringDeltaCapturesLifecycleAdds)
{
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildGraphWiringDelta;
    using spreadsheetengine::detail::substrate::buildLifecyclePilotTransition;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr); // B1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeIr = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
        aBeforeShadow, *m_pDoc);

    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr); // C1
    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);

    spreadsheetengine::detail::substrate::LifecyclePilotInput aInput;
    aInput.maComputationalShadow = aBeforeShadow;
    aInput.maGraphShadow = aBeforeGraph;
    aInput.maIrShadow = aBeforeIr;
    aInput.maMutation = translateSetFormula(ScAddress(2, 0, 0), u"=B1+1"_ustr);
    aInput.mbCleanBaseline = true;

    const auto oFormula = aAfterFacade.getFormulaCellDescriptor({ 0, 2, 0 });
    CPPUNIT_ASSERT(oFormula);
    aInput.moFormulaCachedValueAfter = oFormula->maCachedValue;

    const auto aTransition = buildLifecyclePilotTransition(aInput);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable,
        aTransition.meVerdict);

    const auto aDelta = buildGraphWiringDelta(aTransition);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::facade::MutationKind::SetFormula,
        aDelta.maMutation.meKind);
    CPPUNIT_ASSERT(!aDelta.maFormulaTreeDeltas.empty());
    CPPUNIT_ASSERT(!aDelta.maBroadcasterNodeDeltas.empty());
    CPPUNIT_ASSERT(!aDelta.maListenerEdgeDeltas.empty());
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), aDelta.getRemoveCount());
    CPPUNIT_ASSERT(!aDelta.maRecalcPlan.maQueue.empty());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalWiringApplyRebuildsLifecycleState)
{
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::compat::libreoffice::substratewiring::WiringApplyResultKind;
    using spreadsheetengine::compat::libreoffice::substratewiring::rebuildAdmittedLiveWiring;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildGraphWiringDelta;
    using spreadsheetengine::detail::substrate::buildLifecyclePilotTransition;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr); // B1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeIr = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
        aBeforeShadow, *m_pDoc);

    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr); // C1
    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);

    spreadsheetengine::detail::substrate::LifecyclePilotInput aInput;
    aInput.maComputationalShadow = aBeforeShadow;
    aInput.maGraphShadow = aBeforeGraph;
    aInput.maIrShadow = aBeforeIr;
    aInput.maMutation = translateSetFormula(ScAddress(2, 0, 0), u"=B1+1"_ustr);
    aInput.mbCleanBaseline = true;
    const auto oFormula = aAfterFacade.getFormulaCellDescriptor({ 0, 2, 0 });
    CPPUNIT_ASSERT(oFormula);
    aInput.moFormulaCachedValueAfter = oFormula->maCachedValue;

    const auto aTransition = buildLifecyclePilotTransition(aInput);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable,
        aTransition.meVerdict);

    const auto aDelta = buildGraphWiringDelta(aTransition);
    const auto aApply = rebuildAdmittedLiveWiring(*m_pDoc, aDelta);
    CPPUNIT_ASSERT_EQUAL(WiringApplyResultKind::Applied, aApply.meKind);

    const auto aLiveObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aLiveShadow = buildComputationalWorkbookShadow(aAfterFacade, aLiveObservation);
    const auto aComparison
        = compareDependencyGraphShadow(aTransition.maGraphAfter, aLiveShadow, aLiveObservation);
    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::Exact, aComparison.meKind);
    CPPUNIT_ASSERT(aComparison.mbFullMatch);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalWiringApplyRebuildsStructuralState)
{
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::compat::libreoffice::substratewiring::WiringApplyResultKind;
    using spreadsheetengine::compat::libreoffice::substratewiring::rebuildAdmittedLiveWiring;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildGraphWiringDelta;
    using spreadsheetengine::detail::substrate::buildStructuralPilotTransition;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0); // A1
    m_pDoc->SetValue(0, 1, 0, 20.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A2*2"_ustr); // B1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeIr = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
        aBeforeShadow, *m_pDoc);

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);
    const auto aAfterObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));

    spreadsheetengine::detail::substrate::StructuralPilotInput aInput;
    aInput.maComputationalShadow = aBeforeShadow;
    aInput.maGraphShadow = aBeforeGraph;
    aInput.maIrShadow = aBeforeIr;
    aInput.maObservedAfterComputationalShadow
        = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
    aInput.maObservedAfterIrShadow
        = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
            aInput.maObservedAfterComputationalShadow, *m_pDoc);
    aInput.maMutation = translateInsertRows(0, 1, 1);
    aInput.mbCleanBaseline = true;

    const auto aTransition = buildStructuralPilotTransition(aInput, aAfterFacade, aAfterObservation);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::substrate::StructuralPilotVerdict::Applicable,
        aTransition.meVerdict);

    const auto aDelta = buildGraphWiringDelta(aTransition);
    const auto aApply = rebuildAdmittedLiveWiring(*m_pDoc, aDelta);
    CPPUNIT_ASSERT_EQUAL(WiringApplyResultKind::Applied, aApply.meKind);

    const auto aLiveObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aLiveShadow = buildComputationalWorkbookShadow(aAfterFacade, aLiveObservation);
    const auto aComparison
        = compareDependencyGraphShadow(aTransition.maGraphAfter, aLiveShadow, aLiveObservation);
    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::Exact, aComparison.meKind);
    CPPUNIT_ASSERT(aComparison.mbFullMatch);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalCellStorageMirrorRebuildsLifecycleState)
{
    using spreadsheetengine::compat::libreoffice::bootstrapMutableComputationalSubstrateState;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::substratecellstorage::CellStorageMirrorResultKind;
    using spreadsheetengine::compat::libreoffice::substratecellstorage::mirrorAdmittedCellStorage;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::compat::libreoffice::substratewiring::WiringApplyResultKind;
    using spreadsheetengine::compat::libreoffice::substratewiring::realizeAdmittedWiringContainers;
    using spreadsheetengine::detail::substrate::applyMutableLifecycleTransition;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildLifecyclePilotTransition;
    using spreadsheetengine::detail::substrate::compareComputationalShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr); // B1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeIr = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
        aBeforeShadow, *m_pDoc);

    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr); // C1
    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);

    spreadsheetengine::detail::substrate::LifecyclePilotInput aInput;
    aInput.maComputationalShadow = aBeforeShadow;
    aInput.maGraphShadow = aBeforeGraph;
    aInput.maIrShadow = aBeforeIr;
    aInput.maMutation = translateSetFormula(ScAddress(2, 0, 0), u"=B1+1"_ustr);
    aInput.mbCleanBaseline = true;
    const auto oFormula = aAfterFacade.getFormulaCellDescriptor({ 0, 2, 0 });
    CPPUNIT_ASSERT(oFormula);
    aInput.moFormulaCachedValueAfter = oFormula->maCachedValue;

    const auto aTransition = buildLifecyclePilotTransition(aInput);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable,
        aTransition.meVerdict);

    auto aMutableState = bootstrapMutableComputationalSubstrateState(aBeforeShadow);
    CPPUNIT_ASSERT(applyMutableLifecycleTransition(aMutableState, aTransition));

    m_pDoc->SetEmptyCell(ScAddress(2, 0, 0));

    const auto aMirror = mirrorAdmittedCellStorage(*m_pDoc, aMutableState.maCellStorage);
    CPPUNIT_ASSERT_EQUAL(CellStorageMirrorResultKind::Applied, aMirror.meKind);
    m_pDoc->CalcAll();

    const auto aApply = realizeAdmittedWiringContainers(*m_pDoc, aMutableState.maWiringContainers);
    CPPUNIT_ASSERT_EQUAL(WiringApplyResultKind::Applied, aApply.meKind);
    CPPUNIT_ASSERT_EQUAL(aMutableState.maWiringContainers.getBroadcasterNodeCount(),
        aApply.mnBroadcasterNodesRealized);

    const CalcWorkbookFacade aLiveFacade(*m_pDoc, 1);
    const auto aLiveObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aComputationalComparison
        = compareComputationalShadow(aMutableState.maShadow, aLiveFacade, aLiveObservation);
    CPPUNIT_ASSERT(aComputationalComparison.mbFullMatch);
    const auto aLiveShadow = buildComputationalWorkbookShadow(aLiveFacade, aLiveObservation);
    const auto aGraphComparison
        = compareDependencyGraphShadow(aMutableState.maGraphShadow, aLiveShadow, aLiveObservation);
    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::Exact, aGraphComparison.meKind);
    CPPUNIT_ASSERT(aGraphComparison.mbFullMatch);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalCellStorageMirrorRebuildsStructuralState)
{
    using spreadsheetengine::compat::libreoffice::bootstrapMutableComputationalSubstrateState;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::substratecellstorage::CellStorageMirrorResultKind;
    using spreadsheetengine::compat::libreoffice::substratecellstorage::mirrorAdmittedCellStorage;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::compat::libreoffice::substratewiring::WiringApplyResultKind;
    using spreadsheetengine::compat::libreoffice::substratewiring::realizeAdmittedWiringContainers;
    using spreadsheetengine::detail::substrate::applyMutableStructuralTransition;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildStructuralPilotTransition;
    using spreadsheetengine::detail::substrate::compareComputationalShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0); // A1
    m_pDoc->SetValue(0, 1, 0, 20.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A2*2"_ustr); // B1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeIr = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
        aBeforeShadow, *m_pDoc);

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);
    const auto aAfterObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));

    spreadsheetengine::detail::substrate::StructuralPilotInput aInput;
    aInput.maComputationalShadow = aBeforeShadow;
    aInput.maGraphShadow = aBeforeGraph;
    aInput.maIrShadow = aBeforeIr;
    aInput.maObservedAfterComputationalShadow
        = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
    aInput.maObservedAfterIrShadow
        = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
            aInput.maObservedAfterComputationalShadow, *m_pDoc);
    aInput.maMutation = translateInsertRows(0, 1, 1);
    aInput.mbCleanBaseline = true;

    const auto aTransition = buildStructuralPilotTransition(aInput, aAfterFacade, aAfterObservation);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::substrate::StructuralPilotVerdict::Applicable,
        aTransition.meVerdict);

    auto aMutableState = bootstrapMutableComputationalSubstrateState(aBeforeShadow);
    CPPUNIT_ASSERT(applyMutableStructuralTransition(aMutableState, aTransition));

    m_pDoc->SetEmptyCell(ScAddress(0, 2, 0));
    m_pDoc->SetEmptyCell(ScAddress(1, 0, 0));

    const auto aMirror = mirrorAdmittedCellStorage(*m_pDoc, aMutableState.maCellStorage);
    CPPUNIT_ASSERT_EQUAL(CellStorageMirrorResultKind::Applied, aMirror.meKind);
    m_pDoc->CalcAll();

    const auto aApply = realizeAdmittedWiringContainers(*m_pDoc, aMutableState.maWiringContainers);
    CPPUNIT_ASSERT_EQUAL(WiringApplyResultKind::Applied, aApply.meKind);
    CPPUNIT_ASSERT_EQUAL(aMutableState.maWiringContainers.getBroadcasterNodeCount(),
        aApply.mnBroadcasterNodesRealized);

    const CalcWorkbookFacade aLiveFacade(*m_pDoc, 1);
    const auto aLiveObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aComputationalComparison
        = compareComputationalShadow(aMutableState.maShadow, aLiveFacade, aLiveObservation);
    CPPUNIT_ASSERT(aComputationalComparison.mbFullMatch);
    const auto aLiveShadow = buildComputationalWorkbookShadow(aLiveFacade, aLiveObservation);
    const auto aGraphComparison
        = compareDependencyGraphShadow(aMutableState.maGraphShadow, aLiveShadow, aLiveObservation);
    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::Exact, aGraphComparison.meKind);
    CPPUNIT_ASSERT(aGraphComparison.mbFullMatch);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalFormulaCellLifetimeRealizesLifecycleInsertState)
{
    using spreadsheetengine::compat::libreoffice::bootstrapMutableComputationalSubstrateState;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::substratecellstorage::CellStorageMirrorResultKind;
    using spreadsheetengine::compat::libreoffice::substratecellstorage::mirrorAdmittedScalarCellStorage;
    using spreadsheetengine::compat::libreoffice::substrateformulalifetime::FormulaCellLifetimeResultKind;
    using spreadsheetengine::compat::libreoffice::substrateformulalifetime::
        realizeAdmittedFormulaCellLifetime;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::compat::libreoffice::substratewiring::WiringApplyResultKind;
    using spreadsheetengine::compat::libreoffice::substratewiring::realizeAdmittedWiringContainers;
    using spreadsheetengine::detail::substrate::applyMutableLifecycleTransition;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildLifecyclePilotTransition;
    using spreadsheetengine::detail::substrate::compareComputationalShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr); // B1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeIr = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
        aBeforeShadow, *m_pDoc);

    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr); // C1
    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);

    spreadsheetengine::detail::substrate::LifecyclePilotInput aInput;
    aInput.maComputationalShadow = aBeforeShadow;
    aInput.maGraphShadow = aBeforeGraph;
    aInput.maIrShadow = aBeforeIr;
    aInput.maMutation = translateSetFormula(ScAddress(2, 0, 0), u"=B1+1"_ustr);
    aInput.mbCleanBaseline = true;
    const auto oFormula = aAfterFacade.getFormulaCellDescriptor({ 0, 2, 0 });
    CPPUNIT_ASSERT(oFormula);
    aInput.moFormulaCachedValueAfter = oFormula->maCachedValue;

    const auto aTransition = buildLifecyclePilotTransition(aInput);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable,
        aTransition.meVerdict);

    auto aMutableState = bootstrapMutableComputationalSubstrateState(aBeforeShadow);
    CPPUNIT_ASSERT(applyMutableLifecycleTransition(aMutableState, aTransition));

    m_pDoc->SetValue(1, 0, 0, 99.0); // B1 no longer a formula cell
    m_pDoc->SetEmptyCell(ScAddress(2, 0, 0)); // C1 removed

    const auto aLifetime
        = realizeAdmittedFormulaCellLifetime(*m_pDoc, aMutableState.maFormulaCellLifetime);
    CPPUNIT_ASSERT_EQUAL(FormulaCellLifetimeResultKind::Applied, aLifetime.meKind);
    CPPUNIT_ASSERT_EQUAL(aMutableState.maFormulaCellLifetime.getFormulaCellCount(),
        aLifetime.mnFormulaCellsRealized);

    const auto aMirror = mirrorAdmittedScalarCellStorage(*m_pDoc, aMutableState.maCellStorage);
    CPPUNIT_ASSERT_EQUAL(CellStorageMirrorResultKind::Applied, aMirror.meKind);
    m_pDoc->CalcAll();

    const auto aApply = realizeAdmittedWiringContainers(*m_pDoc, aMutableState.maWiringContainers);
    CPPUNIT_ASSERT_EQUAL(WiringApplyResultKind::Applied, aApply.meKind);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            aTransition.maRecalcPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(*m_pDoc));

    const CalcWorkbookFacade aLiveFacade(*m_pDoc, 1);
    const auto aLiveObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aComputationalComparison
        = compareComputationalShadow(aMutableState.maShadow, aLiveFacade, aLiveObservation);
    CPPUNIT_ASSERT(aComputationalComparison.mbFullMatch);
    const auto aLiveShadow = buildComputationalWorkbookShadow(aLiveFacade, aLiveObservation);
    const auto aGraphComparison
        = compareDependencyGraphShadow(aMutableState.maGraphShadow, aLiveShadow, aLiveObservation);
    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::Exact, aGraphComparison.meKind);
    CPPUNIT_ASSERT(aGraphComparison.mbFullMatch);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalFormulaCellLifetimeRealizesLifecycleRemoveState)
{
    using spreadsheetengine::compat::libreoffice::bootstrapMutableComputationalSubstrateState;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::mutation::translateClearCell;
    using spreadsheetengine::compat::libreoffice::substratecellstorage::CellStorageMirrorResultKind;
    using spreadsheetengine::compat::libreoffice::substratecellstorage::mirrorAdmittedScalarCellStorage;
    using spreadsheetengine::compat::libreoffice::substrateformulalifetime::FormulaCellLifetimeResultKind;
    using spreadsheetengine::compat::libreoffice::substrateformulalifetime::
        realizeAdmittedFormulaCellLifetime;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::compat::libreoffice::substratewiring::WiringApplyResultKind;
    using spreadsheetengine::compat::libreoffice::substratewiring::realizeAdmittedWiringContainers;
    using spreadsheetengine::detail::substrate::applyMutableLifecycleTransition;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildLifecyclePilotTransition;
    using spreadsheetengine::detail::substrate::compareComputationalShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr); // B1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeIr = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
        aBeforeShadow, *m_pDoc);

    m_pDoc->SetEmptyCell(ScAddress(1, 0, 0));

    spreadsheetengine::detail::substrate::LifecyclePilotInput aInput;
    aInput.maComputationalShadow = aBeforeShadow;
    aInput.maGraphShadow = aBeforeGraph;
    aInput.maIrShadow = aBeforeIr;
    aInput.maMutation = translateClearCell(ScAddress(1, 0, 0));
    aInput.mbCleanBaseline = true;

    const auto aTransition = buildLifecyclePilotTransition(aInput);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable,
        aTransition.meVerdict);

    auto aMutableState = bootstrapMutableComputationalSubstrateState(aBeforeShadow);
    CPPUNIT_ASSERT(applyMutableLifecycleTransition(aMutableState, aTransition));

    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr); // Reintroduce removed formula

    const auto aLifetime
        = realizeAdmittedFormulaCellLifetime(*m_pDoc, aMutableState.maFormulaCellLifetime);
    CPPUNIT_ASSERT_EQUAL(FormulaCellLifetimeResultKind::Applied, aLifetime.meKind);
    CPPUNIT_ASSERT_EQUAL(1, aLifetime.mnFormulaCellsRemoved);

    const auto aMirror = mirrorAdmittedScalarCellStorage(*m_pDoc, aMutableState.maCellStorage);
    CPPUNIT_ASSERT_EQUAL(CellStorageMirrorResultKind::Applied, aMirror.meKind);
    m_pDoc->CalcAll();

    const auto aApply = realizeAdmittedWiringContainers(*m_pDoc, aMutableState.maWiringContainers);
    CPPUNIT_ASSERT_EQUAL(WiringApplyResultKind::Applied, aApply.meKind);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            aTransition.maRecalcPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(*m_pDoc));

    const CalcWorkbookFacade aLiveFacade(*m_pDoc, 1);
    const auto aLiveObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aComputationalComparison
        = compareComputationalShadow(aMutableState.maShadow, aLiveFacade, aLiveObservation);
    CPPUNIT_ASSERT(aComputationalComparison.mbFullMatch);
    const auto aLiveShadow = buildComputationalWorkbookShadow(aLiveFacade, aLiveObservation);
    const auto aGraphComparison
        = compareDependencyGraphShadow(aMutableState.maGraphShadow, aLiveShadow, aLiveObservation);
    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::Exact, aGraphComparison.meKind);
    CPPUNIT_ASSERT(aGraphComparison.mbFullMatch);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalFormulaCellLifetimeRealizesStructuralState)
{
    using spreadsheetengine::compat::libreoffice::bootstrapMutableComputationalSubstrateState;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;
    using spreadsheetengine::compat::libreoffice::substratecellstorage::CellStorageMirrorResultKind;
    using spreadsheetengine::compat::libreoffice::substratecellstorage::mirrorAdmittedScalarCellStorage;
    using spreadsheetengine::compat::libreoffice::substrateformulalifetime::FormulaCellLifetimeResultKind;
    using spreadsheetengine::compat::libreoffice::substrateformulalifetime::
        realizeAdmittedFormulaCellLifetime;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::compat::libreoffice::substratewiring::WiringApplyResultKind;
    using spreadsheetengine::compat::libreoffice::substratewiring::realizeAdmittedWiringContainers;
    using spreadsheetengine::detail::substrate::applyMutableStructuralTransition;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildStructuralPilotTransition;
    using spreadsheetengine::detail::substrate::compareComputationalShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=$A$1+1"_ustr); // B1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeIr = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
        aBeforeShadow, *m_pDoc);

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);
    const auto aAfterObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));

    spreadsheetengine::detail::substrate::StructuralPilotInput aInput;
    aInput.maComputationalShadow = aBeforeShadow;
    aInput.maGraphShadow = aBeforeGraph;
    aInput.maIrShadow = aBeforeIr;
    aInput.maObservedAfterComputationalShadow
        = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
    aInput.maObservedAfterIrShadow
        = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
            aInput.maObservedAfterComputationalShadow, *m_pDoc);
    aInput.maMutation = translateInsertColumns(0, 0, 1);
    aInput.mbCleanBaseline = true;

    const auto aTransition = buildStructuralPilotTransition(aInput, aAfterFacade, aAfterObservation);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::substrate::StructuralPilotVerdict::Applicable,
        aTransition.meVerdict);

    auto aMutableState = bootstrapMutableComputationalSubstrateState(aBeforeShadow);
    CPPUNIT_ASSERT(applyMutableStructuralTransition(aMutableState, aTransition));

    m_pDoc->SetEmptyCell(ScAddress(2, 0, 0)); // C1 should be formula after insert

    const auto aLifetime
        = realizeAdmittedFormulaCellLifetime(*m_pDoc, aMutableState.maFormulaCellLifetime);
    CPPUNIT_ASSERT_EQUAL(FormulaCellLifetimeResultKind::Applied, aLifetime.meKind);
    CPPUNIT_ASSERT_EQUAL(1, aLifetime.mnFormulaCellsRealized);

    const auto aMirror = mirrorAdmittedScalarCellStorage(*m_pDoc, aMutableState.maCellStorage);
    CPPUNIT_ASSERT_EQUAL(CellStorageMirrorResultKind::Applied, aMirror.meKind);
    m_pDoc->CalcAll();

    const auto aApply = realizeAdmittedWiringContainers(*m_pDoc, aMutableState.maWiringContainers);
    CPPUNIT_ASSERT_EQUAL(WiringApplyResultKind::Applied, aApply.meKind);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            aTransition.maRecalcPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(*m_pDoc));

    const CalcWorkbookFacade aLiveFacade(*m_pDoc, 1);
    const auto aLiveObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aComputationalComparison
        = compareComputationalShadow(aMutableState.maShadow, aLiveFacade, aLiveObservation);
    CPPUNIT_ASSERT(aComputationalComparison.mbFullMatch);
    const auto aLiveShadow = buildComputationalWorkbookShadow(aLiveFacade, aLiveObservation);
    const auto aGraphComparison
        = compareDependencyGraphShadow(aMutableState.maGraphShadow, aLiveShadow, aLiveObservation);
    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::Exact, aGraphComparison.meKind);
    CPPUNIT_ASSERT(aGraphComparison.mbFullMatch);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalObjectRealizationClassifierKinds)
{
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison;
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparisonKind;
    using spreadsheetengine::compat::libreoffice::substratecellstorage::CellStorageMirrorResult;
    using spreadsheetengine::compat::libreoffice::substratecellstorage::CellStorageMirrorResultKind;
    using spreadsheetengine::compat::libreoffice::substrateformulalifetime::FormulaCellLifetimeResult;
    using spreadsheetengine::compat::libreoffice::substrateformulalifetime::FormulaCellLifetimeResultKind;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        classifyObjectRealizationObservation;
    using spreadsheetengine::compat::libreoffice::substratewiring::WiringApplyResult;
    using spreadsheetengine::compat::libreoffice::substratewiring::WiringApplyResultKind;

    FormulaCellLifetimeResult aLifetime;
    aLifetime.meKind = FormulaCellLifetimeResultKind::Applied;
    CellStorageMirrorResult aCellStorage;
    aCellStorage.meKind = CellStorageMirrorResultKind::Applied;
    WiringApplyResult aWiring;
    aWiring.meKind = WiringApplyResultKind::Applied;

    ShadowComparison aQueue;
    aQueue.meKind = ShadowComparisonKind::Exact;

    spreadsheetengine::detail::substrate::ComputationalShadowComparison aComputational;
    aComputational.mbCellPopulationMatch = true;
    aComputational.mbFormulaTreeMatch = true;
    aComputational.mbFormulaTrackMatch = true;
    aComputational.mbBroadcasterMatch = true;
    aComputational.mbGroupMatch = true;
    aComputational.mbNamedRangeMatch = true;
    aComputational.mbFullMatch = true;

    spreadsheetengine::detail::substrate::DependencyGraphShadowComparison aGraph;
    aGraph.meKind = GraphComparisonKind::Exact;
    aGraph.mbFormulaNodeMatch = true;
    aGraph.mbFormulaGroupNodeMatch = true;
    aGraph.mbListenerAnchorMatch = true;
    aGraph.mbBroadcasterNodeMatch = true;
    aGraph.mbEdgeMatch = true;
    aGraph.mbFormulaTreeExactMatch = true;
    aGraph.mbFormulaTrackExactMatch = true;
    aGraph.mbFormulaTreeNormalizedMatch = true;
    aGraph.mbFormulaTrackNormalizedMatch = true;
    aGraph.mbFullMatch = true;

    spreadsheetengine::detail::substrate::BroadcasterCanonicalizationComparison aBroadcasters;
    aBroadcasters.meKind = BroadcasterCanonicalizationKind::Exact;
    aBroadcasters.mbExactMatch = true;
    aBroadcasters.mnExpectedCellBroadcasters = 1;
    aBroadcasters.mnLiveCellBroadcasters = 1;

    CPPUNIT_ASSERT_EQUAL(
        ObjectRealizationObservationKind::Exact,
        classifyObjectRealizationObservation(
            aLifetime, aCellStorage, aWiring, aQueue, aComputational, aGraph, aBroadcasters)
            .meKind);

    aQueue.meKind = ShadowComparisonKind::OrderMismatch;
    aBroadcasters.meKind = BroadcasterCanonicalizationKind::OrderingOnly;
    aBroadcasters.mbExactMatch = false;
    aBroadcasters.mbOrderingEquivalent = true;
    CPPUNIT_ASSERT_EQUAL(
        ObjectRealizationObservationKind::OrderingOnly,
        classifyObjectRealizationObservation(
            aLifetime, aCellStorage, aWiring, aQueue, aComputational, aGraph, aBroadcasters)
            .meKind);

    aQueue.meKind = ShadowComparisonKind::Exact;
    aBroadcasters.meKind = BroadcasterCanonicalizationKind::Unknown;
    aBroadcasters.mbOrderingEquivalent = false;
    aBroadcasters.mnExpectedCellBroadcasters = 2;
    aBroadcasters.mnLiveCellBroadcasters = 1;
    CPPUNIT_ASSERT_EQUAL(
        ObjectRealizationObservationKind::MissingRealizedObjects,
        classifyObjectRealizationObservation(
            aLifetime, aCellStorage, aWiring, aQueue, aComputational, aGraph, aBroadcasters)
            .meKind);

    aBroadcasters.meKind = BroadcasterCanonicalizationKind::DuplicateMaterializationOnly;
    aBroadcasters.mnExpectedCellBroadcasters = 1;
    aBroadcasters.mnLiveCellBroadcasters = 1;
    aBroadcasters.mnLiveDuplicateBroadcasterCount = 1;
    CPPUNIT_ASSERT_EQUAL(
        ObjectRealizationObservationKind::HostOnlyRepairOrReconstruction,
        classifyObjectRealizationObservation(
            aLifetime, aCellStorage, aWiring, aQueue, aComputational, aGraph, aBroadcasters)
            .meKind);

    aBroadcasters.meKind = BroadcasterCanonicalizationKind::Exact;
    aBroadcasters.mbExactMatch = true;
    aBroadcasters.mnLiveDuplicateBroadcasterCount = 0;
    aComputational.mbFullMatch = false;
    CPPUNIT_ASSERT_EQUAL(
        ObjectRealizationObservationKind::QueueOrStateMismatch,
        classifyObjectRealizationObservation(
            aLifetime, aCellStorage, aWiring, aQueue, aComputational, aGraph, aBroadcasters)
            .meKind);

    aComputational.mbFullMatch = true;
    aLifetime.meKind = FormulaCellLifetimeResultKind::RejectedOutOfContract;
    CPPUNIT_ASSERT_EQUAL(
        ObjectRealizationObservationKind::OutOfContract,
        classifyObjectRealizationObservation(
            aLifetime, aCellStorage, aWiring, aQueue, aComputational, aGraph, aBroadcasters)
            .meKind);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalObjectRealizationObservationExactState)
{
    using spreadsheetengine::compat::libreoffice::bootstrapMutableComputationalSubstrateState;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::recalcshadow::detail::comparePlanToDocument;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        AdmittedObjectRealization;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        ObjectRealizationResultKind;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        buildAdmittedObjectRealization;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        classifyObjectRealizationObservation;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        realizeAdmittedObjectRealization;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::applyMutableLifecycleTransition;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildLifecyclePilotTransition;
    using spreadsheetengine::detail::substrate::compareComputationalShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::detail::compareBroadcasterCanonicalization;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr); // B1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeIr = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
        aBeforeShadow, *m_pDoc);

    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr); // C1
    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);

    spreadsheetengine::detail::substrate::LifecyclePilotInput aInput;
    aInput.maComputationalShadow = aBeforeShadow;
    aInput.maGraphShadow = aBeforeGraph;
    aInput.maIrShadow = aBeforeIr;
    aInput.maMutation = translateSetFormula(ScAddress(2, 0, 0), u"=B1+1"_ustr);
    aInput.mbCleanBaseline = true;
    const auto oFormula = aAfterFacade.getFormulaCellDescriptor({ 0, 2, 0 });
    CPPUNIT_ASSERT(oFormula);
    aInput.moFormulaCachedValueAfter = oFormula->maCachedValue;

    const auto aTransition = buildLifecyclePilotTransition(aInput);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable,
        aTransition.meVerdict);

    auto aMutableState = bootstrapMutableComputationalSubstrateState(aBeforeShadow);
    CPPUNIT_ASSERT(applyMutableLifecycleTransition(aMutableState, aTransition));
    const AdmittedObjectRealization aObjectRealization
        = buildAdmittedObjectRealization(aMutableState);

    m_pDoc->SetValue(1, 0, 0, 99.0); // B1 no longer a formula cell
    m_pDoc->SetEmptyCell(ScAddress(2, 0, 0)); // C1 removed

    const auto aRealization = realizeAdmittedObjectRealization(*m_pDoc, aObjectRealization);
    CPPUNIT_ASSERT_EQUAL(ObjectRealizationResultKind::Applied, aRealization.meKind);

    const CalcWorkbookFacade aLiveFacade(*m_pDoc, 1);
    const auto aLiveObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aQueueComparison = comparePlanToDocument(aTransition.maRecalcPlan, aLiveFacade, *m_pDoc);
    const auto aComputationalComparison
        = compareComputationalShadow(aMutableState.maShadow, aLiveFacade, aLiveObservation);
    const auto aLiveShadow = buildComputationalWorkbookShadow(aLiveFacade, aLiveObservation);
    const auto aGraphComparison
        = compareDependencyGraphShadow(aMutableState.maGraphShadow, aLiveShadow, aLiveObservation);
    const auto aBroadcasterComparison
        = compareBroadcasterCanonicalization(aMutableState.maShadow, aLiveObservation);
    const auto aObjectObservation = classifyObjectRealizationObservation(
        aRealization, aQueueComparison, aComputationalComparison, aGraphComparison,
        aBroadcasterComparison);

    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        describeObjectRealizationObservation(aObjectObservation), ObjectRealizationObservationKind::Exact,
        aObjectObservation.meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalRollbackObservationClassifierKinds)
{
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison;
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparisonKind;
    using spreadsheetengine::compat::libreoffice::substratecellstorage::CellStorageMirrorResultKind;
    using spreadsheetengine::compat::libreoffice::substrateformulalifetime::
        FormulaCellLifetimeResultKind;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        ObjectRealizationResult;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        ObjectRealizationResultKind;
    using spreadsheetengine::compat::libreoffice::substraterollback::classifyRollbackObservation;
    using spreadsheetengine::compat::libreoffice::substratewiring::WiringApplyResultKind;

    ObjectRealizationResult aRealization;
    aRealization.meKind = ObjectRealizationResultKind::Applied;
    aRealization.maFormulaCellLifetime.meKind = FormulaCellLifetimeResultKind::Applied;
    aRealization.maCellStorage.meKind = CellStorageMirrorResultKind::Applied;
    aRealization.maWiring.meKind = WiringApplyResultKind::Applied;

    ShadowComparison aQueue;
    aQueue.meKind = ShadowComparisonKind::Exact;

    spreadsheetengine::detail::substrate::ComputationalShadowComparison aComputational;
    aComputational.mbCellPopulationMatch = true;
    aComputational.mbFormulaTreeMatch = true;
    aComputational.mbFormulaTrackMatch = true;
    aComputational.mbBroadcasterMatch = true;
    aComputational.mbGroupMatch = true;
    aComputational.mbNamedRangeMatch = true;
    aComputational.mbFullMatch = true;

    spreadsheetengine::detail::substrate::DependencyGraphShadowComparison aGraph;
    aGraph.meKind = GraphComparisonKind::Exact;
    aGraph.mbFormulaNodeMatch = true;
    aGraph.mbFormulaGroupNodeMatch = true;
    aGraph.mbListenerAnchorMatch = true;
    aGraph.mbBroadcasterNodeMatch = true;
    aGraph.mbEdgeMatch = true;
    aGraph.mbFormulaTreeExactMatch = true;
    aGraph.mbFormulaTrackExactMatch = true;
    aGraph.mbFormulaTreeNormalizedMatch = true;
    aGraph.mbFormulaTrackNormalizedMatch = true;
    aGraph.mbFullMatch = true;

    spreadsheetengine::detail::substrate::BroadcasterCanonicalizationComparison aBroadcasters;
    aBroadcasters.meKind = BroadcasterCanonicalizationKind::Exact;
    aBroadcasters.mbExactMatch = true;
    aBroadcasters.mnExpectedCellBroadcasters = 1;
    aBroadcasters.mnLiveCellBroadcasters = 1;

    CPPUNIT_ASSERT_EQUAL(
        RollbackObservationKind::Exact,
        classifyRollbackObservation(
            aRealization, aQueue, aComputational, aGraph, aBroadcasters)
            .meKind);

    aQueue.meKind = ShadowComparisonKind::OrderMismatch;
    aBroadcasters.meKind = BroadcasterCanonicalizationKind::OrderingOnly;
    aBroadcasters.mbExactMatch = false;
    aBroadcasters.mbOrderingEquivalent = true;
    CPPUNIT_ASSERT_EQUAL(
        RollbackObservationKind::OrderingOnly,
        classifyRollbackObservation(
            aRealization, aQueue, aComputational, aGraph, aBroadcasters)
            .meKind);

    aQueue.meKind = ShadowComparisonKind::Exact;
    aBroadcasters.meKind = BroadcasterCanonicalizationKind::Unknown;
    aBroadcasters.mbOrderingEquivalent = false;
    aBroadcasters.mnExpectedCellBroadcasters = 2;
    aBroadcasters.mnLiveCellBroadcasters = 1;
    CPPUNIT_ASSERT_EQUAL(
        RollbackObservationKind::MissingRestoredObjects,
        classifyRollbackObservation(
            aRealization, aQueue, aComputational, aGraph, aBroadcasters)
            .meKind);

    aBroadcasters.meKind = BroadcasterCanonicalizationKind::DuplicateMaterializationOnly;
    aBroadcasters.mnExpectedCellBroadcasters = 1;
    aBroadcasters.mnLiveCellBroadcasters = 1;
    aBroadcasters.mnLiveDuplicateBroadcasterCount = 1;
    CPPUNIT_ASSERT_EQUAL(
        RollbackObservationKind::HostOnlyRollbackReconstruction,
        classifyRollbackObservation(
            aRealization, aQueue, aComputational, aGraph, aBroadcasters)
            .meKind);

    aBroadcasters.meKind = BroadcasterCanonicalizationKind::Exact;
    aBroadcasters.mbExactMatch = true;
    aBroadcasters.mnLiveDuplicateBroadcasterCount = 0;
    aComputational.mbFullMatch = false;
    CPPUNIT_ASSERT_EQUAL(
        RollbackObservationKind::QueueOrStateMismatch,
        classifyRollbackObservation(
            aRealization, aQueue, aComputational, aGraph, aBroadcasters)
            .meKind);

    aComputational.mbFullMatch = true;
    aRealization.meKind = ObjectRealizationResultKind::RejectedOutOfContract;
    aRealization.maReason = u"rollback_out_of_contract";
    CPPUNIT_ASSERT_EQUAL(
        RollbackObservationKind::OutOfContract,
        classifyRollbackObservation(
            aRealization, aQueue, aComputational, aGraph, aBroadcasters)
            .meKind);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalPrimitiveRealizationObservationClassifierKinds)
{
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        ObjectRealizationObservation;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        classifyPrimitiveRealizationObservation;

    ObjectRealizationObservation aObjectObservation;
    aObjectObservation.meKind = ObjectRealizationObservationKind::Exact;
    aObjectObservation.mbQueueExact = true;
    aObjectObservation.mbComputationalFullMatch = true;
    aObjectObservation.mbGraphFullMatch = true;
    aObjectObservation.mbBroadcasterExact = true;

    const auto aExact
        = classifyPrimitiveRealizationObservation(true, aObjectObservation);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveRealizationObservation(aExact),
        PrimitiveRealizationObservationKind::Exact, aExact.meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::OrderingOnly;
    CPPUNIT_ASSERT_EQUAL(
        PrimitiveRealizationObservationKind::OrderingOnly,
        classifyPrimitiveRealizationObservation(true, aObjectObservation).meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::HostOnlyRepairOrReconstruction;
    CPPUNIT_ASSERT_EQUAL(
        PrimitiveRealizationObservationKind::HiddenHostRealizationOrchestration,
        classifyPrimitiveRealizationObservation(true, aObjectObservation).meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::MissingRealizedObjects;
    CPPUNIT_ASSERT_EQUAL(
        PrimitiveRealizationObservationKind::MissingRealizedObjects,
        classifyPrimitiveRealizationObservation(true, aObjectObservation).meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::QueueOrStateMismatch;
    CPPUNIT_ASSERT_EQUAL(
        PrimitiveRealizationObservationKind::QueueOrStateMismatch,
        classifyPrimitiveRealizationObservation(true, aObjectObservation).meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::OutOfContract;
    aObjectObservation.maReason = u"primitive_realization_out_of_contract";
    CPPUNIT_ASSERT_EQUAL(
        PrimitiveRealizationObservationKind::OutOfContract,
        classifyPrimitiveRealizationObservation(true, aObjectObservation).meKind);

    CPPUNIT_ASSERT_EQUAL(
        PrimitiveRealizationObservationKind::OutOfContract,
        classifyPrimitiveRealizationObservation(false, std::nullopt,
            u"missing_primitive_realization")
            .meKind);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalRollbackObservationExactRestore)
{
    using spreadsheetengine::compat::libreoffice::bootstrapMutableComputationalSubstrateState;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::recalcqueue::captureFormulaState;
    using spreadsheetengine::compat::libreoffice::substraterollback::applyAdmittedRollback;
    using spreadsheetengine::compat::libreoffice::substraterollback::buildAdmittedRollbackRecord;
    using spreadsheetengine::compat::libreoffice::substraterollback::classifyRollbackObservation;
    using spreadsheetengine::compat::libreoffice::substraterollback::compareRollbackQueueToDocument;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::compareComputationalShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::detail::compareBroadcasterCanonicalization;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeFormulaState = captureFormulaState(*m_pDoc);
    const auto aBeforeMutableState = bootstrapMutableComputationalSubstrateState(aBeforeShadow);
    const auto aBeforeRollback = buildAdmittedRollbackRecord(aBeforeMutableState, aBeforeFormulaState);

    m_pDoc->SetValue(0, 0, 0, 9.0);
    m_pDoc->SetValue(1, 0, 0, 99.0);
    m_pDoc->SetEmptyCell(ScAddress(2, 0, 0));

    const auto aRollbackResult = applyAdmittedRollback(*m_pDoc, aBeforeRollback);

    const CalcWorkbookFacade aRestoredFacade(*m_pDoc, 0);
    const auto aRestoredObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aQueueComparison = compareRollbackQueueToDocument(aBeforeFormulaState, *m_pDoc);
    const auto aComputationalComparison
        = compareComputationalShadow(aBeforeMutableState.maShadow, aRestoredFacade, aRestoredObservation);
    const auto aRestoredShadow = buildComputationalWorkbookShadow(aRestoredFacade, aRestoredObservation);
    const auto aGraphComparison
        = compareDependencyGraphShadow(aBeforeGraph, aRestoredShadow, aRestoredObservation);
    const auto aBroadcasterComparison
        = compareBroadcasterCanonicalization(aBeforeMutableState.maShadow, aRestoredObservation);
    const auto aRollbackObservation = classifyRollbackObservation(
        aRollbackResult, aQueueComparison, aComputationalComparison, aGraphComparison,
        aBroadcasterComparison);

    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeRollbackObservation(aRollbackObservation),
        RollbackObservationKind::Exact, aRollbackObservation.meKind);
    assertFormulaStateEqual(aBeforeFormulaState, captureFormulaState(*m_pDoc));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalPrimitiveRollbackObservationClassifierKinds)
{
    using spreadsheetengine::compat::libreoffice::substraterollback::RollbackObservation;
    using spreadsheetengine::compat::libreoffice::substraterollback::
        classifyPrimitiveRollbackObservation;

    RollbackObservation aRollbackObservation;
    aRollbackObservation.meKind = RollbackObservationKind::Exact;
    aRollbackObservation.mbQueueExact = true;
    aRollbackObservation.mbComputationalFullMatch = true;
    aRollbackObservation.mbGraphFullMatch = true;
    aRollbackObservation.mbBroadcasterExact = true;

    const auto aExact = classifyPrimitiveRollbackObservation(true, aRollbackObservation);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveRollbackObservation(aExact),
        PrimitiveRollbackObservationKind::Exact, aExact.meKind);

    aRollbackObservation.meKind = RollbackObservationKind::OrderingOnly;
    CPPUNIT_ASSERT_EQUAL(
        PrimitiveRollbackObservationKind::OrderingOnly,
        classifyPrimitiveRollbackObservation(true, aRollbackObservation).meKind);

    aRollbackObservation.meKind = RollbackObservationKind::HostOnlyRollbackReconstruction;
    CPPUNIT_ASSERT_EQUAL(
        PrimitiveRollbackObservationKind::HiddenHostRollbackOrchestration,
        classifyPrimitiveRollbackObservation(true, aRollbackObservation).meKind);

    aRollbackObservation.meKind = RollbackObservationKind::MissingRestoredObjects;
    CPPUNIT_ASSERT_EQUAL(
        PrimitiveRollbackObservationKind::MissingRestoredObjects,
        classifyPrimitiveRollbackObservation(true, aRollbackObservation).meKind);

    aRollbackObservation.meKind = RollbackObservationKind::QueueOrStateMismatch;
    CPPUNIT_ASSERT_EQUAL(
        PrimitiveRollbackObservationKind::QueueOrStateMismatch,
        classifyPrimitiveRollbackObservation(true, aRollbackObservation).meKind);

    aRollbackObservation.meKind = RollbackObservationKind::OutOfContract;
    aRollbackObservation.maReason = u"primitive_rollback_out_of_contract";
    CPPUNIT_ASSERT_EQUAL(
        PrimitiveRollbackObservationKind::OutOfContract,
        classifyPrimitiveRollbackObservation(true, aRollbackObservation).meKind);

    CPPUNIT_ASSERT_EQUAL(
        PrimitiveRollbackObservationKind::OutOfContract,
        classifyPrimitiveRollbackObservation(false, std::nullopt,
            u"missing_primitive_rollback")
            .meKind);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalRollbackObservationClassifiesMissingRestoredObjects)
{
    using spreadsheetengine::compat::libreoffice::bootstrapMutableComputationalSubstrateState;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::recalcqueue::captureFormulaState;
    using spreadsheetengine::compat::libreoffice::substraterollback::applyAdmittedRollback;
    using spreadsheetengine::compat::libreoffice::substraterollback::buildAdmittedRollbackRecord;
    using spreadsheetengine::compat::libreoffice::substraterollback::classifyRollbackObservation;
    using spreadsheetengine::compat::libreoffice::substraterollback::compareRollbackQueueToDocument;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::compareComputationalShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::detail::compareBroadcasterCanonicalization;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeFormulaState = captureFormulaState(*m_pDoc);
    const auto aBeforeMutableState = bootstrapMutableComputationalSubstrateState(aBeforeShadow);
    const auto aBeforeRollback = buildAdmittedRollbackRecord(aBeforeMutableState, aBeforeFormulaState);

    m_pDoc->SetValue(0, 0, 0, 9.0);
    m_pDoc->SetValue(1, 0, 0, 99.0);
    m_pDoc->SetEmptyCell(ScAddress(2, 0, 0));

    const auto aRollbackResult = applyAdmittedRollback(*m_pDoc, aBeforeRollback);
    m_pDoc->SetEmptyCell(ScAddress(2, 0, 0));

    const CalcWorkbookFacade aRestoredFacade(*m_pDoc, 0);
    const auto aRestoredObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aQueueComparison = compareRollbackQueueToDocument(aBeforeFormulaState, *m_pDoc);
    const auto aComputationalComparison
        = compareComputationalShadow(aBeforeMutableState.maShadow, aRestoredFacade, aRestoredObservation);
    const auto aRestoredShadow = buildComputationalWorkbookShadow(aRestoredFacade, aRestoredObservation);
    const auto aGraphComparison
        = compareDependencyGraphShadow(aBeforeGraph, aRestoredShadow, aRestoredObservation);
    const auto aBroadcasterComparison
        = compareBroadcasterCanonicalization(aBeforeMutableState.maShadow, aRestoredObservation);
    const auto aRollbackObservation = classifyRollbackObservation(
        aRollbackResult, aQueueComparison, aComputationalComparison, aGraphComparison,
        aBroadcasterComparison);

    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeRollbackObservation(aRollbackObservation),
        RollbackObservationKind::MissingRestoredObjects, aRollbackObservation.meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalRawMutationObservationClassifierKinds)
{
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison;
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparisonKind;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::ObjectRealizationObservation;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::ObjectRealizationObservationKind;
    using spreadsheetengine::compat::libreoffice::substraterawmutation::classifyRawMutationObservation;
    using spreadsheetengine::compat::libreoffice::substraterollback::RollbackObservation;

    ShadowComparison aQueue;
    aQueue.meKind = ShadowComparisonKind::Exact;

    spreadsheetengine::detail::substrate::ComputationalShadowComparison aComputational;
    aComputational.mbCellPopulationMatch = true;
    aComputational.mbFormulaTreeMatch = true;
    aComputational.mbFormulaTrackMatch = true;
    aComputational.mbBroadcasterMatch = true;
    aComputational.mbGroupMatch = true;
    aComputational.mbNamedRangeMatch = true;
    aComputational.mbFullMatch = true;

    spreadsheetengine::detail::substrate::DependencyGraphShadowComparison aGraph;
    aGraph.meKind = GraphComparisonKind::Exact;
    aGraph.mbFormulaNodeMatch = true;
    aGraph.mbFormulaGroupNodeMatch = true;
    aGraph.mbListenerAnchorMatch = true;
    aGraph.mbBroadcasterNodeMatch = true;
    aGraph.mbEdgeMatch = true;
    aGraph.mbFormulaTreeExactMatch = true;
    aGraph.mbFormulaTrackExactMatch = true;
    aGraph.mbFormulaTreeNormalizedMatch = true;
    aGraph.mbFormulaTrackNormalizedMatch = true;
    aGraph.mbFullMatch = true;

    ObjectRealizationObservation aObjectObservation;
    aObjectObservation.meKind = ObjectRealizationObservationKind::Exact;

    CPPUNIT_ASSERT_EQUAL(
        RawMutationObservationKind::Exact,
        classifyRawMutationObservation(true, false, aQueue, aComputational, aGraph, aObjectObservation,
            std::nullopt)
            .meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::OrderingOnly;
    CPPUNIT_ASSERT_EQUAL(
        RawMutationObservationKind::OrderingOnly,
        classifyRawMutationObservation(true, false, aQueue, aComputational, aGraph, aObjectObservation,
            std::nullopt)
            .meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::MissingRealizedObjects;
    CPPUNIT_ASSERT_EQUAL(
        RawMutationObservationKind::MissingRealizedOrRolledBackObjects,
        classifyRawMutationObservation(true, false, aQueue, aComputational, aGraph, aObjectObservation,
            std::nullopt)
            .meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::HostOnlyRepairOrReconstruction;
    CPPUNIT_ASSERT_EQUAL(
        RawMutationObservationKind::HiddenHostMutationReconstruction,
        classifyRawMutationObservation(true, false, aQueue, aComputational, aGraph, aObjectObservation,
            std::nullopt)
            .meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::QueueOrStateMismatch;
    CPPUNIT_ASSERT_EQUAL(
        RawMutationObservationKind::QueueOrStateMismatch,
        classifyRawMutationObservation(true, false, aQueue, aComputational, aGraph, aObjectObservation,
            std::nullopt)
            .meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::OutOfContract;
    aObjectObservation.maReason = u"raw_out_of_contract";
    CPPUNIT_ASSERT_EQUAL(
        RawMutationObservationKind::OutOfContract,
        classifyRawMutationObservation(true, false, aQueue, aComputational, aGraph, aObjectObservation,
            std::nullopt)
            .meKind);

    RollbackObservation aRollbackObservation;
    aRollbackObservation.meKind = RollbackObservationKind::Exact;
    CPPUNIT_ASSERT_EQUAL(
        RawMutationObservationKind::Exact,
        classifyRawMutationObservation(true, true, aQueue, aComputational, aGraph, std::nullopt,
            aRollbackObservation)
            .meKind);

    aRollbackObservation.meKind = RollbackObservationKind::HostOnlyRollbackReconstruction;
    CPPUNIT_ASSERT_EQUAL(
        RawMutationObservationKind::HiddenHostMutationReconstruction,
        classifyRawMutationObservation(true, true, aQueue, aComputational, aGraph, std::nullopt,
            aRollbackObservation)
            .meKind);

    CPPUNIT_ASSERT_EQUAL(
        RawMutationObservationKind::OutOfContract,
        classifyRawMutationObservation(false, false, aQueue, aComputational, aGraph, std::nullopt,
            std::nullopt, u"missing_raw_mutation_record")
            .meKind);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalLiveApplyObservationClassifierKinds)
{
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::ObjectRealizationObservation;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::ObjectRealizationObservationKind;
    using spreadsheetengine::compat::libreoffice::substrateliveapply::classifyLiveApplyObservation;
    using spreadsheetengine::compat::libreoffice::substraterawmutation::RawMutationObservation;
    using spreadsheetengine::compat::libreoffice::substraterawmutation::RawMutationObservationKind;
    using spreadsheetengine::compat::libreoffice::substraterollback::RollbackObservation;
    using spreadsheetengine::compat::libreoffice::substraterollback::RollbackObservationKind;

    RawMutationObservation aRawObservation;
    aRawObservation.meKind = RawMutationObservationKind::Exact;
    aRawObservation.mbQueueExact = true;
    aRawObservation.mbComputationalFullMatch = true;
    aRawObservation.mbGraphFullMatch = true;

    ObjectRealizationObservation aObjectObservation;
    aObjectObservation.meKind = ObjectRealizationObservationKind::Exact;
    const auto aExactApply
        = classifyLiveApplyObservation(aRawObservation, aObjectObservation, std::nullopt);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeLiveApplyObservation(aExactApply),
        LiveApplyObservationKind::Exact, aExactApply.meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::OrderingOnly;
    CPPUNIT_ASSERT_EQUAL(
        LiveApplyObservationKind::OrderingOnly,
        classifyLiveApplyObservation(aRawObservation, aObjectObservation, std::nullopt).meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::MissingRealizedObjects;
    CPPUNIT_ASSERT_EQUAL(
        LiveApplyObservationKind::MissingRealizedOrRolledBackObjects,
        classifyLiveApplyObservation(aRawObservation, aObjectObservation, std::nullopt).meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::HostOnlyRepairOrReconstruction;
    CPPUNIT_ASSERT_EQUAL(
        LiveApplyObservationKind::HiddenHostApplyOrchestration,
        classifyLiveApplyObservation(aRawObservation, aObjectObservation, std::nullopt).meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::QueueOrStateMismatch;
    CPPUNIT_ASSERT_EQUAL(
        LiveApplyObservationKind::QueueOrStateMismatch,
        classifyLiveApplyObservation(aRawObservation, aObjectObservation, std::nullopt).meKind);

    aObjectObservation.meKind = ObjectRealizationObservationKind::OutOfContract;
    aObjectObservation.maReason = u"object_out_of_contract";
    CPPUNIT_ASSERT_EQUAL(
        LiveApplyObservationKind::OutOfContract,
        classifyLiveApplyObservation(aRawObservation, aObjectObservation, std::nullopt).meKind);

    RollbackObservation aRollbackObservation;
    aRollbackObservation.meKind = RollbackObservationKind::Exact;
    CPPUNIT_ASSERT_EQUAL(
        LiveApplyObservationKind::Exact,
        classifyLiveApplyObservation(aRawObservation, std::nullopt, aRollbackObservation).meKind);

    aRollbackObservation.meKind = RollbackObservationKind::HostOnlyRollbackReconstruction;
    CPPUNIT_ASSERT_EQUAL(
        LiveApplyObservationKind::HiddenHostApplyOrchestration,
        classifyLiveApplyObservation(aRawObservation, std::nullopt, aRollbackObservation).meKind);

    aRawObservation.meKind = RawMutationObservationKind::OutOfContract;
    aRawObservation.maReason = u"raw_out_of_contract";
    CPPUNIT_ASSERT_EQUAL(
        LiveApplyObservationKind::OutOfContract,
        classifyLiveApplyObservation(aRawObservation, std::nullopt, std::nullopt).meKind);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalFinalVerificationObservationClassifierKinds)
{
    using spreadsheetengine::compat::libreoffice::substratefinalverification::
        FinalVerificationObservation;
    using spreadsheetengine::compat::libreoffice::substratefinalverification::
        classifyFinalVerificationObservation;
    using spreadsheetengine::compat::libreoffice::substrateliveapply::LiveApplyObservation;
    using spreadsheetengine::compat::libreoffice::substrateliveapply::LiveApplyObservationKind;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        PrimitiveRealizationObservation;
    using spreadsheetengine::compat::libreoffice::substraterollback::
        PrimitiveRollbackObservation;

    spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison aQueue;
    aQueue.meKind = RecalcShadowComparisonKind::Exact;

    spreadsheetengine::detail::substrate::ComputationalShadowComparison aComputational;
    aComputational.mbCellPopulationMatch = true;
    aComputational.mbFormulaTreeMatch = true;
    aComputational.mbFormulaTrackMatch = true;
    aComputational.mbBroadcasterMatch = true;
    aComputational.mbGroupMatch = true;
    aComputational.mbNamedRangeMatch = true;
    aComputational.mbFullMatch = true;

    spreadsheetengine::detail::substrate::DependencyGraphShadowComparison aGraph;
    aGraph.meKind = GraphComparisonKind::Exact;
    aGraph.mbFormulaNodeMatch = true;
    aGraph.mbFormulaGroupNodeMatch = true;
    aGraph.mbListenerAnchorMatch = true;
    aGraph.mbBroadcasterNodeMatch = true;
    aGraph.mbEdgeMatch = true;
    aGraph.mbFormulaTreeExactMatch = true;
    aGraph.mbFormulaTrackExactMatch = true;
    aGraph.mbFormulaTreeNormalizedMatch = true;
    aGraph.mbFormulaTrackNormalizedMatch = true;
    aGraph.mbFullMatch = true;

    spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison aIr;
    aIr.meKind = ExecutionIrComparisonKind::Exact;
    aIr.mbSnapshotMatch = true;
    aIr.mbGrammarMatch = true;
    aIr.mbFormulaRecordExactMatch = true;
    aIr.mbFormulaRecordNormalizedMatch = true;
    aIr.mbFormulaGroupExactMatch = true;
    aIr.mbFormulaGroupNormalizedMatch = true;
    aIr.mbBuildFailureExactMatch = true;
    aIr.mbBuildFailureNormalizedMatch = true;
    aIr.mbFullMatch = true;

    spreadsheetengine::detail::substrate::BroadcasterCanonicalizationComparison aBroadcasters;
    aBroadcasters.meKind = BroadcasterCanonicalizationKind::Exact;
    aBroadcasters.mbExactMatch = true;

    LiveApplyObservation aLiveApply;
    aLiveApply.meKind = LiveApplyObservationKind::Exact;
    aLiveApply.mbQueueExact = true;
    aLiveApply.mbComputationalFullMatch = true;
    aLiveApply.mbGraphFullMatch = true;
    aLiveApply.mbRawMutationExact = true;
    aLiveApply.mbObjectRealizationExact = true;

    PrimitiveRealizationObservation aPrimitiveRealization;
    aPrimitiveRealization.meKind = PrimitiveRealizationObservationKind::Exact;
    aPrimitiveRealization.mbPrimitiveRealizationApplied = true;
    aPrimitiveRealization.mbObjectRealizationExact = true;
    aPrimitiveRealization.mbQueueExact = true;
    aPrimitiveRealization.mbComputationalFullMatch = true;
    aPrimitiveRealization.mbGraphFullMatch = true;
    aPrimitiveRealization.mbBroadcasterExact = true;

    auto aExact = classifyFinalVerificationObservation(true, aQueue, aComputational, aGraph, aIr,
        aBroadcasters, aLiveApply, aPrimitiveRealization, std::nullopt);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeFinalVerificationObservation(aExact),
        FinalVerificationObservationKind::Exact, aExact.meKind);

    aGraph.meKind = GraphComparisonKind::NormalizedEquivalent;
    auto aNormalized = classifyFinalVerificationObservation(true, aQueue, aComputational, aGraph, aIr,
        aBroadcasters, aLiveApply, aPrimitiveRealization, std::nullopt);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeFinalVerificationObservation(aNormalized),
        FinalVerificationObservationKind::NormalizedEquivalent, aNormalized.meKind);

    aGraph.meKind = GraphComparisonKind::Exact;
    aQueue.meKind = RecalcShadowComparisonKind::OrderMismatch;
    aBroadcasters.meKind = BroadcasterCanonicalizationKind::OrderingOnly;
    aBroadcasters.mbExactMatch = false;
    aBroadcasters.mbOrderingEquivalent = true;
    auto aOrdering = classifyFinalVerificationObservation(true, aQueue, aComputational, aGraph, aIr,
        aBroadcasters, aLiveApply, aPrimitiveRealization, std::nullopt);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeFinalVerificationObservation(aOrdering),
        FinalVerificationObservationKind::OrderingOnly, aOrdering.meKind);

    aQueue.meKind = RecalcShadowComparisonKind::Exact;
    aBroadcasters.meKind = BroadcasterCanonicalizationKind::Exact;
    aBroadcasters.mbExactMatch = true;
    aBroadcasters.mbOrderingEquivalent = false;
    aLiveApply.meKind = LiveApplyObservationKind::HiddenHostApplyOrchestration;
    auto aHiddenHost = classifyFinalVerificationObservation(true, aQueue, aComputational, aGraph, aIr,
        aBroadcasters, aLiveApply, aPrimitiveRealization, std::nullopt);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeFinalVerificationObservation(aHiddenHost),
        FinalVerificationObservationKind::HiddenHostVerificationOrchestration, aHiddenHost.meKind);

    aLiveApply.meKind = LiveApplyObservationKind::Exact;
    aPrimitiveRealization.meKind = PrimitiveRealizationObservationKind::MissingRealizedObjects;
    auto aMissing = classifyFinalVerificationObservation(true, aQueue, aComputational, aGraph, aIr,
        aBroadcasters, aLiveApply, aPrimitiveRealization, std::nullopt);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeFinalVerificationObservation(aMissing),
        FinalVerificationObservationKind::MissingVerificationInputs, aMissing.meKind);

    aPrimitiveRealization.meKind = PrimitiveRealizationObservationKind::Exact;
    aLiveApply.meKind = LiveApplyObservationKind::QueueOrStateMismatch;
    auto aMismatch = classifyFinalVerificationObservation(true, aQueue, aComputational, aGraph, aIr,
        aBroadcasters, aLiveApply, aPrimitiveRealization, std::nullopt);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeFinalVerificationObservation(aMismatch),
        FinalVerificationObservationKind::QueueOrStateMismatch, aMismatch.meKind);

    const auto aOutOfContract = classifyFinalVerificationObservation(
        false, aQueue, aComputational, aGraph, aIr, aBroadcasters, std::nullopt, std::nullopt,
        std::nullopt, u"verification_not_executed");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeFinalVerificationObservation(aOutOfContract),
        FinalVerificationObservationKind::OutOfContract, aOutOfContract.meKind);

    PrimitiveRollbackObservation aPrimitiveRollback;
    aPrimitiveRollback.meKind = PrimitiveRollbackObservationKind::Exact;
    aPrimitiveRollback.mbPrimitiveRollbackApplied = true;
    aPrimitiveRollback.mbRollbackExact = true;
    aPrimitiveRollback.mbQueueExact = true;
    aPrimitiveRollback.mbComputationalFullMatch = true;
    aPrimitiveRollback.mbGraphFullMatch = true;
    aPrimitiveRollback.mbBroadcasterExact = true;
    aLiveApply.meKind = LiveApplyObservationKind::Exact;
    aExact = classifyFinalVerificationObservation(true, aQueue, aComputational, aGraph, aIr,
        aBroadcasters, aLiveApply, std::nullopt, aPrimitiveRollback);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeFinalVerificationObservation(aExact),
        FinalVerificationObservationKind::Exact, aExact.meKind);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalRawDocumentMutationObservationClassifierKinds)
{
    using spreadsheetengine::compat::libreoffice::substraterawmutation::RawMutationObservation;
    using spreadsheetengine::compat::libreoffice::substraterawmutation::
        classifyRawDocumentMutationObservation;

    RawMutationObservation aRawObservation;
    aRawObservation.meKind = RawMutationObservationKind::Exact;
    aRawObservation.mbQueueExact = true;
    aRawObservation.mbComputationalFullMatch = true;
    aRawObservation.mbGraphFullMatch = true;
    aRawObservation.mbObjectRealizationExact = true;

    const auto aExact
        = classifyRawDocumentMutationObservation(true, aRawObservation);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeRawDocumentMutationObservation(aExact),
        RawDocumentMutationObservationKind::Exact, aExact.meKind);

    aRawObservation.meKind = RawMutationObservationKind::OrderingOnly;
    CPPUNIT_ASSERT_EQUAL(
        RawDocumentMutationObservationKind::OrderingOnly,
        classifyRawDocumentMutationObservation(true, aRawObservation).meKind);

    aRawObservation.meKind = RawMutationObservationKind::HiddenHostMutationReconstruction;
    CPPUNIT_ASSERT_EQUAL(
        RawDocumentMutationObservationKind::HiddenHostMutationOrchestration,
        classifyRawDocumentMutationObservation(true, aRawObservation).meKind);

    aRawObservation.meKind = RawMutationObservationKind::MissingRealizedOrRolledBackObjects;
    CPPUNIT_ASSERT_EQUAL(
        RawDocumentMutationObservationKind::MissingRealizedOrRolledBackObjects,
        classifyRawDocumentMutationObservation(true, aRawObservation).meKind);

    aRawObservation.meKind = RawMutationObservationKind::QueueOrStateMismatch;
    CPPUNIT_ASSERT_EQUAL(
        RawDocumentMutationObservationKind::QueueOrStateMismatch,
        classifyRawDocumentMutationObservation(true, aRawObservation).meKind);

    aRawObservation.meKind = RawMutationObservationKind::OutOfContract;
    aRawObservation.maReason = u"primitive_out_of_contract";
    CPPUNIT_ASSERT_EQUAL(
        RawDocumentMutationObservationKind::OutOfContract,
        classifyRawDocumentMutationObservation(true, aRawObservation).meKind);

    CPPUNIT_ASSERT_EQUAL(
        RawDocumentMutationObservationKind::OutOfContract,
        classifyRawDocumentMutationObservation(false, std::nullopt, u"missing_primitive_mutation")
            .meKind);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalPrimitiveExecutionObservationClassifierKinds)
{
    using spreadsheetengine::compat::libreoffice::substratefinalverification::
        FinalVerificationObservation;
    using spreadsheetengine::compat::libreoffice::substratefinalverification::
        FinalVerificationObservationKind;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        PrimitiveRealizationObservation;
    using spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::
        classifyPrimitiveExecutionObservation;
    using spreadsheetengine::compat::libreoffice::substraterawmutation::
        RawDocumentMutationObservation;
    using spreadsheetengine::compat::libreoffice::substraterollback::
        PrimitiveRollbackObservation;

    RawDocumentMutationObservation aRawDocument;
    aRawDocument.meKind = RawDocumentMutationObservationKind::Exact;
    aRawDocument.mbPrimitiveMutationApplied = true;
    aRawDocument.mbRawMutationExact = true;
    aRawDocument.mbQueueExact = true;
    aRawDocument.mbComputationalFullMatch = true;
    aRawDocument.mbGraphFullMatch = true;
    aRawDocument.mbObjectRealizationExact = true;

    PrimitiveRealizationObservation aPrimitiveRealization;
    aPrimitiveRealization.meKind = PrimitiveRealizationObservationKind::Exact;
    aPrimitiveRealization.mbPrimitiveRealizationApplied = true;
    aPrimitiveRealization.mbObjectRealizationExact = true;
    aPrimitiveRealization.mbQueueExact = true;
    aPrimitiveRealization.mbComputationalFullMatch = true;
    aPrimitiveRealization.mbGraphFullMatch = true;
    aPrimitiveRealization.mbBroadcasterExact = true;

    FinalVerificationObservation aFinalVerification;
    aFinalVerification.meKind = FinalVerificationObservationKind::Exact;
    aFinalVerification.mbVerificationExecuted = true;
    aFinalVerification.mbQueueExact = true;
    aFinalVerification.mbComputationalFullMatch = true;
    aFinalVerification.mbGraphFullMatch = true;
    aFinalVerification.mbIrExact = true;
    aFinalVerification.mbBroadcasterExact = true;
    aFinalVerification.mbLiveApplyExact = true;
    aFinalVerification.mbPrimitiveRealizationExact = true;

    auto aExact = classifyPrimitiveExecutionObservation(
        true, aRawDocument, aPrimitiveRealization, std::nullopt, aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveExecutionObservation(aExact),
        PrimitiveExecutionObservationKind::Exact, aExact.meKind);

    aFinalVerification.meKind = FinalVerificationObservationKind::NormalizedEquivalent;
    auto aNormalized = classifyPrimitiveExecutionObservation(
        true, aRawDocument, aPrimitiveRealization, std::nullopt, aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveExecutionObservation(aNormalized),
        PrimitiveExecutionObservationKind::NormalizedEquivalent, aNormalized.meKind);

    aFinalVerification.meKind = FinalVerificationObservationKind::Exact;
    aRawDocument.meKind = RawDocumentMutationObservationKind::OrderingOnly;
    auto aOrdering = classifyPrimitiveExecutionObservation(
        true, aRawDocument, aPrimitiveRealization, std::nullopt, aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveExecutionObservation(aOrdering),
        PrimitiveExecutionObservationKind::OrderingOnly, aOrdering.meKind);

    aRawDocument.meKind = RawDocumentMutationObservationKind::Exact;
    aPrimitiveRealization.meKind
        = PrimitiveRealizationObservationKind::HiddenHostRealizationOrchestration;
    auto aHiddenHost = classifyPrimitiveExecutionObservation(
        true, aRawDocument, aPrimitiveRealization, std::nullopt, aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveExecutionObservation(aHiddenHost),
        PrimitiveExecutionObservationKind::HiddenHostPrimitiveExecutionOrchestration,
        aHiddenHost.meKind);

    aPrimitiveRealization.meKind = PrimitiveRealizationObservationKind::MissingRealizedObjects;
    auto aMissing = classifyPrimitiveExecutionObservation(
        true, aRawDocument, aPrimitiveRealization, std::nullopt, aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveExecutionObservation(aMissing),
        PrimitiveExecutionObservationKind::MissingPrimitiveExecutionInputs, aMissing.meKind);

    aPrimitiveRealization.meKind = PrimitiveRealizationObservationKind::Exact;
    aFinalVerification.meKind = FinalVerificationObservationKind::QueueOrStateMismatch;
    auto aMismatch = classifyPrimitiveExecutionObservation(
        true, aRawDocument, aPrimitiveRealization, std::nullopt, aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveExecutionObservation(aMismatch),
        PrimitiveExecutionObservationKind::QueueOrStateMismatch, aMismatch.meKind);

    PrimitiveRollbackObservation aPrimitiveRollback;
    aPrimitiveRollback.meKind = PrimitiveRollbackObservationKind::Exact;
    aPrimitiveRollback.mbPrimitiveRollbackApplied = true;
    aPrimitiveRollback.mbRollbackExact = true;
    aPrimitiveRollback.mbQueueExact = true;
    aPrimitiveRollback.mbComputationalFullMatch = true;
    aPrimitiveRollback.mbGraphFullMatch = true;
    aPrimitiveRollback.mbBroadcasterExact = true;
    aFinalVerification.meKind = FinalVerificationObservationKind::Exact;
    aFinalVerification.mbPrimitiveRollbackExact = true;
    auto aRollbackExact = classifyPrimitiveExecutionObservation(
        true, aRawDocument, std::nullopt, aPrimitiveRollback, aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveExecutionObservation(aRollbackExact),
        PrimitiveExecutionObservationKind::Exact, aRollbackExact.meKind);

    const auto aOutOfContract = classifyPrimitiveExecutionObservation(
        false, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        u"primitive_execution_not_applied");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveExecutionObservation(aOutOfContract),
        PrimitiveExecutionObservationKind::OutOfContract, aOutOfContract.meKind);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalPrimitiveHostExecutorObservationClassifierKinds)
{
    using spreadsheetengine::compat::libreoffice::substratefinalverification::
        FinalVerificationObservation;
    using spreadsheetengine::compat::libreoffice::substratefinalverification::
        FinalVerificationObservationKind;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        PrimitiveRealizationObservation;
    using spreadsheetengine::compat::libreoffice::substrateprimitiveexecution::
        PrimitiveExecutionObservation;
    using spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor::
        classifyPrimitiveHostExecutorObservation;
    using spreadsheetengine::compat::libreoffice::substraterawmutation::
        RawDocumentMutationObservation;
    using spreadsheetengine::compat::libreoffice::substraterollback::
        PrimitiveRollbackObservation;

    RawDocumentMutationObservation aRawDocument;
    aRawDocument.meKind = RawDocumentMutationObservationKind::Exact;
    aRawDocument.mbPrimitiveMutationApplied = true;
    aRawDocument.mbRawMutationExact = true;
    aRawDocument.mbQueueExact = true;
    aRawDocument.mbComputationalFullMatch = true;
    aRawDocument.mbGraphFullMatch = true;
    aRawDocument.mbObjectRealizationExact = true;

    PrimitiveExecutionObservation aPrimitiveExecution;
    aPrimitiveExecution.meKind = PrimitiveExecutionObservationKind::Exact;
    aPrimitiveExecution.mbPrimitiveExecutionApplied = true;
    aPrimitiveExecution.mbRawDocumentMutationExact = true;
    aPrimitiveExecution.mbPrimitiveRealizationExact = true;
    aPrimitiveExecution.mbFinalVerificationExact = true;
    aPrimitiveExecution.mbQueueExact = true;
    aPrimitiveExecution.mbComputationalFullMatch = true;
    aPrimitiveExecution.mbGraphFullMatch = true;

    PrimitiveRealizationObservation aPrimitiveRealization;
    aPrimitiveRealization.meKind = PrimitiveRealizationObservationKind::Exact;
    aPrimitiveRealization.mbPrimitiveRealizationApplied = true;
    aPrimitiveRealization.mbObjectRealizationExact = true;
    aPrimitiveRealization.mbQueueExact = true;
    aPrimitiveRealization.mbComputationalFullMatch = true;
    aPrimitiveRealization.mbGraphFullMatch = true;
    aPrimitiveRealization.mbBroadcasterExact = true;

    FinalVerificationObservation aFinalVerification;
    aFinalVerification.meKind = FinalVerificationObservationKind::Exact;
    aFinalVerification.mbVerificationExecuted = true;
    aFinalVerification.mbQueueExact = true;
    aFinalVerification.mbComputationalFullMatch = true;
    aFinalVerification.mbGraphFullMatch = true;
    aFinalVerification.mbIrExact = true;
    aFinalVerification.mbBroadcasterExact = true;
    aFinalVerification.mbLiveApplyExact = true;
    aFinalVerification.mbPrimitiveRealizationExact = true;

    auto aExact = classifyPrimitiveHostExecutorObservation(
        true, aRawDocument, aPrimitiveExecution, aPrimitiveRealization, std::nullopt,
        aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveHostExecutorObservation(aExact),
        PrimitiveHostExecutorObservationKind::Exact, aExact.meKind);

    aPrimitiveExecution.meKind = PrimitiveExecutionObservationKind::NormalizedEquivalent;
    aFinalVerification.meKind = FinalVerificationObservationKind::NormalizedEquivalent;
    auto aNormalized = classifyPrimitiveHostExecutorObservation(
        true, aRawDocument, aPrimitiveExecution, aPrimitiveRealization, std::nullopt,
        aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveHostExecutorObservation(aNormalized),
        PrimitiveHostExecutorObservationKind::NormalizedEquivalent, aNormalized.meKind);

    aPrimitiveExecution.meKind = PrimitiveExecutionObservationKind::Exact;
    aFinalVerification.meKind = FinalVerificationObservationKind::Exact;
    aRawDocument.meKind = RawDocumentMutationObservationKind::OrderingOnly;
    auto aOrdering = classifyPrimitiveHostExecutorObservation(
        true, aRawDocument, aPrimitiveExecution, aPrimitiveRealization, std::nullopt,
        aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveHostExecutorObservation(aOrdering),
        PrimitiveHostExecutorObservationKind::OrderingOnly, aOrdering.meKind);

    aRawDocument.meKind = RawDocumentMutationObservationKind::Exact;
    aPrimitiveExecution.meKind
        = PrimitiveExecutionObservationKind::HiddenHostPrimitiveExecutionOrchestration;
    auto aHiddenHost = classifyPrimitiveHostExecutorObservation(
        true, aRawDocument, aPrimitiveExecution, aPrimitiveRealization, std::nullopt,
        aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveHostExecutorObservation(aHiddenHost),
        PrimitiveHostExecutorObservationKind::HiddenHostCallOrchestration,
        aHiddenHost.meKind);

    aPrimitiveExecution.meKind = PrimitiveExecutionObservationKind::Exact;
    aPrimitiveRealization.meKind = PrimitiveRealizationObservationKind::MissingRealizedObjects;
    auto aMissing = classifyPrimitiveHostExecutorObservation(
        true, aRawDocument, aPrimitiveExecution, aPrimitiveRealization, std::nullopt,
        aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveHostExecutorObservation(aMissing),
        PrimitiveHostExecutorObservationKind::MissingHostCallInputs, aMissing.meKind);

    aPrimitiveRealization.meKind = PrimitiveRealizationObservationKind::Exact;
    aFinalVerification.meKind = FinalVerificationObservationKind::QueueOrStateMismatch;
    auto aMismatch = classifyPrimitiveHostExecutorObservation(
        true, aRawDocument, aPrimitiveExecution, aPrimitiveRealization, std::nullopt,
        aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveHostExecutorObservation(aMismatch),
        PrimitiveHostExecutorObservationKind::QueueOrStateMismatch, aMismatch.meKind);

    PrimitiveRollbackObservation aPrimitiveRollback;
    aPrimitiveRollback.meKind = PrimitiveRollbackObservationKind::Exact;
    aPrimitiveRollback.mbPrimitiveRollbackApplied = true;
    aPrimitiveRollback.mbRollbackExact = true;
    aPrimitiveRollback.mbQueueExact = true;
    aPrimitiveRollback.mbComputationalFullMatch = true;
    aPrimitiveRollback.mbGraphFullMatch = true;
    aPrimitiveRollback.mbBroadcasterExact = true;
    aPrimitiveExecution.mbRolledBack = true;
    aPrimitiveExecution.mbPrimitiveRealizationExact = false;
    aPrimitiveExecution.mbPrimitiveRollbackExact = true;
    aFinalVerification.meKind = FinalVerificationObservationKind::Exact;
    aFinalVerification.mbPrimitiveRealizationExact = false;
    aFinalVerification.mbPrimitiveRollbackExact = true;
    auto aRollbackExact = classifyPrimitiveHostExecutorObservation(
        true, aRawDocument, aPrimitiveExecution, std::nullopt, aPrimitiveRollback,
        aFinalVerification);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveHostExecutorObservation(aRollbackExact),
        PrimitiveHostExecutorObservationKind::Exact, aRollbackExact.meKind);

    const auto aOutOfContract = classifyPrimitiveHostExecutorObservation(
        false, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        u"primitive_host_executor_not_applied");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(describePrimitiveHostExecutorObservation(aOutOfContract),
        PrimitiveHostExecutorObservationKind::OutOfContract, aOutOfContract.meKind);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalObjectRealizationObservationClassifiesMissingObjects)
{
    using spreadsheetengine::compat::libreoffice::bootstrapMutableComputationalSubstrateState;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::recalcshadow::detail::comparePlanToDocument;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        AdmittedObjectRealization;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        ObjectRealizationResultKind;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        buildAdmittedObjectRealization;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        classifyObjectRealizationObservation;
    using spreadsheetengine::compat::libreoffice::substrateobjectrealization::
        realizeAdmittedObjectRealization;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::applyMutableLifecycleTransition;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildLifecyclePilotTransition;
    using spreadsheetengine::detail::substrate::compareComputationalShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::detail::compareBroadcasterCanonicalization;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr); // B1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeIr = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
        aBeforeShadow, *m_pDoc);

    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr); // C1
    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);

    spreadsheetengine::detail::substrate::LifecyclePilotInput aInput;
    aInput.maComputationalShadow = aBeforeShadow;
    aInput.maGraphShadow = aBeforeGraph;
    aInput.maIrShadow = aBeforeIr;
    aInput.maMutation = translateSetFormula(ScAddress(2, 0, 0), u"=B1+1"_ustr);
    aInput.mbCleanBaseline = true;
    const auto oFormula = aAfterFacade.getFormulaCellDescriptor({ 0, 2, 0 });
    CPPUNIT_ASSERT(oFormula);
    aInput.moFormulaCachedValueAfter = oFormula->maCachedValue;

    const auto aTransition = buildLifecyclePilotTransition(aInput);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable,
        aTransition.meVerdict);

    auto aMutableState = bootstrapMutableComputationalSubstrateState(aBeforeShadow);
    CPPUNIT_ASSERT(applyMutableLifecycleTransition(aMutableState, aTransition));
    const AdmittedObjectRealization aObjectRealization
        = buildAdmittedObjectRealization(aMutableState);

    m_pDoc->SetValue(1, 0, 0, 99.0); // B1 no longer a formula cell
    m_pDoc->SetEmptyCell(ScAddress(2, 0, 0)); // C1 removed

    const auto aRealization = realizeAdmittedObjectRealization(*m_pDoc, aObjectRealization);
    CPPUNIT_ASSERT_EQUAL(ObjectRealizationResultKind::Applied, aRealization.meKind);

    m_pDoc->SetEmptyCell(ScAddress(2, 0, 0)); // remove a realized formula object

    const CalcWorkbookFacade aLiveFacade(*m_pDoc, 1);
    const auto aLiveObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aQueueComparison = comparePlanToDocument(aTransition.maRecalcPlan, aLiveFacade, *m_pDoc);
    const auto aComputationalComparison
        = compareComputationalShadow(aMutableState.maShadow, aLiveFacade, aLiveObservation);
    const auto aLiveShadow = buildComputationalWorkbookShadow(aLiveFacade, aLiveObservation);
    const auto aGraphComparison
        = compareDependencyGraphShadow(aMutableState.maGraphShadow, aLiveShadow, aLiveObservation);
    const auto aBroadcasterComparison
        = compareBroadcasterCanonicalization(aMutableState.maShadow, aLiveObservation);
    const auto aObjectObservation = classifyObjectRealizationObservation(
        aRealization, aQueueComparison, aComputationalComparison, aGraphComparison,
        aBroadcasterComparison);

    CPPUNIT_ASSERT_EQUAL_MESSAGE(describeObjectRealizationObservation(aObjectObservation),
        ObjectRealizationObservationKind::MissingRealizedObjects, aObjectObservation.meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDependencyGraphShadowNormalizedComparison)
{
    using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr); // C1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aFacade(*m_pDoc, 1);
    auto aObservation = makeComputationalObservationState(collectLiveComputationalState(*m_pDoc));
    aObservation.maFormulaTree = { { 0, 2, 0 }, { 0, 1, 0 } };
    const auto aShadow = buildComputationalWorkbookShadow(aFacade, aObservation);
    const auto aGraph = buildDependencyGraphShadow(aShadow, aObservation);
    const auto aComparison = compareDependencyGraphShadow(aGraph, aShadow, aObservation);

    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::NormalizedEquivalent, aComparison.meKind);
    CPPUNIT_ASSERT(aComparison.mbFullMatch);
    CPPUNIT_ASSERT(!aComparison.mbFormulaTreeExactMatch);
    CPPUNIT_ASSERT(aComparison.mbFormulaTreeNormalizedMatch);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDependencyGraphShadowDelayedListenerCoverage)
{
    using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::BroadcasterNodeId;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;
    namespace graphmapping = spreadsheetengine::detail::substrate::graphmapping;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);

    {
        sc::EndListeningContext aEndCxt(*m_pDoc);
        pFormula->EndListeningTo(aEndCxt);
        aEndCxt.purgeEmptyBroadcasters();
    }

    ScTable* pTable = m_pDoc->FetchTable(0);
    CPPUNIT_ASSERT(pTable);
    ScColumn& rFormulaColumn = pTable->CreateColumnIfNotExists(1);

    m_pDoc->EnableDelayStartListeningFormulaCells(&rFormulaColumn, true);
    rFormulaColumn.StartListeningUnshared({ 0, 0 });

    const CalcWorkbookFacade aDelayedFacade(*m_pDoc, 1);
    const auto aDelayedObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aDelayedShadow = buildComputationalWorkbookShadow(aDelayedFacade, aDelayedObservation);
    const auto aDelayedGraph = buildDependencyGraphShadow(aDelayedShadow, aDelayedObservation);
    const auto aDelayedComparison
        = compareDependencyGraphShadow(aDelayedGraph, aDelayedShadow, aDelayedObservation);
    assertComparableGraph(aDelayedComparison);

    const auto aBroadcaster = BroadcasterNodeId::forCell({ 0, 0, 0 });
    const auto aFormulaAnchor
        = graphmapping::makeGraphFormulaCellListenerAnchorId({ 0, 1, 0 });
    CPPUNIT_ASSERT(!hasGraphEdge(aDelayedGraph, aBroadcaster, aFormulaAnchor));

    m_pDoc->EnableDelayStartListeningFormulaCells(&rFormulaColumn, false);

    const CalcWorkbookFacade aRestoredFacade(*m_pDoc, 2);
    const auto aRestoredObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aRestoredShadow = buildComputationalWorkbookShadow(aRestoredFacade, aRestoredObservation);
    const auto aRestoredGraph = buildDependencyGraphShadow(aRestoredShadow, aRestoredObservation);
    const auto aRestoredComparison
        = compareDependencyGraphShadow(aRestoredGraph, aRestoredShadow, aRestoredObservation);
    assertComparableGraph(aRestoredComparison);
    CPPUNIT_ASSERT(hasGraphEdge(aRestoredGraph, aBroadcaster, aFormulaAnchor));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDependencyGraphShadowDelayedBroadcasterDeletionCoverage)
{
    using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::BroadcasterNodeId;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);

    ScTable* pTable = m_pDoc->FetchTable(0);
    CPPUNIT_ASSERT(pTable);
    ScColumn& rSourceColumn = pTable->CreateColumnIfNotExists(0);
    const auto aBroadcaster = BroadcasterNodeId::forCell({ 0, 0, 0 });

    {
        sc::DelayDeletingBroadcasters aDelay(*m_pDoc);
        rSourceColumn.EndListening(*pFormula, 0);

        const CalcWorkbookFacade aDelayedFacade(*m_pDoc, 1);
        const auto aDelayedObservation = makeComputationalObservationState(
            collectLiveComputationalState(*m_pDoc));
        const auto aDelayedShadow
            = buildComputationalWorkbookShadow(aDelayedFacade, aDelayedObservation);
        const auto aDelayedGraph
            = buildDependencyGraphShadow(aDelayedShadow, aDelayedObservation);
        const auto aDelayedComparison
            = compareDependencyGraphShadow(aDelayedGraph, aDelayedShadow, aDelayedObservation);
        assertComparableGraph(aDelayedComparison);
        const auto* pBroadcaster = aDelayedGraph.findBroadcasterNode(aBroadcaster);
        CPPUNIT_ASSERT(pBroadcaster);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), pBroadcaster->mnListenerCount);
    }

    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 2);
    const auto aAfterObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aAfterShadow = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
    const auto aAfterGraph = buildDependencyGraphShadow(aAfterShadow, aAfterObservation);
    const auto aAfterComparison
        = compareDependencyGraphShadow(aAfterGraph, aAfterShadow, aAfterObservation);
    assertComparableGraph(aAfterComparison);
    CPPUNIT_ASSERT(!aAfterGraph.findBroadcasterNode(aBroadcaster));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDependencyGraphShadowStructuralGateCoverage)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::rebuildDependencyGraphShadowAfterMutation;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(1, 1, 0, 2.0); // B2
    m_pDoc->SetString(2, 1, 0, u"=A1+B2"_ustr); // C2
    m_pDoc->CalcAll();

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    auto aRowInsertState = rebuildDependencyGraphShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 1), *m_pDoc, translateInsertRows(0, 1, 1));
    const auto aRowInsertComparison = compareDependencyGraphShadow(
        aRowInsertState.maGraphShadow, aRowInsertState.maComputationalShadow, aRowInsertState.maObservation);
    assertComparableGraph(aRowInsertComparison);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aRowInsertState.maGraphShadow.getFormulaNodeCount());

    m_pDoc->DeleteCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    auto aDeleteColumnState = rebuildDependencyGraphShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 2), *m_pDoc, translateDeleteColumns(0, 0, 1));
    const auto aDeleteColumnComparison = compareDependencyGraphShadow(
        aDeleteColumnState.maGraphShadow, aDeleteColumnState.maComputationalShadow,
        aDeleteColumnState.maObservation);
    assertComparableGraph(aDeleteColumnComparison);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aDeleteColumnState.maGraphShadow.getFormulaNodeCount());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testExecutionIrShadowRebuildAfterSafeMutations)
{
    using spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow;
    using spreadsheetengine::compat::libreoffice::mutation::translateClearCell;
    using spreadsheetengine::compat::libreoffice::mutation::translateRenameNamedRange;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::rebuildExecutionIrShadowAfterMutation;
    using spreadsheetengine::detail::substrate::compareExecutionIrWorkbookShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    auto* pGlobalName
        = new ScRangeData(*m_pDoc, u"Metric"_ustr, u"$Data.$A$1:$A$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pGlobalName));
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metric)"_ustr); // C1
    m_pDoc->CalcAll();

    auto assertIrMatch = [&](const spreadsheetengine::detail::facade::MutationEvent& rMutation,
                             sal_Int64 nGeneration) {
        const CalcWorkbookFacade aFacade(*m_pDoc, nGeneration);
        const auto aState = rebuildExecutionIrShadowAfterMutation(aFacade, *m_pDoc, rMutation);
        const auto aExpected = buildExecutionIrWorkbookShadow(aState.maComputationalShadow, *m_pDoc);
        const auto aComparison = compareExecutionIrWorkbookShadow(aState.maIrShadow, aExpected);
        assertComparableExecutionIr(aComparison);
        CPPUNIT_ASSERT(aState.maIrShadow.maBuildFailures.empty());
    };

    m_pDoc->SetValue(0, 0, 0, 7.0);
    assertIrMatch(translateSetScalarValue(ScAddress(0, 0, 0)), 1);

    m_pDoc->SetString(1, 0, 0, u"=A2*3"_ustr);
    assertIrMatch(translateSetFormula(ScAddress(1, 0, 0), u"=A2*3"_ustr), 2);

    m_pDoc->SetString(3, 0, 0, u"=B1+C1"_ustr);
    assertIrMatch(translateSetFormula(ScAddress(3, 0, 0), u"=B1+C1"_ustr), 3);

    m_pDoc->SetEmptyCell(ScAddress(0, 1, 0));
    assertIrMatch(translateClearCell(ScAddress(0, 1, 0)), 4);

    pGlobalName->SetNewName(u"MetricRenamed"_ustr);
    assertIrMatch(
        translateRenameNamedRange(*m_pDoc, *pGlobalName, std::nullopt, u"Metric"_ustr), 5);

    m_pDoc->DiscardFormulaGroupContext();
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testExecutionIrShadowNormalizedComparison)
{
    using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
    using spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow;
    using spreadsheetengine::detail::substrate::compareExecutionIrWorkbookShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(1, 0, 0, 2.0); // B1
    m_pDoc->SetString(2, 0, 0, u"=A1+B1"_ustr); // C1
    m_pDoc->SetString(3, 0, 0, u"=SUM(A1:B1)"_ustr); // D1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aFacade(*m_pDoc, 1);
    auto aIrShadow = buildExecutionIrWorkbookShadow(aFacade, *m_pDoc);
    std::reverse(aIrShadow.maFormulaRecords.begin(), aIrShadow.maFormulaRecords.end());

    const auto aExpected = buildExecutionIrWorkbookShadow(aFacade, *m_pDoc);
    const auto aComparison = compareExecutionIrWorkbookShadow(aIrShadow, aExpected);

    CPPUNIT_ASSERT_EQUAL(ExecutionIrComparisonKind::NormalizedEquivalent, aComparison.meKind);
    CPPUNIT_ASSERT(aComparison.mbFullMatch);
    CPPUNIT_ASSERT(!aComparison.mbFormulaRecordExactMatch);
    CPPUNIT_ASSERT(aComparison.mbFormulaRecordNormalizedMatch);

    m_pDoc->DiscardFormulaGroupContext();
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testExecutionIrShadowStructuralGateCoverage)
{
    using spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow;
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::rebuildExecutionIrShadowAfterMutation;
    using spreadsheetengine::detail::substrate::compareExecutionIrWorkbookShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(1, 1, 0, 2.0); // B2
    m_pDoc->SetString(2, 1, 0, u"=A1+B2"_ustr); // C2
    m_pDoc->CalcAll();

    auto aRowInsertState = rebuildExecutionIrShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 1), *m_pDoc, translateInsertRows(0, 1, 1));
    const auto aRowInsertExpected
        = buildExecutionIrWorkbookShadow(aRowInsertState.maComputationalShadow, *m_pDoc);
    const auto aRowInsertComparison
        = compareExecutionIrWorkbookShadow(aRowInsertState.maIrShadow, aRowInsertExpected);
    assertComparableExecutionIr(aRowInsertComparison);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aRowInsertState.maIrShadow.getFormulaCount());

    m_pDoc->DeleteCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    auto aDeleteColumnState = rebuildExecutionIrShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 2), *m_pDoc, translateDeleteColumns(0, 0, 1));
    const auto aDeleteColumnExpected
        = buildExecutionIrWorkbookShadow(aDeleteColumnState.maComputationalShadow, *m_pDoc);
    const auto aDeleteColumnComparison = compareExecutionIrWorkbookShadow(
        aDeleteColumnState.maIrShadow, aDeleteColumnExpected);
    assertComparableExecutionIr(aDeleteColumnComparison);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aDeleteColumnState.maIrShadow.getFormulaCount());

    m_pDoc->DiscardFormulaGroupContext();
    m_pDoc->DeleteTab(0);
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
