/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <vector>

#include <spreadsheetengine/api/SharedFormula.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::sharedformula::GroupRunAction;
    using spreadsheetengine::api::sharedformula::GroupRunPlan;
    using spreadsheetengine::api::sharedformula::JoinAction;
    using spreadsheetengine::api::sharedformula::JoinPlan;
    using spreadsheetengine::api::sharedformula::TokenCompareState;
    using spreadsheetengine::standalone::test::fail;

    if (!spreadsheetengine::api::sharedformula::shouldJoinFormulaCells(
            TokenCompareState::EqualInvariant)
        || !spreadsheetengine::api::sharedformula::shouldJoinFormulaCells(
            TokenCompareState::EqualRelativeRef)
        || !spreadsheetengine::api::sharedformula::shouldReturnSharedTopFormulaCell(true, true)
        || spreadsheetengine::api::sharedformula::shouldReturnSharedTopFormulaCell(false, true)
        || spreadsheetengine::api::sharedformula::shouldReturnSharedTopFormulaCell(true, false)
        || !spreadsheetengine::api::sharedformula::canJoinFormulaCellAbove(true, 1)
        || spreadsheetengine::api::sharedformula::canJoinFormulaCellAbove(true, 0)
        || spreadsheetengine::api::sharedformula::canJoinFormulaCellAbove(false, 1)
        || spreadsheetengine::api::sharedformula::shouldJoinFormulaCells(
            TokenCompareState::NotEqual))
    {
        return fail("spreadsheetengine_sharedformula_tests", "join candidate policy mismatch");
    }

    if (spreadsheetengine::api::sharedformula::makeGroupRunPlan(
            TokenCompareState::NotEqual, false)
            != GroupRunPlan { GroupRunAction::None, false }
        || spreadsheetengine::api::sharedformula::makeGroupRunPlan(
               TokenCompareState::EqualRelativeRef, true)
               != GroupRunPlan { GroupRunAction::ExtendExistingGroup, false }
        || spreadsheetengine::api::sharedformula::makeGroupRunPlan(
               TokenCompareState::EqualInvariant, false)
               != GroupRunPlan { GroupRunAction::CreateGroup, true })
    {
        return fail("spreadsheetengine_sharedformula_tests", "group run plan mismatch");
    }

    if (!spreadsheetengine::api::sharedformula::isValidListenAddress({ 0, 1, 2 })
        || spreadsheetengine::api::sharedformula::isValidListenAddress({ -1, 1, 2 })
        || spreadsheetengine::api::sharedformula::makeGroupSingleRefListenPlan({ 0, 1, 2 })
               != spreadsheetengine::api::sharedformula::GroupSingleRefListenPlan {
                   { 0, 1, 2 }, true }
        || spreadsheetengine::api::sharedformula::makeGroupSingleRefListenPlan({ 0, -1, 2 })
               != spreadsheetengine::api::sharedformula::GroupSingleRefListenPlan {
                   { 0, -1, 2 }, false })
    {
        return fail("spreadsheetengine_sharedformula_tests",
                    "single-ref listening plan mismatch");
    }

    if (spreadsheetengine::api::sharedformula::makeGroupDoubleRefListenPlan(
            { { 0, 1, 2 }, { 0, 3, 4 } }, true, true, 5)
            != spreadsheetengine::api::sharedformula::GroupDoubleRefListenPlan {
                { { 0, 1, 2 }, { 0, 3, 4 } },
                { { 0, 1, 2 }, { 0, 3, 8 } },
                false, false }
        || spreadsheetengine::api::sharedformula::makeGroupDoubleRefListenPlan(
               { { 1, 5, 6 }, { 1, 7, 8 } }, false, false, 3)
               != spreadsheetengine::api::sharedformula::GroupDoubleRefListenPlan {
                   { { 1, 5, 6 }, { 1, 7, 8 } },
                   { { 1, 5, 6 }, { 1, 7, 8 } },
                   true, true })
    {
        return fail("spreadsheetengine_sharedformula_tests",
                    "double-ref listening plan mismatch");
    }

    if (spreadsheetengine::api::sharedformula::makeJoinPlan(
            TokenCompareState::NotEqual, false, false, false)
            != JoinPlan { JoinAction::None, false }
        || spreadsheetengine::api::sharedformula::makeJoinPlan(
               TokenCompareState::EqualRelativeRef, true, true, true)
               != JoinPlan { JoinAction::None, false }
        || spreadsheetengine::api::sharedformula::makeJoinPlan(
               TokenCompareState::EqualRelativeRef, true, true, false)
               != JoinPlan { JoinAction::MergeGroups, false }
        || spreadsheetengine::api::sharedformula::makeJoinPlan(
               TokenCompareState::EqualRelativeRef, true, false, false)
               != JoinPlan { JoinAction::ExtendUpperGroup, false }
        || spreadsheetengine::api::sharedformula::makeJoinPlan(
               TokenCompareState::EqualRelativeRef, false, true, false)
               != JoinPlan { JoinAction::AdoptLowerGroup, false }
        || spreadsheetengine::api::sharedformula::makeJoinPlan(
               TokenCompareState::EqualInvariant, false, false, false)
               != JoinPlan { JoinAction::CreateGroup, true }
        || spreadsheetengine::api::sharedformula::makeJoinPlan(
               TokenCompareState::EqualRelativeRef, false, false, false)
               != JoinPlan { JoinAction::CreateGroup, false })
    {
        return fail("spreadsheetengine_sharedformula_tests", "join plan mismatch");
    }

    if (!spreadsheetengine::api::sharedformula::canSplitSharedFormulaGroup(
            true, 1, true, 12, 10)
        || spreadsheetengine::api::sharedformula::canSplitSharedFormulaGroup(
            false, 1, true, 12, 10)
        || spreadsheetengine::api::sharedformula::canSplitSharedFormulaGroup(
            true, 0, true, 12, 10)
        || spreadsheetengine::api::sharedformula::canSplitSharedFormulaGroup(
            true, 1, false, 12, 10)
        || spreadsheetengine::api::sharedformula::canSplitSharedFormulaGroup(
            true, 1, true, 10, 10)
        || spreadsheetengine::api::sharedformula::makeSplitPlan(10, 5, 12)
               != spreadsheetengine::api::sharedformula::SplitPlan { true, true, false, 2, 3 }
        || spreadsheetengine::api::sharedformula::makeSplitPlan(10, 3, 11)
               != spreadsheetengine::api::sharedformula::SplitPlan { true, true, true, 1, 2 }
        || spreadsheetengine::api::sharedformula::makeSplitPlan(10, 2, 11)
               != spreadsheetengine::api::sharedformula::SplitPlan { true, false, true, 1, 1 })
    {
        return fail("spreadsheetengine_sharedformula_tests", "split plan mismatch");
    }

    if (spreadsheetengine::api::sharedformula::classifyUnsharePosition(0, 0, 0)
            != spreadsheetengine::api::sharedformula::UnsharePosition::None
        || spreadsheetengine::api::sharedformula::classifyUnsharePosition(10, 10, 3)
               != spreadsheetengine::api::sharedformula::UnsharePosition::Top
        || spreadsheetengine::api::sharedformula::classifyUnsharePosition(12, 10, 3)
               != spreadsheetengine::api::sharedformula::UnsharePosition::Bottom
        || spreadsheetengine::api::sharedformula::classifyUnsharePosition(11, 10, 3)
               != spreadsheetengine::api::sharedformula::UnsharePosition::Middle
        || spreadsheetengine::api::sharedformula::makeUnsharePlan(10, 10, 2)
               != spreadsheetengine::api::sharedformula::UnsharePlan {
                   spreadsheetengine::api::sharedformula::UnsharePosition::Top,
                   false, true, false, 1, 0 }
        || spreadsheetengine::api::sharedformula::makeUnsharePlan(12, 10, 3)
               != spreadsheetengine::api::sharedformula::UnsharePlan {
                   spreadsheetengine::api::sharedformula::UnsharePosition::Bottom,
                   false, false, false, 2, 0 }
        || spreadsheetengine::api::sharedformula::makeUnsharePlan(11, 10, 4)
               != spreadsheetengine::api::sharedformula::UnsharePlan {
                   spreadsheetengine::api::sharedformula::UnsharePosition::Middle,
                   true, false, true, 1, 2 }
        || spreadsheetengine::api::sharedformula::makeUnsharePlan(11, 10, 3)
               != spreadsheetengine::api::sharedformula::UnsharePlan {
                   spreadsheetengine::api::sharedformula::UnsharePosition::Middle,
                   true, true, false, 1, 1 })
    {
        return fail("spreadsheetengine_sharedformula_tests", "unshare plan mismatch");
    }

    std::vector<sal_Int32> aRows { 7, 3, 3, 5, 7, 1 };
    spreadsheetengine::api::sharedformula::sortAndUniqueRows(aRows);
    if (aRows != std::vector<sal_Int32>({ 1, 3, 5, 7 }))
    {
        return fail("spreadsheetengine_sharedformula_tests", "sort/unique rows mismatch");
    }

    const std::vector<sal_Int32> aBounds
        = spreadsheetengine::api::sharedformula::makeUnshareBoundaryRows(
            std::vector<sal_Int32> { 8, 2, 2, 10, 11 }, 10);
    if (aBounds != std::vector<sal_Int32>({ 2, 3, 8, 9, 10 }))
    {
        return fail("spreadsheetengine_sharedformula_tests",
                    "unshare boundary row planning mismatch");
    }

    struct JoinScenario
    {
        TokenCompareState meState;
        bool mbUpperShared = false;
        bool mbLowerShared = false;
        bool mbStickyBoundary = false;
        JoinPlan maExpected;
    };
    const std::vector<JoinScenario> aJoinScenarios
        = { { TokenCompareState::EqualInvariant, false, false, false,
                { JoinAction::CreateGroup, true } },
            { TokenCompareState::EqualRelativeRef, true, false, false,
                { JoinAction::ExtendUpperGroup, false } },
            { TokenCompareState::EqualRelativeRef, false, true, false,
                { JoinAction::AdoptLowerGroup, false } },
            { TokenCompareState::EqualRelativeRef, true, true, false,
                { JoinAction::MergeGroups, false } },
            { TokenCompareState::EqualRelativeRef, true, true, true,
                { JoinAction::None, false } } };
    for (const auto& rScenario : aJoinScenarios)
    {
        if (spreadsheetengine::api::sharedformula::makeJoinPlan(
                rScenario.meState, rScenario.mbUpperShared, rScenario.mbLowerShared,
                rScenario.mbStickyBoundary)
            != rScenario.maExpected)
        {
            return fail("spreadsheetengine_sharedformula_tests",
                        "table-driven join scenario mismatch");
        }
    }

    struct UnshareScenario
    {
        sal_Int32 mnRow = 0;
        sal_Int32 mnTopRow = 0;
        sal_Int32 mnLength = 0;
        spreadsheetengine::api::sharedformula::UnsharePlan maExpected;
    };
    const std::vector<UnshareScenario> aUnshareScenarios
        = { { 10, 10, 2,
                { spreadsheetengine::api::sharedformula::UnsharePosition::Top,
                    false, true, false, 1, 0 } },
            { 12, 10, 3,
                { spreadsheetengine::api::sharedformula::UnsharePosition::Bottom,
                    false, false, false, 2, 0 } },
            { 11, 10, 4,
                { spreadsheetengine::api::sharedformula::UnsharePosition::Middle,
                    true, false, true, 1, 2 } } };
    for (const auto& rScenario : aUnshareScenarios)
    {
        if (spreadsheetengine::api::sharedformula::makeUnsharePlan(
                rScenario.mnRow, rScenario.mnTopRow, rScenario.mnLength)
            != rScenario.maExpected)
        {
            return fail("spreadsheetengine_sharedformula_tests",
                        "table-driven unshare scenario mismatch");
        }
    }

    std::cout << "spreadsheetengine sharedformula api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
