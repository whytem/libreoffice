/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/detail/substrate/AuthorityPilot.hxx>
#include <spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx>
#include <spreadsheetengine/detail/substrate/LifecyclePilot.hxx>
#include <spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx>
#include <spreadsheetengine/detail/substrate/MutationEntry.hxx>
#include <spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx>
#include <spreadsheetengine/detail/substrate/StructuralPilot.hxx>
#include <spreadsheetengine/detail/substrate/StructuralPilotBuilder.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIr.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowMapping.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadow.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowMutation.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrComparison.hxx>
#include <spreadsheetengine/detail/workbook/InMemoryWorkbookFacade.hxx>

#include "TestSupport.hxx"

int main()
{
    using namespace spreadsheetengine::detail::facade;
    using namespace spreadsheetengine::detail::substrate;
    using spreadsheetengine::detail::substrate::authoritydetail::classifyAuthorityMutation;
    using spreadsheetengine::detail::substrate::authoritydetail::makeAuthorityVerification;
    using spreadsheetengine::detail::substrate::lifecycledetail::classifyLifecycleMutation;
    using spreadsheetengine::detail::substrate::lifecycledetail::makeLifecycleVerification;
    using spreadsheetengine::detail::substrate::mutationentrydetail::classifyMutationEntryPath;
    using spreadsheetengine::detail::substrate::structuraldetail::classifyStructuralMutation;
    using spreadsheetengine::detail::substrate::structuraldetail::makeStructuralVerification;
    namespace mapping = spreadsheetengine::detail::substrate::mapping;
    using spreadsheetengine::api::CellValue;
    using spreadsheetengine::standalone::test::fail;

    {
        ExecutionIrInstruction aInstruction {
            ExecutionIrInstructionKind::SingleReference,
            spreadsheetengine::detail::token::kOpCodePush,
            spreadsheetengine::api::refdata::SingleRefData {}
        };
        if (!aInstruction.carriesReference())
            return fail("computational_substrate", "execution ir reference classification mismatch");

        ExecutionIrFormulaRecord aFormula;
        aFormula.maId = { { 0, 1, 2 } };
        aFormula.maSource = { u"=A1", {} };
        aFormula.maInstructions.push_back(aInstruction);
        aFormula.maInstructions.push_back({
            ExecutionIrInstructionKind::PlainOpcode,
            spreadsheetengine::detail::token::kOpCodeAdd,
            {}
        });
        aFormula.mbInFormulaTree = true;

        ExecutionIrWorkbookShadow aIrShadow;
        aIrShadow.maFormulaRecords.push_back(aFormula);
        if (aIrShadow.getFormulaCount() != 1 || aIrShadow.getInstructionCount() != 2
            || !aIrShadow.findFormula({ 0, 1, 2 }) || !aIrShadow.isFullyLowered()
            || aIrShadow.findFormula({ 0, 2, 2 }))
        {
            return fail("computational_substrate", "execution ir schema lookup mismatch");
        }
        if (aIrShadow.findFormula({ 0, 1, 2 })->getReferenceInstructionCount() != 1)
            return fail("computational_substrate", "execution ir reference count mismatch");

        aIrShadow.maBuildFailures.push_back({ { { 0, 3, 4 } }, u"=BAD()", u"lower failed" });
        if (aIrShadow.isFullyLowered())
            return fail("computational_substrate", "execution ir failure tracking mismatch");
    }

    if (!(mapping::makeShadowCellId({ 3, 4, 5 }) == ShadowCellId { { 3, 4, 5 } }))
        return fail("computational_substrate", "shadow cell id mapping mismatch");

    {
        const auto aCellBroadcaster = BroadcasterNodeId::forCell({ 0, 2, 1 });
        if (aCellBroadcaster.meKind != BroadcasterNodeKind::Cell
            || !(aCellBroadcaster.maCellAddress == spreadsheetengine::api::CellAddress { 0, 2, 1 }))
        {
            return fail("computational_substrate", "cell broadcaster id mapping mismatch");
        }

        const auto aAreaBroadcaster
            = BroadcasterNodeId::forArea({ { 1, 3, 4 }, { 1, 5, 6 } });
        if (aAreaBroadcaster.meKind != BroadcasterNodeKind::Area
            || !(aAreaBroadcaster.maAreaRange
                 == spreadsheetengine::api::CellRange { { 1, 3, 4 }, { 1, 5, 6 } }))
        {
            return fail("computational_substrate", "area broadcaster id mapping mismatch");
        }

        DependencyGraphShadow aGraph;
        aGraph.maListenerAnchors.push_back({ mapping::makeListenerAnchorId(
            ListenerAnchorKind::FormulaCell, { 0, 2, 1 }, 1) });
        if (aGraph.getListenerAnchorCount() != 1
            || !aGraph.findListenerAnchor(mapping::makeListenerAnchorId(
                   ListenerAnchorKind::FormulaCell, { 0, 2, 1 }, 1)))
        {
            return fail("computational_substrate", "graph schema lookup mismatch");
        }
    }

    {
        const FormulaGroupDescriptor aDescriptor { { 0, 1, 2 }, 4, true };
        if (!(mapping::makeShadowFormulaGroupId(aDescriptor)
              == ShadowFormulaGroupId { { 0, 1, 2 }, 4 }))
        {
            return fail("computational_substrate", "shadow group id mapping mismatch");
        }
    }

    InMemoryWorkbookFacade aFacade;
    aFacade.setGrammar({ spreadsheetengine::api::FormulaLanguage::Odff,
        spreadsheetengine::api::AddressConvention::OdfA1, false });
    aFacade.setGeneration(9);

    const auto nData = aFacade.addSheet(u"Data");
    aFacade.addSheet(u"Summary");

    aFacade.setCell({ nData, 0, 0 }, CellValue::number(10.0));
    aFacade.setCell({ nData, 0, 1 }, CellValue::text(u"seed"));
    aFacade.setFormulaCell({ nData, 1, 0 }, u"=A1*2", CellValue::number(20.0));
    aFacade.setFormulaCell({ nData, 1, 1 }, u"=SUM(A1:B1)", CellValue::number(30.0),
        FormulaCellKind::SharedGroupMember, true, true);
    aFacade.addFormulaGroup({ nData, 1, 0 }, 2, true);
    aFacade.addNamedRange(u"Metric", std::nullopt, { nData, 0, 0 }, u"$Data.$A$1:$B$2");

    ComputationalObservationState aObservation;
    aObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 } };
    aObservation.maFormulaTrack = { { nData, 1, 1 } };
    aObservation.maCellBroadcasters.push_back({
        { nData, 0, 0 },
        { { ListenerAnchorKind::FormulaCell, { nData, 1, 0 }, 1 },
            { ListenerAnchorKind::FormulaGroup, { nData, 1, 0 }, 2 } } });

    const ComputationalWorkbookShadow aShadow
        = buildComputationalWorkbookShadow(aFacade, aObservation);
    const auto aInitialComparison = compareComputationalShadow(aShadow, aFacade, aObservation);
    const auto aInitialBroadcasterCanonicalization
        = spreadsheetengine::detail::substrate::detail::compareBroadcasterCanonicalization(
            aShadow, aObservation);

    if (aShadow.maSnapshot.mnGeneration != 9)
        return fail("computational_substrate", "snapshot generation mismatch");
    if (aShadow.getCellCount() != 4)
        return fail("computational_substrate", "cell count mismatch");
    if (aShadow.getFormulaCellCount() != 2)
        return fail("computational_substrate", "formula cell count mismatch");
    if (aShadow.maNamedRanges.size() != 1)
        return fail("computational_substrate", "named range count mismatch");
    if (aShadow.maFormulaGroups.size() != 1)
        return fail("computational_substrate", "formula group count mismatch");
    if (aShadow.maCellBroadcasters.size() != 1)
        return fail("computational_substrate", "cell broadcaster count mismatch");

    const ShadowCellRecord* pScalar = aShadow.findCell({ nData, 0, 0 });
    if (!pScalar || pScalar->hasFormula() || pScalar->mbInFormulaTree)
        return fail("computational_substrate", "scalar cell projection mismatch");

    const ShadowCellRecord* pFormula = aShadow.findCell({ nData, 1, 1 });
    if (!pFormula || !pFormula->hasFormula() || !pFormula->mbInFormulaTrack
        || !pFormula->moFormulaGroup.has_value() || !pFormula->moFormula->mbDirty
        || !pFormula->moFormula->mbNeedsRecalc)
    {
        return fail("computational_substrate", "formula cell projection mismatch");
    }

        if (aShadow.maFormulaGroups.front().maMembers.size() != 2)
            return fail("computational_substrate", "formula group membership mismatch");
        if (!aInitialComparison.mbFullMatch)
            return fail("computational_substrate", "initial shadow comparison mismatch");
        if (!aInitialBroadcasterCanonicalization.mbExactMatch
            || aInitialBroadcasterCanonicalization.meKind
                   != BroadcasterCanonicalizationKind::Exact)
        {
            return fail("computational_substrate",
                "initial broadcaster canonicalization mismatch");
        }

        {
            ComputationalObservationState aDuplicateObservation = aObservation;
            aDuplicateObservation.maCellBroadcasters.front().maListeners.push_back(
                aDuplicateObservation.maCellBroadcasters.front().maListeners.front());

            const auto aDuplicateComparison
                = spreadsheetengine::detail::substrate::detail::compareBroadcasterCanonicalization(
                    aShadow, aDuplicateObservation);
            if (aDuplicateComparison.meKind
                    != BroadcasterCanonicalizationKind::DuplicateMaterializationOnly
                || !aDuplicateComparison.mbDeduplicatedEquivalent
                || aDuplicateComparison.mnLiveDuplicateListenerCount <= 0
                || aDuplicateComparison.mbExactMatch)
            {
                return fail("computational_substrate",
                    "duplicate broadcaster canonicalization mismatch");
            }
        }

        {
            ComputationalObservationState aEmptyObservation = aObservation;
            aEmptyObservation.maCellBroadcasters.push_back({ { nData, 4, 4 }, {} });

            const auto aEmptyComparison
                = spreadsheetengine::detail::substrate::detail::compareBroadcasterCanonicalization(
                    aShadow, aEmptyObservation);
            if (aEmptyComparison.meKind
                    != BroadcasterCanonicalizationKind::EmptyBroadcastersOnly
                || !aEmptyComparison.mbDropEmptyEquivalent
                || aEmptyComparison.mnLiveEmptyBroadcasterCount <= 0
                || aEmptyComparison.mbExactMatch)
            {
                return fail("computational_substrate",
                    "empty broadcaster canonicalization mismatch");
            }
        }

        const auto aAdmittedContract
            = classifyAuthorityMutation(MutationEvent::setFormula({ nData, 1, 0 }, u"=A1*4"));
        if (!aAdmittedContract.isAdmitted() || !aAdmittedContract.mbRequiresCleanBaseline)
            return fail("computational_substrate", "authority contract admission mismatch");

        const auto aValidationOnlyContract = classifyAuthorityMutation(
            MutationEvent::renameNamedRange(aFacade.getNamedRangeDescriptors().front(),
                aFacade.getNamedRangeDescriptors().front()));
        if (aValidationOnlyContract.meMutationClass != AuthorityMutationClass::ValidationOnly)
            return fail("computational_substrate", "authority contract validation-only mismatch");

        const auto aRejectedContract
            = classifyAuthorityMutation(MutationEvent::clearRange({ { nData, 0, 0 }, { nData, 1, 1 } }));
        if (aRejectedContract.meMutationClass != AuthorityMutationClass::Rejected)
            return fail("computational_substrate", "authority contract rejection mismatch");

        AuthorityPilotInput aAuthorityInput;
        aAuthorityInput.maComputationalShadow = aShadow;
        aAuthorityInput.maMutation = MutationEvent::clearCell({ nData, 0, 1 });
        aAuthorityInput.mbCleanBaseline = true;

        AuthorityPilotTransition aTransition;
        aTransition.maInput = aAuthorityInput;
        aTransition.maContract = classifyAuthorityMutation(aAuthorityInput.maMutation);
        aTransition.maVerification = makeAuthorityVerification(aTransition.maContract);
        aTransition.meVerdict = AuthorityPilotVerdict::Applicable;

        if (aTransition.isRejected()
            || aTransition.maVerification.meQueueMode != AuthorityVerificationMode::Exact
            || aTransition.meVerdict != AuthorityPilotVerdict::Applicable)
        {
            return fail("computational_substrate", "authority transition schema mismatch");
        }

        InMemoryWorkbookFacade aPilotFacade;
        aPilotFacade.setGrammar(aFacade.getGrammar());
        aPilotFacade.setGeneration(21);
        const auto nPilotSheet = aPilotFacade.addSheet(u"Pilot");
        aPilotFacade.setCell({ nPilotSheet, 0, 0 }, CellValue::number(10.0));
        aPilotFacade.setFormulaCell({ nPilotSheet, 1, 0 }, u"=A1*2", CellValue::number(20.0));
        aPilotFacade.setFormulaCell({ nPilotSheet, 2, 0 }, u"=B1+1", CellValue::number(21.0));

        ComputationalObservationState aPilotObservation;
        aPilotObservation.maFormulaTree = { { nPilotSheet, 1, 0 }, { nPilotSheet, 2, 0 } };
        aPilotObservation.maCellBroadcasters.push_back({
            { nPilotSheet, 0, 0 },
            { { ListenerAnchorKind::FormulaCell, { nPilotSheet, 1, 0 }, 1 } } });
        aPilotObservation.maCellBroadcasters.push_back({
            { nPilotSheet, 1, 0 },
            { { ListenerAnchorKind::FormulaCell, { nPilotSheet, 2, 0 }, 1 } } });

        const auto aPilotShadow = buildComputationalWorkbookShadow(aPilotFacade, aPilotObservation);
        aAuthorityInput.maComputationalShadow = aPilotShadow;
        aAuthorityInput.maGraphShadow = buildDependencyGraphShadow(aPilotShadow, aPilotObservation);
        aAuthorityInput.maIrShadow
            = authoritybuilddetail::buildAuthorityExecutionIrShadow(aPilotShadow, aPilotFacade);
        aAuthorityInput.maMutation = MutationEvent::setScalarValue({ nPilotSheet, 0, 0 });
        aAuthorityInput.moScalarValueAfter = CellValue::number(99.0);

        const auto aAuthorityPlan = buildAuthorityPilotTransition(aAuthorityInput);
        if (aAuthorityPlan.meVerdict != AuthorityPilotVerdict::Applicable)
        {
            if (aAuthorityPlan.maReason == u"mutation_out_of_contract")
                return fail("computational_substrate", "authority verdict out-of-contract");
            if (aAuthorityPlan.maReason == u"dirty_baseline")
                return fail("computational_substrate", "authority verdict dirty baseline");
            if (aAuthorityPlan.maReason == u"opaque_dependency_surface")
                return fail("computational_substrate", "authority verdict opaque dependency");
            if (aAuthorityPlan.maReason == u"missing_scalar_value_after")
                return fail("computational_substrate", "authority verdict missing scalar value");
            return fail("computational_substrate", "authority verdict mismatch");
        }
        if (aAuthorityPlan.maRecalcPlan.maQueue.size() != 2)
            return fail("computational_substrate", "authority queue size mismatch");
        if (aAuthorityPlan.maGraphAfter.getEdgeCount() < 2)
            return fail("computational_substrate", "authority graph edge mismatch");
        if (aAuthorityPlan.maIrAfter.getFormulaCount() != 2)
            return fail("computational_substrate", "authority ir rebuild mismatch");
        if (aAuthorityPlan.maComputationalAfter.maCellBroadcasters.size() != 2
            || !aAuthorityPlan.maComputationalAfter.maAreaBroadcasters.empty())
        {
            return fail("computational_substrate",
                "authority computational broadcaster projection mismatch");
        }

        const auto* pAuthorityScalar
            = aAuthorityPlan.maComputationalAfter.findCell({ nPilotSheet, 0, 0 });
        const auto* pAuthorityFormula
            = aAuthorityPlan.maComputationalAfter.findCell({ nPilotSheet, 1, 0 });
        if (!pAuthorityScalar || !pAuthorityScalar->maCell.maValue.isNumber()
            || pAuthorityScalar->maCell.maValue.mfNumber != 99.0 || !pAuthorityFormula
            || !pAuthorityFormula->mbInFormulaTree)
        {
            return fail("computational_substrate", "authority projected shadow mismatch");
        }

        auto aMutableState = bootstrapMutableComputationalSubstrateState(aPilotShadow);
        if (!compareAdmittedCellStorage(aMutableState.maCellStorage, aPilotShadow).mbFullMatch)
            return fail("computational_substrate", "bootstrap cell storage mismatch");
        if (!compareAdmittedFormulaCellLifetime(aMutableState.maFormulaCellLifetime, aPilotShadow)
                 .mbFullMatch)
        {
            return fail("computational_substrate", "bootstrap formula lifetime mismatch");
        }
        if (!compareAdmittedWiringContainers(aMutableState.maWiringContainers,
                aAuthorityInput.maGraphShadow)
                 .mbFullMatch)
        {
            return fail("computational_substrate", "bootstrap wiring container mismatch");
        }
        if (!applyMutableAuthorityTransition(aMutableState, aAuthorityPlan)
            || aMutableState.mnAppliedMutationCount != 1
            || !(aMutableState.maLastMutation == aAuthorityInput.maMutation))
        {
            return fail("computational_substrate", "mutable authority state mismatch");
        }
        const auto aMutableAuthorityComparison = compareComputationalShadow(
            aMutableState.maShadow, aMutableState.maFacade, aMutableState.maObservation);
        if (!aMutableAuthorityComparison.mbFullMatch)
            return fail("computational_substrate", "mutable authority shadow mismatch");
        if (!compareAdmittedCellStorage(aMutableState.maCellStorage,
                aAuthorityPlan.maComputationalAfter)
                 .mbFullMatch)
        {
            return fail("computational_substrate", "mutable authority cell storage mismatch");
        }
        if (!compareAdmittedFormulaCellLifetime(aMutableState.maFormulaCellLifetime,
                aAuthorityPlan.maComputationalAfter)
                 .mbFullMatch)
        {
            return fail("computational_substrate", "mutable authority formula lifetime mismatch");
        }
        if (!compareAdmittedWiringContainers(aMutableState.maWiringContainers,
                aAuthorityPlan.maGraphAfter)
                 .mbFullMatch)
        {
            return fail("computational_substrate", "mutable authority wiring container mismatch");
        }
        const auto aMutableScalar
            = aMutableState.maFacade.getCellDescriptor({ nPilotSheet, 0, 0 });
        if (!aMutableScalar.maValue.isNumber() || aMutableScalar.maValue.mfNumber != 99.0)
            return fail("computational_substrate", "mutable authority facade mismatch");

        {
            MutationEntryBuildInput aEntryInput;
            aEntryInput.maComputationalShadow = aPilotShadow;
            aEntryInput.maGraphShadow = aAuthorityInput.maGraphShadow;
            aEntryInput.maIrShadow = aAuthorityInput.maIrShadow;
            aEntryInput.maRequest
                = MutationEntryRequest::setScalarValue({ nPilotSheet, 0, 0 }, CellValue::number(99.0));
            aEntryInput.mbCleanBaseline = true;

            const auto oPath = classifyMutationEntryPath(aEntryInput);
            if (!oPath || *oPath != MutationEntryPath::Authority)
                return fail("computational_substrate", "mutation entry authority routing mismatch");

            const auto aEntryTransition = buildMutationEntryTransition(
                aEntryInput, aPilotFacade, aPilotObservation);
            if (!aEntryTransition.moAuthorityTransition
                || aEntryTransition.moAuthorityTransition->meVerdict
                       != AuthorityPilotVerdict::Applicable)
            {
                return fail("computational_substrate", "mutation entry authority build mismatch");
            }

            auto aEntryState = bootstrapMutableComputationalSubstrateState(aPilotShadow);
            if (!applyMutableMutationEntryTransition(aEntryState, aEntryTransition))
                return fail("computational_substrate", "mutation entry authority apply mismatch");
            if (!findMutationEntryComputationalAfter(aEntryTransition)
                || !compareAdmittedCellStorage(aEntryState.maCellStorage,
                       *findMutationEntryComputationalAfter(aEntryTransition))
                        .mbFullMatch)
            {
                return fail("computational_substrate", "mutation entry authority storage mismatch");
            }
        }

        const auto aLifecycleAdmitted
            = classifyLifecycleMutation(MutationEvent::setFormula({ nPilotSheet, 3, 0 }, u"=A1+B1"));
        if (!aLifecycleAdmitted.isAdmitted() || !aLifecycleAdmitted.mbRequiresFormulaLifecycleShape)
            return fail("computational_substrate", "lifecycle contract admission mismatch");

        const auto aLifecycleRejected
            = classifyLifecycleMutation(MutationEvent::setScalarValue({ nPilotSheet, 0, 0 }));
        if (aLifecycleRejected.meMutationClass != LifecycleMutationClass::Rejected)
            return fail("computational_substrate", "lifecycle contract rejection mismatch");

        LifecyclePilotInput aLifecycleInput;
        aLifecycleInput.maComputationalShadow = aPilotShadow;
        aLifecycleInput.maGraphShadow = aAuthorityInput.maGraphShadow;
        aLifecycleInput.maIrShadow = aAuthorityInput.maIrShadow;
        aLifecycleInput.maMutation = MutationEvent::clearCell({ nPilotSheet, 1, 0 });
        aLifecycleInput.mbCleanBaseline = true;

        LifecyclePilotTransition aLifecycleTransition;
        aLifecycleTransition.maInput = aLifecycleInput;
        aLifecycleTransition.maContract = classifyLifecycleMutation(aLifecycleInput.maMutation);
        aLifecycleTransition.maVerification = makeLifecycleVerification(aLifecycleTransition.maContract);
        aLifecycleTransition.maSyncActions.push_back({
            LifecycleSyncActionKind::RemoveFormulaCell, { nPilotSheet, 1, 0 }, std::nullopt,
            std::nullopt, true, false });
        aLifecycleTransition.meVerdict = LifecyclePilotVerdict::Applicable;

        if (aLifecycleTransition.isRejected()
            || aLifecycleTransition.maVerification.meComputationalMode
                   != LifecycleVerificationMode::Exact
            || !aLifecycleTransition.maVerification.mbObserveIrOnly
            || aLifecycleTransition.meVerdict != LifecyclePilotVerdict::Applicable
            || aLifecycleTransition.maSyncActions.size() != 1)
        {
            return fail("computational_substrate", "lifecycle transition schema mismatch");
        }

        LifecyclePilotInput aLifecycleInsertInput;
        aLifecycleInsertInput.maComputationalShadow = aPilotShadow;
        aLifecycleInsertInput.maGraphShadow = aAuthorityInput.maGraphShadow;
        aLifecycleInsertInput.maIrShadow = aAuthorityInput.maIrShadow;
        aLifecycleInsertInput.maMutation
            = MutationEvent::setFormula({ nPilotSheet, 3, 0 }, u"=A1+B1");
        aLifecycleInsertInput.mbCleanBaseline = true;

        const auto aLifecycleInsertPlan = buildLifecyclePilotTransition(aLifecycleInsertInput);
        if (aLifecycleInsertPlan.meVerdict != LifecyclePilotVerdict::Applicable
            || aLifecycleInsertPlan.maSyncActions.size() != 1
            || aLifecycleInsertPlan.maSyncActions.front().meKind
                   != LifecycleSyncActionKind::InsertFormulaCell
            || !aLifecycleInsertPlan.maComputationalAfter.findCell({ nPilotSheet, 3, 0 }))
        {
            return fail("computational_substrate", "lifecycle insertion transition mismatch");
        }

        LifecyclePilotInput aLifecycleReplaceInput;
        aLifecycleReplaceInput.maComputationalShadow = aPilotShadow;
        aLifecycleReplaceInput.maGraphShadow = aAuthorityInput.maGraphShadow;
        aLifecycleReplaceInput.maIrShadow = aAuthorityInput.maIrShadow;
        aLifecycleReplaceInput.maMutation
            = MutationEvent::setFormula({ nPilotSheet, 1, 0 }, u"=A1*3");
        aLifecycleReplaceInput.mbCleanBaseline = true;

        const auto aLifecycleReplacePlan = buildLifecyclePilotTransition(aLifecycleReplaceInput);
        if (aLifecycleReplacePlan.meVerdict != LifecyclePilotVerdict::Applicable
            || aLifecycleReplacePlan.maSyncActions.size() != 1
            || aLifecycleReplacePlan.maSyncActions.front().meKind
                   != LifecycleSyncActionKind::ReplaceFormulaCell
            || !aLifecycleReplacePlan.maComputationalAfter.findCell({ nPilotSheet, 1, 0 }))
        {
            return fail("computational_substrate", "lifecycle replacement transition mismatch");
        }

        LifecyclePilotInput aLifecycleRemoveInput;
        aLifecycleRemoveInput.maComputationalShadow = aPilotShadow;
        aLifecycleRemoveInput.maGraphShadow = aAuthorityInput.maGraphShadow;
        aLifecycleRemoveInput.maIrShadow = aAuthorityInput.maIrShadow;
        aLifecycleRemoveInput.maMutation = MutationEvent::clearCell({ nPilotSheet, 1, 0 });
        aLifecycleRemoveInput.mbCleanBaseline = true;

        const auto aLifecycleRemovePlan = buildLifecyclePilotTransition(aLifecycleRemoveInput);
        if (aLifecycleRemovePlan.meVerdict != LifecyclePilotVerdict::Applicable
            || aLifecycleRemovePlan.maSyncActions.size() != 1
            || aLifecycleRemovePlan.maSyncActions.front().meKind
                   != LifecycleSyncActionKind::RemoveFormulaCell
            || aLifecycleRemovePlan.maComputationalAfter.findCell({ nPilotSheet, 1, 0 }))
        {
            return fail("computational_substrate", "lifecycle removal transition mismatch");
        }

        auto aMutableLifecycleState = bootstrapMutableComputationalSubstrateState(aPilotShadow);
        if (!applyMutableLifecycleTransition(aMutableLifecycleState, aLifecycleInsertPlan)
            || aMutableLifecycleState.mnAppliedMutationCount != 1
            || !aMutableLifecycleState.maFacade.getFormulaCellDescriptor({ nPilotSheet, 3, 0 }))
        {
            return fail("computational_substrate", "mutable lifecycle insertion mismatch");
        }
        const auto aLifecycleComparison = compareComputationalShadow(aMutableLifecycleState.maShadow,
            aMutableLifecycleState.maFacade, aMutableLifecycleState.maObservation);
        if (!aLifecycleComparison.mbFullMatch)
            return fail("computational_substrate", "mutable lifecycle shadow mismatch");
        if (!compareAdmittedCellStorage(aMutableLifecycleState.maCellStorage,
                aLifecycleInsertPlan.maComputationalAfter)
                 .mbFullMatch)
        {
            return fail("computational_substrate", "mutable lifecycle insertion storage mismatch");
        }
        if (!compareAdmittedFormulaCellLifetime(aMutableLifecycleState.maFormulaCellLifetime,
                aLifecycleInsertPlan.maComputationalAfter)
                 .mbFullMatch)
        {
            return fail("computational_substrate",
                "mutable lifecycle insertion formula lifetime mismatch");
        }
        if (!compareAdmittedWiringContainers(aMutableLifecycleState.maWiringContainers,
                aLifecycleInsertPlan.maGraphAfter)
                 .mbFullMatch)
        {
            return fail("computational_substrate",
                "mutable lifecycle insertion wiring mismatch");
        }
        if (!applyMutableLifecycleTransition(aMutableLifecycleState, aLifecycleRemovePlan)
            || aMutableLifecycleState.mnAppliedMutationCount != 2
            || aMutableLifecycleState.maFacade.getFormulaCellDescriptor({ nPilotSheet, 1, 0 }))
        {
            return fail("computational_substrate", "mutable lifecycle removal mismatch");
        }
        if (!compareAdmittedCellStorage(aMutableLifecycleState.maCellStorage,
                aLifecycleRemovePlan.maComputationalAfter)
                 .mbFullMatch)
        {
            return fail("computational_substrate", "mutable lifecycle removal storage mismatch");
        }
        if (!compareAdmittedFormulaCellLifetime(aMutableLifecycleState.maFormulaCellLifetime,
                aLifecycleRemovePlan.maComputationalAfter)
                 .mbFullMatch)
        {
            return fail("computational_substrate",
                "mutable lifecycle removal formula lifetime mismatch");
        }
        if (!compareAdmittedWiringContainers(aMutableLifecycleState.maWiringContainers,
                aLifecycleRemovePlan.maGraphAfter)
                 .mbFullMatch)
        {
            return fail("computational_substrate",
                "mutable lifecycle removal wiring mismatch");
        }

        {
            MutationEntryBuildInput aEntryInput;
            aEntryInput.maComputationalShadow = aPilotShadow;
            aEntryInput.maGraphShadow = aAuthorityInput.maGraphShadow;
            aEntryInput.maIrShadow = aAuthorityInput.maIrShadow;
            aEntryInput.maRequest
                = MutationEntryRequest::setFormula({ nPilotSheet, 3, 0 }, u"=A1+B1",
                    CellValue::number(119.0));
            aEntryInput.mbCleanBaseline = true;

            const auto oPath = classifyMutationEntryPath(aEntryInput);
            if (!oPath || *oPath != MutationEntryPath::Lifecycle)
                return fail("computational_substrate", "mutation entry lifecycle routing mismatch");

            const auto aEntryTransition = buildMutationEntryTransition(
                aEntryInput, aPilotFacade, aPilotObservation);
            if (!aEntryTransition.moLifecycleTransition
                || aEntryTransition.moLifecycleTransition->meVerdict
                       != LifecyclePilotVerdict::Applicable
                || aEntryTransition.moLifecycleTransition->maSyncActions.empty())
            {
                return fail("computational_substrate", "mutation entry lifecycle build mismatch");
            }

            auto aEntryState = bootstrapMutableComputationalSubstrateState(aPilotShadow);
            if (!applyMutableMutationEntryTransition(aEntryState, aEntryTransition))
                return fail("computational_substrate", "mutation entry lifecycle apply mismatch");
            if (!findMutationEntryComputationalAfter(aEntryTransition)
                || !compareAdmittedFormulaCellLifetime(aEntryState.maFormulaCellLifetime,
                       *findMutationEntryComputationalAfter(aEntryTransition))
                        .mbFullMatch)
            {
                return fail("computational_substrate",
                    "mutation entry lifecycle lifetime mismatch");
            }
        }

        const auto aStructuralAdmitted
            = classifyStructuralMutation(MutationEvent::insertRows(nPilotSheet, 1, 1));
        if (!aStructuralAdmitted.isAdmitted()
            || !aStructuralAdmitted.mbRequiresScalarStructuralSlice)
        {
            return fail("computational_substrate", "structural contract admission mismatch");
        }

        const auto aStructuralDeleteRowAdmitted
            = classifyStructuralMutation(MutationEvent::deleteRows(nPilotSheet, 1, 1));
        if (!aStructuralDeleteRowAdmitted.isAdmitted())
        {
            return fail("computational_substrate", "structural delete-row admission mismatch");
        }

        const auto aStructuralInsertColumnAdmitted
            = classifyStructuralMutation(MutationEvent::insertColumns(nPilotSheet, 1, 1));
        if (!aStructuralInsertColumnAdmitted.isAdmitted())
        {
            return fail("computational_substrate",
                "structural insert-column admission mismatch");
        }

        const auto aStructuralRejected
            = classifyStructuralMutation(MutationEvent::setFormula({ nPilotSheet, 1, 0 }, u"=A1"));
        if (aStructuralRejected.meMutationClass != StructuralMutationClass::Rejected)
        {
            return fail("computational_substrate", "structural contract rejection mismatch");
        }

        const auto aStructuralVerification = makeStructuralVerification(aStructuralAdmitted);
        if (aStructuralVerification.meQueueMode != StructuralVerificationMode::Exact
            || !aStructuralVerification.mbObserveIrOnly)
        {
            return fail("computational_substrate", "structural verification mismatch");
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(31);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(10.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(20.0));
            aBeforeFacade.setFormulaCell(
                { nSheet, 0, 2 }, u"=$A$2*1", CellValue::number(20.0));

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 0, 2 } };
            aBeforeObservation.maCellBroadcasters.push_back({
                { nSheet, 0, 1 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 0, 2 }, 1 } } });
            const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(32);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(10.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(20.0));
            aAfterFacade.setFormulaCell(
                { nSheet, 0, 3 }, u"=$A$3*1", CellValue::number(20.0));

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 0, 3 } };
            aAfterObservation.maCellBroadcasters.push_back({
                { nSheet, 0, 2 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 0, 3 }, 1 } } });
            const auto aAfterShadow = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::insertRows(nSheet, 1, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aStructuralPlan
                = buildStructuralPilotTransition(aStructuralInput, aAfterFacade, aAfterObservation);
            if (aStructuralPlan.meVerdict != StructuralPilotVerdict::Applicable
                || aStructuralPlan.maSyncActions.size() != 1
                || aStructuralPlan.maSyncActions.front().meKind
                       != StructuralSyncActionKind::InsertRows
                || aStructuralPlan.maComputationalAfter.getFormulaCellCount() != 1
                || aStructuralPlan.maRecalcPlan.maQueue.size() != 1
                || aStructuralPlan.maGraphAfter.getEdgeCount() < 1
                || aStructuralPlan.maReferenceUpdates.size() != 1
                || !aStructuralPlan.maReferenceUpdates.front().moAfterId.has_value()
                || !(aStructuralPlan.maReferenceUpdates.front().moAfterId->maAddress
                     == spreadsheetengine::api::CellAddress { nSheet, 0, 3 }))
            {
                return fail("computational_substrate", "structural insert-row transition mismatch");
            }

            auto aMutableStructuralState = bootstrapMutableComputationalSubstrateState(aBeforeShadow);
            if (!applyMutableStructuralTransition(aMutableStructuralState, aStructuralPlan)
                || aMutableStructuralState.mnAppliedMutationCount != 1
                || !aMutableStructuralState.maFacade.getFormulaCellDescriptor({ nSheet, 0, 3 }))
            {
                return fail("computational_substrate", "mutable structural state mismatch");
            }
            const auto aStructuralComparison = compareComputationalShadow(
                aMutableStructuralState.maShadow, aMutableStructuralState.maFacade,
                aMutableStructuralState.maObservation);
            if (!aStructuralComparison.mbFullMatch)
                return fail("computational_substrate", "mutable structural shadow mismatch");
            if (!compareAdmittedCellStorage(aMutableStructuralState.maCellStorage,
                    aStructuralPlan.maComputationalAfter)
                     .mbFullMatch)
            {
                return fail("computational_substrate", "mutable structural storage mismatch");
            }
            if (!compareAdmittedFormulaCellLifetime(aMutableStructuralState.maFormulaCellLifetime,
                    aStructuralPlan.maComputationalAfter)
                     .mbFullMatch)
            {
                return fail("computational_substrate",
                    "mutable structural formula lifetime mismatch");
            }
            if (!compareAdmittedWiringContainers(aMutableStructuralState.maWiringContainers,
                    aStructuralPlan.maGraphAfter)
                     .mbFullMatch)
            {
                return fail("computational_substrate",
                    "mutable structural wiring mismatch");
            }

            MutationEntryBuildInput aEntryInput;
            aEntryInput.maComputationalShadow = aBeforeShadow;
            aEntryInput.maGraphShadow = aBeforeGraph;
            aEntryInput.maIrShadow = aBeforeIr;
            aEntryInput.maRequest = MutationEntryRequest::insertRows(nSheet, 1, 1);
            aEntryInput.moObservedAfterComputationalShadow = aAfterShadow;
            aEntryInput.moObservedAfterIrShadow = aAfterIr;
            aEntryInput.mbCleanBaseline = true;

            const auto oPath = classifyMutationEntryPath(aEntryInput);
            if (!oPath || *oPath != MutationEntryPath::Structural)
                return fail("computational_substrate", "mutation entry structural routing mismatch");

            const auto aEntryTransition = buildMutationEntryTransition(
                aEntryInput, aAfterFacade, aAfterObservation);
            if (!aEntryTransition.moStructuralTransition
                || aEntryTransition.moStructuralTransition->meVerdict
                       != StructuralPilotVerdict::Applicable)
            {
                return fail("computational_substrate", "mutation entry structural build mismatch");
            }

            auto aEntryState = bootstrapMutableComputationalSubstrateState(aBeforeShadow);
            if (!applyMutableMutationEntryTransition(aEntryState, aEntryTransition))
                return fail("computational_substrate", "mutation entry structural apply mismatch");
            if (!findMutationEntryGraphAfter(aEntryTransition)
                || !compareAdmittedWiringContainers(aEntryState.maWiringContainers,
                       *findMutationEntryGraphAfter(aEntryTransition))
                        .mbFullMatch)
            {
                return fail("computational_substrate", "mutation entry structural wiring mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(41);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 1, 0 }, CellValue::number(2.0));
            aBeforeFacade.setFormulaCell({ nSheet, 2, 0 }, u"=$B$1+1", CellValue::number(3.0));

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 2, 0 } };
            aBeforeObservation.maCellBroadcasters.push_back({
                { nSheet, 1, 0 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 2, 0 }, 1 } } });
            const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(42);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(2.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=$A$1+1", CellValue::number(3.0));

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 } };
            aAfterObservation.maCellBroadcasters.push_back({
                { nSheet, 0, 0 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 1, 0 }, 1 } } });
            const auto aAfterShadow = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::deleteColumns(nSheet, 0, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aStructuralPlan
                = buildStructuralPilotTransition(aStructuralInput, aAfterFacade, aAfterObservation);
            if (aStructuralPlan.meVerdict != StructuralPilotVerdict::Applicable
                || aStructuralPlan.maSyncActions.size() != 1
                || aStructuralPlan.maSyncActions.front().meKind
                       != StructuralSyncActionKind::DeleteColumns
                || aStructuralPlan.maComputationalAfter.getCellCount() != 2
                || aStructuralPlan.maReferenceUpdates.size() != 1
                || !aStructuralPlan.maReferenceUpdates.front().maSummary.mbChanged
                || aStructuralPlan.maReferenceUpdates.front().mbRemovedByStructure)
            {
                return fail("computational_substrate",
                    "structural delete-column transition mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(51);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(10.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(20.0));
            aBeforeFacade.setFormulaCell(
                { nSheet, 0, 2 }, u"=$A$2*1", CellValue::number(20.0));

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 0, 2 } };
            aBeforeObservation.maCellBroadcasters.push_back({
                { nSheet, 0, 1 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 0, 2 }, 1 } } });
            const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(52);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(20.0));
            aAfterFacade.setFormulaCell(
                { nSheet, 0, 1 }, u"=$A$1*1", CellValue::number(20.0));

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 0, 1 } };
            aAfterObservation.maCellBroadcasters.push_back({
                { nSheet, 0, 0 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 0, 1 }, 1 } } });
            const auto aAfterShadow = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::deleteRows(nSheet, 0, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aStructuralPlan
                = buildStructuralPilotTransition(aStructuralInput, aAfterFacade, aAfterObservation);
            if (aStructuralPlan.meVerdict != StructuralPilotVerdict::Applicable
                || aStructuralPlan.maSyncActions.size() != 1
                || aStructuralPlan.maSyncActions.front().meKind
                       != StructuralSyncActionKind::DeleteRows
                || aStructuralPlan.maComputationalAfter.getFormulaCellCount() != 1
                || aStructuralPlan.maRecalcPlan.maQueue.size() != 1
                || aStructuralPlan.maGraphAfter.getEdgeCount() < 1
                || aStructuralPlan.maReferenceUpdates.size() != 1
                || !aStructuralPlan.maReferenceUpdates.front().moAfterId.has_value()
                || !(aStructuralPlan.maReferenceUpdates.front().moAfterId->maAddress
                     == spreadsheetengine::api::CellAddress { nSheet, 0, 1 }))
            {
                return fail("computational_substrate", "structural delete-row transition mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(61);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setFormulaCell(
                { nSheet, 1, 0 }, u"=$A$1+1", CellValue::number(2.0));

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 } };
            aBeforeObservation.maCellBroadcasters.push_back({
                { nSheet, 0, 0 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 1, 0 }, 1 } } });
            const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(62);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 1, 0 }, CellValue::number(1.0));
            aAfterFacade.setFormulaCell(
                { nSheet, 2, 0 }, u"=$B$1+1", CellValue::number(2.0));

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 2, 0 } };
            aAfterObservation.maCellBroadcasters.push_back({
                { nSheet, 1, 0 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 2, 0 }, 1 } } });
            const auto aAfterShadow = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::insertColumns(nSheet, 0, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aStructuralPlan
                = buildStructuralPilotTransition(aStructuralInput, aAfterFacade, aAfterObservation);
            if (aStructuralPlan.meVerdict != StructuralPilotVerdict::Applicable
                || aStructuralPlan.maSyncActions.size() != 1
                || aStructuralPlan.maSyncActions.front().meKind
                       != StructuralSyncActionKind::InsertColumns
                || aStructuralPlan.maComputationalAfter.getFormulaCellCount() != 1
                || aStructuralPlan.maRecalcPlan.maQueue.size() != 1
                || aStructuralPlan.maGraphAfter.getEdgeCount() < 1
                || aStructuralPlan.maReferenceUpdates.size() != 1
                || !aStructuralPlan.maReferenceUpdates.front().moAfterId.has_value()
                || !(aStructuralPlan.maReferenceUpdates.front().moAfterId->maAddress
                     == spreadsheetengine::api::CellAddress { nSheet, 2, 0 }))
            {
                return fail("computational_substrate",
                    "structural insert-column transition mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(71);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setFormulaCell(
                { nSheet, 2, 0 }, u"=SUM(Metrics)", CellValue::number(3.0));
            aBeforeFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$A$1:$A$2");

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 2, 0 } };
            aBeforeObservation.maAreaBroadcasters.push_back({
                { { nSheet, 0, 0 }, { nSheet, 0, 1 } },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 2, 0 }, 1 } } });
            const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(72);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 1, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 1, 1 }, CellValue::number(2.0));
            aAfterFacade.setFormulaCell(
                { nSheet, 3, 0 }, u"=SUM(Metrics)", CellValue::number(3.0));
            aAfterFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$B$1:$B$2");

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 3, 0 } };
            aAfterObservation.maAreaBroadcasters.push_back({
                { { nSheet, 1, 0 }, { nSheet, 1, 1 } },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 3, 0 }, 1 } } });
            const auto aAfterShadow = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            const auto aPredicted = structuralbuilddetail::buildPredictedStructuralComputationalShadow(
                aBeforeShadow, MutationEvent::insertColumns(nSheet, 0, 1), aAfterShadow);
            if (aPredicted.maNamedRanges != aAfterShadow.maNamedRanges)
            {
                return fail("computational_substrate",
                    "named-range structural prediction mismatch");
            }

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::insertColumns(nSheet, 0, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aStructuralPlan = buildStructuralPilotTransition(aStructuralInput,
                aAfterFacade, aAfterObservation, StructuralPilotBuildMode::Validation);
            if (aStructuralPlan.meVerdict != StructuralPilotVerdict::Applicable
                || aStructuralPlan.maContract.meMutationClass
                       != StructuralMutationClass::ValidationOnly
                || aStructuralPlan.maSyncActions.size() != 1
                || aStructuralPlan.maGraphAfter.getEdgeCount() < 1)
            {
                return fail("computational_substrate",
                    "named-range validation-only structural transition mismatch");
            }

            const auto aAuthorityRejected
                = buildStructuralPilotTransition(aStructuralInput, aAfterFacade, aAfterObservation);
            if (aAuthorityRejected.meVerdict != StructuralPilotVerdict::RejectedOutOfContract
                || aAuthorityRejected.maReason != u"structural_slice_out_of_contract")
            {
                return fail("computational_substrate",
                    "named-range structural authority rejection mismatch");
            }

            const auto aAuthorityCandidate = buildStructuralPilotTransition(aStructuralInput,
                aAfterFacade, aAfterObservation, StructuralPilotBuildMode::AuthorityOnly, true);
            if (aAuthorityCandidate.meVerdict != StructuralPilotVerdict::Applicable
                || aAuthorityCandidate.maContract.meMutationClass
                       != StructuralMutationClass::Admitted
                || aAuthorityCandidate.maSyncActions.size() != 1
                || aAuthorityCandidate.maGraphAfter.getEdgeCount() < 1)
            {
                return fail("computational_substrate",
                    "named-range structural authority candidate mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(73);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setFormulaCell(
                { nSheet, 2, 0 }, u"=SUM(Metrics)", CellValue::number(3.0));
            aBeforeFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$A$1:$A$2");

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 2, 0 } };
            aBeforeObservation.maAreaBroadcasters.push_back({
                { { nSheet, 0, 0 }, { nSheet, 0, 1 } },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 2, 0 }, 1 } } });
            const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(74);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 1, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 1, 1 }, CellValue::number(2.0));
            aAfterFacade.setFormulaCell(
                { nSheet, 3, 0 }, u"=SUM(Metrics)", CellValue::number(3.0));
            aAfterFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$B$1:$B$2");

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 3, 0 } };
            aAfterObservation.maAreaBroadcasters.push_back({
                { { nSheet, 1, 0 }, { nSheet, 1, 1 } },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 3, 0 }, 1 } } });
            const auto aAfterShadow = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            const auto aPredicted = structuralbuilddetail::buildPredictedStructuralComputationalShadow(
                aBeforeShadow, MutationEvent::insertColumns(nSheet, 0, 1), aAfterShadow);
            if (aPredicted.maNamedRanges != aAfterShadow.maNamedRanges)
            {
                return fail("computational_substrate",
                    "named-range explicit-sheet-prefix prediction mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(81);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setFormulaCell(
                { nSheet, 2, 0 }, u"=SUM(Metrics)", CellValue::number(3.0));
            aBeforeFacade.addNamedRange(u"Metrics", std::nullopt, { nSheet, 0, 0 },
                u"$A$1:$A$1~$A$2:$A$2");

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 2, 0 } };
            const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(82);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 1, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 1, 1 }, CellValue::number(2.0));
            aAfterFacade.setFormulaCell(
                { nSheet, 3, 0 }, u"=SUM(Metrics)", CellValue::number(3.0));
            aAfterFacade.addNamedRange(u"Metrics", std::nullopt, { nSheet, 0, 0 },
                u"$B$1:$B$1~$B$2:$B$2");

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 3, 0 } };
            const auto aAfterShadow = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::insertColumns(nSheet, 0, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aStructuralPlan = buildStructuralPilotTransition(aStructuralInput,
                aAfterFacade, aAfterObservation, StructuralPilotBuildMode::Validation);
            if (aStructuralPlan.meVerdict != StructuralPilotVerdict::RejectedOutOfContract
                || aStructuralPlan.maReason != u"structural_slice_out_of_contract")
            {
                return fail("computational_substrate",
                    "named-range multi-area structural rejection mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(91);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 2, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(92);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(2.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(2.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 1 }, 2, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 1 }, { nSheet, 1, 2 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            ComputationalWorkbookShadow aCorruptedAfterShadow = aAfterShadow;
            aCorruptedAfterShadow.maFormulaGroups.clear();
            for (auto& rSheet : aCorruptedAfterShadow.maSheets)
            {
                for (auto& rCell : rSheet.maCells)
                    rCell.moFormulaGroup.reset();
            }

            const auto aPredicted = structuralbuilddetail::buildPredictedStructuralComputationalShadow(
                aBeforeShadow, MutationEvent::insertRows(nSheet, 0, 1), aCorruptedAfterShadow, false);
            if (!structuralbuilddetail::matchesPredictedStructuralPopulation(aPredicted, aAfterShadow))
            {
                return fail("computational_substrate",
                    "shared-group preserve prediction should not depend on observed topology");
            }

            const auto* pAnchor = aPredicted.findCell({ nSheet, 1, 1 });
            const auto* pMember = aPredicted.findCell({ nSheet, 1, 2 });
            if (!pAnchor || !pMember || !pAnchor->moFormulaGroup || !pMember->moFormulaGroup
                || !(*pAnchor->moFormulaGroup == ShadowFormulaGroupId { { nSheet, 1, 1 }, 2 })
                || !(*pMember->moFormulaGroup == ShadowFormulaGroupId { { nSheet, 1, 1 }, 2 }))
            {
                return fail("computational_substrate",
                    "shared-group preserve bindings mismatch");
            }

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::insertRows(nSheet, 0, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aAuthorityCandidate = buildStructuralPilotTransition(aStructuralInput,
                aAfterFacade, aAfterObservation, StructuralPilotBuildMode::AuthorityOnly, false, true);
            if (aAuthorityCandidate.meVerdict != StructuralPilotVerdict::Applicable
                || aAuthorityCandidate.maContract.meMutationClass
                       != StructuralMutationClass::Admitted)
            {
                return fail("computational_substrate",
                    "shared-group preserve authority candidate mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aAuthorityCandidate.maDependencySnapshot, aAuthorityCandidate.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aAuthorityCandidate.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch)
            {
                return fail("computational_substrate",
                    "shared-group preserve computational exactness mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aAuthorityCandidate.maGraphAfter, aAuthorityCandidate.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group preserve graph exactness mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aAuthorityCandidate.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aAuthorityCandidate.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aAuthorityCandidate.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group preserve IR exactness mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(93);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 2, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(94);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(2.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(4.0));

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 2 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            ComputationalWorkbookShadow aCorruptedAfterShadow = aAfterShadow;
            ShadowFormulaGroupRecord aFakeGroup;
            aFakeGroup.maId = { { nSheet, 1, 0 }, 2 };
            aFakeGroup.maDescriptor = { { nSheet, 1, 0 }, 2, true };
            aFakeGroup.maMembers = { { { nSheet, 1, 0 } }, { { nSheet, 1, 2 } } };
            aCorruptedAfterShadow.maFormulaGroups.push_back(aFakeGroup);
            const spreadsheetengine::api::CellAddress aFirstSplitAddress { nSheet, 1, 0 };
            const spreadsheetengine::api::CellAddress aSecondSplitAddress { nSheet, 1, 2 };
            for (auto& rSheet : aCorruptedAfterShadow.maSheets)
            {
                for (auto& rCell : rSheet.maCells)
                {
                    if (rCell.maId.maAddress == aFirstSplitAddress
                        || rCell.maId.maAddress == aSecondSplitAddress)
                    {
                        rCell.moFormulaGroup = aFakeGroup.maId;
                    }
                }
            }

            const auto aPredicted = structuralbuilddetail::buildPredictedStructuralComputationalShadow(
                aBeforeShadow, MutationEvent::insertRows(nSheet, 1, 1), aCorruptedAfterShadow, false);
            if (!structuralbuilddetail::matchesPredictedStructuralPopulation(aPredicted, aAfterShadow)
                || !aPredicted.maFormulaGroups.empty())
            {
                return fail("computational_substrate",
                    "shared-group split prediction should not depend on observed topology");
            }

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::insertRows(nSheet, 1, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aAuthorityCandidate = buildStructuralPilotTransition(aStructuralInput,
                aAfterFacade, aAfterObservation, StructuralPilotBuildMode::AuthorityOnly, false, true);
            if (aAuthorityCandidate.meVerdict != StructuralPilotVerdict::Applicable
                || aAuthorityCandidate.maContract.meMutationClass
                       != StructuralMutationClass::Admitted
                || !aAuthorityCandidate.maComputationalAfter.maFormulaGroups.empty())
            {
                return fail("computational_substrate",
                    "shared-group split authority candidate mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aAuthorityCandidate.maDependencySnapshot, aAuthorityCandidate.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aAuthorityCandidate.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch)
            {
                return fail("computational_substrate",
                    "shared-group split computational exactness mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aAuthorityCandidate.maGraphAfter, aAuthorityCandidate.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group split graph exactness mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aAuthorityCandidate.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aAuthorityCandidate.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aAuthorityCandidate.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group split IR exactness mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(95);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(96);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 3 }, CellValue::number(3.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(4.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 3 }, u"=A4*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 2 }, 2, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree
                = { { nSheet, 1, 0 }, { nSheet, 1, 2 }, { nSheet, 1, 3 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            ComputationalWorkbookShadow aCorruptedAfterShadow = aAfterShadow;
            aCorruptedAfterShadow.maFormulaGroups.clear();
            for (auto& rSheet : aCorruptedAfterShadow.maSheets)
            {
                for (auto& rCell : rSheet.maCells)
                    rCell.moFormulaGroup.reset();
            }

            const auto aPredicted = structuralbuilddetail::buildPredictedStructuralComputationalShadow(
                aBeforeShadow, MutationEvent::insertRows(nSheet, 1, 1), aCorruptedAfterShadow, false);
            if (!structuralbuilddetail::matchesPredictedStructuralPopulation(aPredicted, aAfterShadow))
            {
                return fail("computational_substrate",
                    "shared-group rebuild prediction should not depend on observed topology");
            }

            const auto* pUpper = aPredicted.findCell({ nSheet, 1, 0 });
            const auto* pAnchor = aPredicted.findCell({ nSheet, 1, 2 });
            const auto* pMember = aPredicted.findCell({ nSheet, 1, 3 });
            if (!pUpper || pUpper->moFormulaGroup || !pAnchor || !pMember || !pAnchor->moFormulaGroup
                || !pMember->moFormulaGroup
                || !(*pAnchor->moFormulaGroup == ShadowFormulaGroupId { { nSheet, 1, 2 }, 2 })
                || !(*pMember->moFormulaGroup == ShadowFormulaGroupId { { nSheet, 1, 2 }, 2 }))
            {
                return fail("computational_substrate",
                    "shared-group rebuild bindings mismatch");
            }

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::insertRows(nSheet, 1, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aAuthorityCandidate = buildStructuralPilotTransition(aStructuralInput,
                aAfterFacade, aAfterObservation, StructuralPilotBuildMode::AuthorityOnly, false, true);
            if (aAuthorityCandidate.meVerdict != StructuralPilotVerdict::Applicable
                || aAuthorityCandidate.maContract.meMutationClass
                       != StructuralMutationClass::Admitted)
            {
                return fail("computational_substrate",
                    "shared-group rebuild authority candidate mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aAuthorityCandidate.maDependencySnapshot, aAuthorityCandidate.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aAuthorityCandidate.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch)
            {
                return fail("computational_substrate",
                    "shared-group rebuild computational exactness mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aAuthorityCandidate.maGraphAfter, aAuthorityCandidate.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group rebuild graph exactness mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aAuthorityCandidate.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aAuthorityCandidate.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aAuthorityCandidate.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group rebuild IR exactness mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(105);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(10.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(20.0));
            aBeforeFacade.setFormulaCell(
                { nSheet, 0, 2 }, u"=$A$2*1", CellValue::number(20.0));

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 0, 2 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(106);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(10.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(20.0));
            aAfterFacade.setFormulaCell(
                { nSheet, 0, 3 }, u"=$A$2*1", CellValue::number(0.0));

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 0, 3 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::insertRows(nSheet, 1, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aStructuralPlan
                = buildStructuralPilotTransition(aStructuralInput, aAfterFacade, aAfterObservation);
            if (aStructuralPlan.meVerdict != StructuralPilotVerdict::RepairDetected
                || aStructuralPlan.maReason != u"structural_formula_reference_update_mismatch"
                || !aStructuralPlan.mbRequiresRollback)
            {
                return fail("computational_substrate",
                    "plain structural repair bucket mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(107);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setFormulaCell(
                { nSheet, 2, 0 }, u"=SUM(Metrics)", CellValue::number(3.0));
            aBeforeFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$A$1:$A$2");

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 2, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(108);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 1, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 1, 1 }, CellValue::number(2.0));
            aAfterFacade.setFormulaCell(
                { nSheet, 3, 0 }, u"=SUM(Metrics)+1", CellValue::number(4.0));
            aAfterFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$B$1:$B$2");

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 3, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::insertColumns(nSheet, 0, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aStructuralPlan = buildStructuralPilotTransition(aStructuralInput,
                aAfterFacade, aAfterObservation, StructuralPilotBuildMode::Validation);
            if (aStructuralPlan.meVerdict != StructuralPilotVerdict::RepairDetected
                || aStructuralPlan.maReason
                       != u"structural_named_range_reference_update_mismatch"
                || !aStructuralPlan.mbRequiresRollback)
            {
                return fail("computational_substrate",
                    "named-range structural repair bucket mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(109);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 2, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(110);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(2.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(2.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*3", CellValue::number(6.0));

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 1 }, { nSheet, 1, 2 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::insertRows(nSheet, 0, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aStructuralPlan = buildStructuralPilotTransition(aStructuralInput,
                aAfterFacade, aAfterObservation, StructuralPilotBuildMode::Validation, false, true);
            if (aStructuralPlan.meVerdict != StructuralPilotVerdict::RepairDetected
                || aStructuralPlan.maReason
                       != u"structural_shared_group_reference_update_mismatch"
                || !aStructuralPlan.mbRequiresRollback)
            {
                return fail("computational_substrate",
                    "shared-group structural repair bucket mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(101);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(102);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.setCell({ nSheet, 1, 0 }, CellValue::number(9.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 1 }, 2, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 1 }, { nSheet, 1, 2 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            AuthorityPilotInput aAuthorityInput;
            aAuthorityInput.maComputationalShadow = aBeforeShadow;
            aAuthorityInput.maGraphShadow = aBeforeGraph;
            aAuthorityInput.maIrShadow = aBeforeIr;
            aAuthorityInput.maMutation = MutationEvent::setScalarValue({ nSheet, 1, 0 });
            aAuthorityInput.moScalarValueAfter = CellValue::number(9.0);
            aAuthorityInput.moObservedAfterComputationalShadow = aAfterShadow;
            aAuthorityInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aAuthorityInput.mbCleanBaseline = true;

            spreadsheetengine::api::String aPredictionReason;
            const auto oPredictedShadow
                = authoritybuilddetail::buildPredictedSharedGroupNonStructuralComputationalShadow(
                    aAuthorityInput, aPredictionReason);
            if (!oPredictedShadow || !aPredictionReason.empty())
                return fail("computational_substrate",
                    "shared-group non-structural authority prediction mismatch");
            if (!oPredictedShadow->findCell({ nSheet, 1, 0 })
                || oPredictedShadow->findCell({ nSheet, 1, 0 })->hasFormula()
                || oPredictedShadow->maFormulaGroups.size() != 1
                || !(oPredictedShadow->maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nSheet, 1, 1 }, 2 }))
            {
                return fail("computational_substrate",
                    "shared-group non-structural authority topology mismatch");
            }

            ComputationalWorkbookShadow aCorruptedAfterShadow = aAfterShadow;
            aCorruptedAfterShadow.maFormulaGroups.clear();
            for (auto& rSheet : aCorruptedAfterShadow.maSheets)
            {
                for (auto& rCell : rSheet.maCells)
                    rCell.moFormulaGroup.reset();
            }

            AuthorityPilotInput aRejectedInput = aAuthorityInput;
            aRejectedInput.moObservedAfterComputationalShadow = aCorruptedAfterShadow;
            spreadsheetengine::api::String aRejectedReason;
            const auto oRejectedPrediction
                = authoritybuilddetail::buildPredictedSharedGroupNonStructuralComputationalShadow(
                    aRejectedInput, aRejectedReason);
            if (oRejectedPrediction || aRejectedReason != u"shared_group_topology_out_of_contract")
            {
                return fail("computational_substrate",
                    "shared-group non-structural authority topology validation mismatch");
            }

            const auto aAuthorityPlan = buildAuthorityPilotTransition(aAuthorityInput);
            if (aAuthorityPlan.meVerdict != AuthorityPilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group non-structural authority verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aAuthorityPlan.maDependencySnapshot, aAuthorityPlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aAuthorityPlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch)
            {
                return fail("computational_substrate",
                    "shared-group non-structural authority computational mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aAuthorityPlan.maGraphAfter, aAuthorityPlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group non-structural authority graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aAuthorityPlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aAuthorityPlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aAuthorityPlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group non-structural authority IR mismatch");
            }

            auto aMutableState = bootstrapMutableComputationalSubstrateState(aBeforeShadow);
            if (!applyMutableAuthorityTransition(aMutableState, aAuthorityPlan))
            {
                return fail("computational_substrate",
                    "shared-group non-structural mutable authority apply mismatch");
            }
            if (!compareComputationalShadow(
                    aMutableState.maShadow, aMutableState.maFacade, aMutableState.maObservation)
                     .mbFullMatch)
            {
                return fail("computational_substrate",
                    "shared-group non-structural mutable authority shadow mismatch");
            }
            if (!compareAdmittedFormulaCellLifetime(aMutableState.maFormulaCellLifetime,
                    aAuthorityPlan.maComputationalAfter)
                     .mbFullMatch)
            {
                return fail("computational_substrate",
                    "shared-group non-structural mutable authority lifetime mismatch");
            }

            const auto oRebuiltGroup = aMutableState.maFacade.getFormulaGroupDescriptor({ nSheet, 1, 1 });
            if (!oRebuiltGroup || oRebuiltGroup->mnLength != 2
                || aMutableState.maFacade.getFormulaGroupDescriptor({ nSheet, 1, 0 }))
            {
                return fail("computational_substrate",
                    "shared-group non-structural mutable authority facade mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(103);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(104);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*5", CellValue::number(10.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(6.0));

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula({ nSheet, 1, 1 }, u"=A2*5");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(10.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable
                || aLifecyclePlan.maSyncActions.size() != 1
                || aLifecyclePlan.maSyncActions.front().meKind
                       != LifecycleSyncActionKind::ReplaceFormulaCell)
            {
                return fail("computational_substrate",
                    "shared-group non-structural lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aLifecyclePlan.maComputationalAfter.maFormulaGroups.empty())
            {
                return fail("computational_substrate",
                    "shared-group non-structural lifecycle computational mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aLifecyclePlan.maGraphAfter, aLifecyclePlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group non-structural lifecycle graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aLifecyclePlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aLifecyclePlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aLifecyclePlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group non-structural lifecycle IR mismatch");
            }

            auto aMutableState = bootstrapMutableComputationalSubstrateState(aBeforeShadow);
            if (!applyMutableLifecycleTransition(aMutableState, aLifecyclePlan))
            {
                return fail("computational_substrate",
                    "shared-group non-structural mutable lifecycle apply mismatch");
            }
            if (!compareComputationalShadow(
                    aMutableState.maShadow, aMutableState.maFacade, aMutableState.maObservation)
                     .mbFullMatch)
            {
                return fail("computational_substrate",
                    "shared-group non-structural mutable lifecycle shadow mismatch");
            }
            if (!compareAdmittedFormulaCellLifetime(aMutableState.maFormulaCellLifetime,
                    aLifecyclePlan.maComputationalAfter)
                     .mbFullMatch)
            {
                return fail("computational_substrate",
                    "shared-group non-structural mutable lifecycle lifetime mismatch");
            }
            if (aMutableState.maFacade.getFormulaGroupDescriptor({ nSheet, 1, 0 })
                || aMutableState.maFacade.getFormulaGroupDescriptor({ nSheet, 1, 1 })
                || aMutableState.maFacade.getFormulaGroupDescriptor({ nSheet, 1, 2 }))
            {
                return fail("computational_substrate",
                    "shared-group non-structural mutable lifecycle facade mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(105);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(106);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula({ nSheet, 1, 1 }, u"=A2*2");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(4.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group same-text preserve lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nSheet, 1, 0 }, 3 }))
            {
                return fail("computational_substrate",
                    "shared-group same-text preserve lifecycle mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(106);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=COUNTA(Metrics)+A1",
                CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);
            aBeforeFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 2, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(107);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$A$1:$A$2");
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=COUNTA(Metrics)+A1",
                CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);
            aAfterFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 2, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula(
                { nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(4.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group named-range preserve lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbNamedRangeMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nSheet, 1, 0 }, 3 }))
            {
                return fail("computational_substrate",
                    "shared-group named-range preserve lifecycle mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aLifecyclePlan.maGraphAfter, aLifecyclePlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range preserve graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aLifecyclePlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aLifecyclePlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aLifecyclePlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range preserve IR mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(1061);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=COUNTA(Metrics)+A1*3",
                CellValue::number(5.0), FormulaCellKind::Ordinary, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2*2",
                CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3*2",
                CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 1 }, 2, true);
            aBeforeFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 2, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(1062);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$A$1:$A$2");
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=COUNTA(Metrics)+A1*3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2*3",
                CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3*2",
                CellValue::number(8.0), FormulaCellKind::Ordinary, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 0 }, 2, true);
            aAfterFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 2, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula(
                { nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2*3");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(8.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group named-range regroup lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbNamedRangeMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nSheet, 1, 0 }, 2 }))
            {
                return fail("computational_substrate",
                    "shared-group named-range regroup lifecycle mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aLifecyclePlan.maGraphAfter, aLifecyclePlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range regroup graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aLifecyclePlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aLifecyclePlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aLifecyclePlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range regroup IR mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(1063);
            const auto nSheet = aBeforeFacade.addSheet(u"Data");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setCell({ nSheet, 0, 3 }, CellValue::number(4.0));
            aBeforeFacade.setCell({ nSheet, 0, 4 }, CellValue::number(5.0));
            aBeforeFacade.setCell({ nSheet, 0, 5 }, CellValue::number(6.0));
            aBeforeFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Data.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=COUNTA(Metrics)+A1*3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2*3",
                CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3*2",
                CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 3 }, u"=COUNTA(Metrics)+A4*2",
                CellValue::number(10.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 4 }, u"=COUNTA(Metrics)+A5*3",
                CellValue::number(17.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 5 }, u"=COUNTA(Metrics)+A6*3",
                CellValue::number(20.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 2, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 2 }, 2, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 4 }, 2, true);
            aBeforeFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 1, 3 }, { nSheet, 1, 4 }, { nSheet, 1, 5 }, { nSheet, 2, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(1064);
            aAfterFacade.addSheet(u"Data");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.setCell({ nSheet, 0, 3 }, CellValue::number(4.0));
            aAfterFacade.setCell({ nSheet, 0, 4 }, CellValue::number(5.0));
            aAfterFacade.setCell({ nSheet, 0, 5 }, CellValue::number(6.0));
            aAfterFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Data.$A$1:$A$2");
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=COUNTA(Metrics)+A1*3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2*3",
                CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3*3",
                CellValue::number(11.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 3 }, u"=COUNTA(Metrics)+A4*2",
                CellValue::number(10.0), FormulaCellKind::Ordinary, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 4 }, u"=COUNTA(Metrics)+A5*3",
                CellValue::number(17.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 5 }, u"=COUNTA(Metrics)+A6*3",
                CellValue::number(20.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 4 }, 2, true);
            aAfterFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            const auto aNamedRangeBoundary
                = facade::consumers::classifySharedFormulaNamedRangeMutationBoundary(
                    aBeforeFacade, aAfterFacade,
                    MutationEvent::setFormula({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3*3"));
            if (aNamedRangeBoundary.meBoundary
                    != facade::consumers::SharedFormulaNamedRangeMutationBoundary::
                           GlobalSingleAreaSameSheet)
            {
                return fail("computational_substrate",
                    "shared-group named-range split-outcome boundary mismatch");
            }
            const auto aClassification = facade::consumers::classifySharedFormulaMutation(
                aBeforeFacade, aAfterFacade,
                MutationEvent::setFormula({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3*3"));
            if (aClassification.meFamily != facade::consumers::SharedFormulaMutationFamily::Regroup
                || aClassification.maTransition.meKind
                       != facade::consumers::SharedFormulaGroupTransitionKind::Split)
            {
                return fail("computational_substrate",
                    "shared-group named-range split-outcome classification mismatch");
            }

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 1, 3 }, { nSheet, 1, 4 }, { nSheet, 1, 5 }, { nSheet, 2, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula(
                { nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3*3");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(11.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group named-range split-outcome lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbNamedRangeMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 2
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nSheet, 1, 0 }, 3 })
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.back().maId
                     == ShadowFormulaGroupId { { nSheet, 1, 4 }, 2 }))
            {
                return fail("computational_substrate",
                    "shared-group named-range split-outcome lifecycle mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aLifecyclePlan.maGraphAfter, aLifecyclePlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range split-outcome graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aLifecyclePlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aLifecyclePlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aLifecyclePlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range split-outcome IR mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(1065);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=COUNTA(Metrics)+A1*2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2*2",
                CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 2, true);
            aBeforeFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 2, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(1066);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$A$1:$A$2");
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=COUNTA(Metrics)+A1*2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2*2",
                CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3*2",
                CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);
            aAfterFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 2, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula(
                { nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3*2");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(8.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group named-range one-sided insert lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbNamedRangeMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nSheet, 1, 0 }, 3 }))
            {
                return fail("computational_substrate",
                    "shared-group named-range one-sided insert lifecycle mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aLifecyclePlan.maGraphAfter, aLifecyclePlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range one-sided insert graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aLifecyclePlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aLifecyclePlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aLifecyclePlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range one-sided insert IR mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(108);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=COUNTA(Metrics)+A1",
                CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);
            aBeforeFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 2, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(109);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$A$1:$A$2");
            aAfterFacade.setCell({ nSheet, 1, 0 }, CellValue::number(9.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 1 }, 2, true);
            aAfterFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 1 }, { nSheet, 1, 2 }, { nSheet, 2, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            AuthorityPilotInput aAuthorityInput;
            aAuthorityInput.maComputationalShadow = aBeforeShadow;
            aAuthorityInput.maGraphShadow = aBeforeGraph;
            aAuthorityInput.maIrShadow = aBeforeIr;
            aAuthorityInput.maMutation = MutationEvent::setScalarValue({ nSheet, 1, 0 });
            aAuthorityInput.moScalarValueAfter = CellValue::number(9.0);
            aAuthorityInput.moObservedAfterComputationalShadow = aAfterShadow;
            aAuthorityInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aAuthorityInput.mbCleanBaseline = true;

            const auto aAuthorityPlan = buildAuthorityPilotTransition(aAuthorityInput);
            if (aAuthorityPlan.meVerdict != AuthorityPilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group named-range member-exit authority verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aAuthorityPlan.maDependencySnapshot, aAuthorityPlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aAuthorityPlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch)
            {
                return fail("computational_substrate",
                    "shared-group named-range member-exit computational exactness mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aAuthorityPlan.maGraphAfter, aAuthorityPlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range member-exit graph exactness mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aAuthorityPlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aAuthorityPlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aAuthorityPlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range member-exit IR exactness mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(109);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=COUNTA(Metrics)+A1",
                CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);
            aBeforeFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 2, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(110);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$A$1:$A$2");
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 1 }, 2, true);
            aAfterFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 1 }, { nSheet, 1, 2 }, { nSheet, 2, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            AuthorityPilotInput aAuthorityInput;
            aAuthorityInput.maComputationalShadow = aBeforeShadow;
            aAuthorityInput.maGraphShadow = aBeforeGraph;
            aAuthorityInput.maIrShadow = aBeforeIr;
            aAuthorityInput.maMutation = MutationEvent::clearCell({ nSheet, 1, 0 });
            aAuthorityInput.moObservedAfterComputationalShadow = aAfterShadow;
            aAuthorityInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aAuthorityInput.mbCleanBaseline = true;

            const auto aAuthorityPlan = buildAuthorityPilotTransition(aAuthorityInput);
            if (aAuthorityPlan.meVerdict != AuthorityPilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group named-range clear member-exit authority verdict mismatch");
            }

            const auto aObservationBuildOptions
                = authoritybuilddetail::buildAuthorityObservationBuildOptions(
                    aBeforeShadow, aAfterShadow, MutationEvent::clearCell({ nSheet, 1, 0 }));
            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aAuthorityPlan.maDependencySnapshot, aAuthorityPlan.maRecalcPlan,
                aObservationBuildOptions);
            const auto aComputationalComparison = compareComputationalShadow(
                aAuthorityPlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch)
            {
                return fail("computational_substrate",
                    "shared-group named-range clear member-exit computational exactness mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aAuthorityPlan.maGraphAfter, aAuthorityPlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range clear member-exit graph exactness mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aAuthorityPlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aAuthorityPlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aAuthorityPlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range clear member-exit IR exactness mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(109);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=COUNTA(Metrics)+A1",
                CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);
            aBeforeFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 2, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(110);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.addNamedRange(
                u"Metrics", std::nullopt, { nSheet, 0, 0 }, u"$Pilot.$A$1:$A$2");
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=COUNTA(Metrics)+A1*10",
                CellValue::number(12.0), FormulaCellKind::Ordinary, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 1 }, 2, true);
            aAfterFacade.setFormulaCell({ nSheet, 2, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 2, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation
                = MutationEvent::setFormula({ nSheet, 1, 0 }, u"=COUNTA(Metrics)+A1*10");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(12.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group named-range setformula member-exit lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nSheet, 1, 1 }, 2 }))
            {
                return fail("computational_substrate",
                    "shared-group named-range setformula member-exit computational mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aLifecyclePlan.maGraphAfter, aLifecyclePlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range setformula member-exit graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aLifecyclePlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aLifecyclePlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aLifecyclePlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group named-range setformula member-exit IR mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(110);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*3", CellValue::number(3.0),
                FormulaCellKind::Ordinary, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 1 }, 2, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(111);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*3", CellValue::number(3.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*3", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::Ordinary, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 0 }, 2, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula({ nSheet, 1, 1 }, u"=A2*3");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(6.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group regroup lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nSheet, 1, 0 }, 2 }))
            {
                return fail("computational_substrate",
                    "shared-group regroup lifecycle mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(109);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setCell({ nSheet, 0, 3 }, CellValue::number(4.0));
            aBeforeFacade.setCell({ nSheet, 0, 4 }, CellValue::number(5.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 3 }, u"=A4*2", CellValue::number(8.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 4 }, u"=A5*2", CellValue::number(10.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 2, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 3 }, 2, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 3 },
                { nSheet, 1, 4 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(110);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.setCell({ nSheet, 0, 3 }, CellValue::number(4.0));
            aAfterFacade.setCell({ nSheet, 0, 4 }, CellValue::number(5.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 3 }, u"=A4*2", CellValue::number(8.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 4 }, u"=A5*2", CellValue::number(10.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 0 }, 5, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 1, 3 }, { nSheet, 1, 4 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula({ nSheet, 1, 2 }, u"=A3*2");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(6.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group merge lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nSheet, 1, 0 }, 5 }))
            {
                return fail("computational_substrate",
                    "shared-group merge lifecycle mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(111);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setCell({ nSheet, 0, 3 }, CellValue::number(4.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*3", CellValue::number(3.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*3", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 3 }, u"=A4*2", CellValue::number(8.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 2, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 2 }, 2, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 1, 3 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(112);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.setCell({ nSheet, 0, 3 }, CellValue::number(4.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*3", CellValue::number(3.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*3", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*3", CellValue::number(9.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 3 }, u"=A4*2", CellValue::number(8.0),
                FormulaCellKind::Ordinary, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 1, 3 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula({ nSheet, 1, 2 }, u"=A3*3");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(9.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group replacement-merge lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            const auto* pResidualCell
                = aLifecyclePlan.maComputationalAfter.findCell({ nSheet, 1, 3 });
            if (!aComputationalComparison.mbFullMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nSheet, 1, 0 }, 3 })
                || !pResidualCell || pResidualCell->moFormulaGroup.has_value())
            {
                return fail("computational_substrate",
                    "shared-group replacement-merge lifecycle mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(113);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 2, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(114);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*2", CellValue::number(2.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 0 }, 3, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula({ nSheet, 1, 2 }, u"=A3*2");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(6.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group one-sided insert lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nSheet, 1, 0 }, 3 }))
            {
                return fail("computational_substrate",
                    "shared-group one-sided insert lifecycle mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(115);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setCell({ nSheet, 0, 3 }, CellValue::number(4.0));
            aBeforeFacade.setCell({ nSheet, 0, 4 }, CellValue::number(5.0));
            aBeforeFacade.setCell({ nSheet, 0, 5 }, CellValue::number(6.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*3", CellValue::number(3.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*3", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 3 }, u"=A4*2", CellValue::number(8.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 4 }, u"=A5*3", CellValue::number(15.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 5 }, u"=A6*3", CellValue::number(18.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 2, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 2 }, 2, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 4 }, 2, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 1, 3 }, { nSheet, 1, 4 }, { nSheet, 1, 5 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(116);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.setCell({ nSheet, 0, 3 }, CellValue::number(4.0));
            aAfterFacade.setCell({ nSheet, 0, 4 }, CellValue::number(5.0));
            aAfterFacade.setCell({ nSheet, 0, 5 }, CellValue::number(6.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*3", CellValue::number(3.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*3", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*3", CellValue::number(9.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 3 }, u"=A4*3", CellValue::number(12.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 4 }, u"=A5*3", CellValue::number(15.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 5 }, u"=A6*3", CellValue::number(18.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 0 }, 6, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 1, 3 }, { nSheet, 1, 4 }, { nSheet, 1, 5 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula({ nSheet, 1, 2 }, u"=A3*3");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(9.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group multi-group collapse lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nSheet, 1, 0 }, 6 }))
            {
                return fail("computational_substrate",
                    "shared-group multi-group collapse lifecycle mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(117);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setCell({ nSheet, 0, 3 }, CellValue::number(4.0));
            aBeforeFacade.setCell({ nSheet, 0, 4 }, CellValue::number(5.0));
            aBeforeFacade.setCell({ nSheet, 0, 5 }, CellValue::number(6.0));
            aBeforeFacade.setCell({ nSheet, 0, 6 }, CellValue::number(7.0));
            aBeforeFacade.setCell({ nSheet, 0, 7 }, CellValue::number(8.0));
            aBeforeFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*4", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*4", CellValue::number(8.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*3", CellValue::number(9.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 3 }, u"=A4*3", CellValue::number(12.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 4 }, u"=A5*2", CellValue::number(10.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 5 }, u"=A6*2", CellValue::number(12.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 6 }, u"=A7*4", CellValue::number(28.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nSheet, 1, 7 }, u"=A8*4", CellValue::number(32.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 0 }, 2, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 2 }, 2, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 4 }, 2, true);
            aBeforeFacade.addFormulaGroup({ nSheet, 1, 6 }, 2, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 1, 3 }, { nSheet, 1, 4 }, { nSheet, 1, 5 }, { nSheet, 1, 6 },
                { nSheet, 1, 7 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(118);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nSheet, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.setCell({ nSheet, 0, 3 }, CellValue::number(4.0));
            aAfterFacade.setCell({ nSheet, 0, 4 }, CellValue::number(5.0));
            aAfterFacade.setCell({ nSheet, 0, 5 }, CellValue::number(6.0));
            aAfterFacade.setCell({ nSheet, 0, 6 }, CellValue::number(7.0));
            aAfterFacade.setCell({ nSheet, 0, 7 }, CellValue::number(8.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=A1*4", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 1 }, u"=A2*4", CellValue::number(8.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 2 }, u"=A3*4", CellValue::number(12.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 3 }, u"=A4*4", CellValue::number(16.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 4 }, u"=A5*4", CellValue::number(20.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 5 }, u"=A6*4", CellValue::number(24.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 6 }, u"=A7*4", CellValue::number(28.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nSheet, 1, 7 }, u"=A8*4", CellValue::number(32.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nSheet, 1, 0 }, 8, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 }, { nSheet, 1, 1 }, { nSheet, 1, 2 },
                { nSheet, 1, 3 }, { nSheet, 1, 4 }, { nSheet, 1, 5 }, { nSheet, 1, 6 },
                { nSheet, 1, 7 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula({ nSheet, 1, 2 }, u"=A3*4");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(12.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::RejectedOutOfContract)
            {
                return fail("computational_substrate",
                    "shared-group four-plus collapse rejection mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(119);
            const auto nData = aBeforeFacade.addSheet(u"Data");
            const auto nSummary = aBeforeFacade.addSheet(u"Summary");
            aBeforeFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.addNamedRange(u"Metrics", std::nullopt, { nData, 0, 0 }, u"$Data.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nData, 1, 0 }, u"=COUNTA(Metrics)+A1", CellValue::number(3.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 1 }, u"=COUNTA(Metrics)+A2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 2 }, u"=COUNTA(Metrics)+A3", CellValue::number(5.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nData, 1, 0 }, 3, true);
            aBeforeFacade.setFormulaCell(
                { nSummary, 2, 0 }, u"=Data.B1+Data.B2+Data.B3", CellValue::number(12.0),
                FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nSummary, 2, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(118);
            aAfterFacade.addSheet(u"Data");
            aAfterFacade.addSheet(u"Summary");
            aAfterFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.addNamedRange(u"Metrics", std::nullopt, { nData, 0, 0 }, u"$Data.$A$1:$A$2");
            aAfterFacade.setFormulaCell({ nData, 1, 0 }, u"=COUNTA(Metrics)+A1", CellValue::number(3.0),
                FormulaCellKind::Ordinary, true, true);
            aAfterFacade.setCell({ nData, 1, 1 }, CellValue::number(99.0));
            aAfterFacade.setFormulaCell({ nData, 1, 2 }, u"=COUNTA(Metrics)+A3", CellValue::number(5.0),
                FormulaCellKind::Ordinary, true, true);
            aAfterFacade.setFormulaCell(
                { nSummary, 2, 0 }, u"=Data.B1+Data.B2+Data.B3", CellValue::number(107.0),
                FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 2 }, { nSummary, 2, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            AuthorityPilotInput aAuthorityInput;
            aAuthorityInput.maComputationalShadow = aBeforeShadow;
            aAuthorityInput.maGraphShadow = aBeforeGraph;
            aAuthorityInput.maIrShadow = aBeforeIr;
            aAuthorityInput.maMutation = MutationEvent::setScalarValue({ nData, 1, 1 });
            aAuthorityInput.moScalarValueAfter = CellValue::number(99.0);
            aAuthorityInput.moObservedAfterComputationalShadow = aAfterShadow;
            aAuthorityInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aAuthorityInput.mbCleanBaseline = true;

            const auto aAuthorityPlan = buildAuthorityPilotTransition(aAuthorityInput);
            if (aAuthorityPlan.meVerdict != AuthorityPilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet member-exit authority verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aAuthorityPlan.maDependencySnapshot, aAuthorityPlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aAuthorityPlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet member-exit computational exactness mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aAuthorityPlan.maGraphAfter, aAuthorityPlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet member-exit graph exactness mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aAuthorityPlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aAuthorityPlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aAuthorityPlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet member-exit IR exactness mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(120);
            const auto nData = aBeforeFacade.addSheet(u"Data");
            const auto nSummary = aBeforeFacade.addSheet(u"Summary");
            aBeforeFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setFormulaCell({ nData, 1, 0 }, u"=A1*2", CellValue::number(2.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nData, 1, 0 }, 3, true);
            aBeforeFacade.setFormulaCell(
                { nSummary, 2, 0 }, u"=Data.B1+Data.B2+Data.B3", CellValue::number(12.0),
                FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nSummary, 2, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade = aBeforeFacade;
            aAfterFacade.setGeneration(121);

            ComputationalObservationState aAfterObservation = aBeforeObservation;
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            AuthorityPilotInput aAuthorityInput;
            aAuthorityInput.maComputationalShadow = aBeforeShadow;
            aAuthorityInput.maGraphShadow = aBeforeGraph;
            aAuthorityInput.maIrShadow = aBeforeIr;
            aAuthorityInput.maMutation = MutationEvent::setFormula({ nData, 1, 1 }, u"=A2*2");
            aAuthorityInput.moFormulaCachedValueAfter = CellValue::number(4.0);
            aAuthorityInput.moObservedAfterComputationalShadow = aAfterShadow;
            aAuthorityInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aAuthorityInput.mbCleanBaseline = true;

            const auto aAuthorityPlan = buildAuthorityPilotTransition(aAuthorityInput);
            if (aAuthorityPlan.meVerdict != AuthorityPilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet same-text-preserve authority verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aAuthorityPlan.maDependencySnapshot, aAuthorityPlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aAuthorityPlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet same-text-preserve computational exactness mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aAuthorityPlan.maGraphAfter, aAuthorityPlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet same-text-preserve graph exactness mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aAuthorityPlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aAuthorityPlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aAuthorityPlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet same-text-preserve IR exactness mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(122);
            const auto nData = aBeforeFacade.addSheet(u"Data");
            const auto nSummary = aBeforeFacade.addSheet(u"Summary");
            aBeforeFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setFormulaCell({ nData, 1, 0 }, u"=A1*3", CellValue::number(3.0),
                FormulaCellKind::Ordinary, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 1 }, u"=A2*2", CellValue::number(4.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nData, 1, 1 }, 2, true);
            aBeforeFacade.setFormulaCell(
                { nSummary, 2, 0 }, u"=Data.B1+Data.B2+Data.B3", CellValue::number(13.0),
                FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nSummary, 2, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(123);
            aAfterFacade.addSheet(u"Data");
            aAfterFacade.addSheet(u"Summary");
            aAfterFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.setFormulaCell({ nData, 1, 0 }, u"=A1*3", CellValue::number(3.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 1 }, u"=A2*3", CellValue::number(6.0),
                FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 2 }, u"=A3*2", CellValue::number(6.0),
                FormulaCellKind::Ordinary, true, true);
            aAfterFacade.addFormulaGroup({ nData, 1, 0 }, 2, true);
            aAfterFacade.setFormulaCell(
                { nSummary, 2, 0 }, u"=Data.B1+Data.B2+Data.B3", CellValue::number(15.0),
                FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nSummary, 2, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            AuthorityPilotInput aAuthorityInput;
            aAuthorityInput.maComputationalShadow = aBeforeShadow;
            aAuthorityInput.maGraphShadow = aBeforeGraph;
            aAuthorityInput.maIrShadow = aBeforeIr;
            aAuthorityInput.maMutation = MutationEvent::setFormula({ nData, 1, 1 }, u"=A2*3");
            aAuthorityInput.moFormulaCachedValueAfter = CellValue::number(6.0);
            aAuthorityInput.moObservedAfterComputationalShadow = aAfterShadow;
            aAuthorityInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aAuthorityInput.mbCleanBaseline = true;

            const auto aAuthorityPlan = buildAuthorityPilotTransition(aAuthorityInput);
            if (aAuthorityPlan.meVerdict != AuthorityPilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet regroup authority verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aAuthorityPlan.maDependencySnapshot, aAuthorityPlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aAuthorityPlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet regroup computational exactness mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aAuthorityPlan.maGraphAfter, aAuthorityPlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet regroup graph exactness mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aAuthorityPlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aAuthorityPlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aAuthorityPlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet regroup IR exactness mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(124);
            const auto nData = aBeforeFacade.addSheet(u"Data");
            const auto nSummary = aBeforeFacade.addSheet(u"Summary");
            aBeforeFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.addNamedRange(u"Metrics", std::nullopt, { nData, 0, 0 },
                u"$Data.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nData, 1, 0 }, u"=COUNTA(Metrics)+A1",
                CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nData, 1, 0 }, 3, true);
            aBeforeFacade.setFormulaCell({ nSummary, 0, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nSummary, 0, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade = aBeforeFacade;
            aAfterFacade.setGeneration(125);

            ComputationalObservationState aAfterObservation = aBeforeObservation;
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula(
                { nData, 1, 1 }, u"=COUNTA(Metrics)+A2");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(4.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range same-text lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbNamedRangeMatch)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range same-text lifecycle mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aLifecyclePlan.maGraphAfter, aLifecyclePlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range same-text graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aLifecyclePlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aLifecyclePlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aLifecyclePlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range same-text IR mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(126);
            const auto nData = aBeforeFacade.addSheet(u"Data");
            const auto nSummary = aBeforeFacade.addSheet(u"Summary");
            aBeforeFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.addNamedRange(u"Metrics", std::nullopt, { nData, 0, 0 },
                u"$Data.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nData, 1, 0 }, u"=COUNTA(Metrics)+A1*3",
                CellValue::number(5.0), FormulaCellKind::Ordinary, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 1 }, u"=COUNTA(Metrics)+A2*2",
                CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 2 }, u"=COUNTA(Metrics)+A3*2",
                CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nData, 1, 1 }, 2, true);
            aBeforeFacade.setFormulaCell({ nSummary, 0, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nSummary, 0, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(127);
            aAfterFacade.addSheet(u"Data");
            aAfterFacade.addSheet(u"Summary");
            aAfterFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.addNamedRange(u"Metrics", std::nullopt, { nData, 0, 0 },
                u"$Data.$A$1:$A$2");
            aAfterFacade.setFormulaCell({ nData, 1, 0 }, u"=COUNTA(Metrics)+A1*3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 1 }, u"=COUNTA(Metrics)+A2*3",
                CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 2 }, u"=COUNTA(Metrics)+A3*2",
                CellValue::number(8.0), FormulaCellKind::Ordinary, true, true);
            aAfterFacade.addFormulaGroup({ nData, 1, 0 }, 2, true);
            aAfterFacade.setFormulaCell({ nSummary, 0, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nSummary, 0, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula(
                { nData, 1, 1 }, u"=COUNTA(Metrics)+A2*3");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(8.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range regroup lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbNamedRangeMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nData, 1, 0 }, 2 }))
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range regroup lifecycle mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aLifecyclePlan.maGraphAfter, aLifecyclePlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range regroup graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aLifecyclePlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aLifecyclePlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aLifecyclePlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range regroup IR mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(128);
            const auto nData = aBeforeFacade.addSheet(u"Data");
            const auto nSummary = aBeforeFacade.addSheet(u"Summary");
            aBeforeFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.addNamedRange(u"Metrics", std::nullopt, { nData, 0, 0 },
                u"$Data.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nData, 1, 0 }, u"=COUNTA(Metrics)+A1*2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 1 }, u"=COUNTA(Metrics)+A2*2",
                CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nData, 1, 0 }, 2, true);
            aBeforeFacade.setFormulaCell({ nSummary, 0, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nSummary, 0, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(129);
            aAfterFacade.addSheet(u"Data");
            aAfterFacade.addSheet(u"Summary");
            aAfterFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.addNamedRange(u"Metrics", std::nullopt, { nData, 0, 0 },
                u"$Data.$A$1:$A$2");
            aAfterFacade.setFormulaCell({ nData, 1, 0 }, u"=COUNTA(Metrics)+A1*2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 1 }, u"=COUNTA(Metrics)+A2*2",
                CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 2 }, u"=COUNTA(Metrics)+A3*2",
                CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nData, 1, 0 }, 3, true);
            aAfterFacade.setFormulaCell({ nSummary, 0, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nSummary, 0, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula(
                { nData, 1, 2 }, u"=COUNTA(Metrics)+A3*2");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(8.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range one-sided insert lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbNamedRangeMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nData, 1, 0 }, 3 }))
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range one-sided insert lifecycle mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aLifecyclePlan.maGraphAfter, aLifecyclePlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range one-sided insert graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aLifecyclePlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aLifecyclePlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aLifecyclePlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range one-sided insert IR mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(130);
            const auto nData = aBeforeFacade.addSheet(u"Data");
            const auto nSummary = aBeforeFacade.addSheet(u"Summary");
            aBeforeFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.addNamedRange(u"Metrics", std::nullopt, { nData, 0, 0 },
                u"$Data.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nData, 1, 0 }, u"=COUNTA(Metrics)+A1",
                CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nData, 1, 0 }, 3, true);
            aBeforeFacade.setFormulaCell({ nSummary, 0, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nSummary, 0, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(131);
            aAfterFacade.addSheet(u"Data");
            aAfterFacade.addSheet(u"Summary");
            aAfterFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.addNamedRange(u"Metrics", std::nullopt, { nData, 0, 0 },
                u"$Data.$A$1:$A$2");
            aAfterFacade.setCell({ nData, 1, 0 }, CellValue::number(9.0));
            aAfterFacade.setFormulaCell({ nData, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nData, 1, 1 }, 2, true);
            aAfterFacade.setFormulaCell({ nSummary, 0, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nData, 1, 1 }, { nData, 1, 2 }, { nSummary, 0, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            AuthorityPilotInput aAuthorityInput;
            aAuthorityInput.maComputationalShadow = aBeforeShadow;
            aAuthorityInput.maGraphShadow = aBeforeGraph;
            aAuthorityInput.maIrShadow = aBeforeIr;
            aAuthorityInput.maMutation = MutationEvent::setScalarValue({ nData, 1, 0 });
            aAuthorityInput.moScalarValueAfter = CellValue::number(9.0);
            aAuthorityInput.moObservedAfterComputationalShadow = aAfterShadow;
            aAuthorityInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aAuthorityInput.mbCleanBaseline = true;

            const auto aAuthorityPlan = buildAuthorityPilotTransition(aAuthorityInput);
            if (aAuthorityPlan.meVerdict != AuthorityPilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range member-exit authority verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aAuthorityPlan.maDependencySnapshot, aAuthorityPlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aAuthorityPlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch
                || !aComputationalComparison.mbNamedRangeMatch)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range member-exit computational mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aAuthorityPlan.maGraphAfter, aAuthorityPlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range member-exit graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aAuthorityPlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aAuthorityPlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aAuthorityPlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range member-exit IR mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(132);
            const auto nData = aBeforeFacade.addSheet(u"Data");
            const auto nSummary = aBeforeFacade.addSheet(u"Summary");
            aBeforeFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.addNamedRange(u"Metrics", std::nullopt, { nData, 0, 0 },
                u"$Data.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nData, 1, 0 }, u"=COUNTA(Metrics)+A1",
                CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nData, 1, 0 }, 3, true);
            aBeforeFacade.setFormulaCell({ nSummary, 0, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nSummary, 0, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(133);
            aAfterFacade.addSheet(u"Data");
            aAfterFacade.addSheet(u"Summary");
            aAfterFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.addNamedRange(u"Metrics", std::nullopt, { nData, 0, 0 },
                u"$Data.$A$1:$A$2");
            aAfterFacade.setFormulaCell({ nData, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nData, 1, 1 }, 2, true);
            aAfterFacade.setFormulaCell({ nSummary, 0, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nData, 1, 1 }, { nData, 1, 2 }, { nSummary, 0, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            AuthorityPilotInput aAuthorityInput;
            aAuthorityInput.maComputationalShadow = aBeforeShadow;
            aAuthorityInput.maGraphShadow = aBeforeGraph;
            aAuthorityInput.maIrShadow = aBeforeIr;
            aAuthorityInput.maMutation = MutationEvent::clearCell({ nData, 1, 0 });
            aAuthorityInput.moObservedAfterComputationalShadow = aAfterShadow;
            aAuthorityInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aAuthorityInput.mbCleanBaseline = true;

            const auto aAuthorityPlan = buildAuthorityPilotTransition(aAuthorityInput);
            if (aAuthorityPlan.meVerdict != AuthorityPilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range clear member-exit authority verdict mismatch");
            }

            const auto aObservationBuildOptions
                = authoritybuilddetail::buildAuthorityObservationBuildOptions(
                    aBeforeShadow, aAfterShadow, MutationEvent::clearCell({ nData, 1, 0 }));
            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aAuthorityPlan.maDependencySnapshot, aAuthorityPlan.maRecalcPlan,
                aObservationBuildOptions);
            const auto aComputationalComparison = compareComputationalShadow(
                aAuthorityPlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch
                || !aComputationalComparison.mbNamedRangeMatch)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range clear member-exit computational mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aAuthorityPlan.maGraphAfter, aAuthorityPlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range clear member-exit graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aAuthorityPlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aAuthorityPlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aAuthorityPlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range clear member-exit IR mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(134);
            const auto nData = aBeforeFacade.addSheet(u"Data");
            const auto nSummary = aBeforeFacade.addSheet(u"Summary");
            aBeforeFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.addNamedRange(u"Metrics", std::nullopt, { nData, 0, 0 },
                u"$Data.$A$1:$A$2");
            aBeforeFacade.setFormulaCell({ nData, 1, 0 }, u"=COUNTA(Metrics)+A1",
                CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nData, 1, 0 }, 3, true);
            aBeforeFacade.setFormulaCell({ nSummary, 0, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nSummary, 0, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(135);
            aAfterFacade.addSheet(u"Data");
            aAfterFacade.addSheet(u"Summary");
            aAfterFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.addNamedRange(u"Metrics", std::nullopt, { nData, 0, 0 },
                u"$Data.$A$1:$A$2");
            aAfterFacade.setFormulaCell({ nData, 1, 0 }, u"=COUNTA(Metrics)+A1*10",
                CellValue::number(12.0), FormulaCellKind::Ordinary, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 1 }, u"=COUNTA(Metrics)+A2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 2 }, u"=COUNTA(Metrics)+A3",
                CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nData, 1, 1 }, 2, true);
            aAfterFacade.setFormulaCell({ nSummary, 0, 0 }, u"=COUNTA(Metrics)",
                CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nSummary, 0, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation
                = MutationEvent::setFormula({ nData, 1, 0 }, u"=COUNTA(Metrics)+A1*10");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(12.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range setformula member-exit lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch
                || !aComputationalComparison.mbNamedRangeMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nData, 1, 1 }, 2 }))
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range setformula member-exit mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aLifecyclePlan.maGraphAfter, aLifecyclePlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range setformula member-exit graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aLifecyclePlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aLifecyclePlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aLifecyclePlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet named-range setformula member-exit IR mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(136);
            const auto nData = aBeforeFacade.addSheet(u"Data");
            const auto nSummary = aBeforeFacade.addSheet(u"Summary");
            aBeforeFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setCell({ nData, 0, 3 }, CellValue::number(4.0));
            aBeforeFacade.setCell({ nData, 0, 4 }, CellValue::number(5.0));
            aBeforeFacade.setFormulaCell({ nData, 1, 0 }, u"=A1*2",
                CellValue::number(2.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 1 }, u"=A2*2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 3 }, u"=A4*2",
                CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 4 }, u"=A5*2",
                CellValue::number(10.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nData, 1, 0 }, 2, true);
            aBeforeFacade.addFormulaGroup({ nData, 1, 3 }, 2, true);
            aBeforeFacade.setFormulaCell({ nSummary, 2, 0 },
                u"=Data.B1+Data.B2+Data.B3+Data.B4+Data.B5", CellValue::number(24.0),
                FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 },
                { nData, 1, 3 }, { nData, 1, 4 }, { nSummary, 2, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(137);
            aAfterFacade.addSheet(u"Data");
            aAfterFacade.addSheet(u"Summary");
            aAfterFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.setCell({ nData, 0, 3 }, CellValue::number(4.0));
            aAfterFacade.setCell({ nData, 0, 4 }, CellValue::number(5.0));
            aAfterFacade.setFormulaCell({ nData, 1, 0 }, u"=A1*2",
                CellValue::number(2.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 1 }, u"=A2*2",
                CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 2 }, u"=A3*2",
                CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 3 }, u"=A4*2",
                CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 4 }, u"=A5*2",
                CellValue::number(10.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.addFormulaGroup({ nData, 1, 0 }, 5, true);
            aAfterFacade.setFormulaCell({ nSummary, 2, 0 },
                u"=Data.B1+Data.B2+Data.B3+Data.B4+Data.B5", CellValue::number(30.0),
                FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nData, 1, 3 }, { nData, 1, 4 }, { nSummary, 2, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula({ nData, 1, 2 }, u"=A3*2");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(6.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet merge lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nData, 1, 0 }, 5 }))
            {
                return fail("computational_substrate",
                    "shared-group off-sheet merge lifecycle mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aLifecyclePlan.maGraphAfter, aLifecyclePlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet merge graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aLifecyclePlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aLifecyclePlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aLifecyclePlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet merge IR mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(138);
            const auto nData = aBeforeFacade.addSheet(u"Data");
            const auto nSummary = aBeforeFacade.addSheet(u"Summary");
            aBeforeFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aBeforeFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aBeforeFacade.setCell({ nData, 0, 3 }, CellValue::number(4.0));
            aBeforeFacade.setFormulaCell({ nData, 1, 0 }, u"=A1*3",
                CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 1 }, u"=A2*3",
                CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 2 }, u"=A3*2",
                CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.setFormulaCell({ nData, 1, 3 }, u"=A4*2",
                CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
            aBeforeFacade.addFormulaGroup({ nData, 1, 0 }, 2, true);
            aBeforeFacade.addFormulaGroup({ nData, 1, 2 }, 2, true);
            aBeforeFacade.setFormulaCell({ nSummary, 2, 0 }, u"=Data.B1+Data.B2+Data.B3+Data.B4",
                CellValue::number(23.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 },
                { nData, 1, 2 }, { nData, 1, 3 }, { nSummary, 2, 0 } };
            const auto aBeforeShadow
                = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph
                = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(139);
            aAfterFacade.addSheet(u"Data");
            aAfterFacade.addSheet(u"Summary");
            aAfterFacade.setCell({ nData, 0, 0 }, CellValue::number(1.0));
            aAfterFacade.setCell({ nData, 0, 1 }, CellValue::number(2.0));
            aAfterFacade.setCell({ nData, 0, 2 }, CellValue::number(3.0));
            aAfterFacade.setCell({ nData, 0, 3 }, CellValue::number(4.0));
            aAfterFacade.setFormulaCell({ nData, 1, 0 }, u"=A1*3",
                CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 1 }, u"=A2*3",
                CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 2 }, u"=A3*3",
                CellValue::number(9.0), FormulaCellKind::SharedGroupMember, true, true);
            aAfterFacade.setFormulaCell({ nData, 1, 3 }, u"=A4*2",
                CellValue::number(8.0), FormulaCellKind::Ordinary, true, true);
            aAfterFacade.addFormulaGroup({ nData, 1, 0 }, 3, true);
            aAfterFacade.setFormulaCell({ nSummary, 2, 0 }, u"=Data.B1+Data.B2+Data.B3+Data.B4",
                CellValue::number(26.0), FormulaCellKind::Ordinary, true, true);

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 }, { nData, 1, 2 },
                { nData, 1, 3 }, { nSummary, 2, 0 } };
            const auto aAfterShadow
                = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);

            LifecyclePilotInput aLifecycleInput;
            aLifecycleInput.maComputationalShadow = aBeforeShadow;
            aLifecycleInput.maGraphShadow = aBeforeGraph;
            aLifecycleInput.maIrShadow = aBeforeIr;
            aLifecycleInput.maMutation = MutationEvent::setFormula({ nData, 1, 2 }, u"=A3*3");
            aLifecycleInput.moFormulaCachedValueAfter = CellValue::number(9.0);
            aLifecycleInput.moObservedAfterComputationalShadow = aAfterShadow;
            aLifecycleInput.mbAllowSharedGroupNonStructuralAdmission = true;
            aLifecycleInput.mbCleanBaseline = true;

            const auto aLifecyclePlan = buildLifecyclePilotTransition(aLifecycleInput);
            if (aLifecyclePlan.meVerdict != LifecyclePilotVerdict::Applicable)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet replacement-merge lifecycle verdict mismatch");
            }

            const auto aPredictedObservation = authoritybuilddetail::buildAuthorityObservationState(
                aLifecyclePlan.maDependencySnapshot, aLifecyclePlan.maRecalcPlan);
            const auto aComputationalComparison = compareComputationalShadow(
                aLifecyclePlan.maComputationalAfter, aAfterFacade, aPredictedObservation);
            if (!aComputationalComparison.mbFullMatch || !aComputationalComparison.mbGroupMatch
                || aLifecyclePlan.maComputationalAfter.maFormulaGroups.size() != 1
                || !(aLifecyclePlan.maComputationalAfter.maFormulaGroups.front().maId
                     == ShadowFormulaGroupId { { nData, 1, 0 }, 3 }))
            {
                return fail("computational_substrate",
                    "shared-group off-sheet replacement-merge lifecycle mismatch");
            }

            const auto aGraphComparison = compareDependencyGraphShadow(
                aLifecyclePlan.maGraphAfter, aLifecyclePlan.maComputationalAfter,
                aPredictedObservation);
            if (aGraphComparison.meKind != graphmapping::GraphComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet replacement-merge graph mismatch");
            }

            auto aPredictedFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
                aLifecyclePlan.maComputationalAfter);
            const auto aExpectedIr = authoritybuilddetail::buildAuthorityExecutionIrShadow(
                aLifecyclePlan.maComputationalAfter, aPredictedFacade);
            const auto aIrComparison
                = compareExecutionIrWorkbookShadow(aLifecyclePlan.maIrAfter, aExpectedIr);
            if (aIrComparison.meKind != ExecutionIrComparisonKind::Exact)
            {
                return fail("computational_substrate",
                    "shared-group off-sheet replacement-merge IR mismatch");
            }
        }

    // --- Safe mutation rebuild path ---
    {
        aFacade.setCell({ nData, 0, 0 }, CellValue::number(11.0));
        auto aMutationState = rebuildComputationalShadowAfterMutation(
            aFacade, aObservation, MutationEvent::setScalarValue({ nData, 0, 0 }));
        if (!compareComputationalShadow(aMutationState.maShadow, aFacade, aObservation).mbFullMatch)
            return fail("computational_substrate", "scalar mutation comparison mismatch");
        const auto* pUpdatedScalar = aMutationState.maShadow.findCell({ nData, 0, 0 });
        if (!pUpdatedScalar || !pUpdatedScalar->maCell.maValue.isNumber()
            || pUpdatedScalar->maCell.maValue.mfNumber != 11.0)
        {
            return fail("computational_substrate", "scalar mutation rebuild mismatch");
        }

        aFacade.setFormulaCell({ nData, 1, 0 }, u"=A1*4", CellValue::number(44.0));
        aMutationState = rebuildComputationalShadowAfterMutation(
            aFacade, aObservation, MutationEvent::setFormula({ nData, 1, 0 }, u"=A1*4"));
        if (!compareComputationalShadow(aMutationState.maShadow, aFacade, aObservation).mbFullMatch)
            return fail("computational_substrate", "formula edit comparison mismatch");
        const auto* pEditedFormula = aMutationState.maShadow.findCell({ nData, 1, 0 });
        if (!pEditedFormula || !pEditedFormula->moFormula
            || pEditedFormula->moFormula->maFormulaSource != u"=A1*4")
        {
            return fail("computational_substrate", "formula edit rebuild mismatch");
        }

        aFacade.setFormulaCell({ nData, 2, 0 }, u"=A1+B1", CellValue::number(55.0));
        aObservation.maFormulaTree.push_back({ nData, 2, 0 });
        aMutationState = rebuildComputationalShadowAfterMutation(
            aFacade, aObservation, MutationEvent::setFormula({ nData, 2, 0 }, u"=A1+B1"));
        if (!compareComputationalShadow(aMutationState.maShadow, aFacade, aObservation).mbFullMatch)
            return fail("computational_substrate", "formula insert comparison mismatch");
        if (aMutationState.maShadow.getFormulaCellCount() != 3
            || !aMutationState.maShadow.findCell({ nData, 2, 0 }))
        {
            return fail("computational_substrate", "formula insertion rebuild mismatch");
        }

        aFacade.clearCell({ nData, 0, 1 });
        aMutationState = rebuildComputationalShadowAfterMutation(
            aFacade, aObservation, MutationEvent::clearCell({ nData, 0, 1 }));
        if (!compareComputationalShadow(aMutationState.maShadow, aFacade, aObservation).mbFullMatch)
            return fail("computational_substrate", "clear cell comparison mismatch");
        if (aMutationState.maShadow.findCell({ nData, 0, 1 }))
            return fail("computational_substrate", "clear cell rebuild mismatch");

        const auto aBeforeRange = *aFacade.findNamedRange(u"Metric", std::nullopt);
        if (!aFacade.renameNamedRange(u"Metric", u"MetricRenamed"))
            return fail("computational_substrate", "named range rename setup failed");
        const auto aAfterRange = *aFacade.findNamedRange(u"MetricRenamed", std::nullopt);
        aMutationState = rebuildComputationalShadowAfterMutation(
            aFacade, aObservation, MutationEvent::renameNamedRange(aBeforeRange, aAfterRange));
        if (!compareComputationalShadow(aMutationState.maShadow, aFacade, aObservation).mbFullMatch)
            return fail("computational_substrate", "named range comparison mismatch");
        if (aMutationState.maShadow.maNamedRanges.size() != 1
            || aMutationState.maShadow.maNamedRanges.front().maName != u"MetricRenamed")
        {
            return fail("computational_substrate", "named range rebuild mismatch");
        }
    }

    std::cout << "computational_substrate_tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
