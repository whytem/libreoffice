/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <limits>

#include <spreadsheetengine/api/Logic.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::logic::IfBranchAction;
    using spreadsheetengine::api::logic::IfErrorAction;
    using spreadsheetengine::api::logic::IfsAction;
    using spreadsheetengine::standalone::test::fail;

    if (spreadsheetengine::api::logic::selectIfBranch(true, false, true, true)
            != IfBranchAction::ThenPath
        || spreadsheetengine::api::logic::selectIfBranch(true, false, false, true)
               != IfBranchAction::ReturnTrue
        || spreadsheetengine::api::logic::selectIfBranch(false, false, true, true)
               != IfBranchAction::ElsePath
        || spreadsheetengine::api::logic::selectIfBranch(false, false, true, false)
               != IfBranchAction::ReturnFalse
        || spreadsheetengine::api::logic::selectIfBranch(true, true, true, true)
               != IfBranchAction::PropagateError)
    {
        return fail("spreadsheetengine_logic_tests", "if branch selection mismatch");
    }

    if (!spreadsheetengine::api::logic::matchesIfErrorPolicy(Error::DivisionByZero, false)
        || spreadsheetengine::api::logic::matchesIfErrorPolicy(Error::DivisionByZero, true)
        || !spreadsheetengine::api::logic::matchesIfErrorPolicy(Error::NotAvailable, true)
        || spreadsheetengine::api::logic::matchesIfErrorPolicy(Error::None, false))
    {
        return fail("spreadsheetengine_logic_tests", "iferror policy mismatch");
    }

    if (spreadsheetengine::api::logic::selectIfErrorAction(Error::NoValue, false)
            != IfErrorAction::EvaluateAlternate
        || spreadsheetengine::api::logic::selectIfErrorAction(Error::NoValue, true)
               != IfErrorAction::KeepPrimary
        || spreadsheetengine::api::logic::selectIfErrorAction(Error::NotAvailable, true)
               != IfErrorAction::EvaluateAlternate)
    {
        return fail("spreadsheetengine_logic_tests", "iferror action mismatch");
    }

    const auto oChoose2 = spreadsheetengine::api::logic::normalizeChooseIndex(2.9, 4);
    const auto oChooseBad = spreadsheetengine::api::logic::normalizeChooseIndex(4.0, 4);
    const auto oChooseNaN
        = spreadsheetengine::api::logic::normalizeChooseIndex(std::numeric_limits<double>::quiet_NaN(), 4);
    if (!oChoose2 || *oChoose2 != 2 || oChooseBad || oChooseNaN)
        return fail("spreadsheetengine_logic_tests", "choose normalization mismatch");

    const auto aChooseOk = spreadsheetengine::api::logic::chooseJumpIndex(2, 4);
    const auto aChooseBad = spreadsheetengine::api::logic::chooseJumpIndex(5, 4);
    if (!aChooseOk || aChooseOk.maValue != 2 || aChooseBad
        || aChooseBad.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_logic_tests", "choose jump index mismatch");
    }

    if (spreadsheetengine::api::logic::evaluateIfsCondition(true, false, 1)
            != IfsAction::SelectCurrentResult
        || spreadsheetengine::api::logic::evaluateIfsCondition(false, false, 3)
               != IfsAction::SkipCurrentResult
        || spreadsheetengine::api::logic::evaluateIfsCondition(false, false, 1)
               != IfsAction::ReturnNotAvailable
        || spreadsheetengine::api::logic::evaluateIfsCondition(true, false, 0)
               != IfsAction::ReturnParameterExpected
        || spreadsheetengine::api::logic::evaluateIfsCondition(false, true, 3)
               != IfsAction::ReturnNoValue)
    {
        return fail("spreadsheetengine_logic_tests", "ifs condition mismatch");
    }

    std::cout << "spreadsheetengine logic api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
