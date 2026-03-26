/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <limits>

#include <spreadsheetengine/api/Logic.hxx>

#include "SharedCaseSupport.hxx"
#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::logic::IfBranchAction;
    using spreadsheetengine::api::logic::IfErrorAction;
    using spreadsheetengine::api::logic::IfsAction;
    using spreadsheetengine::standalone::test::decodeUtf8TestString;
    using spreadsheetengine::standalone::test::fail;
    using spreadsheetengine::standalone::test::loadSharedCaseRows;
    using spreadsheetengine::standalone::test::parseBool;
    using spreadsheetengine::standalone::test::parseDouble;
    using spreadsheetengine::standalone::test::parseExpectedError;

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

    for (const auto& rRow : loadSharedCaseRows("logic_cases.tsv"))
    {
        if (rRow.maColumns.size() < 8)
            return failSharedCase(
                "spreadsheetengine_logic_tests", rRow, "logic shared case column mismatch");

        const auto& rFunction = rRow.maColumns[0];
        const auto aExpectedValue = decodeUtf8TestString(rRow.maColumns[6]);
        const auto eExpectedError = parseExpectedError(rRow.maColumns[7]);

        if (rFunction == "IF")
        {
            const auto eAction = spreadsheetengine::api::logic::selectIfBranch(
                parseBool(rRow.maColumns[1]), false, true, true);
            const auto aActualValue = eAction == IfBranchAction::ThenPath
                                          ? decodeUtf8TestString(rRow.maColumns[2])
                                          : decodeUtf8TestString(rRow.maColumns[3]);
            if (eExpectedError != Error::None || aActualValue != aExpectedValue)
                return failSharedCase("spreadsheetengine_logic_tests", rRow, "IF mismatch");
        }
        else if (rFunction == "IF2")
        {
            const auto eAction = spreadsheetengine::api::logic::selectIfBranch(
                parseBool(rRow.maColumns[1]), false, true, false);
            const auto aActualValue = eAction == IfBranchAction::ThenPath
                                          ? decodeUtf8TestString(rRow.maColumns[2])
                                          : spreadsheetengine::api::String(u"FALSE");
            if (eExpectedError != Error::None || aActualValue != aExpectedValue)
                return failSharedCase("spreadsheetengine_logic_tests", rRow, "IF2 mismatch");
        }
        else if (rFunction == "IFERROR" || rFunction == "IFNA")
        {
            const auto ePrimaryError = parseExpectedError(rRow.maColumns[1]);
            const auto eAction = spreadsheetengine::api::logic::selectIfErrorAction(
                ePrimaryError, rFunction == "IFNA");

            if (eAction == IfErrorAction::EvaluateAlternate)
            {
                if (eExpectedError != Error::None
                    || decodeUtf8TestString(rRow.maColumns[3]) != aExpectedValue)
                {
                    return failSharedCase(
                        "spreadsheetengine_logic_tests", rRow, "IFERROR/IFNA alternate mismatch");
                }
            }
            else if (ePrimaryError != Error::None)
            {
                if (ePrimaryError != eExpectedError)
                {
                    return failSharedCase(
                        "spreadsheetengine_logic_tests", rRow, "IFERROR/IFNA error mismatch");
                }
            }
            else if (decodeUtf8TestString(rRow.maColumns[2]) != aExpectedValue
                     || eExpectedError != Error::None)
            {
                return failSharedCase(
                    "spreadsheetengine_logic_tests", rRow, "IFERROR/IFNA primary mismatch");
            }
        }
        else if (rFunction == "CHOOSE")
        {
            const auto oIndex = spreadsheetengine::api::logic::normalizeChooseIndex(
                parseDouble(rRow.maColumns[1]), 4);
            if (!oIndex)
                return failSharedCase("spreadsheetengine_logic_tests", rRow, "CHOOSE index mismatch");

            const auto aIndex = spreadsheetengine::api::logic::chooseJumpIndex(*oIndex, 4);
            if (!aIndex)
                return failSharedCase("spreadsheetengine_logic_tests", rRow, "CHOOSE jump mismatch");

            const auto aActualValue = aIndex.maValue == 1
                                          ? decodeUtf8TestString(rRow.maColumns[2])
                                          : (aIndex.maValue == 2
                                                 ? decodeUtf8TestString(rRow.maColumns[3])
                                                 : decodeUtf8TestString(rRow.maColumns[4]));
            if (eExpectedError != Error::None || aActualValue != aExpectedValue)
                return failSharedCase("spreadsheetengine_logic_tests", rRow, "CHOOSE mismatch");
        }
        else if (rFunction == "IFS")
        {
            const double fSelector = parseDouble(rRow.maColumns[1]);
            const double fCond1 = parseDouble(rRow.maColumns[2]);
            const auto eAction1 = spreadsheetengine::api::logic::evaluateIfsCondition(
                fSelector == fCond1, false, 3);

            if (eAction1 == IfsAction::SelectCurrentResult)
            {
                if (eExpectedError != Error::None
                    || decodeUtf8TestString(rRow.maColumns[3]) != aExpectedValue)
                {
                    return failSharedCase("spreadsheetengine_logic_tests", rRow, "IFS first-branch mismatch");
                }
            }
            else if (eAction1 != IfsAction::SkipCurrentResult)
            {
                return failSharedCase(
                    "spreadsheetengine_logic_tests", rRow, "IFS first-condition planning mismatch");
            }
            else
            {
                const double fCond2 = parseDouble(rRow.maColumns[4]);
                const auto eAction2 = spreadsheetengine::api::logic::evaluateIfsCondition(
                    fSelector == fCond2, false, 1);

                if (eAction2 == IfsAction::SelectCurrentResult)
                {
                    if (eExpectedError != Error::None
                        || decodeUtf8TestString(rRow.maColumns[5]) != aExpectedValue)
                    {
                        return failSharedCase(
                            "spreadsheetengine_logic_tests", rRow, "IFS second-branch mismatch");
                    }
                }
                else if (eAction2 != IfsAction::ReturnNotAvailable
                         || eExpectedError != Error::NotAvailable)
                {
                    return failSharedCase(
                        "spreadsheetengine_logic_tests", rRow, "IFS fallback mismatch");
                }
            }
        }
        else
        {
            return failSharedCase(
                "spreadsheetengine_logic_tests", rRow, "unknown logic shared-case function");
        }
    }

    std::cout << "spreadsheetengine logic api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
