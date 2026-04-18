/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "helper/qahelper.hxx"

#include <docoptio.hxx>
#include <formula/errorcodes.hxx>
#include <formula/grammar.hxx>
#include <formulacell.hxx>
#include <interpre.hxx>
#include <refupdatecontext.hxx>
#include <rangelst.hxx>
#include <rangenam.hxx>
#include <rtl/math.hxx>
#include <scopetools.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/detail/FodsLoader.hxx>
#include <spreadsheetengine/detail/OdfFormulaParser.hxx>
#include <spreadsheetengine/detail/WorkbookModel.hxx>
#include <spreadsheetengine/runtime/DateTimeParse.hxx>
#include <spreadsheetengine/runtime/DateTimeParts.hxx>
#include <spreadsheetengine/runtime/DateTimeWorkday.hxx>

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace
{

using spreadsheetengine::compat::libreoffice::toFormulaError;
using spreadsheetengine::compat::libreoffice::toApiString;
using spreadsheetengine::compat::libreoffice::toLibreOfficeString;
using spreadsheetengine::compat::libreoffice::interprettaileval::DiagnosticSample;
using spreadsheetengine::compat::libreoffice::interprettaileval::FallbackReason;
using spreadsheetengine::compat::libreoffice::interprettaileval::FunctionKind;
using spreadsheetengine::compat::libreoffice::interprettaileval::ObservedFormulaCellStatus;
using spreadsheetengine::compat::libreoffice::interprettaileval::StatsSnapshot;
using spreadsheetengine::core::fods::loadWorkbook;
using spreadsheetengine::core::workbook::Cell;
using spreadsheetengine::core::workbook::FormulaSearchType;
using spreadsheetengine::core::workbook::NamedRange;
using spreadsheetengine::core::workbook::Sheet;
using spreadsheetengine::core::workbook::Workbook;

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

class TestInterpretTailCorpus : public ScUcalcTestBase
{
};

const char* functionKindName(FunctionKind eFunction);
const char* fallbackReasonName(FallbackReason eReason);
sal_uInt64 totalFallbackCount(const StatsSnapshot& rStats);
sal_uInt64 statsSeenCount(const StatsSnapshot& rStats);
void accumulateStats(StatsSnapshot& rTarget, const StatsSnapshot& rSource);

struct ProbeDiagnosticSample
{
    OUString maWorkbookLabel;
    OUString maCellAddress;
    OUString maFormulaSource;
    OUString maFunctionName;
    OUString maOutcome;
    OUString maCalcResult;
    OUString maLiveHostResult;
    OUString maEngineResult;
};

struct ProbeValue
{
    enum class Kind
    {
        Empty,
        Error,
        Number,
        String,
    };

    Kind meKind = Kind::Empty;
    FormulaError meError = FormulaError::NONE;
    double mfValue = 0.0;
    OUString maString;
};

struct SupportedProbeRun
{
    std::size_t mnRawFormulaCount = 0;
    std::size_t mnLiveAuthoritativeFormulaCount = 0;
    std::size_t mnLiveTargetFormulaCount = 0;
    std::size_t mnHostTruthArtifactFormulaCount = 0;
    StatsSnapshot maRawStats;
    StatsSnapshot maLiveAuthoritativeStats;
    StatsSnapshot maLiveTargetStats;
    std::array<std::size_t, static_cast<std::size_t>(FunctionKind::Count)>
        maHostTruthArtifactFunctionCount {};
};

void recordProbeAuthoritativeRoute(StatsSnapshot& rStats, FunctionKind eFunction);
void recordProbeAuthoritativeFallback(
    StatsSnapshot& rStats, FallbackReason eReason, FunctionKind eFunction);
ProbeValue probeValueFromWorkbookCell(const Cell& rCell);
ProbeValue probeWorkbookComparableValueFromWorkbookCell(const Cell& rCell);
ProbeValue probeValueFromLiveHostCell(ScDocument& rDoc, const ScAddress& rPos);
ProbeValue probeValueFromEngineAttempt(
    const spreadsheetengine::compat::libreoffice::interprettaileval::EvaluationAttempt& rAttempt);
bool probeValuesMatch(const ProbeValue& rLeft, const ProbeValue& rRight);
OUString probeValueToDiagnosticString(const ProbeValue& rValue);

struct ReplayEligibilityDiagnosticSample
{
    OUString maWorkbookLabel;
    OUString maCellAddress;
    OUString maFunctionName;
    OUString maSharedState;
    OUString maOutcome;
    bool mbNeedsInterpretBeforeDirty = false;
    bool mbNeedsInterpretAfterDirty = false;
    bool mbDirtyAfterInterpret = false;
    bool mbSeenViaSharedTop = false;
};

struct ReplayEligibilityInventory
{
    std::size_t mnPromotedFormulaCells = 0;
    std::size_t mnSharedFormulaCells = 0;
    std::size_t mnSharedTopFormulaCells = 0;
    std::size_t mnSharedMemberFormulaCells = 0;
    std::size_t mnNonSharedFormulaCells = 0;
    std::size_t mnNeedsInterpretBeforeDirty = 0;
    std::size_t mnNeedsInterpretAfterDirty = 0;
    std::size_t mnDirectSeen = 0;
    std::size_t mnDirectSupported = 0;
    std::size_t mnDirectFallback = 0;
    std::size_t mnDirectUnseen = 0;
    std::size_t mnInterpretReturnedFalse = 0;
    std::size_t mnDirtyAfterInterpret = 0;
    std::size_t mnNeedsInterpretAfterInterpret = 0;
    std::size_t mnSharedMemberSeenViaTop = 0;
    std::size_t mnSharedMemberFallbackViaTop = 0;
    std::size_t mnSharedMemberStillUnseenViaTop = 0;
    std::size_t mnUnseenSharedTop = 0;
    std::size_t mnUnseenSharedMember = 0;
    std::size_t mnUnseenNonShared = 0;
    std::size_t mnMatrixFormulaCells = 0;
    std::size_t mnHyperLinkFormulaCells = 0;
    std::size_t mnMissingCodeFormulaCells = 0;
    std::size_t mnRawErrorFormulaCells = 0;
    std::array<std::size_t, static_cast<std::size_t>(FunctionKind::Count)> maFunctionFormulaCells {};
    std::array<std::size_t, static_cast<std::size_t>(FunctionKind::Count)> maFunctionDirectSeen {};
    std::array<std::size_t, static_cast<std::size_t>(FunctionKind::Count)> maFunctionDirectUnseen {};
};

struct ObserveSurfaceInventory
{
    struct UnknownRootInventoryEntry
    {
        std::size_t mnFormulaCells = 0;
        std::size_t mnFallbackFormulaCells = 0;
        std::size_t mnUnsupportedFunctionFormulaCells = 0;
        std::size_t mnUnseenFormulaCells = 0;
    };

    std::size_t mnFormulaCells = 0;
    std::size_t mnSeenFormulaCells = 0;
    std::size_t mnSupportedFormulaCells = 0;
    std::size_t mnFallbackFormulaCells = 0;
    std::size_t mnUnseenFormulaCells = 0;
    std::size_t mnUnsupportedFunctionFormulaCells = 0;
    StatsSnapshot maAttemptStats;
    std::array<std::size_t, static_cast<std::size_t>(FunctionKind::Count)> maFunctionFormulaCells {};
    std::array<std::size_t, static_cast<std::size_t>(FunctionKind::Count)> maFunctionSeenCells {};
    std::array<std::size_t, static_cast<std::size_t>(FunctionKind::Count)>
        maFunctionSupportedCells {};
    std::array<std::size_t, static_cast<std::size_t>(FunctionKind::Count)> maFunctionFallbackCells {};
    std::array<std::size_t, static_cast<std::size_t>(FunctionKind::Count)> maFunctionUnseenCells {};
    std::array<std::size_t, static_cast<std::size_t>(FunctionKind::Count)>
        maFunctionUnsupportedFunctionCells {};
    std::map<OUString, UnknownRootInventoryEntry> maUnknownRootInventory;
};

struct ScAddressLess
{
    bool operator()(const ScAddress& rLeft, const ScAddress& rRight) const
    {
        if (rLeft.Tab() != rRight.Tab())
            return rLeft.Tab() < rRight.Tab();
        if (rLeft.Row() != rRight.Row())
            return rLeft.Row() < rRight.Row();
        return rLeft.Col() < rRight.Col();
    }
};

std::vector<ProbeDiagnosticSample>& probeDiagnosticSamples()
{
    static std::vector<ProbeDiagnosticSample> aSamples;
    return aSamples;
}

std::vector<ReplayEligibilityDiagnosticSample>& replayEligibilityDiagnosticSamples()
{
    static std::vector<ReplayEligibilityDiagnosticSample> aSamples;
    return aSamples;
}

bool envEnabled(const char* pName)
{
    const char* pValue = std::getenv(pName);
    if (!pValue)
        return false;

    const std::string aValue(pValue);
    return !aValue.empty() && aValue != "0" && aValue != "off" && aValue != "false";
}

std::size_t countLegacyInterpreterSubroutines()
{
    const std::filesystem::path aRepoRoot
        = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT).parent_path();
    const std::filesystem::path aHeaderPath
        = aRepoRoot / "sc" / "source" / "core" / "inc" / "interpre.hxx";

    std::ifstream aStream(aHeaderPath);
    if (!aStream.is_open())
        return 0;

    std::ostringstream aBuffer;
    aBuffer << aStream.rdbuf();
    const std::string aText = aBuffer.str();
    static const std::regex aPattern(R"(\bvoid\s+(Sc[A-Za-z0-9_]+)\s*\()");

    return static_cast<std::size_t>(
        std::distance(std::sregex_iterator(aText.begin(), aText.end(), aPattern),
            std::sregex_iterator()));
}

struct Interp4LegacyLambdaInventory
{
    std::size_t mnLambdaCount = 0;
    std::size_t mnDispatchLambdaCount = 0;
    std::size_t mnDispatchCallCount = 0;
    std::size_t mnQuarantineCoveredLambdaCount = 0;
    std::size_t mnQuarantineMissingLambdaCount = 0;
    std::size_t mnQuarantineCoveredDispatchLambdaCount = 0;
    std::size_t mnQuarantineMissingDispatchLambdaCount = 0;
    std::vector<std::string> maMissingDispatchLambdaNames;
};

struct Interp4EngineDispatchInventory
{
    std::size_t mnAttemptCaseCount = 0;
};

void accumulateDispatchRuntimeStats(ScInterpreterDispatchRuntimeStatsSnapshot& rAccumulator,
                                    const ScInterpreterDispatchRuntimeStatsSnapshot& rDelta)
{
    rAccumulator.mnEngineAttemptedCount += rDelta.mnEngineAttemptedCount;
    rAccumulator.mnEngineSucceededCount += rDelta.mnEngineSucceededCount;
    rAccumulator.mnEngineDeclinedCount += rDelta.mnEngineDeclinedCount;
}

void accumulateReachabilityStats(ScInterpreterReachabilityStatsSnapshot& rAccumulator,
                                 const ScInterpreterReachabilityStatsSnapshot& rDelta)
{
    rAccumulator.mnFormulaCellInterpretCount += rDelta.mnFormulaCellInterpretCount;
    rAccumulator.mnFormulaGroupAttemptCount += rDelta.mnFormulaGroupAttemptCount;
    rAccumulator.mnFormulaGroupHandledCount += rDelta.mnFormulaGroupHandledCount;
    rAccumulator.mnInterpretTailCount += rDelta.mnInterpretTailCount;
    rAccumulator.mnClassicInterpretCount += rDelta.mnClassicInterpretCount;
}

Interp4LegacyLambdaInventory countInterp4LegacyLambdas()
{
    const std::filesystem::path aRepoRoot
        = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT).parent_path();
    const std::filesystem::path aSourcePath
        = aRepoRoot / "sc" / "source" / "core" / "tool" / "interpr4.cxx";

    std::ifstream aStream(aSourcePath);
    if (!aStream.is_open())
        return {};

    std::vector<std::string> aLines;
    for (std::string aLine; std::getline(aStream, aLine);)
        aLines.push_back(aLine);

    static const std::regex aDefinitionPattern(
        R"(const\s+auto\s+(pushLegacy[A-Za-z0-9_]+)\s*=)");
    static const std::regex aPushCallPattern(R"(\b(pushLegacy[A-Za-z0-9_]+)\s*\()");
    static const std::regex aOpcodeCasePattern(R"(\bcase\s+oc[A-Za-z0-9_]+\b)");
    const std::array<std::string_view, 5> aWarningMarkers = {
        "warnIfLegacy", "warnTextUtilityDispatch", "warnInformationPredicateDispatch",
        "warnLogicalDispatch", "warnConditionalDispatch"
    };

    struct LambdaRecord
    {
        std::string maName;
        bool mbDirectWarning = false;
        bool mbDispatchReferenced = false;
        std::set<std::string> maDependencies;
    };

    std::vector<std::pair<std::size_t, std::string>> aDefinitions;
    for (std::size_t nLine = 0; nLine < aLines.size(); ++nLine)
    {
        std::smatch aMatch;
        if (std::regex_search(aLines[nLine], aMatch, aDefinitionPattern))
            aDefinitions.emplace_back(nLine, aMatch[1].str());
    }

    Interp4LegacyLambdaInventory aInventory;
    if (aDefinitions.empty())
        return aInventory;

    std::map<std::string, std::size_t> aLambdaIndexByName;
    std::vector<LambdaRecord> aRecords;
    aRecords.reserve(aDefinitions.size());
    for (std::size_t nIndex = 0; nIndex < aDefinitions.size(); ++nIndex)
    {
        const auto& [nStartLine, rName] = aDefinitions[nIndex];
        const std::size_t nEndLine
            = (nIndex + 1 < aDefinitions.size()) ? aDefinitions[nIndex + 1].first : aLines.size();

        std::ostringstream aBlockBuilder;
        for (std::size_t nLine = nStartLine; nLine < nEndLine; ++nLine)
            aBlockBuilder << aLines[nLine] << '\n';
        const std::string aBlock = aBlockBuilder.str();

        LambdaRecord aRecord;
        aRecord.maName = rName;
        for (std::string_view rMarker : aWarningMarkers)
        {
            if (aBlock.find(rMarker) != std::string::npos)
            {
                aRecord.mbDirectWarning = true;
                break;
            }
        }

        for (std::sregex_iterator aIt(aBlock.begin(), aBlock.end(), aPushCallPattern), aEnd;
             aIt != aEnd; ++aIt)
        {
            const std::string aDependency = (*aIt)[1].str();
            if (aDependency != aRecord.maName)
                aRecord.maDependencies.insert(aDependency);
        }

        aLambdaIndexByName.emplace(aRecord.maName, aRecords.size());
        aRecords.push_back(std::move(aRecord));
    }

    bool bInsideOpcodeCase = false;
    std::ostringstream aCaseBlockBuilder;
    for (const std::string& rLine : aLines)
    {
        if (std::regex_search(rLine, aOpcodeCasePattern) && !bInsideOpcodeCase)
        {
            bInsideOpcodeCase = true;
            aCaseBlockBuilder.str({});
            aCaseBlockBuilder.clear();
        }
        if (!bInsideOpcodeCase)
            continue;

        aCaseBlockBuilder << rLine << '\n';
        if (rLine.find("break;") == std::string::npos)
            continue;

        const std::string aCaseBlock = aCaseBlockBuilder.str();
        for (std::sregex_iterator aIt(aCaseBlock.begin(), aCaseBlock.end(), aPushCallPattern), aEnd;
             aIt != aEnd; ++aIt)
        {
            const std::string aName = (*aIt)[1].str();
            auto aFound = aLambdaIndexByName.find(aName);
            if (aFound == aLambdaIndexByName.end())
                continue;
            aRecords[aFound->second].mbDispatchReferenced = true;
            ++aInventory.mnDispatchCallCount;
        }

        bInsideOpcodeCase = false;
    }

    std::vector<bool> aCovered(aRecords.size(), false);
    std::vector<bool> aVisiting(aRecords.size(), false);
    std::function<bool(std::size_t)> aIsCovered = [&](std::size_t nIndex) -> bool {
        if (aCovered[nIndex])
            return true;
        if (aVisiting[nIndex])
            return false;
        aVisiting[nIndex] = true;
        if (aRecords[nIndex].mbDirectWarning)
        {
            aVisiting[nIndex] = false;
            aCovered[nIndex] = true;
            return true;
        }
        for (const std::string& rDependency : aRecords[nIndex].maDependencies)
        {
            auto aFound = aLambdaIndexByName.find(rDependency);
            if (aFound != aLambdaIndexByName.end() && aIsCovered(aFound->second))
            {
                aVisiting[nIndex] = false;
                aCovered[nIndex] = true;
                return true;
            }
        }
        aVisiting[nIndex] = false;
        return false;
    };

    aInventory.mnLambdaCount = aRecords.size();
    for (std::size_t nIndex = 0; nIndex < aRecords.size(); ++nIndex)
    {
        const bool bCovered = aIsCovered(nIndex);
        if (bCovered)
            ++aInventory.mnQuarantineCoveredLambdaCount;
        if (aRecords[nIndex].mbDispatchReferenced)
        {
            ++aInventory.mnDispatchLambdaCount;
            if (bCovered)
                ++aInventory.mnQuarantineCoveredDispatchLambdaCount;
            else
                aInventory.maMissingDispatchLambdaNames.push_back(aRecords[nIndex].maName);
        }
    }

    aInventory.mnQuarantineMissingLambdaCount
        = aInventory.mnLambdaCount - aInventory.mnQuarantineCoveredLambdaCount;
    aInventory.mnQuarantineMissingDispatchLambdaCount
        = aInventory.mnDispatchLambdaCount - aInventory.mnQuarantineCoveredDispatchLambdaCount;
    return aInventory;
}

Interp4EngineDispatchInventory countInterp4EngineDispatchAttempts()
{
    const std::filesystem::path aRepoRoot
        = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT).parent_path();
    const std::filesystem::path aSourcePath
        = aRepoRoot / "sc" / "source" / "core" / "tool" / "interpr4.cxx";

    std::ifstream aStream(aSourcePath);
    if (!aStream.is_open())
        return {};

    Interp4EngineDispatchInventory aInventory;
    static const std::regex aAttemptPattern(R"(\bif\s*\(!tryPushEngineScalarBinaryOp\s*\()");
    for (std::string aLine; std::getline(aStream, aLine);)
    {
        if (std::regex_search(aLine, aAttemptPattern))
            ++aInventory.mnAttemptCaseCount;
    }
    return aInventory;
}

void resetProbeDiagnosticSamples()
{
    probeDiagnosticSamples().clear();
}

void resetReplayEligibilityDiagnosticSamples()
{
    replayEligibilityDiagnosticSamples().clear();
}

bool probeDiagnosticsTrackLiveAuthoritativeSurface()
{
    if (const char* pSurface
        = std::getenv("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_PROBE_DIAGNOSTIC_SURFACE"))
    {
        return OString(pSurface).equalsIgnoreAsciiCase("live_authoritative");
    }

    return false;
}

void maybeAddProbeDiagnosticSample(const OUString& rWorkbookLabel, const ScDocument& rDoc,
    const ScAddress& rPos, const OUString& rFormulaSource, FunctionKind eFunction,
    const OUString& rOutcome, const OUString& rCalcResult, const OUString& rLiveHostResult,
    const OUString& rEngineResult)
{
    if (!envEnabled("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_PROBE_DIAGNOSTICS"))
        return;

    // Keep the limited probe diagnostic buffer focused on actionable rows.
    if (rOutcome == u"authoritative")
        return;

    if (const char* pFilter
        = std::getenv("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_PROBE_DIAGNOSTIC_FUNCTION"))
    {
        const OString aFilter = OString(pFilter);
        const OString aFunctionName(functionKindName(eFunction));
        if (!aFilter.equalsIgnoreAsciiCase(aFunctionName))
            return;
    }

    auto& rSamples = probeDiagnosticSamples();
    std::size_t nLimit = 20;
    if (const char* pValue = std::getenv("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_PROBE_DIAGNOSTIC_LIMIT"))
    {
        try
        {
            const int nConfiguredLimit = std::stoi(pValue);
            if (nConfiguredLimit > 0)
                nLimit = static_cast<std::size_t>(nConfiguredLimit);
        }
        catch (...)
        {
        }
    }
    if (rSamples.size() >= nLimit)
        return;

    ProbeDiagnosticSample aSample;
    aSample.maWorkbookLabel = rWorkbookLabel;
    aSample.maCellAddress = rPos.Format(ScRefFlags::ADDR_ABS_3D, &rDoc, rDoc.GetAddressConvention());
    aSample.maFormulaSource = rFormulaSource;
    aSample.maFunctionName = OUString::fromUtf8(functionKindName(eFunction));
    aSample.maOutcome = rOutcome;
    aSample.maCalcResult = rCalcResult;
    aSample.maLiveHostResult = rLiveHostResult;
    aSample.maEngineResult = rEngineResult;
    rSamples.push_back(std::move(aSample));
}

void maybeAddReplayEligibilityDiagnosticSample(const OUString& rWorkbookLabel,
    const ScDocument& rDoc, const ScAddress& rPos, FunctionKind eFunction, std::u16string_view rSharedState,
    std::u16string_view rOutcome, bool bNeedsInterpretBeforeDirty, bool bNeedsInterpretAfterDirty,
    bool bDirtyAfterInterpret, bool bSeenViaSharedTop)
{
    if (!envEnabled("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_ELIGIBILITY_DIAGNOSTICS"))
        return;

    auto& rSamples = replayEligibilityDiagnosticSamples();
    std::size_t nLimit = 24;
    if (const char* pValue
        = std::getenv("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_ELIGIBILITY_DIAGNOSTIC_LIMIT"))
    {
        try
        {
            const int nConfiguredLimit = std::stoi(pValue);
            if (nConfiguredLimit > 0)
                nLimit = static_cast<std::size_t>(nConfiguredLimit);
        }
        catch (...)
        {
        }
    }
    if (rSamples.size() >= nLimit)
        return;

    ReplayEligibilityDiagnosticSample aSample;
    aSample.maWorkbookLabel = rWorkbookLabel;
    aSample.maCellAddress = rPos.Format(ScRefFlags::ADDR_ABS_3D, &rDoc, rDoc.GetAddressConvention());
    aSample.maFunctionName = OUString::fromUtf8(functionKindName(eFunction));
    aSample.maSharedState = OUString(rSharedState.data(), rSharedState.size());
    aSample.maOutcome = OUString(rOutcome.data(), rOutcome.size());
    aSample.mbNeedsInterpretBeforeDirty = bNeedsInterpretBeforeDirty;
    aSample.mbNeedsInterpretAfterDirty = bNeedsInterpretAfterDirty;
    aSample.mbDirtyAfterInterpret = bDirtyAfterInterpret;
    aSample.mbSeenViaSharedTop = bSeenViaSharedTop;
    rSamples.push_back(std::move(aSample));
}

std::vector<std::filesystem::path> collectFodsFiles(const std::filesystem::path& rPath)
{
    std::vector<std::filesystem::path> aFiles;

    if (std::filesystem::is_regular_file(rPath))
    {
        if (rPath.extension() == ".fods")
            aFiles.push_back(rPath);
        return aFiles;
    }

    if (!std::filesystem::is_directory(rPath))
        return aFiles;

    for (const auto& rEntry : std::filesystem::directory_iterator(rPath))
    {
        if (rEntry.is_regular_file() && rEntry.path().extension() == ".fods")
            aFiles.push_back(rEntry.path());
    }

    std::sort(aFiles.begin(), aFiles.end());
    return aFiles;
}

std::vector<std::filesystem::path> collectDefaultReplayCorpus()
{
    if (const char* pPath = std::getenv("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_PATH"))
    {
        const auto aFiles = collectFodsFiles(std::filesystem::path(pPath));
        if (!aFiles.empty())
            return aFiles;
    }

    const std::filesystem::path aRepoRoot
        = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT).parent_path();

    std::vector<std::filesystem::path> aFiles
        = collectFodsFiles(aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "logical"
                           / "fods");

    const std::filesystem::path aMathRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "mathematical" / "fods";
    const auto aMathFiles = collectFodsFiles(aMathRoot);
    aFiles.insert(aFiles.end(), aMathFiles.begin(), aMathFiles.end());

    const std::filesystem::path aTextRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "text" / "fods";
    const auto aTextFiles = collectFodsFiles(aTextRoot);
    aFiles.insert(aFiles.end(), aTextFiles.begin(), aTextFiles.end());

    const std::filesystem::path aDateTimeRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "date_time" / "fods";
    const auto aDateTimeFiles = collectFodsFiles(aDateTimeRoot);
    aFiles.insert(aFiles.end(), aDateTimeFiles.begin(), aDateTimeFiles.end());

    const std::filesystem::path aSpreadsheetRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "spreadsheet" / "fods";
    const auto aSpreadsheetFiles = collectFodsFiles(aSpreadsheetRoot);
    aFiles.insert(aFiles.end(), aSpreadsheetFiles.begin(), aSpreadsheetFiles.end());

    const std::filesystem::path aInformationRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "information" / "fods";
    const auto aInformationFiles = collectFodsFiles(aInformationRoot);
    aFiles.insert(aFiles.end(), aInformationFiles.begin(), aInformationFiles.end());

    const std::filesystem::path aFinancialRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "financial" / "fods";
    const auto aFinancialFiles = collectFodsFiles(aFinancialRoot);
    aFiles.insert(aFiles.end(), aFinancialFiles.begin(), aFinancialFiles.end());

    const std::filesystem::path aStatisticalRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "statistical" / "fods";
    const auto aStatisticalFiles = collectFodsFiles(aStatisticalRoot);
    aFiles.insert(aFiles.end(), aStatisticalFiles.begin(), aStatisticalFiles.end());

    const std::filesystem::path aAddinRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "addin" / "fods";
    const auto aAddinFiles = collectFodsFiles(aAddinRoot);
    aFiles.insert(aFiles.end(), aAddinFiles.begin(), aAddinFiles.end());

    const std::filesystem::path aArrayRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "array" / "fods";
    const auto aArrayFiles = collectFodsFiles(aArrayRoot);
    aFiles.insert(aFiles.end(), aArrayFiles.begin(), aArrayFiles.end());

    const std::filesystem::path aDatabaseRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "database" / "fods";
    const auto aDatabaseFiles = collectFodsFiles(aDatabaseRoot);
    aFiles.insert(aFiles.end(), aDatabaseFiles.begin(), aDatabaseFiles.end());

    return aFiles;
}

void applyWorkbookSearchSettings(ScDocument& rDoc, const Workbook& rWorkbook)
{
    ScDocOptions aOptions(rDoc.GetDocOptions());
    aOptions.SetFormulaRegexEnabled(false);
    aOptions.SetFormulaWildcardsEnabled(false);

    switch (rWorkbook.meFormulaSearchType)
    {
        case FormulaSearchType::Regex:
            aOptions.SetFormulaRegexEnabled(true);
            break;
        case FormulaSearchType::Wildcard:
            aOptions.SetFormulaWildcardsEnabled(true);
            break;
        case FormulaSearchType::Normal:
            break;
    }

    aOptions.SetMatchWholeCell(rWorkbook.mbSearchCriteriaMustApplyToWholeCell);
    rDoc.SetDocOptions(aOptions);
    rDoc.SetStorageGrammar(formula::FormulaGrammar::GRAM_ODFF);
    rDoc.SetGrammar(formula::FormulaGrammar::GRAM_ODFF);
}

void prepareSheetLayout(ScDocument& rDoc, const Workbook& rWorkbook, std::string_view rWorkbookLabel)
{
    CPPUNIT_ASSERT_MESSAGE("replay workbook must have at least one sheet",
        !rWorkbook.maSheets.empty());

    while (rDoc.GetTableCount() > 1)
        CPPUNIT_ASSERT(rDoc.DeleteTab(rDoc.GetTableCount() - 1));

    const OUString aFirstSheetName = toLibreOfficeString(rWorkbook.maSheets.front().maName);
    if (rDoc.GetTableCount() == 0)
    {
        CPPUNIT_ASSERT_MESSAGE(
            ("failed to insert first sheet for " + std::string(rWorkbookLabel) + ": "
             + aFirstSheetName.toUtf8().getStr())
                .c_str(),
            rDoc.InsertTab(0, aFirstSheetName));
    }
    else
    {
        OUString aCurrentName;
        CPPUNIT_ASSERT(rDoc.GetName(0, aCurrentName));
        if (aCurrentName != aFirstSheetName)
        {
            CPPUNIT_ASSERT_MESSAGE(
                ("failed to rename first sheet for " + std::string(rWorkbookLabel) + ": "
                 + aFirstSheetName.toUtf8().getStr())
                    .c_str(),
                rDoc.RenameTab(0, aFirstSheetName));
        }
    }

    for (std::size_t nIndex = 1; nIndex < rWorkbook.maSheets.size(); ++nIndex)
    {
        const OUString aSheetName = toLibreOfficeString(rWorkbook.maSheets[nIndex].maName);
        CPPUNIT_ASSERT_MESSAGE(
            ("failed to insert sheet for " + std::string(rWorkbookLabel) + ": "
             + aSheetName.toUtf8().getStr())
                .c_str(),
            rDoc.InsertTab(static_cast<SCTAB>(nIndex), aSheetName));
    }
}

ScAddress parseBaseAddress(const Workbook& rWorkbook, ScDocument& rDoc, const NamedRange& rNamedRange)
{
    SCTAB nScopeTab = 0;
    if (!rNamedRange.maScopeSheetName.empty())
    {
        const auto oScopeSheet = rWorkbook.findSheetId(rNamedRange.maScopeSheetName);
        CPPUNIT_ASSERT_MESSAGE("named-range scope sheet missing", oScopeSheet.has_value());
        nScopeTab = static_cast<SCTAB>(*oScopeSheet);
    }

    ScAddress aBase(0, 0, nScopeTab);
    if (rNamedRange.maBaseCellAddress.empty())
        return aBase;

    ScAddress aParsed;
    const auto nFlags
        = aParsed.Parse(toLibreOfficeString(rNamedRange.maBaseCellAddress), rDoc);
    CPPUNIT_ASSERT_MESSAGE("named-range base address parse failed",
        (nFlags & ScRefFlags::VALID) == ScRefFlags::VALID);
    return aParsed;
}

void materializeNamedRanges(const Workbook& rWorkbook, ScDocument& rDoc)
{
    for (const auto& rNamedRange : rWorkbook.maNamedRanges)
    {
        const ScAddress aBase = parseBaseAddress(rWorkbook, rDoc, rNamedRange);
        ScRangeName* pNames = nullptr;
        if (rNamedRange.isGlobal())
            pNames = rDoc.GetRangeName();
        else
        {
            const auto oScopeSheet = rWorkbook.findSheetId(rNamedRange.maScopeSheetName);
            CPPUNIT_ASSERT_MESSAGE("named-range scope sheet missing", oScopeSheet.has_value());
            const SCTAB nScopeTab = static_cast<SCTAB>(*oScopeSheet);
            pNames = rDoc.GetRangeName(nScopeTab);
            if (!pNames)
            {
                rDoc.SetRangeName(nScopeTab, std::make_unique<ScRangeName>());
                pNames = rDoc.GetRangeName(nScopeTab);
            }
        }

        CPPUNIT_ASSERT(pNames);
        auto pData = new ScRangeData(rDoc, toLibreOfficeString(rNamedRange.maName),
            toLibreOfficeString(rNamedRange.maCellRangeAddress), aBase, ScRangeData::Type::Name,
            formula::FormulaGrammar::GRAM_ODFF);
        CPPUNIT_ASSERT_MESSAGE("named-range insertion failed", pNames->insert(pData));
    }
}

void materializeScalarCell(ScDocument& rDoc, const ScAddress& rPos, const Cell& rCell)
{
    const auto& rValue = rCell.maValue;
    if (rValue.isEmpty())
        return;

    if (rValue.isNumber() || rValue.isBoolean())
    {
        rDoc.SetValue(rPos, rValue.mfNumber);
        return;
    }

    if (rValue.isText())
    {
        rDoc.SetTextCell(rPos, toLibreOfficeString(rValue.maString));
        return;
    }

    if (rValue.isError())
        rDoc.SetError(rPos.Col(), rPos.Row(), rPos.Tab(), toFormulaError(rValue.meError));
}

[[nodiscard]] bool hasArrayConstant(spreadsheetengine::api::StringView rFormula)
{
    bool bInString = false;
    for (char16_t cChar : rFormula)
    {
        if (bInString)
        {
            if (cChar == u'"')
                bInString = false;
            continue;
        }

        if (cChar == u'"')
        {
            bInString = true;
            continue;
        }

        if (cChar == u'{')
            return true;
    }

    return false;
}

void materializeSheetCells(const Sheet& rSheet, SCTAB nTab, ScDocument& rDoc)
{
    for (const auto& rEntry : rSheet.maCells)
    {
        const ScAddress aPos(static_cast<SCCOL>(rEntry.first.first),
            static_cast<SCROW>(rEntry.first.second), nTab);
        const Cell& rCell = rEntry.second;
        if (!rCell.hasFormula())
            materializeScalarCell(rDoc, aPos, rCell);
    }

    for (const auto& rEntry : rSheet.maCells)
    {
        const ScAddress aPos(static_cast<SCCOL>(rEntry.first.first),
            static_cast<SCROW>(rEntry.first.second), nTab);
        const Cell& rCell = rEntry.second;
        if (rCell.hasFormula())
        {
            rDoc.SetFormula(aPos, toLibreOfficeString(rCell.maFormula),
                formula::FormulaGrammar::GRAM_ODFF);
            ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
            CPPUNIT_ASSERT(pFormula);

            if (hasArrayConstant(rCell.maFormula))
            {
                // Preserve the imported raw ODF formula as a token-backed canonical source
                // sidecar so InterpretTail can bypass the ambiguous display rewrite.
                pFormula->SetHybridFormula(
                    toLibreOfficeString(rCell.maFormula), formula::FormulaGrammar::GRAM_ODFF);
                continue;
            }

            // Seed replay-imported cached scalar results so referenced formula
            // reads can observe workbook parity instead of forcing live
            // recalculation.
            if (rCell.maValue.isNumber() || rCell.maValue.isBoolean())
            {
                pFormula->SetHybridDouble(rCell.maValue.mfNumber);
                const OUString aCachedString = !rCell.maRawValue.empty()
                    ? toLibreOfficeString(rCell.maRawValue)
                    : OUString::number(rCell.maValue.mfNumber);
                pFormula->SetHybridString(rDoc.GetSharedStringPool().intern(aCachedString));
                pFormula->ResetDirty();
            }
            else if (rCell.maValue.isText())
            {
                const OUString aCachedString = !rCell.maRawValue.empty()
                    ? toLibreOfficeString(rCell.maRawValue)
                    : toLibreOfficeString(rCell.maValue.maString);
                pFormula->SetHybridString(rDoc.GetSharedStringPool().intern(aCachedString));
                pFormula->ResetDirty();
            }
            else if (rCell.maValue.isError())
            {
                const OUString aErrorString = !rCell.maRawValue.empty()
                    ? toLibreOfficeString(rCell.maRawValue)
                    : ScGlobal::GetErrorString(toFormulaError(rCell.maValue.meError));
                pFormula->SetHybridString(rDoc.GetSharedStringPool().intern(aErrorString));
                pFormula->ResetDirty();
            }
        }
    }
}

std::size_t materializeWorkbookToCalc(
    const Workbook& rWorkbook, ScDocument& rDoc, std::string_view rWorkbookLabel)
{
    applyWorkbookSearchSettings(rDoc, rWorkbook);
    prepareSheetLayout(rDoc, rWorkbook, rWorkbookLabel);

    sc::AutoCalcSwitch aPopulateSwitch(rDoc, false);
    materializeNamedRanges(rWorkbook, rDoc);

    std::size_t nFormulaCellCount = 0;
    for (std::size_t nSheet = 0; nSheet < rWorkbook.maSheets.size(); ++nSheet)
    {
        materializeSheetCells(rWorkbook.maSheets[nSheet], static_cast<SCTAB>(nSheet), rDoc);
        for (const auto& rEntry : rWorkbook.maSheets[nSheet].maCells)
        {
            if (rEntry.second.hasFormula())
                ++nFormulaCellCount;
        }
    }

    rDoc.StartAllListeners();
    rDoc.TrackFormulas();

    return nFormulaCellCount;
}

FunctionKind classifySupportedProbeFunction(std::u16string_view rFormula)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    const auto aNormalized = setaileval::detail::normalizeFormulaSource(rFormula);
    const auto aParse = spreadsheetengine::core::formula::parseFormula(aNormalized);
    if (!aParse || !aParse.mpRoot)
    {
        return FunctionKind::Unknown;
    }

    return setaileval::detail::classifyDelegatedFunctionNode(*aParse.mpRoot);
}

OUString binaryOperatorLabel(spreadsheetengine::core::formula::BinaryOperator eOperator)
{
    using spreadsheetengine::core::formula::BinaryOperator;
    switch (eOperator)
    {
        case BinaryOperator::Add:
            return u"operator:+"_ustr;
        case BinaryOperator::Subtract:
            return u"operator:-"_ustr;
        case BinaryOperator::Multiply:
            return u"operator:*"_ustr;
        case BinaryOperator::Divide:
            return u"operator:/"_ustr;
        case BinaryOperator::Power:
            return u"operator:^"_ustr;
        case BinaryOperator::Concat:
            return u"operator:&"_ustr;
        case BinaryOperator::Equal:
            return u"operator:="_ustr;
        case BinaryOperator::NotEqual:
            return u"operator:<>"_ustr;
        case BinaryOperator::Less:
            return u"operator:<"_ustr;
        case BinaryOperator::LessEqual:
            return u"operator:<="_ustr;
        case BinaryOperator::Greater:
            return u"operator:>"_ustr;
        case BinaryOperator::GreaterEqual:
            return u"operator:>="_ustr;
    }

    return u"operator:binary"_ustr;
}

OUString unaryOperatorLabel(spreadsheetengine::core::formula::UnaryOperator eOperator)
{
    using spreadsheetengine::core::formula::UnaryOperator;
    switch (eOperator)
    {
        case UnaryOperator::Plus:
            return u"operator:unary+"_ustr;
        case UnaryOperator::Minus:
            return u"operator:unary-"_ustr;
    }

    return u"operator:unary"_ustr;
}

OUString classifyUnknownRootLabel(std::u16string_view rFormula)
{
    namespace seformula = spreadsheetengine::core::formula;
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    const auto aNormalized = setaileval::detail::normalizeFormulaSource(rFormula);
    const auto aParse = seformula::parseFormula(aNormalized);
    if (!aParse || !aParse.mpRoot)
        return u"parse_failure"_ustr;

    const auto& rRoot = *aParse.mpRoot;
    switch (rRoot.meKind)
    {
        case seformula::NodeKind::FunctionCall:
            return toLibreOfficeString(setaileval::detail::uppercaseAscii(rRoot.maPrimaryText));
        case seformula::NodeKind::BinaryOperation:
            return binaryOperatorLabel(rRoot.meBinaryOperator);
        case seformula::NodeKind::UnaryOperation:
            return unaryOperatorLabel(rRoot.meUnaryOperator);
        case seformula::NodeKind::CellReference:
            return u"root:cell_reference"_ustr;
        case seformula::NodeKind::RangeReference:
            return u"root:range_reference"_ustr;
        case seformula::NodeKind::NamedReference:
            return u"root:named_reference"_ustr;
        case seformula::NodeKind::RangeConstructor:
            return u"root:range_constructor"_ustr;
        case seformula::NodeKind::ReferenceList:
            return u"root:reference_list"_ustr;
        case seformula::NodeKind::ArrayConstant:
            return u"root:array_constant"_ustr;
        case seformula::NodeKind::NumberLiteral:
            return u"root:number_literal"_ustr;
        case seformula::NodeKind::StringLiteral:
            return u"root:string_literal"_ustr;
        case seformula::NodeKind::BooleanLiteral:
            return u"root:boolean_literal"_ustr;
        case seformula::NodeKind::ErrorLiteral:
            return u"root:error_literal"_ustr;
        case seformula::NodeKind::EmptyArgument:
            return u"root:empty_argument"_ustr;
    }

    return u"root:unknown"_ustr;
}

void recordUnknownRootInventory(ObserveSurfaceInventory& rInventory, const OUString& rRootLabel,
    bool bFallback, bool bUnsupportedFunction, bool bSeen)
{
    auto& rEntry = rInventory.maUnknownRootInventory[rRootLabel];
    ++rEntry.mnFormulaCells;
    if (bFallback)
        ++rEntry.mnFallbackFormulaCells;
    if (bUnsupportedFunction)
        ++rEntry.mnUnsupportedFunctionFormulaCells;
    if (!bSeen)
        ++rEntry.mnUnseenFormulaCells;
}

void accumulateUnknownRootInventory(
    ObserveSurfaceInventory& rTarget, const ObserveSurfaceInventory& rSource)
{
    for (const auto& rEntry : rSource.maUnknownRootInventory)
    {
        auto& rTargetEntry = rTarget.maUnknownRootInventory[rEntry.first];
        rTargetEntry.mnFormulaCells += rEntry.second.mnFormulaCells;
        rTargetEntry.mnFallbackFormulaCells += rEntry.second.mnFallbackFormulaCells;
        rTargetEntry.mnUnsupportedFunctionFormulaCells
            += rEntry.second.mnUnsupportedFunctionFormulaCells;
        rTargetEntry.mnUnseenFormulaCells += rEntry.second.mnUnseenFormulaCells;
    }
}

SupportedProbeRun runSupportedInterpretTailProbe(
    const Workbook& rWorkbook, ScDocument& rDoc, const OUString& rWorkbookLabel)
{
    SupportedProbeRun aRun;
    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    for (std::size_t nSheet = 0; nSheet < rWorkbook.maSheets.size(); ++nSheet)
    {
        for (const auto& rEntry : rWorkbook.maSheets[nSheet].maCells)
        {
            const Cell& rCell = rEntry.second;
            if (!rCell.hasFormula())
                continue;

            const ScAddress aPos(static_cast<SCCOL>(rEntry.first.first),
                static_cast<SCROW>(rEntry.first.second), static_cast<SCTAB>(nSheet));
            ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
            if (!pFormula)
                continue;

            const OUString aFormulaSource = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF,
                pContext);
            const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
            const FunctionKind eProbeFunction = classifySupportedProbeFunction(
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()));
            if (eProbeFunction == FunctionKind::Unknown)
                continue;

            ++aRun.mnRawFormulaCount;
            ++aRun.mnLiveAuthoritativeFormulaCount;

            const auto aAttempt
                = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
                    rDoc, *pContext, aPos,
                    std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
                    rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
                    std::u16string_view(aCanonicalFormulaSource.getStr(),
                        aCanonicalFormulaSource.getLength()));

            if (!aAttempt.mbSupported)
            {
                pFormula->SetDirty();
                pFormula->Interpret();
                const ProbeValue aWorkbookValue = probeValueFromWorkbookCell(rCell);
                const ProbeValue aLiveHostValue = probeValueFromLiveHostCell(rDoc, aPos);
                const bool bHostTruthArtifact = !probeValuesMatch(aWorkbookValue, aLiveHostValue);
                if (bHostTruthArtifact)
                {
                    ++aRun.mnHostTruthArtifactFormulaCount;
                    ++aRun.maHostTruthArtifactFunctionCount[static_cast<std::size_t>(
                        aAttempt.meFunction != FunctionKind::Unknown ? aAttempt.meFunction : eProbeFunction)];
                }
                else
                {
                    ++aRun.mnLiveTargetFormulaCount;
                    recordProbeAuthoritativeFallback(
                        aRun.maLiveTargetStats, aAttempt.meFallbackReason,
                        aAttempt.meFunction != FunctionKind::Unknown ? aAttempt.meFunction : eProbeFunction);
                }

                recordProbeAuthoritativeFallback(
                    aRun.maLiveAuthoritativeStats, aAttempt.meFallbackReason,
                    aAttempt.meFunction != FunctionKind::Unknown ? aAttempt.meFunction
                                                                 : eProbeFunction);

                maybeAddProbeDiagnosticSample(
                    rWorkbookLabel, rDoc, aPos, aFormulaSource,
                    aAttempt.meFunction != FunctionKind::Unknown ? aAttempt.meFunction : eProbeFunction,
                    u"fallback"_ustr, OUString(), probeValueToDiagnosticString(aLiveHostValue),
                    OUString::fromUtf8(fallbackReasonName(aAttempt.meFallbackReason)));
                recordProbeAuthoritativeFallback(
                    aRun.maRawStats, aAttempt.meFallbackReason,
                    aAttempt.meFunction != FunctionKind::Unknown ? aAttempt.meFunction : eProbeFunction);
                continue;
            }

            const ProbeValue aWorkbookValue = probeValueFromWorkbookCell(rCell);
            const ProbeValue aWorkbookComparableValue
                = probeWorkbookComparableValueFromWorkbookCell(rCell);
            const ProbeValue aEngineValue = probeValueFromEngineAttempt(aAttempt);
            pFormula->SetDirty();
            pFormula->Interpret();
            const ProbeValue aLiveHostValue = probeValueFromLiveHostCell(rDoc, aPos);

            const bool bMatchesWorkbook = probeValuesMatch(aEngineValue, aWorkbookComparableValue);
            const bool bMatchesLiveHost = probeValuesMatch(aEngineValue, aLiveHostValue);
            const bool bHostTruthArtifact = !probeValuesMatch(aWorkbookValue, aLiveHostValue);

            if (bHostTruthArtifact)
            {
                ++aRun.mnHostTruthArtifactFormulaCount;
                ++aRun.maHostTruthArtifactFunctionCount[static_cast<std::size_t>(aAttempt.meFunction)];
            }
            else
            {
                ++aRun.mnLiveTargetFormulaCount;
                if (bMatchesWorkbook)
                    recordProbeAuthoritativeRoute(aRun.maLiveTargetStats, aAttempt.meFunction);
                else
                    recordProbeAuthoritativeFallback(
                        aRun.maLiveTargetStats, FallbackReason::ShadowMismatch, aAttempt.meFunction);
            }

            if (bMatchesLiveHost)
                recordProbeAuthoritativeRoute(aRun.maLiveAuthoritativeStats, aAttempt.meFunction);
            else
                recordProbeAuthoritativeFallback(
                    aRun.maLiveAuthoritativeStats, FallbackReason::ShadowMismatch,
                    aAttempt.meFunction);

            if (probeDiagnosticsTrackLiveAuthoritativeSurface())
            {
                if (bMatchesLiveHost)
                {
                    maybeAddProbeDiagnosticSample(
                        rWorkbookLabel, rDoc, aPos, aFormulaSource, aAttempt.meFunction,
                        u"authoritative"_ustr, probeValueToDiagnosticString(aWorkbookValue),
                        probeValueToDiagnosticString(aLiveHostValue),
                        probeValueToDiagnosticString(aEngineValue));
                }
                else
                {
                    maybeAddProbeDiagnosticSample(
                        rWorkbookLabel, rDoc, aPos, aFormulaSource, aAttempt.meFunction,
                        u"live_shadow_mismatch"_ustr,
                        probeValueToDiagnosticString(aWorkbookValue),
                        probeValueToDiagnosticString(aLiveHostValue),
                        probeValueToDiagnosticString(aEngineValue));
                }
            }
            else if (bMatchesWorkbook)
            {
                maybeAddProbeDiagnosticSample(
                    rWorkbookLabel, rDoc, aPos, aFormulaSource, aAttempt.meFunction, u"authoritative"_ustr,
                    probeValueToDiagnosticString(aWorkbookValue),
                    probeValueToDiagnosticString(aLiveHostValue),
                    probeValueToDiagnosticString(aEngineValue));
            }
            else
            {
                maybeAddProbeDiagnosticSample(
                    rWorkbookLabel, rDoc, aPos, aFormulaSource, aAttempt.meFunction,
                    u"shadow_mismatch"_ustr, probeValueToDiagnosticString(aWorkbookValue),
                    probeValueToDiagnosticString(aLiveHostValue),
                    probeValueToDiagnosticString(aEngineValue));
            }

            if (bMatchesWorkbook)
                recordProbeAuthoritativeRoute(aRun.maRawStats, aAttempt.meFunction);
            else
                recordProbeAuthoritativeFallback(
                    aRun.maRawStats, FallbackReason::ShadowMismatch, aAttempt.meFunction);
        }
    }

    return aRun;
}

ObserveSurfaceInventory runForcedInterpretObserveSurface(const Workbook& rWorkbook, ScDocument& rDoc)
{
    ObserveSurfaceInventory aInventory;
    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    for (std::size_t nSheet = 0; nSheet < rWorkbook.maSheets.size(); ++nSheet)
    {
        for (const auto& rEntry : rWorkbook.maSheets[nSheet].maCells)
        {
            const Cell& rCell = rEntry.second;
            if (!rCell.hasFormula())
                continue;

            const ScAddress aPos(static_cast<SCCOL>(rEntry.first.first),
                static_cast<SCROW>(rEntry.first.second), static_cast<SCTAB>(nSheet));
            ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
            if (!pFormula)
                continue;

            ++aInventory.mnFormulaCells;

            const OUString aFormulaSource = pFormula->GetFormula(
                formula::FormulaGrammar::GRAM_ODFF, pContext);
            const FunctionKind eDirectFunction = classifySupportedProbeFunction(
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()));
            ++aInventory.maFunctionFormulaCells[static_cast<std::size_t>(eDirectFunction)];
            const OUString aUnknownRootLabel = eDirectFunction == FunctionKind::Unknown
                ? classifyUnknownRootLabel(
                      std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()))
                : OUString();

            spreadsheetengine::compat::libreoffice::interprettaileval::resetStats();
            pFormula->SetDirty();
            (void)pFormula->Interpret();
            const StatsSnapshot aDirectStats
                = spreadsheetengine::compat::libreoffice::interprettaileval::getStatsSnapshot();
            accumulateStats(aInventory.maAttemptStats, aDirectStats);
            const bool bSeen = statsSeenCount(aDirectStats) > 0;
            const bool bSupported = aDirectStats.mnObserveCount
                                        + aDirectStats.mnShadowCompareCount
                                        + aDirectStats.mnAuthoritativeCount
                                    > 0;
            const bool bFallback = totalFallbackCount(aDirectStats) > 0;
            const bool bUnsupportedFunction
                = aDirectStats.maFallbackReasons[static_cast<std::size_t>(
                      FallbackReason::UnsupportedFunction)]
                  > 0;

            if (bSeen)
            {
                ++aInventory.mnSeenFormulaCells;
                ++aInventory.maFunctionSeenCells[static_cast<std::size_t>(eDirectFunction)];
            }
            else
            {
                ++aInventory.mnUnseenFormulaCells;
                ++aInventory.maFunctionUnseenCells[static_cast<std::size_t>(eDirectFunction)];
            }

            if (bSupported)
            {
                ++aInventory.mnSupportedFormulaCells;
                ++aInventory.maFunctionSupportedCells[static_cast<std::size_t>(eDirectFunction)];
            }

            if (bFallback)
            {
                ++aInventory.mnFallbackFormulaCells;
                ++aInventory.maFunctionFallbackCells[static_cast<std::size_t>(eDirectFunction)];
            }

            if (bUnsupportedFunction)
            {
                ++aInventory.mnUnsupportedFunctionFormulaCells;
                ++aInventory.maFunctionUnsupportedFunctionCells[static_cast<std::size_t>(
                    eDirectFunction)];
            }

            if (eDirectFunction == FunctionKind::Unknown)
                recordUnknownRootInventory(
                    aInventory, aUnknownRootLabel, bFallback, bUnsupportedFunction, bSeen);
        }
    }

    return aInventory;
}

ObserveSurfaceInventory buildObserveSurfaceInventory(
    const Workbook& rWorkbook, ScDocument& rDoc,
    const std::vector<ObservedFormulaCellStatus>& rObservedCells,
    const StatsSnapshot& rAttemptStats)
{
    ObserveSurfaceInventory aInventory;
    aInventory.maAttemptStats = rAttemptStats;

    std::map<ScAddress, ObservedFormulaCellStatus, ScAddressLess> aObservedByAddress;
    for (const auto& rStatus : rObservedCells)
        aObservedByAddress[rStatus.maAddress] = rStatus;

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    for (std::size_t nSheet = 0; nSheet < rWorkbook.maSheets.size(); ++nSheet)
    {
        for (const auto& rEntry : rWorkbook.maSheets[nSheet].maCells)
        {
            const Cell& rCell = rEntry.second;
            if (!rCell.hasFormula())
                continue;

            const ScAddress aPos(static_cast<SCCOL>(rEntry.first.first),
                static_cast<SCROW>(rEntry.first.second), static_cast<SCTAB>(nSheet));
            ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
            if (!pFormula)
                continue;

            ++aInventory.mnFormulaCells;

            const OUString aFormulaSource = pFormula->GetFormula(
                formula::FormulaGrammar::GRAM_ODFF, pContext);
            const FunctionKind eDirectFunction = classifySupportedProbeFunction(
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()));
            ++aInventory.maFunctionFormulaCells[static_cast<std::size_t>(eDirectFunction)];
            const OUString aUnknownRootLabel = eDirectFunction == FunctionKind::Unknown
                ? classifyUnknownRootLabel(
                      std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()))
                : OUString();

            const auto it = aObservedByAddress.find(aPos);
            const bool bSeen = it != aObservedByAddress.end() && it->second.mbSeen;
            const bool bSupported = it != aObservedByAddress.end() && it->second.mbSupported;
            const bool bFallback = it != aObservedByAddress.end() && it->second.mbFallback;
            const bool bUnsupportedFunction
                = it != aObservedByAddress.end() && it->second.mbUnsupportedFunctionFallback;

            if (bSeen)
            {
                ++aInventory.mnSeenFormulaCells;
                ++aInventory.maFunctionSeenCells[static_cast<std::size_t>(eDirectFunction)];
            }
            else
            {
                ++aInventory.mnUnseenFormulaCells;
                ++aInventory.maFunctionUnseenCells[static_cast<std::size_t>(eDirectFunction)];
            }

            if (bSupported)
            {
                ++aInventory.mnSupportedFormulaCells;
                ++aInventory.maFunctionSupportedCells[static_cast<std::size_t>(eDirectFunction)];
            }

            if (bFallback)
            {
                ++aInventory.mnFallbackFormulaCells;
                ++aInventory.maFunctionFallbackCells[static_cast<std::size_t>(eDirectFunction)];
            }

            if (bUnsupportedFunction)
            {
                ++aInventory.mnUnsupportedFunctionFormulaCells;
                ++aInventory.maFunctionUnsupportedFunctionCells[static_cast<std::size_t>(
                    eDirectFunction)];
            }

            if (eDirectFunction == FunctionKind::Unknown)
                recordUnknownRootInventory(
                    aInventory, aUnknownRootLabel, bFallback, bUnsupportedFunction, bSeen);
        }
    }

    return aInventory;
}

sal_uInt64 statsSeenCount(const StatsSnapshot& rStats)
{
    sal_uInt64 nFallbackCount = 0;
    for (std::size_t nIndex = 0; nIndex < static_cast<std::size_t>(FallbackReason::Count); ++nIndex)
        nFallbackCount += rStats.maFallbackReasons[nIndex];
    return rStats.mnObserveCount + rStats.mnShadowCompareCount + rStats.mnAuthoritativeCount
           + nFallbackCount;
}

std::u16string_view sharedStateLabel(const ScFormulaCell& rFormula)
{
    if (!rFormula.IsShared())
        return u"non_shared";
    if (rFormula.IsSharedTop())
        return u"shared_top";
    return u"shared_member";
}

ReplayEligibilityInventory runPromotedReplayEligibilityInventory(
    const Workbook& rWorkbook, ScDocument& rDoc, const OUString& rWorkbookLabel)
{
    ReplayEligibilityInventory aInventory;
    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    for (std::size_t nSheet = 0; nSheet < rWorkbook.maSheets.size(); ++nSheet)
    {
        for (const auto& rEntry : rWorkbook.maSheets[nSheet].maCells)
        {
            const Cell& rCell = rEntry.second;
            if (!rCell.hasFormula())
                continue;

            const ScAddress aPos(static_cast<SCCOL>(rEntry.first.first),
                static_cast<SCROW>(rEntry.first.second), static_cast<SCTAB>(nSheet));
            ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
            if (!pFormula)
                continue;

            const OUString aFormulaSource = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF,
                pContext);
            const FunctionKind eProbeFunction = classifySupportedProbeFunction(
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()));
            if (eProbeFunction == FunctionKind::Unknown)
                continue;

            ++aInventory.mnPromotedFormulaCells;
            ++aInventory.maFunctionFormulaCells[static_cast<std::size_t>(eProbeFunction)];

            if (pFormula->IsShared())
            {
                ++aInventory.mnSharedFormulaCells;
                if (pFormula->IsSharedTop())
                    ++aInventory.mnSharedTopFormulaCells;
                else
                    ++aInventory.mnSharedMemberFormulaCells;
            }
            else
            {
                ++aInventory.mnNonSharedFormulaCells;
            }

            if (pFormula->NeedsInterpret())
                ++aInventory.mnNeedsInterpretBeforeDirty;
            if (pFormula->GetMatrixFlag() != ScMatrixMode::NONE)
                ++aInventory.mnMatrixFormulaCells;
            if (pFormula->IsHyperLinkCell())
                ++aInventory.mnHyperLinkFormulaCells;
            if (!pFormula->GetCode())
                ++aInventory.mnMissingCodeFormulaCells;
            if (pFormula->GetRawError() != FormulaError::NONE)
                ++aInventory.mnRawErrorFormulaCells;

            const bool bNeedsInterpretBeforeDirty = pFormula->NeedsInterpret();
            spreadsheetengine::compat::libreoffice::interprettaileval::resetStats();
            pFormula->SetDirty();
            const bool bNeedsInterpretAfterDirty = pFormula->NeedsInterpret();
            if (bNeedsInterpretAfterDirty)
                ++aInventory.mnNeedsInterpretAfterDirty;

            const bool bInterpretReturned = pFormula->Interpret();
            if (!bInterpretReturned)
                ++aInventory.mnInterpretReturnedFalse;

            const StatsSnapshot aDirectStats
                = spreadsheetengine::compat::libreoffice::interprettaileval::getStatsSnapshot();
            const bool bDirectSeen = statsSeenCount(aDirectStats) > 0;
            const bool bDirectSupported = aDirectStats.mnObserveCount
                                              + aDirectStats.mnShadowCompareCount
                                              + aDirectStats.mnAuthoritativeCount
                                          > 0;
            const bool bDirectFallback = totalFallbackCount(aDirectStats) > 0;
            const bool bDirtyAfterInterpret = pFormula->GetDirty();
            const bool bNeedsInterpretAfterInterpret = pFormula->NeedsInterpret();
            if (bDirtyAfterInterpret)
                ++aInventory.mnDirtyAfterInterpret;
            if (bNeedsInterpretAfterInterpret)
                ++aInventory.mnNeedsInterpretAfterInterpret;

            bool bSeenViaSharedTop = false;
            if (bDirectSeen)
            {
                ++aInventory.mnDirectSeen;
                ++aInventory.maFunctionDirectSeen[static_cast<std::size_t>(eProbeFunction)];
            }
            else
            {
                ++aInventory.mnDirectUnseen;
                ++aInventory.maFunctionDirectUnseen[static_cast<std::size_t>(eProbeFunction)];

                if (pFormula->IsShared())
                {
                    if (pFormula->IsSharedTop())
                    {
                        ++aInventory.mnUnseenSharedTop;
                    }
                    else
                    {
                        ++aInventory.mnUnseenSharedMember;

                        ScAddress aTopPos = aPos;
                        aTopPos.SetRow(pFormula->GetSharedTopRow());
                        if (ScFormulaCell* pTopFormula = rDoc.GetFormulaCell(aTopPos))
                        {
                            spreadsheetengine::compat::libreoffice::interprettaileval::resetStats();
                            pTopFormula->SetDirty();
                            (void)pTopFormula->Interpret();
                            const StatsSnapshot aTopStats
                                = spreadsheetengine::compat::libreoffice::interprettaileval::getStatsSnapshot();
                            const bool bTopSupported = aTopStats.mnObserveCount
                                                           + aTopStats.mnShadowCompareCount
                                                           + aTopStats.mnAuthoritativeCount
                                                       > 0;
                            const bool bTopFallback = totalFallbackCount(aTopStats) > 0;
                            bSeenViaSharedTop = bTopSupported || bTopFallback;
                            if (bTopSupported)
                                ++aInventory.mnSharedMemberSeenViaTop;
                            else if (bTopFallback)
                                ++aInventory.mnSharedMemberFallbackViaTop;
                            else
                                ++aInventory.mnSharedMemberStillUnseenViaTop;
                        }
                    }
                }
                else
                {
                    ++aInventory.mnUnseenNonShared;
                }
            }

            if (bDirectSupported)
                ++aInventory.mnDirectSupported;
            if (bDirectFallback)
                ++aInventory.mnDirectFallback;

            if (!bDirectSeen)
            {
                maybeAddReplayEligibilityDiagnosticSample(rWorkbookLabel, rDoc, aPos,
                    eProbeFunction, sharedStateLabel(*pFormula), u"direct_unseen",
                    bNeedsInterpretBeforeDirty, bNeedsInterpretAfterDirty, bDirtyAfterInterpret,
                    bSeenViaSharedTop);
            }
        }
    }

    return aInventory;
}

const char* functionKindName(FunctionKind eFunction)
{
    switch (eFunction)
    {
        case FunctionKind::Unknown:
            return "unknown";
        case FunctionKind::ScalarRoot:
            return "scalar_root";
        case FunctionKind::Conditional:
            return "conditional";
        case FunctionKind::FormulaText:
            return "formula_text";
        case FunctionKind::LogicalConstant:
            return "logical_constant";
        case FunctionKind::TextUtility:
            return "text_utility";
        case FunctionKind::Value:
            return "value";
        case FunctionKind::DateValue:
            return "datevalue";
        case FunctionKind::TimeValue:
            return "timevalue";
        case FunctionKind::NumberValue:
            return "numbervalue";
        case FunctionKind::Rate:
            return "rate";
        case FunctionKind::Round:
            return "round";
        case FunctionKind::Conversion:
            return "conversion";
        case FunctionKind::NumericAggregate:
            return "numeric_aggregate";
        case FunctionKind::RankedAggregate:
            return "ranked_aggregate";
        case FunctionKind::StatisticalAggregate:
            return "statistical_aggregate";
        case FunctionKind::StatisticalDistribution:
            return "statistical_distribution";
        case FunctionKind::GrowthProjection:
            return "growth_projection";
        case FunctionKind::CriteriaAggregate:
            return "criteria_aggregate";
        case FunctionKind::Aggregate:
            return "aggregate";
        case FunctionKind::BusinessDay:
            return "business_day";
        case FunctionKind::CalendarUtility:
            return "calendar_utility";
        case FunctionKind::DateDifference:
            return "date_difference";
        case FunctionKind::DateConstructExtract:
            return "date_construct_extract";
        case FunctionKind::MatrixMath:
            return "matrix_math";
        case FunctionKind::MathScalar:
            return "math_scalar";
        case FunctionKind::InformationPredicate:
            return "information_predicate";
        case FunctionKind::LogicalFold:
            return "logical_fold";
        case FunctionKind::Not:
            return "not";
        case FunctionKind::Match:
            return "match";
        case FunctionKind::XMatch:
            return "xmatch";
        case FunctionKind::Selector:
            return "selector";
        case FunctionKind::SpillArray:
            return "spill_array";
        case FunctionKind::Lookup:
            return "lookup";
        case FunctionKind::VLookup:
            return "vlookup";
        case FunctionKind::HLookup:
            return "hlookup";
        case FunctionKind::XLookup:
            return "xlookup";
        case FunctionKind::Index:
            return "index";
        case FunctionKind::Count:
            break;
    }

    return "count";
}

void printReplayEligibilityInventory(const ReplayEligibilityInventory& rInventory)
{
    std::cout << "interpret_tail_replay_promoted_formula_cells="
              << rInventory.mnPromotedFormulaCells << '\n';
    std::cout << "interpret_tail_replay_promoted_shared_formula_cells="
              << rInventory.mnSharedFormulaCells << '\n';
    std::cout << "interpret_tail_replay_promoted_shared_top_formula_cells="
              << rInventory.mnSharedTopFormulaCells << '\n';
    std::cout << "interpret_tail_replay_promoted_shared_member_formula_cells="
              << rInventory.mnSharedMemberFormulaCells << '\n';
    std::cout << "interpret_tail_replay_promoted_non_shared_formula_cells="
              << rInventory.mnNonSharedFormulaCells << '\n';
    std::cout << "interpret_tail_replay_promoted_needs_interpret_before_dirty="
              << rInventory.mnNeedsInterpretBeforeDirty << '\n';
    std::cout << "interpret_tail_replay_promoted_needs_interpret_after_dirty="
              << rInventory.mnNeedsInterpretAfterDirty << '\n';
    std::cout << "interpret_tail_replay_promoted_direct_seen="
              << rInventory.mnDirectSeen << '\n';
    std::cout << "interpret_tail_replay_promoted_direct_supported="
              << rInventory.mnDirectSupported << '\n';
    std::cout << "interpret_tail_replay_promoted_direct_fallback="
              << rInventory.mnDirectFallback << '\n';
    std::cout << "interpret_tail_replay_promoted_direct_unseen="
              << rInventory.mnDirectUnseen << '\n';
    std::cout << "interpret_tail_replay_promoted_interpret_returned_false="
              << rInventory.mnInterpretReturnedFalse << '\n';
    std::cout << "interpret_tail_replay_promoted_dirty_after_interpret="
              << rInventory.mnDirtyAfterInterpret << '\n';
    std::cout << "interpret_tail_replay_promoted_needs_interpret_after_interpret="
              << rInventory.mnNeedsInterpretAfterInterpret << '\n';
    std::cout << "interpret_tail_replay_promoted_unseen_shared_top="
              << rInventory.mnUnseenSharedTop << '\n';
    std::cout << "interpret_tail_replay_promoted_unseen_shared_member="
              << rInventory.mnUnseenSharedMember << '\n';
    std::cout << "interpret_tail_replay_promoted_unseen_non_shared="
              << rInventory.mnUnseenNonShared << '\n';
    std::cout << "interpret_tail_replay_promoted_shared_member_seen_via_top="
              << rInventory.mnSharedMemberSeenViaTop << '\n';
    std::cout << "interpret_tail_replay_promoted_shared_member_fallback_via_top="
              << rInventory.mnSharedMemberFallbackViaTop << '\n';
    std::cout << "interpret_tail_replay_promoted_shared_member_still_unseen_via_top="
              << rInventory.mnSharedMemberStillUnseenViaTop << '\n';
    std::cout << "interpret_tail_replay_promoted_matrix_formula_cells="
              << rInventory.mnMatrixFormulaCells << '\n';
    std::cout << "interpret_tail_replay_promoted_hyperlink_formula_cells="
              << rInventory.mnHyperLinkFormulaCells << '\n';
    std::cout << "interpret_tail_replay_promoted_missing_code_formula_cells="
              << rInventory.mnMissingCodeFormulaCells << '\n';
    std::cout << "interpret_tail_replay_promoted_raw_error_formula_cells="
              << rInventory.mnRawErrorFormulaCells << '\n';

    for (std::size_t nIndex = 1; nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
    {
        const auto eFunction = static_cast<FunctionKind>(nIndex);
        const char* pName = functionKindName(eFunction);
        std::cout << "interpret_tail_replay_promoted_function_" << pName
                  << "_formula_cells=" << rInventory.maFunctionFormulaCells[nIndex] << '\n';
        std::cout << "interpret_tail_replay_promoted_function_" << pName
                  << "_direct_seen=" << rInventory.maFunctionDirectSeen[nIndex] << '\n';
        std::cout << "interpret_tail_replay_promoted_function_" << pName
                  << "_direct_unseen=" << rInventory.maFunctionDirectUnseen[nIndex] << '\n';
    }
}

void printReplayEligibilityDiagnosticSamples()
{
    if (!envEnabled("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_ELIGIBILITY_DIAGNOSTICS"))
        return;

    const auto& rSamples = replayEligibilityDiagnosticSamples();
    std::cout << "interpret_tail_replay_eligibility_diagnostic_sample_count=" << rSamples.size()
              << '\n';
    for (std::size_t nIndex = 0; nIndex < rSamples.size(); ++nIndex)
    {
        const auto& rSample = rSamples[nIndex];
        std::cout << "interpret_tail_replay_eligibility_diagnostic_" << nIndex << "_workbook="
                  << rSample.maWorkbookLabel.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_replay_eligibility_diagnostic_" << nIndex << "_cell="
                  << rSample.maCellAddress.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_replay_eligibility_diagnostic_" << nIndex << "_function="
                  << rSample.maFunctionName.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_replay_eligibility_diagnostic_" << nIndex << "_shared_state="
                  << rSample.maSharedState.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_replay_eligibility_diagnostic_" << nIndex << "_outcome="
                  << rSample.maOutcome.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_replay_eligibility_diagnostic_" << nIndex
                  << "_needs_interpret_before_dirty="
                  << (rSample.mbNeedsInterpretBeforeDirty ? 1 : 0) << '\n';
        std::cout << "interpret_tail_replay_eligibility_diagnostic_" << nIndex
                  << "_needs_interpret_after_dirty="
                  << (rSample.mbNeedsInterpretAfterDirty ? 1 : 0) << '\n';
        std::cout << "interpret_tail_replay_eligibility_diagnostic_" << nIndex
                  << "_dirty_after_interpret=" << (rSample.mbDirtyAfterInterpret ? 1 : 0)
                  << '\n';
        std::cout << "interpret_tail_replay_eligibility_diagnostic_" << nIndex
                  << "_seen_via_shared_top=" << (rSample.mbSeenViaSharedTop ? 1 : 0) << '\n';
    }
}

void recordProbeAuthoritativeRoute(StatsSnapshot& rStats, FunctionKind eFunction)
{
    ++rStats.mnAuthoritativeCount;
    ++rStats.maFunctionAuthoritativeCount[static_cast<std::size_t>(eFunction)];
}

void recordProbeAuthoritativeFallback(
    StatsSnapshot& rStats, FallbackReason eReason, FunctionKind eFunction)
{
    ++rStats.mnAuthoritativeFallbackCount;
    ++rStats.maFallbackReasons[static_cast<std::size_t>(eReason)];
    ++rStats.maFunctionFallbackCount[static_cast<std::size_t>(eFunction)];
    ++rStats.maFunctionFallbackReasons[static_cast<std::size_t>(eFunction)]
                                       [static_cast<std::size_t>(eReason)];
}

ProbeValue probeValueFromWorkbookCell(const Cell& rCell)
{
    ProbeValue aValue;
    const auto& rExpected = rCell.maValue;
    if (rExpected.isError())
    {
        aValue.meKind = ProbeValue::Kind::Error;
        aValue.meError = toFormulaError(rExpected.meError);
    }
    else if (rExpected.isNumber() || rExpected.isBoolean())
    {
        aValue.meKind = ProbeValue::Kind::Number;
        aValue.mfValue = rExpected.mfNumber;
    }
    else if (rExpected.isText())
    {
        aValue.meKind = ProbeValue::Kind::String;
        aValue.maString = toLibreOfficeString(rExpected.maString);
    }
    return aValue;
}

ProbeValue probeWorkbookComparableValueFromWorkbookCell(const Cell& rCell)
{
    ProbeValue aValue = probeValueFromWorkbookCell(rCell);
    if (aValue.meKind != ProbeValue::Kind::String)
        return aValue;

    const auto aLexical = !rCell.maRawValue.empty() ? rCell.maRawValue : rCell.maValue.maString;
    if (rCell.maRawValueType == u"date")
    {
        if (const auto oStoredDate = spreadsheetengine::core::datetime::parseStoredDateValue(
                aLexical))
        {
            aValue.meKind = ProbeValue::Kind::Number;
            aValue.mfValue = *oStoredDate;
            aValue.maString.clear();
            return aValue;
        }

        if (const auto oParsed = spreadsheetengine::core::datetime::parseStandaloneNumberText(
                aLexical))
        {
            if (oParsed->meKind == spreadsheetengine::api::NumberParseResult::Kind::Date
                || oParsed->meKind == spreadsheetengine::api::NumberParseResult::Kind::DateTime)
            {
                aValue.meKind = ProbeValue::Kind::Number;
                aValue.mfValue = oParsed->mfValue;
                aValue.maString.clear();
                return aValue;
            }
        }
    }

    if (rCell.maRawValueType == u"time")
    {
        if (const auto oDuration = spreadsheetengine::core::datetime::parseOdfTimeDuration(
                aLexical))
        {
            aValue.meKind = ProbeValue::Kind::Number;
            aValue.mfValue = spreadsheetengine::core::datetime::normalizeTimeFraction(*oDuration);
            aValue.maString.clear();
            return aValue;
        }

        if (const auto oParsed = spreadsheetengine::core::datetime::parseStandaloneNumberText(
                aLexical))
        {
            if (oParsed->meKind == spreadsheetengine::api::NumberParseResult::Kind::Time
                || oParsed->meKind == spreadsheetengine::api::NumberParseResult::Kind::DateTime)
            {
                aValue.meKind = ProbeValue::Kind::Number;
                aValue.mfValue
                    = spreadsheetengine::core::datetime::normalizeTimeFraction(oParsed->mfValue);
                aValue.maString.clear();
                return aValue;
            }
        }
    }

    return aValue;
}

ProbeValue probeValueFromLiveHostCell(ScDocument& rDoc, const ScAddress& rPos)
{
    ProbeValue aValue;
    if (const FormulaError eError = rDoc.GetErrCode(rPos); eError != FormulaError::NONE)
    {
        aValue.meKind = ProbeValue::Kind::Error;
        aValue.meError = eError;
        return aValue;
    }

    const ScRefCellValue aCell(rDoc, rPos);
    if (aCell.hasNumeric())
    {
        aValue.meKind = ProbeValue::Kind::Number;
        aValue.mfValue = aCell.getRawValue();
        return aValue;
    }

    if (aCell.hasString())
    {
        aValue.meKind = ProbeValue::Kind::String;
        aValue.maString = aCell.getString(rDoc);
    }

    return aValue;
}

ProbeValue probeValueFromEngineAttempt(
    const spreadsheetengine::compat::libreoffice::interprettaileval::EvaluationAttempt& rAttempt)
{
    ProbeValue aValue;
    switch (rAttempt.maResult.meType)
    {
        case spreadsheetengine::api::formulavalue::ValueType::Error:
            aValue.meKind = ProbeValue::Kind::Error;
            aValue.meError
                = spreadsheetengine::compat::libreoffice::toFormulaError(rAttempt.maResult.meError);
            break;
        case spreadsheetengine::api::formulavalue::ValueType::Value:
            aValue.meKind = ProbeValue::Kind::Number;
            aValue.mfValue = rAttempt.maResult.mfValue;
            break;
        case spreadsheetengine::api::formulavalue::ValueType::String:
            aValue.meKind = ProbeValue::Kind::String;
            aValue.maString = toLibreOfficeString(rAttempt.maResult.maString);
            break;
        default:
            break;
    }
    return aValue;
}

bool probeValuesMatch(const ProbeValue& rLeft, const ProbeValue& rRight)
{
    if (rLeft.meKind != rRight.meKind)
        return false;

    switch (rLeft.meKind)
    {
        case ProbeValue::Kind::Empty:
            return true;
        case ProbeValue::Kind::Error:
            return rLeft.meError == rRight.meError;
        case ProbeValue::Kind::Number:
            return rtl::math::approxEqual(rLeft.mfValue, rRight.mfValue);
        case ProbeValue::Kind::String:
            return rLeft.maString == rRight.maString;
    }

    return false;
}

OUString probeValueToDiagnosticString(const ProbeValue& rValue)
{
    switch (rValue.meKind)
    {
        case ProbeValue::Kind::Empty:
            return OUString();
        case ProbeValue::Kind::Error:
            return u"error:"_ustr + OUString::number(static_cast<int>(rValue.meError));
        case ProbeValue::Kind::Number:
            return OUString::number(rValue.mfValue);
        case ProbeValue::Kind::String:
            return rValue.maString;
    }

    return OUString();
}

const char* fallbackReasonName(FallbackReason eReason)
{
    switch (eReason)
    {
        case FallbackReason::UnsupportedTailContext:
            return "unsupported_tail_context";
        case FallbackReason::UnsupportedFormulaShape:
            return "unsupported_formula_shape";
        case FallbackReason::UnsupportedHostSurface:
            return "unsupported_host_surface";
        case FallbackReason::UnsupportedFunction:
            return "unsupported_function";
        case FallbackReason::ParseFailure:
            return "parse_failure";
        case FallbackReason::ProjectionFailure:
            return "projection_failure";
        case FallbackReason::ShadowMismatch:
            return "shadow_mismatch";
        case FallbackReason::Count:
            break;
    }

    return "count";
}

void accumulateStats(StatsSnapshot& rTarget, const StatsSnapshot& rSource)
{
    rTarget.mnObserveCount += rSource.mnObserveCount;
    rTarget.mnShadowCompareCount += rSource.mnShadowCompareCount;
    rTarget.mnAuthoritativeCount += rSource.mnAuthoritativeCount;
    rTarget.mnAuthoritativeFallbackCount += rSource.mnAuthoritativeFallbackCount;
    rTarget.mnShadowMatchCount += rSource.mnShadowMatchCount;

    for (std::size_t nIndex = 0; nIndex < static_cast<std::size_t>(FallbackReason::Count);
         ++nIndex)
    {
        rTarget.maFallbackReasons[nIndex] += rSource.maFallbackReasons[nIndex];
    }

    for (std::size_t nIndex = 0; nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
    {
        rTarget.maFunctionObserveCount[nIndex] += rSource.maFunctionObserveCount[nIndex];
        rTarget.maFunctionShadowCompareCount[nIndex]
            += rSource.maFunctionShadowCompareCount[nIndex];
        rTarget.maFunctionAuthoritativeCount[nIndex]
            += rSource.maFunctionAuthoritativeCount[nIndex];
        rTarget.maFunctionFallbackCount[nIndex] += rSource.maFunctionFallbackCount[nIndex];

        for (std::size_t nReason = 0; nReason < static_cast<std::size_t>(FallbackReason::Count);
             ++nReason)
        {
            rTarget.maFunctionFallbackReasons[nIndex][nReason]
                += rSource.maFunctionFallbackReasons[nIndex][nReason];
        }
    }
}

sal_uInt64 totalFallbackCount(const StatsSnapshot& rStats)
{
    sal_uInt64 nTotal = 0;
    for (std::size_t nIndex = 0; nIndex < static_cast<std::size_t>(FallbackReason::Count); ++nIndex)
        nTotal += rStats.maFallbackReasons[nIndex];
    return nTotal;
}

sal_uInt64 promotedFunctionSupportedCount(const StatsSnapshot& rStats)
{
    sal_uInt64 nPromotedFunctionSupported = 0;
    for (std::size_t nIndex = 1;
         nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
    {
        nPromotedFunctionSupported += rStats.maFunctionObserveCount[nIndex]
                                     + rStats.maFunctionShadowCompareCount[nIndex]
                                     + rStats.maFunctionAuthoritativeCount[nIndex];
    }
    return nPromotedFunctionSupported;
}

void printRoutingStats(
    std::string_view aPrefix, std::size_t nFormulaCellCount, const StatsSnapshot& rStats)
{
    const sal_uInt64 nSupportedTotal
        = rStats.mnObserveCount + rStats.mnShadowCompareCount + rStats.mnAuthoritativeCount;
    const sal_uInt64 nFallbackTotal = totalFallbackCount(rStats);
    const sal_uInt64 nSeenTotal = nSupportedTotal + nFallbackTotal;
    const sal_uInt64 nUnseenTotal
        = nFormulaCellCount > nSeenTotal ? nFormulaCellCount - nSeenTotal : 0;
    const double fSupportedRate = nFormulaCellCount
                                      ? (static_cast<double>(nSupportedTotal) * 100.0
                                         / static_cast<double>(nFormulaCellCount))
                                      : 0.0;
    const double fSeenRate = nFormulaCellCount
                                 ? (static_cast<double>(nSeenTotal) * 100.0
                                    / static_cast<double>(nFormulaCellCount))
                                 : 0.0;

    std::cout << aPrefix << "_formula_cells=" << nFormulaCellCount << '\n';
    std::cout << aPrefix << "_supported_total=" << nSupportedTotal << '\n';
    std::cout << aPrefix << "_fallback_total=" << nFallbackTotal << '\n';
    std::cout << aPrefix << "_seen_total=" << nSeenTotal << '\n';
    std::cout << aPrefix << "_unseen_formula_cells=" << nUnseenTotal << '\n';

    const sal_uInt64 nPromotedFunctionSupported = promotedFunctionSupportedCount(rStats);
    std::cout << aPrefix << "_promoted_function_supported_total="
              << nPromotedFunctionSupported << '\n';

    const auto aOldFlags = std::cout.flags();
    const auto nOldPrecision = std::cout.precision();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << aPrefix << "_supported_rate=" << fSupportedRate << '\n';
    std::cout << aPrefix << "_seen_rate=" << fSeenRate << '\n';
    std::cout.flags(aOldFlags);
    std::cout.precision(nOldPrecision);

    for (std::size_t nIndex = 1; nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
    {
        const auto eFunction = static_cast<FunctionKind>(nIndex);
        const sal_uInt64 nSupported = rStats.maFunctionObserveCount[nIndex]
                                      + rStats.maFunctionShadowCompareCount[nIndex]
                                      + rStats.maFunctionAuthoritativeCount[nIndex];
        std::cout << aPrefix << "_function_" << functionKindName(eFunction)
                  << "_supported=" << nSupported << '\n';
        std::cout << aPrefix << "_function_" << functionKindName(eFunction)
                  << "_fallback=" << rStats.maFunctionFallbackCount[nIndex] << '\n';
    }

    for (std::size_t nIndex = 0; nIndex < static_cast<std::size_t>(FallbackReason::Count); ++nIndex)
    {
        const auto eReason = static_cast<FallbackReason>(nIndex);
        std::cout << aPrefix << "_fallback_reason_" << fallbackReasonName(eReason)
                  << "=" << rStats.maFallbackReasons[nIndex] << '\n';
    }
}

void printObserveSurfaceInventory(
    std::string_view aPrefix, const ObserveSurfaceInventory& rInventory)
{
    const double fSeenRate = rInventory.mnFormulaCells
                                 ? (static_cast<double>(rInventory.mnSeenFormulaCells) * 100.0
                                    / static_cast<double>(rInventory.mnFormulaCells))
                                 : 0.0;
    const double fSupportedRate
        = rInventory.mnFormulaCells
              ? (static_cast<double>(rInventory.mnSupportedFormulaCells) * 100.0
                 / static_cast<double>(rInventory.mnFormulaCells))
              : 0.0;

    std::cout << aPrefix << "_formula_cells=" << rInventory.mnFormulaCells << '\n';
    std::cout << aPrefix << "_seen_formula_cells=" << rInventory.mnSeenFormulaCells << '\n';
    std::cout << aPrefix << "_supported_formula_cells="
              << rInventory.mnSupportedFormulaCells << '\n';
    std::cout << aPrefix << "_fallback_formula_cells="
              << rInventory.mnFallbackFormulaCells << '\n';
    std::cout << aPrefix << "_unsupported_function_formula_cells="
              << rInventory.mnUnsupportedFunctionFormulaCells << '\n';
    std::cout << aPrefix << "_unseen_formula_cells=" << rInventory.mnUnseenFormulaCells
              << '\n';

    const auto aOldFlags = std::cout.flags();
    const auto nOldPrecision = std::cout.precision();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << aPrefix << "_seen_rate=" << fSeenRate << '\n';
    std::cout << aPrefix << "_supported_rate=" << fSupportedRate << '\n';
    std::cout.flags(aOldFlags);
    std::cout.precision(nOldPrecision);

    for (std::size_t nIndex = 1; nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
    {
        const auto eFunction = static_cast<FunctionKind>(nIndex);
        const char* pName = functionKindName(eFunction);
        std::cout << aPrefix << "_function_" << pName
                  << "_formula_cells=" << rInventory.maFunctionFormulaCells[nIndex] << '\n';
        std::cout << aPrefix << "_function_" << pName
                  << "_seen_formula_cells=" << rInventory.maFunctionSeenCells[nIndex] << '\n';
        std::cout << aPrefix << "_function_" << pName
                  << "_supported_formula_cells="
                  << rInventory.maFunctionSupportedCells[nIndex] << '\n';
        std::cout << aPrefix << "_function_" << pName
                  << "_fallback_formula_cells="
                  << rInventory.maFunctionFallbackCells[nIndex] << '\n';
        std::cout << aPrefix << "_function_" << pName
                  << "_unsupported_function_formula_cells="
                  << rInventory.maFunctionUnsupportedFunctionCells[nIndex] << '\n';
        std::cout << aPrefix << "_function_" << pName
                  << "_unseen_formula_cells=" << rInventory.maFunctionUnseenCells[nIndex]
                  << '\n';
    }
}

void printTopUnsupportedFunctionInventory(
    std::string_view aPrefix, const ObserveSurfaceInventory& rInventory, std::size_t nLimit = 10)
{
    struct RankedUnsupportedFunction
    {
        FunctionKind meFunction = FunctionKind::Unknown;
        std::size_t mnUnsupportedFunctionFormulaCells = 0;
        std::size_t mnFormulaCells = 0;
        std::size_t mnUnseenFormulaCells = 0;
        std::size_t mnFallbackFormulaCells = 0;
    };

    std::vector<RankedUnsupportedFunction> aEntries;
    for (std::size_t nIndex = 0; nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
    {
        const std::size_t nUnsupportedFunctionFormulaCells
            = rInventory.maFunctionUnsupportedFunctionCells[nIndex];
        if (!nUnsupportedFunctionFormulaCells)
            continue;

        aEntries.push_back({ static_cast<FunctionKind>(nIndex), nUnsupportedFunctionFormulaCells,
            rInventory.maFunctionFormulaCells[nIndex], rInventory.maFunctionUnseenCells[nIndex],
            rInventory.maFunctionFallbackCells[nIndex] });
    }

    std::sort(aEntries.begin(), aEntries.end(),
        [](const RankedUnsupportedFunction& rLeft, const RankedUnsupportedFunction& rRight) {
            if (rLeft.mnUnsupportedFunctionFormulaCells != rRight.mnUnsupportedFunctionFormulaCells)
            {
                return rLeft.mnUnsupportedFunctionFormulaCells > rRight.mnUnsupportedFunctionFormulaCells;
            }
            if (rLeft.mnUnseenFormulaCells != rRight.mnUnseenFormulaCells)
                return rLeft.mnUnseenFormulaCells > rRight.mnUnseenFormulaCells;
            if (rLeft.mnFallbackFormulaCells != rRight.mnFallbackFormulaCells)
                return rLeft.mnFallbackFormulaCells > rRight.mnFallbackFormulaCells;
            return std::strcmp(functionKindName(rLeft.meFunction), functionKindName(rRight.meFunction))
                   < 0;
        });

    if (aEntries.size() > nLimit)
        aEntries.resize(nLimit);

    std::cout << aPrefix << "_top_unsupported_function_count=" << aEntries.size() << '\n';
    for (std::size_t nIndex = 0; nIndex < aEntries.size(); ++nIndex)
    {
        const auto& rEntry = aEntries[nIndex];
        const char* pName = functionKindName(rEntry.meFunction);
        std::cout << aPrefix << "_top_unsupported_function_" << nIndex
                  << "_name=" << pName << '\n';
        std::cout << aPrefix << "_top_unsupported_function_" << nIndex
                  << "_unsupported_function_formula_cells="
                  << rEntry.mnUnsupportedFunctionFormulaCells << '\n';
        std::cout << aPrefix << "_top_unsupported_function_" << nIndex
                  << "_formula_cells=" << rEntry.mnFormulaCells << '\n';
        std::cout << aPrefix << "_top_unsupported_function_" << nIndex
                  << "_fallback_formula_cells=" << rEntry.mnFallbackFormulaCells << '\n';
        std::cout << aPrefix << "_top_unsupported_function_" << nIndex
                  << "_unseen_formula_cells=" << rEntry.mnUnseenFormulaCells << '\n';
    }
}

void printTopUnknownRootInventory(
    std::string_view aPrefix, const ObserveSurfaceInventory& rInventory, std::size_t nLimit = 10)
{
    using UnknownRootInventoryEntry = ObserveSurfaceInventory::UnknownRootInventoryEntry;
    struct RankedUnknownRoot
    {
        OUString maLabel;
        UnknownRootInventoryEntry maEntry;
    };

    std::vector<RankedUnknownRoot> aEntries;
    aEntries.reserve(rInventory.maUnknownRootInventory.size());
    for (const auto& rEntry : rInventory.maUnknownRootInventory)
    {
        if (!rEntry.second.mnUnsupportedFunctionFormulaCells)
            continue;
        aEntries.push_back({ rEntry.first, rEntry.second });
    }

    std::sort(aEntries.begin(), aEntries.end(),
        [](const RankedUnknownRoot& rLeft, const RankedUnknownRoot& rRight) {
            if (rLeft.maEntry.mnUnsupportedFunctionFormulaCells
                != rRight.maEntry.mnUnsupportedFunctionFormulaCells)
            {
                return rLeft.maEntry.mnUnsupportedFunctionFormulaCells
                       > rRight.maEntry.mnUnsupportedFunctionFormulaCells;
            }
            if (rLeft.maEntry.mnUnseenFormulaCells != rRight.maEntry.mnUnseenFormulaCells)
                return rLeft.maEntry.mnUnseenFormulaCells > rRight.maEntry.mnUnseenFormulaCells;
            if (rLeft.maEntry.mnFallbackFormulaCells != rRight.maEntry.mnFallbackFormulaCells)
                return rLeft.maEntry.mnFallbackFormulaCells
                       > rRight.maEntry.mnFallbackFormulaCells;
            return rLeft.maLabel < rRight.maLabel;
        });

    if (aEntries.size() > nLimit)
        aEntries.resize(nLimit);

    std::cout << aPrefix << "_top_unknown_root_count=" << aEntries.size() << '\n';
    for (std::size_t nIndex = 0; nIndex < aEntries.size(); ++nIndex)
    {
        const auto& rEntry = aEntries[nIndex];
        std::cout << aPrefix << "_top_unknown_root_" << nIndex
                  << "_label=" << rEntry.maLabel.toUtf8().getStr() << '\n';
        std::cout << aPrefix << "_top_unknown_root_" << nIndex
                  << "_unsupported_function_formula_cells="
                  << rEntry.maEntry.mnUnsupportedFunctionFormulaCells << '\n';
        std::cout << aPrefix << "_top_unknown_root_" << nIndex
                  << "_formula_cells=" << rEntry.maEntry.mnFormulaCells << '\n';
        std::cout << aPrefix << "_top_unknown_root_" << nIndex
                  << "_fallback_formula_cells=" << rEntry.maEntry.mnFallbackFormulaCells
                  << '\n';
        std::cout << aPrefix << "_top_unknown_root_" << nIndex
                  << "_unseen_formula_cells=" << rEntry.maEntry.mnUnseenFormulaCells
                  << '\n';
    }
}

void printTopUnknownRootSurfaceInventory(
    std::string_view aPrefix, const ObserveSurfaceInventory& rInventory, std::size_t nLimit = 10)
{
    using UnknownRootInventoryEntry = ObserveSurfaceInventory::UnknownRootInventoryEntry;
    struct RankedUnknownRoot
    {
        OUString maLabel;
        UnknownRootInventoryEntry maEntry;
    };

    std::vector<RankedUnknownRoot> aEntries;
    aEntries.reserve(rInventory.maUnknownRootInventory.size());
    for (const auto& rEntry : rInventory.maUnknownRootInventory)
        aEntries.push_back({ rEntry.first, rEntry.second });

    std::sort(aEntries.begin(), aEntries.end(),
        [](const RankedUnknownRoot& rLeft, const RankedUnknownRoot& rRight) {
            if (rLeft.maEntry.mnFormulaCells != rRight.maEntry.mnFormulaCells)
                return rLeft.maEntry.mnFormulaCells > rRight.maEntry.mnFormulaCells;
            if (rLeft.maEntry.mnUnseenFormulaCells != rRight.maEntry.mnUnseenFormulaCells)
                return rLeft.maEntry.mnUnseenFormulaCells > rRight.maEntry.mnUnseenFormulaCells;
            if (rLeft.maEntry.mnFallbackFormulaCells != rRight.maEntry.mnFallbackFormulaCells)
                return rLeft.maEntry.mnFallbackFormulaCells > rRight.maEntry.mnFallbackFormulaCells;
            return rLeft.maLabel < rRight.maLabel;
        });

    if (aEntries.size() > nLimit)
        aEntries.resize(nLimit);

    std::cout << aPrefix << "_top_unknown_surface_root_count=" << aEntries.size() << '\n';
    for (std::size_t nIndex = 0; nIndex < aEntries.size(); ++nIndex)
    {
        const auto& rEntry = aEntries[nIndex];
        std::cout << aPrefix << "_top_unknown_surface_root_" << nIndex
                  << "_label=" << rEntry.maLabel.toUtf8().getStr() << '\n';
        std::cout << aPrefix << "_top_unknown_surface_root_" << nIndex
                  << "_formula_cells=" << rEntry.maEntry.mnFormulaCells << '\n';
        std::cout << aPrefix << "_top_unknown_surface_root_" << nIndex
                  << "_fallback_formula_cells=" << rEntry.maEntry.mnFallbackFormulaCells
                  << '\n';
        std::cout << aPrefix << "_top_unknown_surface_root_" << nIndex
                  << "_unseen_formula_cells=" << rEntry.maEntry.mnUnseenFormulaCells
                  << '\n';
        std::cout << aPrefix << "_top_unknown_surface_root_" << nIndex
                  << "_unsupported_function_formula_cells="
                  << rEntry.maEntry.mnUnsupportedFunctionFormulaCells << '\n';
    }
}

void printStats(std::size_t nWorkbookCount, std::size_t nFormulaCellCount, const StatsSnapshot& rStats)
{
    std::cout << "interpret_tail_corpus_workbooks=" << nWorkbookCount << '\n';
    std::cout << "interpret_tail_corpus_formula_cells=" << nFormulaCellCount << '\n';
    std::cout << "interpret_tail_observe_total=" << rStats.mnObserveCount << '\n';
    std::cout << "interpret_tail_shadow_compare_total=" << rStats.mnShadowCompareCount << '\n';
    std::cout << "interpret_tail_authoritative_total=" << rStats.mnAuthoritativeCount << '\n';
    std::cout << "interpret_tail_authoritative_fallback_total="
              << rStats.mnAuthoritativeFallbackCount << '\n';
    std::cout << "interpret_tail_shadow_match_total=" << rStats.mnShadowMatchCount << '\n';

    const auto aOldFlags = std::cout.flags();
    const auto nOldPrecision = std::cout.precision();
    std::cout << std::fixed << std::setprecision(2);
    for (std::size_t nIndex = 1; nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
    {
        const auto eFunction = static_cast<FunctionKind>(nIndex);
        const sal_uInt64 nAuthoritative = rStats.maFunctionAuthoritativeCount[nIndex];
        const sal_uInt64 nFallback = rStats.maFunctionFallbackCount[nIndex];
        const sal_uInt64 nAttempts = nAuthoritative + nFallback;
        const double fSupportRate
            = nAttempts ? (static_cast<double>(nAuthoritative) * 100.0 / nAttempts) : 0.0;
        const char* pName = functionKindName(eFunction);
        std::cout << "interpret_tail_function_" << pName << "_authoritative=" << nAuthoritative
                  << '\n';
        std::cout << "interpret_tail_function_" << pName << "_fallback=" << nFallback << '\n';
        std::cout << "interpret_tail_function_" << pName << "_attempts=" << nAttempts << '\n';
        std::cout << "interpret_tail_function_" << pName << "_support_rate=" << fSupportRate
                  << '\n';
        for (std::size_t nReason = 0; nReason < static_cast<std::size_t>(FallbackReason::Count);
             ++nReason)
        {
            const auto eReason = static_cast<FallbackReason>(nReason);
            std::cout << "interpret_tail_function_" << pName << "_fallback_reason_"
                      << fallbackReasonName(eReason) << "="
                      << rStats.maFunctionFallbackReasons[nIndex][nReason] << '\n';
        }
    }
    std::cout.flags(aOldFlags);
    std::cout.precision(nOldPrecision);

    for (std::size_t nIndex = 0; nIndex < static_cast<std::size_t>(FallbackReason::Count); ++nIndex)
    {
        const auto eReason = static_cast<FallbackReason>(nIndex);
        std::cout << "interpret_tail_fallback_" << fallbackReasonName(eReason)
                  << "=" << rStats.maFallbackReasons[nIndex] << '\n';
    }
}

void printLiveTargetProbeSummary(const SupportedProbeRun& rRun)
{
    const double fLiveReachableRate = rRun.mnRawFormulaCount
                                          ? (static_cast<double>(rRun.mnLiveTargetFormulaCount)
                                             * 100.0 / static_cast<double>(rRun.mnRawFormulaCount))
                                          : 0.0;
    const double fImportedArtifactRate = rRun.mnRawFormulaCount
                                             ? (static_cast<double>(rRun.mnHostTruthArtifactFormulaCount)
                                                * 100.0
                                                / static_cast<double>(rRun.mnRawFormulaCount))
                                             : 0.0;

    std::cout << "interpret_tail_live_target_probe_formula_cells="
              << rRun.mnLiveTargetFormulaCount << '\n';
    std::cout << "interpret_tail_probe_live_reachable_formula_cells="
              << rRun.mnLiveTargetFormulaCount << '\n';
    std::cout << "interpret_tail_probe_host_truth_artifact_formula_cells="
              << rRun.mnHostTruthArtifactFormulaCount << '\n';
    std::cout << "interpret_tail_probe_imported_artifact_formula_cells="
              << rRun.mnHostTruthArtifactFormulaCount << '\n';
    std::cout << "interpret_tail_live_target_authoritative_total="
              << rRun.maLiveTargetStats.mnAuthoritativeCount << '\n';
    std::cout << "interpret_tail_live_target_authoritative_fallback_total="
              << rRun.maLiveTargetStats.mnAuthoritativeFallbackCount << '\n';

    const auto aOldFlags = std::cout.flags();
    const auto nOldPrecision = std::cout.precision();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "interpret_tail_probe_live_reachable_rate=" << fLiveReachableRate << '\n';
    std::cout << "interpret_tail_probe_imported_artifact_rate=" << fImportedArtifactRate << '\n';
    std::cout.flags(aOldFlags);
    std::cout.precision(nOldPrecision);

    for (std::size_t nIndex = 1; nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
    {
        const auto eFunction = static_cast<FunctionKind>(nIndex);
        const char* pName = functionKindName(eFunction);
        std::cout << "interpret_tail_probe_host_truth_artifact_function_" << pName << "="
                  << rRun.maHostTruthArtifactFunctionCount[nIndex] << '\n';
    }
}

void printLiveAuthoritativeSummary(
    std::size_t nCorpusFormulaCount, const SupportedProbeRun& rRun,
    std::size_t nLegacyInterpreterSubroutineCount,
    const Interp4LegacyLambdaInventory& rLegacyLambdaInventory,
    const Interp4EngineDispatchInventory& rEngineDispatchInventory,
    const ScInterpreterDispatchRuntimeStatsSnapshot& rLiveDispatchRuntimeStats,
    const ScInterpreterDispatchRuntimeStatsSnapshot& rCoreForcedSeamDisabledDispatchRuntimeStats,
    const ScInterpreterReachabilityStatsSnapshot& rLiveReachabilityStats,
    const ScInterpreterReachabilityStatsSnapshot& rCoreForcedSeamDisabledReachabilityStats)
{
    const sal_uInt64 nAuthoritative = rRun.maLiveAuthoritativeStats.mnAuthoritativeCount;
    const sal_uInt64 nFallback = rRun.maLiveAuthoritativeStats.mnAuthoritativeFallbackCount;
    const double fCorpusMatchRate = nCorpusFormulaCount
                                        ? (static_cast<double>(nAuthoritative) * 100.0
                                           / static_cast<double>(nCorpusFormulaCount))
                                        : 0.0;
    const double fProbeMatchRate = rRun.mnLiveAuthoritativeFormulaCount
                                       ? (static_cast<double>(nAuthoritative) * 100.0
                                          / static_cast<double>(rRun.mnLiveAuthoritativeFormulaCount))
                                       : 0.0;

    std::cout << "interpret_tail_live_authoritative_corpus_formula_cells="
              << nCorpusFormulaCount << '\n';
    std::cout << "interpret_tail_live_authoritative_probe_formula_cells="
              << rRun.mnLiveAuthoritativeFormulaCount << '\n';
    std::cout << "interpret_tail_live_authoritative_match_total="
              << nAuthoritative << '\n';
    std::cout << "interpret_tail_live_authoritative_fallback_total="
              << nFallback << '\n';
    std::cout << "legacy_interpreter_subroutine_count="
              << nLegacyInterpreterSubroutineCount << '\n';
    std::cout << "interp4_dispatch_legacy_lambda_count="
              << rLegacyLambdaInventory.mnLambdaCount << '\n';
    std::cout << "interp4_dispatch_legacy_dispatch_target_count="
              << rLegacyLambdaInventory.mnDispatchLambdaCount << '\n';
    std::cout << "interp4_dispatch_legacy_call_count="
              << rLegacyLambdaInventory.mnDispatchCallCount << '\n';
    std::cout << "interp4_dispatch_engine_attempt_count="
              << rEngineDispatchInventory.mnAttemptCaseCount << '\n';
    std::cout << "interp4_dispatch_engine_attempted_total="
              << rLiveDispatchRuntimeStats.mnEngineAttemptedCount << '\n';
    std::cout << "interp4_dispatch_engine_succeeded_total="
              << rLiveDispatchRuntimeStats.mnEngineSucceededCount << '\n';
    std::cout << "interp4_dispatch_engine_declined_total="
              << rLiveDispatchRuntimeStats.mnEngineDeclinedCount << '\n';
    std::cout << "interp4_dispatch_engine_attempted_total_core_forced_full_legacy="
              << rCoreForcedSeamDisabledDispatchRuntimeStats.mnEngineAttemptedCount << '\n';
    std::cout << "interp4_dispatch_engine_succeeded_total_core_forced_full_legacy="
              << rCoreForcedSeamDisabledDispatchRuntimeStats.mnEngineSucceededCount << '\n';
    std::cout << "interp4_dispatch_engine_declined_total_core_forced_full_legacy="
              << rCoreForcedSeamDisabledDispatchRuntimeStats.mnEngineDeclinedCount << '\n';
    std::cout << "sc_formula_executor_formula_cell_interpret_total_live="
              << rLiveReachabilityStats.mnFormulaCellInterpretCount << '\n';
    std::cout << "sc_formula_executor_formula_group_attempt_total_live="
              << rLiveReachabilityStats.mnFormulaGroupAttemptCount << '\n';
    std::cout << "sc_formula_executor_formula_group_handled_total_live="
              << rLiveReachabilityStats.mnFormulaGroupHandledCount << '\n';
    std::cout << "sc_formula_executor_interpret_tail_total_live="
              << rLiveReachabilityStats.mnInterpretTailCount << '\n';
    std::cout << "sc_formula_executor_classic_interpret_total_live="
              << rLiveReachabilityStats.mnClassicInterpretCount << '\n';
    std::cout << "sc_formula_executor_formula_cell_interpret_total_core_forced_full_legacy="
              << rCoreForcedSeamDisabledReachabilityStats.mnFormulaCellInterpretCount << '\n';
    std::cout << "sc_formula_executor_formula_group_attempt_total_core_forced_full_legacy="
              << rCoreForcedSeamDisabledReachabilityStats.mnFormulaGroupAttemptCount << '\n';
    std::cout << "sc_formula_executor_formula_group_handled_total_core_forced_full_legacy="
              << rCoreForcedSeamDisabledReachabilityStats.mnFormulaGroupHandledCount << '\n';
    std::cout << "sc_formula_executor_interpret_tail_total_core_forced_full_legacy="
              << rCoreForcedSeamDisabledReachabilityStats.mnInterpretTailCount << '\n';
    std::cout << "sc_formula_executor_classic_interpret_total_core_forced_full_legacy="
              << rCoreForcedSeamDisabledReachabilityStats.mnClassicInterpretCount << '\n';
    std::cout << "interp4_dispatch_legacy_quarantine_covered_lambda_count="
              << rLegacyLambdaInventory.mnQuarantineCoveredLambdaCount << '\n';
    std::cout << "interp4_dispatch_legacy_quarantine_missing_lambda_count="
              << rLegacyLambdaInventory.mnQuarantineMissingLambdaCount << '\n';
    std::cout << "interp4_dispatch_legacy_quarantine_covered_dispatch_target_count="
              << rLegacyLambdaInventory.mnQuarantineCoveredDispatchLambdaCount << '\n';
    std::cout << "interp4_dispatch_legacy_quarantine_missing_dispatch_target_count="
              << rLegacyLambdaInventory.mnQuarantineMissingDispatchLambdaCount << '\n';
    if (!rLegacyLambdaInventory.maMissingDispatchLambdaNames.empty())
    {
        std::ostringstream aMissingNames;
        for (std::size_t nIndex = 0; nIndex < rLegacyLambdaInventory.maMissingDispatchLambdaNames.size();
             ++nIndex)
        {
            if (nIndex > 0)
                aMissingNames << ",";
            aMissingNames << rLegacyLambdaInventory.maMissingDispatchLambdaNames[nIndex];
        }
        std::cout << "interp4_dispatch_legacy_quarantine_missing_dispatch_target_names="
                  << aMissingNames.str() << '\n';
    }

    const auto aOldFlags = std::cout.flags();
    const auto nOldPrecision = std::cout.precision();
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "interpret_tail_live_authoritative_match_rate="
              << fCorpusMatchRate << '\n';
    std::cout << "interpret_tail_live_authoritative_probe_match_rate="
              << fProbeMatchRate << '\n';
    const double fEngineSuccessRate = rLiveDispatchRuntimeStats.mnEngineAttemptedCount
                                          ? (static_cast<double>(
                                                 rLiveDispatchRuntimeStats.mnEngineSucceededCount)
                                             * 100.0
                                             / static_cast<double>(
                                                 rLiveDispatchRuntimeStats.mnEngineAttemptedCount))
                                          : 0.0;
    const double fEngineDeclineRate = rLiveDispatchRuntimeStats.mnEngineAttemptedCount
                                          ? (static_cast<double>(
                                                 rLiveDispatchRuntimeStats.mnEngineDeclinedCount)
                                             * 100.0
                                             / static_cast<double>(
                                                 rLiveDispatchRuntimeStats.mnEngineAttemptedCount))
                                          : 0.0;
    const double fCoreForcedSeamDisabledEngineSuccessRate
        = rCoreForcedSeamDisabledDispatchRuntimeStats.mnEngineAttemptedCount
              ? (static_cast<double>(rCoreForcedSeamDisabledDispatchRuntimeStats.mnEngineSucceededCount)
                 * 100.0
                 / static_cast<double>(
                     rCoreForcedSeamDisabledDispatchRuntimeStats.mnEngineAttemptedCount))
              : 0.0;
    const double fCoreForcedSeamDisabledEngineDeclineRate
        = rCoreForcedSeamDisabledDispatchRuntimeStats.mnEngineAttemptedCount
              ? (static_cast<double>(rCoreForcedSeamDisabledDispatchRuntimeStats.mnEngineDeclinedCount)
                 * 100.0
                 / static_cast<double>(
                     rCoreForcedSeamDisabledDispatchRuntimeStats.mnEngineAttemptedCount))
              : 0.0;
    std::cout << "interp4_dispatch_engine_success_rate="
              << fEngineSuccessRate << '\n';
    std::cout << "interp4_dispatch_engine_decline_rate="
              << fEngineDeclineRate << '\n';
    std::cout << "interp4_dispatch_engine_success_rate_core_forced_full_legacy="
              << fCoreForcedSeamDisabledEngineSuccessRate << '\n';
    std::cout << "interp4_dispatch_engine_decline_rate_core_forced_full_legacy="
              << fCoreForcedSeamDisabledEngineDeclineRate << '\n';
    std::cout.flags(aOldFlags);
    std::cout.precision(nOldPrecision);

    std::cout << std::fixed << std::setprecision(2);
    for (std::size_t nIndex = 1; nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
    {
        const auto eFunction = static_cast<FunctionKind>(nIndex);
        const sal_uInt64 nFunctionAuthoritative
            = rRun.maLiveAuthoritativeStats.maFunctionAuthoritativeCount[nIndex];
        const sal_uInt64 nFunctionFallback
            = rRun.maLiveAuthoritativeStats.maFunctionFallbackCount[nIndex];
        const sal_uInt64 nFunctionAttempts = nFunctionAuthoritative + nFunctionFallback;
        const double fFunctionMatchRate
            = nFunctionAttempts ? (static_cast<double>(nFunctionAuthoritative) * 100.0
                                 / static_cast<double>(nFunctionAttempts))
                               : 0.0;
        const char* pName = functionKindName(eFunction);
        std::cout << "interpret_tail_live_authoritative_function_" << pName
                  << "_match=" << nFunctionAuthoritative << '\n';
        std::cout << "interpret_tail_live_authoritative_function_" << pName
                  << "_fallback=" << nFunctionFallback << '\n';
        std::cout << "interpret_tail_live_authoritative_function_" << pName
                  << "_attempts=" << nFunctionAttempts << '\n';
        std::cout << "interpret_tail_live_authoritative_function_" << pName
                  << "_match_rate=" << fFunctionMatchRate << '\n';
    }
    std::cout.flags(aOldFlags);
    std::cout.precision(nOldPrecision);
}

void printWorkdayRuntimeStats()
{
    if (!envEnabled("SPREADSHEET_ENGINE_WORKDAY_RUNTIME_STATS"))
        return;

    const auto aStats = spreadsheetengine::core::datetime::getWorkdayRuntimeStatsSnapshot();
    std::cout << "workday_runtime_weekend_mask_sequence_calls="
              << aStats.mnWeekendMaskSequenceCalls << '\n';
    std::cout << "workday_runtime_weekend_mask_msspec_calls="
              << aStats.mnWeekendMaskMsSpecCalls << '\n';
    std::cout << "workday_runtime_count_calls=" << aStats.mnCountWorkdaysCalls << '\n';
    std::cout << "workday_runtime_count_total_span_days="
              << aStats.mnCountWorkdaysTotalSpanDays << '\n';
    std::cout << "workday_runtime_count_max_span_days="
              << aStats.mnCountWorkdaysMaxSpanDays << '\n';
    std::cout << "workday_runtime_count_total_holiday_count="
              << aStats.mnCountWorkdaysTotalHolidayCount << '\n';
    std::cout << "workday_runtime_count_max_holiday_count="
              << aStats.mnCountWorkdaysMaxHolidayCount << '\n';
    std::cout << "workday_runtime_count_total_loop_iterations="
              << aStats.mnCountWorkdaysTotalLoopIterations << '\n';
    std::cout << "workday_runtime_count_max_loop_iterations="
              << aStats.mnCountWorkdaysMaxLoopIterations << '\n';
    std::cout << "workday_runtime_advance_calls=" << aStats.mnAdvanceWorkdayCalls << '\n';
    std::cout << "workday_runtime_advance_total_requested_days="
              << aStats.mnAdvanceWorkdayTotalRequestedDays << '\n';
    std::cout << "workday_runtime_advance_max_requested_days="
              << aStats.mnAdvanceWorkdayMaxRequestedDays << '\n';
    std::cout << "workday_runtime_advance_total_holiday_count="
              << aStats.mnAdvanceWorkdayTotalHolidayCount << '\n';
    std::cout << "workday_runtime_advance_max_holiday_count="
              << aStats.mnAdvanceWorkdayMaxHolidayCount << '\n';
    std::cout << "workday_runtime_advance_total_calendar_steps="
              << aStats.mnAdvanceWorkdayTotalCalendarSteps << '\n';
    std::cout << "workday_runtime_advance_max_calendar_steps="
              << aStats.mnAdvanceWorkdayMaxCalendarSteps << '\n';
    std::cout << "workday_runtime_advance_total_weekend_skips="
              << aStats.mnAdvanceWorkdayTotalWeekendSkips << '\n';
    std::cout << "workday_runtime_advance_total_holiday_skips="
              << aStats.mnAdvanceWorkdayTotalHolidaySkips << '\n';
}

void appendDiagnosticSamples(
    std::vector<DiagnosticSample>& rTarget, const std::vector<DiagnosticSample>& rSource)
{
    rTarget.insert(rTarget.end(), rSource.begin(), rSource.end());
}

void printDiagnosticSamples(const std::vector<DiagnosticSample>& rSamples)
{
    if (!envEnabled("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_DIAGNOSTICS"))
        return;

    std::cout << "interpret_tail_diagnostic_sample_count=" << rSamples.size() << '\n';
    for (std::size_t nIndex = 0; nIndex < rSamples.size(); ++nIndex)
    {
        const auto& rSample = rSamples[nIndex];
        std::cout << "interpret_tail_diagnostic_" << nIndex << "_reason="
                  << fallbackReasonName(rSample.meReason) << '\n';
        std::cout << "interpret_tail_diagnostic_" << nIndex << "_workbook="
                  << rSample.maWorkbookLabel.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_diagnostic_" << nIndex << "_cell="
                  << rSample.maCellAddress.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_diagnostic_" << nIndex << "_root_kind="
                  << rSample.maRootKind.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_diagnostic_" << nIndex << "_raw="
                  << rSample.maRawFormula.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_diagnostic_" << nIndex << "_normalized="
                  << rSample.maNormalizedFormula.toUtf8().getStr() << '\n';
    }
}

ScRangeList collectWorkbookUsedRanges(const Workbook& rWorkbook)
{
    ScRangeList aRanges;
    for (std::size_t nSheet = 0; nSheet < rWorkbook.maSheets.size(); ++nSheet)
    {
        if (rWorkbook.maSheets[nSheet].maCells.empty())
            continue;

        SCCOL nStartCol = std::numeric_limits<SCCOL>::max();
        SCROW nStartRow = std::numeric_limits<SCROW>::max();
        SCCOL nEndCol = 0;
        SCROW nEndRow = 0;
        for (const auto& rEntry : rWorkbook.maSheets[nSheet].maCells)
        {
            const SCCOL nCol = static_cast<SCCOL>(rEntry.first.first);
            const SCROW nRow = static_cast<SCROW>(rEntry.first.second);
            nStartCol = std::min(nStartCol, nCol);
            nStartRow = std::min(nStartRow, nRow);
            nEndCol = std::max(nEndCol, nCol);
            nEndRow = std::max(nEndRow, nRow);
        }

        aRanges.Join(ScRange(nStartCol, nStartRow, static_cast<SCTAB>(nSheet), nEndCol, nEndRow,
            static_cast<SCTAB>(nSheet)));
    }

    return aRanges;
}

void printProbeDiagnosticSamples()
{
    if (!envEnabled("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_PROBE_DIAGNOSTICS"))
        return;

    const auto& rSamples = probeDiagnosticSamples();
    std::cout << "interpret_tail_probe_diagnostic_sample_count=" << rSamples.size() << '\n';
    for (std::size_t nIndex = 0; nIndex < rSamples.size(); ++nIndex)
    {
        const auto& rSample = rSamples[nIndex];
        std::cout << "interpret_tail_probe_diagnostic_" << nIndex << "_workbook="
                  << rSample.maWorkbookLabel.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_probe_diagnostic_" << nIndex << "_cell="
                  << rSample.maCellAddress.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_probe_diagnostic_" << nIndex << "_function="
                  << rSample.maFunctionName.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_probe_diagnostic_" << nIndex << "_outcome="
                  << rSample.maOutcome.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_probe_diagnostic_" << nIndex << "_formula="
                  << rSample.maFormulaSource.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_probe_diagnostic_" << nIndex << "_calc_result="
                  << rSample.maCalcResult.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_probe_diagnostic_" << nIndex << "_live_host_result="
                  << rSample.maLiveHostResult.toUtf8().getStr() << '\n';
        std::cout << "interpret_tail_probe_diagnostic_" << nIndex << "_engine_result="
                  << rSample.maEngineResult.toUtf8().getStr() << '\n';
    }
}

OUString makeSafeSheetName(std::size_t nSheetIndex)
{
    return u"ReplaySheet"_ustr + OUString::number(static_cast<sal_Int32>(nSheetIndex + 1));
}

void replaceSheetNameReferences(
    spreadsheetengine::api::String& rText,
    const std::vector<std::pair<OUString, OUString>>& rRenameMap)
{
    if (rText.empty() || rRenameMap.empty())
        return;

    OUString aText = toLibreOfficeString(rText);
    for (const auto& rRename : rRenameMap)
        aText = aText.replaceAll(rRename.first, rRename.second);
    rText = toApiString(aText);
}

void normalizeWorkbookSheetNamesForCalc(Workbook& rWorkbook)
{
    std::vector<std::pair<OUString, OUString>> aRenameMap;
    std::vector<OUString> aTakenNames;
    aTakenNames.reserve(rWorkbook.maSheets.size());

    for (std::size_t nSheet = 0; nSheet < rWorkbook.maSheets.size(); ++nSheet)
    {
        const OUString aOriginalName = toLibreOfficeString(rWorkbook.maSheets[nSheet].maName);
        OUString aCandidate = aOriginalName;

        auto hasTakenName = [&aTakenNames](const OUString& rName) {
            return std::find(aTakenNames.begin(), aTakenNames.end(), rName) != aTakenNames.end();
        };

        if (!ScDocument::ValidTabName(aCandidate) || hasTakenName(aCandidate))
        {
            aCandidate = makeSafeSheetName(nSheet);
            while (!ScDocument::ValidTabName(aCandidate) || hasTakenName(aCandidate))
                aCandidate += u"X"_ustr;
        }

        aTakenNames.push_back(aCandidate);
        if (aCandidate != aOriginalName)
        {
            aRenameMap.emplace_back(aOriginalName, aCandidate);
            rWorkbook.maSheets[nSheet].maName = toApiString(aCandidate);
        }
    }

    if (aRenameMap.empty())
        return;

    for (auto& rNamedRange : rWorkbook.maNamedRanges)
    {
        replaceSheetNameReferences(rNamedRange.maBaseCellAddress, aRenameMap);
        replaceSheetNameReferences(rNamedRange.maCellRangeAddress, aRenameMap);
        for (const auto& rRename : aRenameMap)
        {
            if (rNamedRange.maScopeSheetName == toApiString(rRename.first))
            {
                rNamedRange.maScopeSheetName = toApiString(rRename.second);
                break;
            }
        }
    }

    for (auto& rSheet : rWorkbook.maSheets)
    {
        if (rSheet.moSource)
            replaceSheetNameReferences(rSheet.moSource->maTableName, aRenameMap);

        for (auto& rEntry : rSheet.maCells)
        {
            replaceSheetNameReferences(rEntry.second.maFormula, aRenameMap);
        }
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedMatchExactRangeParity)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/match.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for match.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const ScAddress aPos(0, 76, 1);
    ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
    CPPUNIT_ASSERT(pFormula);

    const OUString aFormulaSource
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
    const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
    CPPUNIT_ASSERT_EQUAL(u"=of:=MATCH(0;[.G77:.G79];0)"_ustr, aFormulaSource);

    const auto aAttempt
        = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
            rDoc, *pContext, aPos,
            std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
            rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
            std::u16string_view(aCanonicalFormulaSource.getStr(),
                aCanonicalFormulaSource.getLength()));
    CPPUNIT_ASSERT(aAttempt.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aAttempt.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aAttempt.maResult.mfValue, 1e-12);
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedMatchRangeLookupValueParity)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/match.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for match.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    const ScAddress aPos(0, 29, 1); // Sheet2.A30
    ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
    CPPUNIT_ASSERT(pFormula);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const OUString aFormulaSource
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
    const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
    CPPUNIT_ASSERT_EQUAL(u"=of:=MATCH([.F29:.F37];[.F29:.F37];0)"_ustr, aFormulaSource);

    const auto aAttempt
        = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
            rDoc, *pContext, aPos,
            std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
            rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
            std::u16string_view(aCanonicalFormulaSource.getStr(),
                aCanonicalFormulaSource.getLength()));
    CPPUNIT_ASSERT(aAttempt.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aAttempt.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aAttempt.maResult.mfValue, 1e-12);
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedMatchLocalizedArrayConstantParity)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/match.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for match.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    for (const auto& [nRow, fExpected] :
         { std::pair(SCROW(157), 1.0), std::pair(SCROW(158), 2.0), std::pair(SCROW(159), 2.0) })
    {
        const ScAddress aPos(0, nRow, 1);
        ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
        CPPUNIT_ASSERT_EQUAL(u"=of:=MATCH(2; {1.2}; 1)"_ustr, aFormulaSource);

        const auto aAttempt
            = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
                rDoc, *pContext, aPos,
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
                rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
                std::u16string_view(aCanonicalFormulaSource.getStr(),
                    aCanonicalFormulaSource.getLength()));
        CPPUNIT_ASSERT(aAttempt.mbSupported);
        CPPUNIT_ASSERT_EQUAL(
            spreadsheetengine::api::formulavalue::ValueType::Value,
            aAttempt.maResult.meType);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(fExpected, aAttempt.maResult.mfValue, 1e-12);
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedMatchLocalizedArrayConstantAuthorityParity)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/match.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for match.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    {
        ScopedEnvironmentOverride aAuthorityMode(
            "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "authority");
        sc::SetFormulaDirtyContext aDirtyCxt;
        rDoc.SetAllFormulasDirty(aDirtyCxt);

        for (const auto& [nRow, fExpected] :
             { std::pair(SCROW(158), 2.0), std::pair(SCROW(159), 2.0) })
        {
            const ScAddress aPos(0, nRow, 1);
            ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
            CPPUNIT_ASSERT(pFormula);
            pFormula->SetDirty();
            pFormula->Interpret();
            CPPUNIT_ASSERT_EQUAL(FormulaError::NONE, rDoc.GetErrCode(aPos));
            CPPUNIT_ASSERT_DOUBLES_EQUAL(fExpected, rDoc.GetValue(aPos), 1e-12);
        }
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedMatchWholeRowLiveHostTruth)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/match.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for match.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    const ScAddress aPos(0, 141, 1); // Sheet2.A142
    ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
    CPPUNIT_ASSERT(pFormula);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const OUString aFormulaSource
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
    CPPUNIT_ASSERT_EQUAL(u"=of:=MATCH([.$B$150];[.$150:.$150];-1)"_ustr, aFormulaSource);

    {
        ScopedEnvironmentOverride aOffMode(
            "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
        sc::SetFormulaDirtyContext aDirtyCxt;
        rDoc.SetAllFormulasDirty(aDirtyCxt);
        pFormula->SetDirty();
        pFormula->Interpret();
    }

    CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(aPos));
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedMatchFrequencyLiveHostTruth)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/match.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for match.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    const ScAddress aPos(0, 110, 1); // Sheet2.A111
    ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
    CPPUNIT_ASSERT(pFormula);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const OUString aFormulaSource
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
    CPPUNIT_ASSERT_EQUAL(u"=of:=MATCH(1;FREQUENCY([.I126];[.H129:.M129]);0)"_ustr, aFormulaSource);

    {
        ScopedEnvironmentOverride aOffMode(
            "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
        pFormula->SetDirty();
        pFormula->Interpret();
    }

    CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(aPos));
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedLogicalFoldLiveHostTruth)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/logical/fods/if.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for if.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const ScAddress aPos(1, 2, 0); // Sheet1.B3
    ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
    CPPUNIT_ASSERT(pFormula);

    const OUString aFormulaSource
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
    CPPUNIT_ASSERT_EQUAL(u"=of:=AND([.B8:.B95])"_ustr, aFormulaSource);

    {
        ScopedEnvironmentOverride aOffMode(
            "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
        pFormula->SetDirty();
        pFormula->Interpret();
    }

    CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(aPos));
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedFormulaTextLiveHostTruth)
{
    const struct ImportedFormulaTextCase
    {
        OUString maWorkbookPath;
        ScAddress maPos;
        OUString maExpectedFormula;
    } aCases[] = {
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/logical/fods/if.fods"),
            ScAddress(3, 3, 1), u"=of:=FORMULA([.A4])"_ustr }, // Sheet2.D4
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/logical/fods/and.fods"),
            ScAddress(3, 7, 1), u"=of:=FORMULA([.A8])"_ustr }, // Sheet2.D8
    };

    for (const auto& rCase : aCases)
    {
        const std::string aWorkbookPathUtf8(rCase.maWorkbookPath.toUtf8().getStr());
        const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
        CPPUNIT_ASSERT_MESSAGE(("loadWorkbook failed for formula-text host truth case: "
                                   + aWorkbookPathUtf8)
                                       .c_str(),
            static_cast<bool>(aLoadResult));

        Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
        normalizeWorkbookSheetNamesForCalc(aWorkbook);

        ScDocShellRef xDocShell = new ScDocShell(
            SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
            | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
        xDocShell->DoInitUnitTest();
        ScDocument& rDoc = xDocShell->GetDocument();
        (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

        ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
        ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
        CPPUNIT_ASSERT(pContext);

        ScFormulaCell* pFormula = rDoc.GetFormulaCell(rCase.maPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        CPPUNIT_ASSERT(aFormulaSource.equalsIgnoreAsciiCase(rCase.maExpectedFormula));

        {
            ScopedEnvironmentOverride aOffMode(
                "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
            pFormula->SetDirty();
            pFormula->Interpret();
        }

        CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(rCase.maPos));
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedLogicalFoldDirectParity)
{
    const struct ImportedLogicalFoldCase
    {
        OUString maWorkbookPath;
        ScAddress maPos;
        OUString maExpectedFormula;
    } aCases[] = {
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/logical/fods/iferror.fods"),
            ScAddress(0, 22, 1), u"=of:=AND(NA();IFERROR(NA();1))"_ustr }, // Sheet2.A23
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/logical/fods/or.fods"),
            ScAddress(0, 11, 1), u"=of:=OR(FALSE();NA())"_ustr }, // Sheet2.A12
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/logical/fods/xor.fods"),
            ScAddress(0, 11, 1), u"=of:=XOR(FALSE();NA())"_ustr }, // Sheet2.A12
    };

    for (const auto& rCase : aCases)
    {
        const std::string aWorkbookPathUtf8(rCase.maWorkbookPath.toUtf8().getStr());
        const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
        CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for direct logical-fold parity case",
            static_cast<bool>(aLoadResult));

        Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
        normalizeWorkbookSheetNamesForCalc(aWorkbook);

        ScDocShellRef xDocShell
            = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT
                             | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                             | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
        xDocShell->DoInitUnitTest();
        ScDocument& rDoc = xDocShell->GetDocument();
        (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

        ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
        ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
        CPPUNIT_ASSERT(pContext);

        ScFormulaCell* pFormula = rDoc.GetFormulaCell(rCase.maPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
        CPPUNIT_ASSERT(aFormulaSource.equalsIgnoreAsciiCase(rCase.maExpectedFormula));

        const auto aAttempt
            = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
                rDoc, *pContext, rCase.maPos,
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
                rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
                std::u16string_view(aCanonicalFormulaSource.getStr(),
                    aCanonicalFormulaSource.getLength()));
        CPPUNIT_ASSERT(aAttempt.mbSupported);
        CPPUNIT_ASSERT_EQUAL(
            spreadsheetengine::api::formulavalue::ValueType::Error,
            aAttempt.maResult.meType);
        CPPUNIT_ASSERT_EQUAL(
            spreadsheetengine::api::Error::VariableExpected, aAttempt.maResult.meError);
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedTextBeforeParity)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/textbefore.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for textbefore.fods",
        static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    const ScAddress aPos(0, 6, 1); // Sheet2.A7
    ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
    CPPUNIT_ASSERT(pFormula);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const OUString aFormulaSource
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
    const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
    CPPUNIT_ASSERT_EQUAL(
        u"=of:=COM.MICROSOFT.TEXTBEFORE([.F3]; \",\"; 2)"_ustr, aFormulaSource);

    const auto aAttempt
        = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
            rDoc, *pContext, aPos,
            std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
            rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
            std::u16string_view(aCanonicalFormulaSource.getStr(),
                aCanonicalFormulaSource.getLength()));
    CPPUNIT_ASSERT(aAttempt.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aAttempt.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"Brown, Lucas"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(aAttempt.maResult.maString));
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedLogicalFoldDirectLiveHostTruth)
{
    const struct ImportedLogicalFoldCase
    {
        OUString maWorkbookPath;
        ScAddress maPos;
        OUString maExpectedFormula;
    } aCases[] = {
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/logical/fods/iferror.fods"),
            ScAddress(0, 22, 1), u"=of:=AND(NA();IFERROR(NA();1))"_ustr }, // Sheet2.A23
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/logical/fods/or.fods"),
            ScAddress(0, 11, 1), u"=of:=OR(FALSE();NA())"_ustr }, // Sheet2.A12
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/logical/fods/xor.fods"),
            ScAddress(0, 11, 1), u"=of:=XOR(FALSE();NA())"_ustr }, // Sheet2.A12
    };

    for (const auto& rCase : aCases)
    {
        const std::string aWorkbookPathUtf8(rCase.maWorkbookPath.toUtf8().getStr());
        const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
        CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for direct logical-fold host truth case",
            static_cast<bool>(aLoadResult));

        Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
        normalizeWorkbookSheetNamesForCalc(aWorkbook);

        ScDocShellRef xDocShell
            = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT
                             | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                             | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
        xDocShell->DoInitUnitTest();
        ScDocument& rDoc = xDocShell->GetDocument();
        (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

        ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
        ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
        CPPUNIT_ASSERT(pContext);

        ScFormulaCell* pFormula = rDoc.GetFormulaCell(rCase.maPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        CPPUNIT_ASSERT_EQUAL(rCase.maExpectedFormula, aFormulaSource);

        {
            ScopedEnvironmentOverride aOffMode(
                "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
            pFormula->SetDirty();
            pFormula->Interpret();
        }

        const OString aCaseLabel = OUStringToOString(rCase.maExpectedFormula, RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(
            aCaseLabel.getStr(), FormulaError::VariableExpected, rDoc.GetErrCode(rCase.maPos));
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedMathScalarLiveHostTruth)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/mathematical/fods/abs.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for abs.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const ScAddress aPos(0, 4, 1); // Sheet2.A5
    ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
    CPPUNIT_ASSERT(pFormula);

    const OUString aFormulaSource
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
    CPPUNIT_ASSERT_EQUAL(u"=of:=ABS([.J2])"_ustr, aFormulaSource);

    {
        ScopedEnvironmentOverride aOffMode(
            "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
        pFormula->SetDirty();
        pFormula->Interpret();
    }

    CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(aPos));
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedMathScalarCachedErrorParity)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/mathematical/fods/abs.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for abs.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    for (const auto& [nRow, aExpectedFormula, eExpectedError] :
         { std::tuple(SCROW(4), OUString(u"=of:=ABS([.J2])"_ustr), FormulaError::DivisionByZero),
           std::tuple(SCROW(7), OUString(u"=of:=ABS([.K2])"_ustr), FormulaError::NoValue) })
    {
        const ScAddress aPos(0, nRow, 1); // Sheet2.A5 / Sheet2.A8
        ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
        CPPUNIT_ASSERT_EQUAL(aExpectedFormula, aFormulaSource);

        const auto aAttempt
            = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
                rDoc, *pContext, aPos,
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
                rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
                std::u16string_view(aCanonicalFormulaSource.getStr(),
                    aCanonicalFormulaSource.getLength()));
        CPPUNIT_ASSERT(aAttempt.mbSupported);
        CPPUNIT_ASSERT_EQUAL(
            spreadsheetengine::api::formulavalue::ValueType::Error,
            aAttempt.maResult.meType);
        CPPUNIT_ASSERT_EQUAL(
            spreadsheetengine::compat::libreoffice::toApiError(eExpectedError),
            aAttempt.maResult.meError);
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedLogicalFoldCachedRangeLiveHostTruth)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/mathematical/fods/abs.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for abs.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    for (const auto& [aPos, aExpectedFormula] :
         { std::pair(ScAddress(1, 2, 0), OUString(u"=of:=AND([.B8:.B95])"_ustr)),
           std::pair(ScAddress(1, 7, 0), OUString(u"=of:=AND([Sheet2.C2:.C92])"_ustr)) })
    {
        ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
        CPPUNIT_ASSERT_EQUAL(aExpectedFormula, aFormulaSource);

        const auto aAttempt
            = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
                rDoc, *pContext, aPos,
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
                rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
                std::u16string_view(aCanonicalFormulaSource.getStr(),
                    aCanonicalFormulaSource.getLength()));
        CPPUNIT_ASSERT(aAttempt.mbSupported);
        CPPUNIT_ASSERT_EQUAL(
            spreadsheetengine::api::formulavalue::ValueType::Error,
            aAttempt.maResult.meType);
        CPPUNIT_ASSERT_EQUAL(
            spreadsheetengine::compat::libreoffice::toApiError(FormulaError::VariableExpected),
            aAttempt.maResult.meError);
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedNotRangeLiveHostTruth)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/logical/fods/not.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for not.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    bool bSeen = false;
    for (std::size_t nSheet = 0; nSheet < aWorkbook.maSheets.size(); ++nSheet)
    {
        for (const auto& rEntry : aWorkbook.maSheets[nSheet].maCells)
        {
            const Cell& rCell = rEntry.second;
            if (!rCell.hasFormula())
                continue;

            const ScAddress aPos(static_cast<SCCOL>(rEntry.first.first),
                static_cast<SCROW>(rEntry.first.second), static_cast<SCTAB>(nSheet));
            ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
            if (!pFormula)
                continue;

            const OUString aFormulaSource
                = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
            if (aFormulaSource != u"=of:=NOT([.J6:.J7])"_ustr)
                continue;

            bSeen = true;
            {
                ScopedEnvironmentOverride aOffMode(
                    "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
                pFormula->SetDirty();
                pFormula->Interpret();
            }

            CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(aPos));
        }
    }

    CPPUNIT_ASSERT_MESSAGE("expected imported NOT range formula not found", bSeen);
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedInformationPredicateCachedReferenceParity)
{
    const struct ImportedPredicateCase
    {
        OUString maWorkbookPath;
        ScAddress maPos;
        OUString maExpectedFormula;
        double mfExpectedValue;
    } aCases[] = {
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/information/fods/info.fods"),
            ScAddress(5, 1, 1), u"=of:=ISERROR([.A2])"_ustr, 0.0 }, // Sheet2.F2
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/information/fods/isblank.fods"),
            ScAddress(0, 2, 1), u"=of:=ISBLANK([.F3])"_ustr, 1.0 }, // Sheet2.A3
    };

    for (const auto& rCase : aCases)
    {
        const std::string aWorkbookPathUtf8(rCase.maWorkbookPath.toUtf8().getStr());
        const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
        CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for information-predicate parity case",
            static_cast<bool>(aLoadResult));

        Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
        normalizeWorkbookSheetNamesForCalc(aWorkbook);

        ScDocShellRef xDocShell
            = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT
                             | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                             | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
        xDocShell->DoInitUnitTest();
        ScDocument& rDoc = xDocShell->GetDocument();
        (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

        ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
        ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
        CPPUNIT_ASSERT(pContext);

        ScFormulaCell* pFormula = rDoc.GetFormulaCell(rCase.maPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
        CPPUNIT_ASSERT(aFormulaSource.equalsIgnoreAsciiCase(rCase.maExpectedFormula));

        if (rCase.maExpectedFormula == u"=of:=ISBLANK([.F3])"_ustr)
        {
            const auto aReferenced = spreadsheetengine::compat::libreoffice::readHostDocumentCellValue(
                rDoc, ScAddress(5, 2, 1));
            CPPUNIT_ASSERT(aReferenced);
            CPPUNIT_ASSERT_EQUAL(
                spreadsheetengine::api::CellValueKind::Empty, aReferenced.maValue.meKind);
        }

        const auto aAttempt
            = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
                rDoc, *pContext, rCase.maPos,
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
                rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
                std::u16string_view(aCanonicalFormulaSource.getStr(),
                    aCanonicalFormulaSource.getLength()));
        CPPUNIT_ASSERT(aAttempt.mbSupported);
        CPPUNIT_ASSERT_EQUAL(
            spreadsheetengine::api::formulavalue::ValueType::Value,
            aAttempt.maResult.meType);
        const OString aCaseLabel = OString::Concat(aWorkbookPathUtf8.c_str()) + " "
            + OUStringToOString(rCase.maExpectedFormula, RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
            aCaseLabel.getStr(), rCase.mfExpectedValue, aAttempt.maResult.mfValue, 1e-12);
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedInformationPredicateLiveHostTruth)
{
    const struct ImportedPredicateHostTruthWorkbook
    {
        OUString maWorkbookPath;
        std::vector<OUString> maExpectedFormulas;
    } aWorkbooks[] = {
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/information/fods/na.fods"),
            { u"=of:=ISNA([.A2])"_ustr, u"=of:=ISERROR([.A3])"_ustr } },
        { m_directories.getPathFromSrc(
              u"/sc/qa/unit/data/functions/spreadsheet/fods/randarray.fods"),
            { u"=of:=ISTEXT([.A26])"_ustr } },
        { m_directories.getPathFromSrc(
              u"/sc/qa/unit/data/functions/information/fods/isblank.fods"),
            { u"=of:=ISBLANK([.F9])"_ustr } },
    };

    for (const auto& rWorkbookCase : aWorkbooks)
    {
        const std::string aWorkbookPathUtf8(rWorkbookCase.maWorkbookPath.toUtf8().getStr());
        const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
        CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for information-predicate host truth case",
            static_cast<bool>(aLoadResult));

        Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
        normalizeWorkbookSheetNamesForCalc(aWorkbook);

        ScDocShellRef xDocShell
            = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT
                             | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                             | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
        xDocShell->DoInitUnitTest();
        ScDocument& rDoc = xDocShell->GetDocument();
        (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

        ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
        ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
        CPPUNIT_ASSERT(pContext);

        struct MatchState
        {
            OUString maFormula;
            bool mbSeen = false;
        };
        std::vector<MatchState> aMatches;
        aMatches.reserve(rWorkbookCase.maExpectedFormulas.size());
        for (const OUString& rFormula : rWorkbookCase.maExpectedFormulas)
            aMatches.push_back({ rFormula, false });

        for (std::size_t nSheet = 0; nSheet < aWorkbook.maSheets.size(); ++nSheet)
        {
            for (const auto& rEntry : aWorkbook.maSheets[nSheet].maCells)
            {
                const Cell& rCell = rEntry.second;
                if (!rCell.hasFormula())
                    continue;

                const ScAddress aPos(static_cast<SCCOL>(rEntry.first.first),
                    static_cast<SCROW>(rEntry.first.second), static_cast<SCTAB>(nSheet));
                ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
                if (!pFormula)
                    continue;

                const OUString aFormulaSource
                    = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
                for (auto& rMatch : aMatches)
                {
                    if (aFormulaSource != rMatch.maFormula)
                        continue;

                    rMatch.mbSeen = true;
                    {
                        ScopedEnvironmentOverride aOffMode(
                            "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
                        pFormula->SetDirty();
                        pFormula->Interpret();
                    }

                    CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(aPos));
                }
            }
        }

        for (const auto& rMatch : aMatches)
        {
            const OUString aMessage
                = u"expected imported information-predicate formula not found: "_ustr
                  + rMatch.maFormula;
            CPPUNIT_ASSERT_MESSAGE(
                OUStringToOString(aMessage, RTL_TEXTENCODING_UTF8).getStr(), rMatch.mbSeen);
        }
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedInformationPredicateDirectParity)
{
    const struct ImportedPredicateCase
    {
        OUString maWorkbookPath;
        ScAddress maPos;
        OUString maExpectedFormula;
    } aCases[] = {
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/information/fods/iserror.fods"),
            ScAddress(0, 1, 1), u"=of:=ISERROR(undefined)"_ustr }, // Sheet2.A2
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/information/fods/iserr.fods"),
            ScAddress(0, 4, 1), u"=of:=ISERR(NA())"_ustr }, // Sheet2.A5
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/information/fods/isna.fods"),
            ScAddress(0, 5, 1), u"=of:=ISNA(NA()=NA())"_ustr }, // Sheet2.A6
    };

    for (const auto& rCase : aCases)
    {
        const std::string aWorkbookPathUtf8(rCase.maWorkbookPath.toUtf8().getStr());
        const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
        CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for information-predicate direct parity case",
            static_cast<bool>(aLoadResult));

        Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
        normalizeWorkbookSheetNamesForCalc(aWorkbook);

        ScDocShellRef xDocShell
            = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT
                             | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                             | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
        xDocShell->DoInitUnitTest();
        ScDocument& rDoc = xDocShell->GetDocument();
        (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

        ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
        ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
        CPPUNIT_ASSERT(pContext);

        ScFormulaCell* pFormula = rDoc.GetFormulaCell(rCase.maPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
        CPPUNIT_ASSERT(aFormulaSource.equalsIgnoreAsciiCase(rCase.maExpectedFormula));

        const auto aAttempt
            = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
                rDoc, *pContext, rCase.maPos,
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
                rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
                std::u16string_view(aCanonicalFormulaSource.getStr(),
                    aCanonicalFormulaSource.getLength()));
        CPPUNIT_ASSERT(aAttempt.mbSupported);
        CPPUNIT_ASSERT_EQUAL(
            spreadsheetengine::api::formulavalue::ValueType::Error,
            aAttempt.maResult.meType);
        CPPUNIT_ASSERT_EQUAL(
            spreadsheetengine::api::Error::VariableExpected, aAttempt.maResult.meError);
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedErrorTypeParity)
{
    const OUString aWorkbookPath = m_directories.getPathFromSrc(
        u"/sc/qa/unit/data/functions/spreadsheet/fods/error.type.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for error.type parity case",
        static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const ScAddress aPos(0, 27, 1); // Sheet2.A28
    ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
    CPPUNIT_ASSERT(pFormula);

    const OUString aFormulaSource
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
    const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
    CPPUNIT_ASSERT_EQUAL(u"=of:=ERROR.TYPE({#N/A})"_ustr, aFormulaSource);

    const auto aAttempt
        = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
            rDoc, *pContext, aPos,
            std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
            rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
            std::u16string_view(aCanonicalFormulaSource.getStr(),
                aCanonicalFormulaSource.getLength()));
    CPPUNIT_ASSERT(aAttempt.mbSupported);

    {
        ScopedEnvironmentOverride aOffMode(
            "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
        pFormula->SetDirty();
        pFormula->Interpret();
    }

    const auto aHostValue
        = spreadsheetengine::compat::libreoffice::readHostDocumentCellValue(rDoc, aPos);
    CPPUNIT_ASSERT(aHostValue);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::CellValueKind::Error, aHostValue.maValue.meKind);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error, aAttempt.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(aHostValue.maValue.meError, aAttempt.maResult.meError);
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedTokenBackedArrayParseParity)
{
    const struct ImportedArrayParseCase
    {
        ScAddress maPos;
        OUString maExpectedFormula;
    } aCases[]
        = { { ScAddress(0, 13, 1),
                u"=of:=MEDIAN({DATE(2015|3|4)|DATE(2015|2|1)|DATE(2015|3|4)})"_ustr },
              { ScAddress(7, 47, 1),
                  u"=of:=COM.MICROSOFT.MODE.MULT({DATE(2015|3|4)|DATE(2015|2|1)|DATE(2015|3|4)})"_ustr } };

    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/statistical/fods/median.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for median.fods parse parity case",
        static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    for (const auto& rCase : aCases)
    {
        ScFormulaCell* pFormula = rDoc.GetFormulaCell(rCase.maPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
        CPPUNIT_ASSERT_EQUAL(rCase.maExpectedFormula, aFormulaSource);

        const auto aAttempt
            = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
                rDoc, *pContext, rCase.maPos,
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
                rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
                std::u16string_view(aCanonicalFormulaSource.getStr(),
                    aCanonicalFormulaSource.getLength()));
        CPPUNIT_ASSERT(aAttempt.mbSupported);

        const auto aHostValue
            = spreadsheetengine::compat::libreoffice::readHostDocumentCellValue(rDoc, rCase.maPos);
        CPPUNIT_ASSERT(aHostValue);
        switch (aHostValue.maValue.meKind)
        {
            case spreadsheetengine::api::CellValueKind::Number:
            case spreadsheetengine::api::CellValueKind::Boolean:
                CPPUNIT_ASSERT_EQUAL(
                    spreadsheetengine::api::formulavalue::ValueType::Value,
                    aAttempt.maResult.meType);
                CPPUNIT_ASSERT_DOUBLES_EQUAL(
                    aHostValue.maValue.mfNumber, aAttempt.maResult.mfValue, 1e-12);
                break;
            case spreadsheetengine::api::CellValueKind::Text:
                CPPUNIT_ASSERT_EQUAL(
                    spreadsheetengine::api::formulavalue::ValueType::String,
                    aAttempt.maResult.meType);
                CPPUNIT_ASSERT_EQUAL(
                    spreadsheetengine::compat::libreoffice::toLibreOfficeString(
                        aHostValue.maValue.maString),
                    spreadsheetengine::compat::libreoffice::toLibreOfficeString(
                        aAttempt.maResult.maString));
                break;
            case spreadsheetengine::api::CellValueKind::Error:
                CPPUNIT_ASSERT_EQUAL(
                    spreadsheetengine::api::formulavalue::ValueType::Error,
                    aAttempt.maResult.meType);
                CPPUNIT_ASSERT_EQUAL(aHostValue.maValue.meError, aAttempt.maResult.meError);
                break;
            case spreadsheetengine::api::CellValueKind::Empty:
                CPPUNIT_ASSERT_EQUAL(
                    spreadsheetengine::api::formulavalue::ValueType::String,
                    aAttempt.maResult.meType);
                CPPUNIT_ASSERT_EQUAL(
                    u""_ustr,
                    spreadsheetengine::compat::libreoffice::toLibreOfficeString(
                        aAttempt.maResult.maString));
                break;
        }
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedInformationPredicateDirectLiveHostTruth)
{
    const struct ImportedPredicateHostTruthCase
    {
        OUString maWorkbookPath;
        ScAddress maPos;
        OUString maExpectedFormula;
    } aCases[] = {
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/information/fods/iserror.fods"),
            ScAddress(0, 1, 1), u"=of:=ISERROR(undefined)"_ustr }, // Sheet2.A2
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/information/fods/iserr.fods"),
            ScAddress(0, 4, 1), u"=of:=ISERR(NA())"_ustr }, // Sheet2.A5
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/information/fods/isna.fods"),
            ScAddress(0, 5, 1), u"=of:=ISNA(NA()=NA())"_ustr }, // Sheet2.A6
    };

    for (const auto& rCase : aCases)
    {
        const std::string aWorkbookPathUtf8(rCase.maWorkbookPath.toUtf8().getStr());
        const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
        CPPUNIT_ASSERT_MESSAGE(
            "loadWorkbook failed for information-predicate direct host truth case",
            static_cast<bool>(aLoadResult));

        Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
        normalizeWorkbookSheetNamesForCalc(aWorkbook);

        ScDocShellRef xDocShell
            = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT
                             | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                             | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
        xDocShell->DoInitUnitTest();
        ScDocument& rDoc = xDocShell->GetDocument();
        (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

        ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
        ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
        CPPUNIT_ASSERT(pContext);

        ScFormulaCell* pFormula = rDoc.GetFormulaCell(rCase.maPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        CPPUNIT_ASSERT_EQUAL(rCase.maExpectedFormula, aFormulaSource);

        {
            ScopedEnvironmentOverride aOffMode(
                "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
            pFormula->SetDirty();
            pFormula->Interpret();
        }

        const OString aCaseLabel = OUStringToOString(rCase.maExpectedFormula, RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(
            aCaseLabel.getStr(), FormulaError::VariableExpected, rDoc.GetErrCode(rCase.maPos));
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedBroadFamilyLiveHostTruth)
{
    const struct ImportedHostTruthCase
    {
        OUString maWorkbookPath;
        ScAddress maPos;
        OUString maExpectedFormula;
    } aCases[] = {
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/text/fods/t.fods"),
            ScAddress(0, 1, 1), u"=of:=T(0)"_ustr },
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/addin/fods/convert.fods"),
            ScAddress(0, 1, 1), u"=of:=CONVERT(1;[.H2];[.H3])"_ustr },
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/statistical/fods/fisher.fods"),
            ScAddress(0, 54, 1), u"=of:=FISHER([.K11])"_ustr },
        { m_directories.getPathFromSrc(
              u"/sc/qa/unit/data/functions/spreadsheet/fods/lookup.fods"),
            ScAddress(0, 33, 1), u"=of:=LOOKUP(1;1)"_ustr },
        { m_directories.getPathFromSrc(
              u"/sc/qa/unit/data/functions/spreadsheet/fods/match.fods"),
            ScAddress(0, 1, 1), u"=of:=MATCH(\"a\";range1)"_ustr },
        { m_directories.getPathFromSrc(
              u"/sc/qa/unit/data/functions/mathematical/fods/aggregate.fods"),
            ScAddress(0, 1, 1), u"=of:=COM.MICROSOFT.AGGREGATE(4; 6; [.J2:.J12])"_ustr },
        { m_directories.getPathFromSrc(
              u"/sc/qa/unit/data/functions/mathematical/fods/round.fods"),
            ScAddress(0, 1, 1), u"=of:=ROUND(2.348;2)"_ustr },
        { m_directories.getPathFromSrc(
              u"/sc/qa/unit/data/functions/date_time/fods/daysinmonth.fods"),
            ScAddress(0, 1, 1), u"=of:=ORG.OPENOFFICE.DAYSINMONTH(\"1990-01-01\")"_ustr },
    };

    for (const auto& rCase : aCases)
    {
        const std::string aWorkbookPathUtf8(rCase.maWorkbookPath.toUtf8().getStr());
        const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
        CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for broad imported host truth case",
            static_cast<bool>(aLoadResult));

        Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
        normalizeWorkbookSheetNamesForCalc(aWorkbook);

        ScDocShellRef xDocShell
            = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT
                             | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                             | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
        xDocShell->DoInitUnitTest();
        ScDocument& rDoc = xDocShell->GetDocument();
        (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

        ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
        ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
        CPPUNIT_ASSERT(pContext);

        ScFormulaCell* pFormula = rDoc.GetFormulaCell(rCase.maPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
        CPPUNIT_ASSERT(aFormulaSource.equalsIgnoreAsciiCase(rCase.maExpectedFormula));

        const auto aAttempt
            = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
                rDoc, *pContext, rCase.maPos,
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
                rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
                std::u16string_view(aCanonicalFormulaSource.getStr(),
                    aCanonicalFormulaSource.getLength()));
        CPPUNIT_ASSERT(aAttempt.mbSupported);
        const OString aCaseLabel
            = OUStringToOString(rCase.maExpectedFormula, RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(
            aCaseLabel.getStr(),
            spreadsheetengine::api::formulavalue::ValueType::Error,
            aAttempt.maResult.meType);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(
            aCaseLabel.getStr(),
            spreadsheetengine::api::Error::VariableExpected, aAttempt.maResult.meError);

        {
            ScopedEnvironmentOverride aOffMode(
                "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
            pFormula->SetDirty();
            pFormula->Interpret();
        }
        CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(rCase.maPos));
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedStoredValueHostTruth)
{
    const struct ImportedStoredValueCase
    {
        OUString maWorkbookPath;
        ScAddress maPos;
        OUString maFormulaFragment;
    } aCases[] = {
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/information/fods/na.fods"),
            ScAddress(0, 1, 1), u"NA()"_ustr },
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/addin/fods/imreal.fods"),
            ScAddress(0, 1, 1), u"IMREAL("_ustr },
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/addin/fods/imaginary.fods"),
            ScAddress(0, 1, 1), u"IMAGINARY("_ustr },
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/addin/fods/besseli.fods"),
            ScAddress(0, 1, 1), u"BESSELI("_ustr },
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/addin/fods/besselj.fods"),
            ScAddress(0, 1, 1), u"BESSELJ("_ustr },
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/addin/fods/besselk.fods"),
            ScAddress(0, 1, 1), u"BESSELK("_ustr },
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/addin/fods/bessely.fods"),
            ScAddress(0, 1, 1), u"BESSELY("_ustr },
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/addin/fods/dec2hex.fods"),
            ScAddress(0, 1, 1), u"DEC2HEX("_ustr },
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/date_time/fods/datedif.fods"),
            ScAddress(0, 1, 1), u"DATEDIF("_ustr },
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/financial/fods/vdb.fods"),
            ScAddress(0, 1, 1), u"VDB("_ustr },
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/financial/fods/price.fods"),
            ScAddress(0, 1, 1), u"PRICE("_ustr },
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/array/fods/sumproduct.fods"),
            ScAddress(0, 1, 1), u"SUMPRODUCT("_ustr },
    };

    auto assertAttemptMatchesHostValue = [](OUString const& rLabel,
                                            const spreadsheetengine::compat::libreoffice::interprettaileval::EvaluationAttempt& rAttempt,
                                            const auto& rHostValue) {
        const OString aLabel = OUStringToOString(rLabel, RTL_TEXTENCODING_UTF8);
        switch (rHostValue.maValue.meKind)
        {
            case spreadsheetengine::api::CellValueKind::Number:
            case spreadsheetengine::api::CellValueKind::Boolean:
                CPPUNIT_ASSERT_EQUAL_MESSAGE(
                    aLabel.getStr(),
                    spreadsheetengine::api::formulavalue::ValueType::Value,
                    rAttempt.maResult.meType);
                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
                    aLabel.getStr(),
                    rHostValue.maValue.mfNumber, rAttempt.maResult.mfValue, 1e-12);
                break;
            case spreadsheetengine::api::CellValueKind::Text:
                CPPUNIT_ASSERT_EQUAL_MESSAGE(
                    aLabel.getStr(),
                    spreadsheetengine::api::formulavalue::ValueType::String,
                    rAttempt.maResult.meType);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(
                    aLabel.getStr(),
                    spreadsheetengine::compat::libreoffice::toLibreOfficeString(
                        rHostValue.maValue.maString),
                    spreadsheetengine::compat::libreoffice::toLibreOfficeString(
                        rAttempt.maResult.maString));
                break;
            case spreadsheetengine::api::CellValueKind::Error:
                CPPUNIT_ASSERT_EQUAL_MESSAGE(
                    aLabel.getStr(),
                    spreadsheetengine::api::formulavalue::ValueType::Error,
                    rAttempt.maResult.meType);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(
                    aLabel.getStr(), rHostValue.maValue.meError, rAttempt.maResult.meError);
                break;
            case spreadsheetengine::api::CellValueKind::Empty:
                CPPUNIT_ASSERT_EQUAL_MESSAGE(
                    aLabel.getStr(),
                    spreadsheetengine::api::formulavalue::ValueType::String,
                    rAttempt.maResult.meType);
                CPPUNIT_ASSERT_MESSAGE(aLabel.getStr(), rAttempt.maResult.maString.empty());
                break;
        }
    };

    for (const auto& rCase : aCases)
    {
        const std::string aWorkbookPathUtf8(rCase.maWorkbookPath.toUtf8().getStr());
        const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
        CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for stored-value host truth case",
            static_cast<bool>(aLoadResult));

        Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
        normalizeWorkbookSheetNamesForCalc(aWorkbook);

        ScDocShellRef xDocShell
            = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT
                             | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                             | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
        xDocShell->DoInitUnitTest();
        ScDocument& rDoc = xDocShell->GetDocument();
        (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

        ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
        ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
        CPPUNIT_ASSERT(pContext);

        ScFormulaCell* pFormula = rDoc.GetFormulaCell(rCase.maPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();

        const auto aAttempt
            = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
                rDoc, *pContext, rCase.maPos,
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
                rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
                std::u16string_view(aCanonicalFormulaSource.getStr(),
                    aCanonicalFormulaSource.getLength()));
        CPPUNIT_ASSERT(aAttempt.mbSupported);

        const auto aHostValue
            = spreadsheetengine::compat::libreoffice::readHostDocumentCellValue(rDoc, rCase.maPos);
        CPPUNIT_ASSERT(aHostValue);
        assertAttemptMatchesHostValue(rCase.maFormulaFragment, aAttempt, aHostValue);
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedLookupArrayFormRangeParity)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/lookup.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for lookup.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const struct LookupArrayFormCase
    {
        ScAddress maPos;
        OUString maExpectedFormula;
        OUString maExpectedString;
    } aCases[] = {
        { ScAddress(0, 59, 1), u"=of:=LOOKUP([.K50];[.L49:.N56])"_ustr, u"0 2"_ustr }, // Sheet2.A60
        { ScAddress(0, 528, 1), u"=of:=LOOKUP([.H489];[.F489:.G528])"_ustr, u"Res8"_ustr }, // Sheet2.A529
    };

    for (const auto& rCase : aCases)
    {
        ScFormulaCell* pFormula = rDoc.GetFormulaCell(rCase.maPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
        CPPUNIT_ASSERT_EQUAL(rCase.maExpectedFormula, aFormulaSource);

        const auto aAttempt
            = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
                rDoc, *pContext, rCase.maPos,
                std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
                rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
                std::u16string_view(aCanonicalFormulaSource.getStr(),
                    aCanonicalFormulaSource.getLength()));
        CPPUNIT_ASSERT(aAttempt.mbSupported);
        CPPUNIT_ASSERT_EQUAL(
            spreadsheetengine::api::formulavalue::ValueType::String,
            aAttempt.maResult.meType);
        CPPUNIT_ASSERT_EQUAL(
            rCase.maExpectedString,
            spreadsheetengine::compat::libreoffice::toLibreOfficeString(
                aAttempt.maResult.maString));
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedLogicalConstantLiveHostTruth)
{
    const struct WorkbookLogicalRow
    {
        OUString maWorkbookPath;
        std::vector<std::pair<SCROW, FormulaError>> maRows;
    } aCases[] = {
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/logical/fods/ifs.fods"),
            { { SCROW(15), FormulaError::NoName } } }, // Sheet2.B16
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/logical/fods/switch.fods"),
            { { SCROW(10), FormulaError::NoName } } }, // Sheet2.B11
        { m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/text/fods/exact.fods"),
            { { SCROW(76), FormulaError::VariableExpected },
              { SCROW(77), FormulaError::VariableExpected },
              { SCROW(79), FormulaError::VariableExpected } } }, // Sheet2.B77/B78/B80
    };

    for (const auto& rCase : aCases)
    {
        const std::string aWorkbookPathUtf8(rCase.maWorkbookPath.toUtf8().getStr());
        const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
        CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for logical-constant host truth case",
            static_cast<bool>(aLoadResult));

        Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
        normalizeWorkbookSheetNamesForCalc(aWorkbook);

        ScDocShellRef xDocShell
            = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT
                             | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                             | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
        xDocShell->DoInitUnitTest();
        ScDocument& rDoc = xDocShell->GetDocument();
        (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

        ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
        ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
        CPPUNIT_ASSERT(pContext);

        for (const auto& [nRow, eExpectedError] : rCase.maRows)
        {
            const ScAddress aPos(1, nRow, 1);
            ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
            CPPUNIT_ASSERT(pFormula);

            const OUString aFormulaSource
                = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
            CPPUNIT_ASSERT(aFormulaSource.indexOf(u"TRUE()") >= 0);

            {
                ScopedEnvironmentOverride aOffMode(
                    "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
                pFormula->SetDirty();
                pFormula->Interpret();
            }

            CPPUNIT_ASSERT_EQUAL(eExpectedError, rDoc.GetErrCode(aPos));
        }
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedVLookupExactLiveHostTruth)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/vlookup.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for vlookup.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const struct ExactHostTruthCase
    {
        SCROW mnRow;
        OUString maExpectedFormula;
    } aCases[] = {
        { SCROW(6), u"=of:=VLOOKUP([.P6];[.$L$2:.$M$8];2;0)"_ustr }, // Sheet2.A7
        { SCROW(72), u"=of:=VLOOKUP(21;[.$AM$2:.$AN$4];2;0)"_ustr }, // Sheet2.A73
    };

    for (const auto& rCase : aCases)
    {
        const ScAddress aPos(0, rCase.mnRow, 1);
        ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        CPPUNIT_ASSERT_EQUAL(rCase.maExpectedFormula, aFormulaSource);

        {
            ScopedEnvironmentOverride aOffMode(
                "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
            pFormula->SetDirty();
            pFormula->Interpret();
        }

        CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(aPos));
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedVLookupCollationLiveHostTruth)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/vlookup.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for vlookup.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    const ScAddress aPos(0, 46, 1); // Sheet2.A47
    ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
    CPPUNIT_ASSERT(pFormula);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const OUString aFormulaSource
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
    CPPUNIT_ASSERT_EQUAL(u"=of:=VLOOKUP([.M22]; [.L$11:.M$32];1;0)"_ustr, aFormulaSource);

    {
        ScopedEnvironmentOverride aOffMode(
            "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
        pFormula->SetDirty();
        pFormula->Interpret();
    }

    CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(aPos));
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedXLookupLiveHostTruth)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/xlookup.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for xlookup.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const struct XLookupHostTruthCase
    {
        SCROW mnRow;
        OUString maExpectedFormula;
    } aCases[] = {
        { SCROW(5), u"=of:=COM.MICROSOFT.XLOOKUP(\"Ireland\";[.H2:.H11];[.J2:.J11];\"\")"_ustr }, // Sheet2.A6
        { SCROW(15), u"=of:=COM.MICROSOFT.XLOOKUP([.G14];[.I14:.R14];[.I15:.R16])"_ustr }, // Sheet2.A16
    };

    for (const auto& rCase : aCases)
    {
        const ScAddress aPos(0, rCase.mnRow, 1);
        ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
        CPPUNIT_ASSERT(pFormula);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        CPPUNIT_ASSERT_EQUAL(rCase.maExpectedFormula, aFormulaSource);

        {
            ScopedEnvironmentOverride aOffMode(
                "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
            pFormula->SetDirty();
            pFormula->Interpret();
        }
        CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(aPos));
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedIndexNestedXMatchLiveHostTruth)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/xmatch.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for xmatch.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    const ScAddress aPos(0, 7, 1); // Sheet2.A8
    ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
    CPPUNIT_ASSERT(pFormula);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const OUString aFormulaSource
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
    CPPUNIT_ASSERT_EQUAL(
        u"=of:=INDEX([.H13:.J19];COM.MICROSOFT.XMATCH([.G10];[.G13:.G19]);COM.MICROSOFT.XMATCH([.H10];[.H12:.J12]))"_ustr,
        aFormulaSource);

    {
        ScopedEnvironmentOverride aOffMode(
            "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
        pFormula->SetDirty();
        pFormula->Interpret();
    }

    CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(aPos));
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedIndexNestedXMatchParity)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/xmatch.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for xmatch.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const ScAddress aPos(0, 7, 1); // Sheet2.A8
    ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
    CPPUNIT_ASSERT(pFormula);

    const OUString aFormulaSource
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
    const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
    CPPUNIT_ASSERT_EQUAL(
        u"=of:=INDEX([.H13:.J19];COM.MICROSOFT.XMATCH([.G10];[.G13:.G19]);COM.MICROSOFT.XMATCH([.H10];[.H12:.J12]))"_ustr,
        aFormulaSource);

    const auto aAttempt
        = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
            rDoc, *pContext, aPos,
            std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
            rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
            std::u16string_view(aCanonicalFormulaSource.getStr(),
                aCanonicalFormulaSource.getLength()));
    CPPUNIT_ASSERT(aAttempt.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error, aAttempt.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::VariableExpected, aAttempt.maResult.meError);
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedIndexLogestLiveHostTruth)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/spreadsheet/fods/index.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for index.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    struct IndexLogestHostTruthCase
    {
        OUString maExpectedFormula;
        FormulaError meExpectedError;
        bool mbSeen = false;
    } aCases[] = {
        { u"=of:=INDEX(LOGEST([.K11:.O11];[.K12:.O12];TRUE();TRUE());2;1)"_ustr,
            FormulaError::VariableExpected },
        { u"=of:=INDEX(LOGEST([.K11:.O11];[.K12:.O12];TRUE();TRUE());2;2)"_ustr,
            FormulaError::VariableExpected },
        { u"=of:=INDEX(LOGEST([.K11:.O11];[.K12:.O12];TRUE();TRUE());2;0)"_ustr,
            FormulaError::VariableExpected },
    };

    for (std::size_t nSheet = 0; nSheet < aWorkbook.maSheets.size(); ++nSheet)
    {
        for (const auto& rEntry : aWorkbook.maSheets[nSheet].maCells)
        {
            const Cell& rCell = rEntry.second;
            if (!rCell.hasFormula())
                continue;

            const ScAddress aPos(static_cast<SCCOL>(rEntry.first.first),
                static_cast<SCROW>(rEntry.first.second), static_cast<SCTAB>(nSheet));
            ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
            if (!pFormula)
                continue;

            const OUString aFormulaSource
                = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
            for (auto& rCase : aCases)
            {
                if (aFormulaSource != rCase.maExpectedFormula)
                    continue;

                rCase.mbSeen = true;
                {
                    ScopedEnvironmentOverride aOffMode(
                        "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
                    pFormula->SetDirty();
                    pFormula->Interpret();
                }

                CPPUNIT_ASSERT_EQUAL(rCase.meExpectedError, rDoc.GetErrCode(aPos));
            }
        }
    }

    for (const auto& rCase : aCases)
    {
        const OUString aMessage
            = u"expected imported INDEX(LOGEST) formula not found: "_ustr
              + rCase.maExpectedFormula;
        CPPUNIT_ASSERT_MESSAGE(
            OUStringToOString(aMessage, RTL_TEXTENCODING_UTF8).getStr(),
            rCase.mbSeen);
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedDateValueMonthNameLiveHostTruth)
{
    const struct DateValueHostTruthWorkbook
    {
        OUString maWorkbookPath;
    } aWorkbooks[] = {
        { m_directories.getPathFromSrc(
            u"/sc/qa/unit/data/functions/date_time/fods/datevalue.fods") },
        { m_directories.getPathFromSrc(
            u"/sc/qa/unit/data/functions/date_time/fods/day.fods") },
    };

    for (const auto& rWorkbookCase : aWorkbooks)
    {
        const std::string aWorkbookPathUtf8(rWorkbookCase.maWorkbookPath.toUtf8().getStr());
        const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
        CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for DATEVALUE host truth workbook",
            static_cast<bool>(aLoadResult));

        Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
        normalizeWorkbookSheetNamesForCalc(aWorkbook);

        ScDocShellRef xDocShell
            = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                             | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
        xDocShell->DoInitUnitTest();
        ScDocument& rDoc = xDocShell->GetDocument();
        (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

        const ScAddress aPos(0, 3, 1); // Sheet2.A4
        ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
        CPPUNIT_ASSERT(pFormula);

        ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
        ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
        CPPUNIT_ASSERT(pContext);

        const OUString aFormulaSource
            = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
        CPPUNIT_ASSERT_EQUAL(u"=of:=DATEVALUE(\"Jan1, 2015\")"_ustr, aFormulaSource);

        {
            ScopedEnvironmentOverride aOffMode(
                "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
            pFormula->SetDirty();
            pFormula->Interpret();
        }

        CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(aPos));
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedBusinessDayLongSpanParity)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/date_time/fods/workday.intl.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for workday.intl.fods",
        static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    const ScAddress aPos(0, 25, 1); // Sheet2.A26
    ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
    CPPUNIT_ASSERT(pFormula);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const OUString aFormulaSource
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
    const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
    CPPUNIT_ASSERT_EQUAL(
        u"=of:=COM.MICROSOFT.WORKDAY.INTL(DATE(2012;1;1);90;11)"_ustr, aFormulaSource);

    const auto aAttempt
        = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
            rDoc, *pContext, aPos,
            std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
            rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
            std::u16string_view(aCanonicalFormulaSource.getStr(),
                aCanonicalFormulaSource.getLength()));
    CPPUNIT_ASSERT(aAttempt.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error, aAttempt.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::VariableExpected, aAttempt.maResult.meError);
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedBusinessDayLongSpanLiveHostTruth)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/date_time/fods/workday.intl.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for workday.intl.fods",
        static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    const ScAddress aPos(0, 25, 1); // Sheet2.A26
    ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
    CPPUNIT_ASSERT(pFormula);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    const OUString aFormulaSource
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
    CPPUNIT_ASSERT_EQUAL(
        u"=of:=COM.MICROSOFT.WORKDAY.INTL(DATE(2012;1;1);90;11)"_ustr, aFormulaSource);

    {
        ScopedEnvironmentOverride aOffMode(
            "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
        pFormula->SetDirty();
        pFormula->Interpret();
    }

    CPPUNIT_ASSERT_EQUAL(FormulaError::VariableExpected, rDoc.GetErrCode(aPos));
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testImportedDateValueReferencedFormulaParity)
{
    const OUString aWorkbookPath
        = m_directories.getPathFromSrc(u"/sc/qa/unit/data/functions/date_time/fods/hour.fods");
    const std::string aWorkbookPathUtf8(aWorkbookPath.toUtf8().getStr());
    const auto aLoadResult = loadWorkbook(aWorkbookPathUtf8);
    CPPUNIT_ASSERT_MESSAGE("loadWorkbook failed for hour.fods", static_cast<bool>(aLoadResult));

    Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
    normalizeWorkbookSheetNamesForCalc(aWorkbook);

    ScDocShellRef xDocShell
        = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                         | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
    xDocShell->DoInitUnitTest();
    ScDocument& rDoc = xDocShell->GetDocument();
    (void)materializeWorkbookToCalc(aWorkbook, rDoc, aWorkbookPathUtf8);

    ScInterpreterContextGetterGuard aContextGetterGuard(rDoc, rDoc.GetFormatTable());
    ScInterpreterContext* pContext = aContextGetterGuard.GetInterpreterContext();
    CPPUNIT_ASSERT(pContext);

    struct ImportedDateValueReferenceCase
    {
        OUString maExpectedFormula;
        double mfExpectedValue;
        bool mbSeen = false;
    } aCases[] = {
        { u"=of:=DATEVALUE([.A2])"_ustr, -4.0 },
        { u"=of:=DATEVALUE([.A24])"_ustr, 2.0 },
    };

    for (std::size_t nSheet = 0; nSheet < aWorkbook.maSheets.size(); ++nSheet)
    {
        for (const auto& rEntry : aWorkbook.maSheets[nSheet].maCells)
        {
            const Cell& rCell = rEntry.second;
            if (!rCell.hasFormula())
                continue;

            const ScAddress aPos(static_cast<SCCOL>(rEntry.first.first),
                static_cast<SCROW>(rEntry.first.second), static_cast<SCTAB>(nSheet));
            ScFormulaCell* pFormula = rDoc.GetFormulaCell(aPos);
            if (!pFormula)
                continue;

            const OUString aFormulaSource
                = pFormula->GetFormula(formula::FormulaGrammar::GRAM_ODFF, pContext);
            const OUString aCanonicalFormulaSource = pFormula->GetHybridFormula();
            for (auto& rCase : aCases)
            {
                if (aFormulaSource != rCase.maExpectedFormula)
                    continue;

                rCase.mbSeen = true;
                const auto aAttempt
                    = spreadsheetengine::compat::libreoffice::interprettaileval::tryEvaluateFormula(
                        rDoc, *pContext, aPos,
                        std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
                        rDoc.GetCalcConfig().mbEmptyStringAsZero, pFormula->GetCode(),
                        std::u16string_view(aCanonicalFormulaSource.getStr(),
                            aCanonicalFormulaSource.getLength()));
                CPPUNIT_ASSERT(aAttempt.mbSupported);
                CPPUNIT_ASSERT_EQUAL(
                    spreadsheetengine::api::formulavalue::ValueType::Value,
                    aAttempt.maResult.meType);
                CPPUNIT_ASSERT_DOUBLES_EQUAL(
                    rCase.mfExpectedValue, aAttempt.maResult.mfValue, 1e-12);
            }
        }
    }

    for (const auto& rCase : aCases)
    {
        const OUString aMessage
            = u"expected imported DATEVALUE reference formula not found: "_ustr
              + rCase.maExpectedFormula;
        CPPUNIT_ASSERT_MESSAGE(
            OUStringToOString(aMessage, RTL_TEXTENCODING_UTF8).getStr(), rCase.mbSeen);
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testAuthorityStats)
{
    if (!envEnabled("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_STATS"))
        return;

    const auto aCorpus = collectDefaultReplayCorpus();
    CPPUNIT_ASSERT_MESSAGE("replay corpus should not be empty", !aCorpus.empty());

    StatsSnapshot aLiveStats;
    StatsSnapshot aForcedInterpretStats;
    StatsSnapshot aProbeStats;
    StatsSnapshot aLiveAuthoritativeProbeStats;
    StatsSnapshot aLiveTargetProbeStats;
    ReplayEligibilityInventory aReplayEligibilityInventory;
    ObserveSurfaceInventory aLiveUniqueInventory;
    ObserveSurfaceInventory aForcedDirectInventory;
    std::vector<DiagnosticSample> aLiveDiagnosticSamples;
    resetProbeDiagnosticSamples();
    resetReplayEligibilityDiagnosticSamples();
    spreadsheetengine::core::datetime::resetWorkdayRuntimeStats();
    ScInterpreterDispatchRuntimeStatsSnapshot aLiveDispatchRuntimeStats;
    ScInterpreterDispatchRuntimeStatsSnapshot aCoreForcedSeamDisabledDispatchRuntimeStats;
    ScInterpreterReachabilityStatsSnapshot aLiveReachabilityStats;
    ScInterpreterReachabilityStatsSnapshot aCoreForcedSeamDisabledReachabilityStats;

    std::size_t nWorkbookCount = 0;
    std::size_t nFormulaCellCount = 0;
    std::size_t nProbeFormulaCount = 0;
    std::size_t nLiveAuthoritativeProbeFormulaCount = 0;
    std::size_t nLiveTargetProbeFormulaCount = 0;
    std::size_t nProbeHostTruthArtifactFormulaCount = 0;
    std::array<std::size_t, static_cast<std::size_t>(FunctionKind::Count)>
        aProbeHostTruthArtifactFunctionCount {};
    for (const auto& rWorkbookPath : aCorpus)
    {
        const auto aLoadResult = loadWorkbook(rWorkbookPath.string());
        CPPUNIT_ASSERT_MESSAGE(("loadWorkbook failed for " + rWorkbookPath.string()).c_str(),
            static_cast<bool>(aLoadResult));
        Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
        normalizeWorkbookSheetNamesForCalc(aWorkbook);
        const ScRangeList aWorkbookRanges = collectWorkbookUsedRanges(aWorkbook);

        ScDocShellRef xDocShell
            = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                             | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
        xDocShell->DoInitUnitTest();
        ScDocument& rDoc = xDocShell->GetDocument();

        nFormulaCellCount += materializeWorkbookToCalc(
            aWorkbook, rDoc, rWorkbookPath.string());

        {
            sc::AutoCalcSwitch aCalcSwitch(rDoc, true);
            {
                ScopedEnvironmentOverride aObserveMode(
                    "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "observe");
                resetScInterpreterDispatchRuntimeStats();
                resetScInterpreterReachabilityStats();
                spreadsheetengine::compat::libreoffice::interprettaileval::resetStats();
                spreadsheetengine::compat::libreoffice::interprettaileval::setDiagnosticWorkbookLabel(
                    OUString::fromUtf8(rWorkbookPath.string()));
                sc::SetFormulaDirtyContext aDirtyCxt;
                rDoc.SetAllFormulasDirty(aDirtyCxt);
                rDoc.InterpretCellsIfNeeded(aWorkbookRanges);
                xDocShell->DoHardRecalc();
                accumulateDispatchRuntimeStats(aLiveDispatchRuntimeStats,
                    getScInterpreterDispatchRuntimeStatsSnapshot());
                accumulateReachabilityStats(aLiveReachabilityStats,
                    getScInterpreterReachabilityStatsSnapshot());
                const StatsSnapshot aWorkbookLiveAttemptStats
                    = spreadsheetengine::compat::libreoffice::interprettaileval::getStatsSnapshot();
                const auto aWorkbookLiveInventory = buildObserveSurfaceInventory(
                    aWorkbook, rDoc,
                    spreadsheetengine::compat::libreoffice::interprettaileval::getObservedFormulaCellStatuses(),
                    aWorkbookLiveAttemptStats);
                appendDiagnosticSamples(aLiveDiagnosticSamples,
                    spreadsheetengine::compat::libreoffice::interprettaileval::getDiagnosticSamples());
                spreadsheetengine::compat::libreoffice::interprettaileval::setDiagnosticWorkbookLabel(
                    OUString());
                accumulateStats(aLiveStats, aWorkbookLiveAttemptStats);
                aLiveUniqueInventory.mnFormulaCells += aWorkbookLiveInventory.mnFormulaCells;
                aLiveUniqueInventory.mnSeenFormulaCells += aWorkbookLiveInventory.mnSeenFormulaCells;
                aLiveUniqueInventory.mnSupportedFormulaCells
                    += aWorkbookLiveInventory.mnSupportedFormulaCells;
                aLiveUniqueInventory.mnFallbackFormulaCells
                    += aWorkbookLiveInventory.mnFallbackFormulaCells;
                aLiveUniqueInventory.mnUnsupportedFunctionFormulaCells
                    += aWorkbookLiveInventory.mnUnsupportedFunctionFormulaCells;
                aLiveUniqueInventory.mnUnseenFormulaCells
                    += aWorkbookLiveInventory.mnUnseenFormulaCells;
                for (std::size_t nIndex = 0;
                     nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
                {
                    aLiveUniqueInventory.maFunctionFormulaCells[nIndex]
                        += aWorkbookLiveInventory.maFunctionFormulaCells[nIndex];
                    aLiveUniqueInventory.maFunctionSeenCells[nIndex]
                        += aWorkbookLiveInventory.maFunctionSeenCells[nIndex];
                    aLiveUniqueInventory.maFunctionSupportedCells[nIndex]
                        += aWorkbookLiveInventory.maFunctionSupportedCells[nIndex];
                    aLiveUniqueInventory.maFunctionFallbackCells[nIndex]
                        += aWorkbookLiveInventory.maFunctionFallbackCells[nIndex];
                    aLiveUniqueInventory.maFunctionUnsupportedFunctionCells[nIndex]
                        += aWorkbookLiveInventory.maFunctionUnsupportedFunctionCells[nIndex];
                    aLiveUniqueInventory.maFunctionUnseenCells[nIndex]
                        += aWorkbookLiveInventory.maFunctionUnseenCells[nIndex];
                }
                accumulateUnknownRootInventory(aLiveUniqueInventory, aWorkbookLiveInventory);
            }

            {
                ScopedEnvironmentOverride aOffMode(
                    "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
                ScopedEnvironmentOverride aCoreMode("SC_FORCE_CALCULATION", "core");
                ScopedEnvironmentOverride aDisableAuthorityWhileOff(
                    "SPREADSHEET_ENGINE_INTERPRET_TAIL_AUTHORITATIVE_WHILE_OFF", "0");
                resetScInterpreterDispatchRuntimeStats();
                resetScInterpreterReachabilityStats();
                sc::SetFormulaDirtyContext aDirtyCxt;
                rDoc.SetAllFormulasDirty(aDirtyCxt);
                rDoc.InterpretCellsIfNeeded(aWorkbookRanges);
                xDocShell->DoHardRecalc();
                accumulateDispatchRuntimeStats(aCoreForcedSeamDisabledDispatchRuntimeStats,
                    getScInterpreterDispatchRuntimeStatsSnapshot());
                accumulateReachabilityStats(aCoreForcedSeamDisabledReachabilityStats,
                    getScInterpreterReachabilityStatsSnapshot());
            }

            {
                ScopedEnvironmentOverride aObserveMode(
                    "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "observe");
                spreadsheetengine::compat::libreoffice::interprettaileval::resetStats();
                spreadsheetengine::compat::libreoffice::interprettaileval::setDiagnosticWorkbookLabel(
                    OUString::fromUtf8(rWorkbookPath.string()));
                sc::SetFormulaDirtyContext aDirtyCxt;
                rDoc.SetAllFormulasDirty(aDirtyCxt);
                const auto aWorkbookForcedDirectInventory
                    = runForcedInterpretObserveSurface(aWorkbook, rDoc);
                appendDiagnosticSamples(aLiveDiagnosticSamples,
                    spreadsheetengine::compat::libreoffice::interprettaileval::getDiagnosticSamples());
                spreadsheetengine::compat::libreoffice::interprettaileval::setDiagnosticWorkbookLabel(
                    OUString());
                accumulateStats(aForcedInterpretStats, aWorkbookForcedDirectInventory.maAttemptStats);
                aForcedDirectInventory.mnFormulaCells
                    += aWorkbookForcedDirectInventory.mnFormulaCells;
                aForcedDirectInventory.mnSeenFormulaCells
                    += aWorkbookForcedDirectInventory.mnSeenFormulaCells;
                aForcedDirectInventory.mnSupportedFormulaCells
                    += aWorkbookForcedDirectInventory.mnSupportedFormulaCells;
                aForcedDirectInventory.mnFallbackFormulaCells
                    += aWorkbookForcedDirectInventory.mnFallbackFormulaCells;
                aForcedDirectInventory.mnUnsupportedFunctionFormulaCells
                    += aWorkbookForcedDirectInventory.mnUnsupportedFunctionFormulaCells;
                aForcedDirectInventory.mnUnseenFormulaCells
                    += aWorkbookForcedDirectInventory.mnUnseenFormulaCells;
                for (std::size_t nIndex = 0;
                     nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
                {
                    aForcedDirectInventory.maFunctionFormulaCells[nIndex]
                        += aWorkbookForcedDirectInventory.maFunctionFormulaCells[nIndex];
                    aForcedDirectInventory.maFunctionSeenCells[nIndex]
                        += aWorkbookForcedDirectInventory.maFunctionSeenCells[nIndex];
                    aForcedDirectInventory.maFunctionSupportedCells[nIndex]
                        += aWorkbookForcedDirectInventory.maFunctionSupportedCells[nIndex];
                    aForcedDirectInventory.maFunctionFallbackCells[nIndex]
                        += aWorkbookForcedDirectInventory.maFunctionFallbackCells[nIndex];
                    aForcedDirectInventory.maFunctionUnsupportedFunctionCells[nIndex]
                        += aWorkbookForcedDirectInventory.maFunctionUnsupportedFunctionCells[nIndex];
                    aForcedDirectInventory.maFunctionUnseenCells[nIndex]
                        += aWorkbookForcedDirectInventory.maFunctionUnseenCells[nIndex];
                }
                accumulateUnknownRootInventory(
                    aForcedDirectInventory, aWorkbookForcedDirectInventory);
            }

            {
                ScopedEnvironmentOverride aProbeMode(
                    "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "off");
                const auto aProbeRun = runSupportedInterpretTailProbe(
                    aWorkbook, rDoc, OUString::fromUtf8(rWorkbookPath.string()));
                nProbeFormulaCount += aProbeRun.mnRawFormulaCount;
                nLiveAuthoritativeProbeFormulaCount += aProbeRun.mnLiveAuthoritativeFormulaCount;
                nLiveTargetProbeFormulaCount += aProbeRun.mnLiveTargetFormulaCount;
                nProbeHostTruthArtifactFormulaCount += aProbeRun.mnHostTruthArtifactFormulaCount;
                accumulateStats(aProbeStats, aProbeRun.maRawStats);
                accumulateStats(aLiveAuthoritativeProbeStats, aProbeRun.maLiveAuthoritativeStats);
                accumulateStats(aLiveTargetProbeStats, aProbeRun.maLiveTargetStats);
                for (std::size_t nIndex = 0;
                     nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
                {
                    aProbeHostTruthArtifactFunctionCount[nIndex]
                        += aProbeRun.maHostTruthArtifactFunctionCount[nIndex];
                }
            }

            {
                ScopedEnvironmentOverride aObserveMode(
                    "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "observe");
                spreadsheetengine::compat::libreoffice::interprettaileval::resetStats();
                auto aWorkbookInventory = runPromotedReplayEligibilityInventory(
                    aWorkbook, rDoc, OUString::fromUtf8(rWorkbookPath.string()));

                aReplayEligibilityInventory.mnPromotedFormulaCells
                    += aWorkbookInventory.mnPromotedFormulaCells;
                aReplayEligibilityInventory.mnSharedFormulaCells
                    += aWorkbookInventory.mnSharedFormulaCells;
                aReplayEligibilityInventory.mnSharedTopFormulaCells
                    += aWorkbookInventory.mnSharedTopFormulaCells;
                aReplayEligibilityInventory.mnSharedMemberFormulaCells
                    += aWorkbookInventory.mnSharedMemberFormulaCells;
                aReplayEligibilityInventory.mnNonSharedFormulaCells
                    += aWorkbookInventory.mnNonSharedFormulaCells;
                aReplayEligibilityInventory.mnNeedsInterpretBeforeDirty
                    += aWorkbookInventory.mnNeedsInterpretBeforeDirty;
                aReplayEligibilityInventory.mnNeedsInterpretAfterDirty
                    += aWorkbookInventory.mnNeedsInterpretAfterDirty;
                aReplayEligibilityInventory.mnDirectSeen += aWorkbookInventory.mnDirectSeen;
                aReplayEligibilityInventory.mnDirectSupported
                    += aWorkbookInventory.mnDirectSupported;
                aReplayEligibilityInventory.mnDirectFallback
                    += aWorkbookInventory.mnDirectFallback;
                aReplayEligibilityInventory.mnDirectUnseen += aWorkbookInventory.mnDirectUnseen;
                aReplayEligibilityInventory.mnInterpretReturnedFalse
                    += aWorkbookInventory.mnInterpretReturnedFalse;
                aReplayEligibilityInventory.mnDirtyAfterInterpret
                    += aWorkbookInventory.mnDirtyAfterInterpret;
                aReplayEligibilityInventory.mnNeedsInterpretAfterInterpret
                    += aWorkbookInventory.mnNeedsInterpretAfterInterpret;
                aReplayEligibilityInventory.mnSharedMemberSeenViaTop
                    += aWorkbookInventory.mnSharedMemberSeenViaTop;
                aReplayEligibilityInventory.mnSharedMemberFallbackViaTop
                    += aWorkbookInventory.mnSharedMemberFallbackViaTop;
                aReplayEligibilityInventory.mnSharedMemberStillUnseenViaTop
                    += aWorkbookInventory.mnSharedMemberStillUnseenViaTop;
                aReplayEligibilityInventory.mnUnseenSharedTop
                    += aWorkbookInventory.mnUnseenSharedTop;
                aReplayEligibilityInventory.mnUnseenSharedMember
                    += aWorkbookInventory.mnUnseenSharedMember;
                aReplayEligibilityInventory.mnUnseenNonShared
                    += aWorkbookInventory.mnUnseenNonShared;
                aReplayEligibilityInventory.mnMatrixFormulaCells
                    += aWorkbookInventory.mnMatrixFormulaCells;
                aReplayEligibilityInventory.mnHyperLinkFormulaCells
                    += aWorkbookInventory.mnHyperLinkFormulaCells;
                aReplayEligibilityInventory.mnMissingCodeFormulaCells
                    += aWorkbookInventory.mnMissingCodeFormulaCells;
                aReplayEligibilityInventory.mnRawErrorFormulaCells
                    += aWorkbookInventory.mnRawErrorFormulaCells;

                for (std::size_t nIndex = 0;
                     nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
                {
                    aReplayEligibilityInventory.maFunctionFormulaCells[nIndex]
                        += aWorkbookInventory.maFunctionFormulaCells[nIndex];
                    aReplayEligibilityInventory.maFunctionDirectSeen[nIndex]
                        += aWorkbookInventory.maFunctionDirectSeen[nIndex];
                    aReplayEligibilityInventory.maFunctionDirectUnseen[nIndex]
                        += aWorkbookInventory.maFunctionDirectUnseen[nIndex];
                }
            }
        }

        xDocShell->DoClose();
        ++nWorkbookCount;
    }

    printRoutingStats("interpret_tail_live", nFormulaCellCount, aLiveStats);
    printObserveSurfaceInventory("interpret_tail_live_unique", aLiveUniqueInventory);
    printTopUnsupportedFunctionInventory("interpret_tail_live_unique", aLiveUniqueInventory);
    printTopUnknownRootInventory("interpret_tail_live_unique", aLiveUniqueInventory);
    printTopUnknownRootSurfaceInventory("interpret_tail_live_unique", aLiveUniqueInventory);
    printRoutingStats("interpret_tail_forced_interpret", aForcedDirectInventory.mnFormulaCells,
        aForcedInterpretStats);
    printObserveSurfaceInventory("interpret_tail_forced_direct", aForcedDirectInventory);
    printTopUnsupportedFunctionInventory("interpret_tail_forced_direct", aForcedDirectInventory);
    printTopUnknownRootInventory("interpret_tail_forced_direct", aForcedDirectInventory);
    printTopUnknownRootSurfaceInventory("interpret_tail_forced_direct", aForcedDirectInventory);
    printStats(nWorkbookCount, nFormulaCellCount, aProbeStats);
    std::cout << "interpret_tail_probe_formula_cells=" << nProbeFormulaCount << '\n';
    const std::size_t nLegacyInterpreterSubroutineCount = countLegacyInterpreterSubroutines();
    CPPUNIT_ASSERT_MESSAGE("legacy interpreter subroutine metric should scan interpre.hxx",
        nLegacyInterpreterSubroutineCount > 0);
    const auto aLegacyLambdaInventory = countInterp4LegacyLambdas();
    CPPUNIT_ASSERT_MESSAGE("interp4 legacy lambda metric should scan interpr4.cxx",
        aLegacyLambdaInventory.mnLambdaCount > 0);
    CPPUNIT_ASSERT_EQUAL(std::size_t(0),
        aLegacyLambdaInventory.mnQuarantineMissingDispatchLambdaCount);
    const auto aEngineDispatchInventory = countInterp4EngineDispatchAttempts();
    CPPUNIT_ASSERT_MESSAGE("interp4 engine attempt metric should scan interpr4.cxx",
        aEngineDispatchInventory.mnAttemptCaseCount > 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        "core-forced full-legacy engine dispatch accounting should stay balanced",
        aCoreForcedSeamDisabledDispatchRuntimeStats.mnEngineAttemptedCount,
        aCoreForcedSeamDisabledDispatchRuntimeStats.mnEngineSucceededCount
            + aCoreForcedSeamDisabledDispatchRuntimeStats.mnEngineDeclinedCount);
    {
        SupportedProbeRun aPrintedProbeRun;
        aPrintedProbeRun.mnRawFormulaCount = nProbeFormulaCount;
        aPrintedProbeRun.mnLiveAuthoritativeFormulaCount = nLiveAuthoritativeProbeFormulaCount;
        aPrintedProbeRun.maLiveAuthoritativeStats = aLiveAuthoritativeProbeStats;
        aPrintedProbeRun.mnLiveTargetFormulaCount = nLiveTargetProbeFormulaCount;
        aPrintedProbeRun.mnHostTruthArtifactFormulaCount = nProbeHostTruthArtifactFormulaCount;
        aPrintedProbeRun.maLiveTargetStats = aLiveTargetProbeStats;
        aPrintedProbeRun.maHostTruthArtifactFunctionCount = aProbeHostTruthArtifactFunctionCount;
        printLiveAuthoritativeSummary(
            nFormulaCellCount, aPrintedProbeRun, nLegacyInterpreterSubroutineCount,
            aLegacyLambdaInventory, aEngineDispatchInventory, aLiveDispatchRuntimeStats,
            aCoreForcedSeamDisabledDispatchRuntimeStats, aLiveReachabilityStats,
            aCoreForcedSeamDisabledReachabilityStats);
        printLiveTargetProbeSummary(aPrintedProbeRun);
    }
    printReplayEligibilityInventory(aReplayEligibilityInventory);
    printWorkdayRuntimeStats();
    printDiagnosticSamples(aLiveDiagnosticSamples);
    printProbeDiagnosticSamples();
    printReplayEligibilityDiagnosticSamples();

    CPPUNIT_ASSERT_EQUAL(aCorpus.size(), nWorkbookCount);
    CPPUNIT_ASSERT_MESSAGE("all-formula InterpretTail live observe should see at least one formula",
        aLiveStats.mnObserveCount + totalFallbackCount(aLiveStats) > 0);
    CPPUNIT_ASSERT_MESSAGE(
        "all-formula InterpretTail live observe should now surface promoted-family support",
        promotedFunctionSupportedCount(aLiveStats) > 0);
    CPPUNIT_ASSERT_MESSAGE(
        "full replay live unique inventory should see at least one formula cell",
        aLiveUniqueInventory.mnSeenFormulaCells > 0);
    CPPUNIT_ASSERT_MESSAGE(
        "full replay live unique inventory should support at least one formula cell",
        aLiveUniqueInventory.mnSupportedFormulaCells > 0);
    CPPUNIT_ASSERT_MESSAGE(
        "full replay live observe should reach formula-cell interpretation",
        aLiveReachabilityStats.mnFormulaCellInterpretCount > 0);
    CPPUNIT_ASSERT_MESSAGE(
        "full replay live unique inventory should cover every formula cell in the corpus",
        aLiveUniqueInventory.mnFormulaCells == nFormulaCellCount);
    CPPUNIT_ASSERT_EQUAL(
        aLiveUniqueInventory.mnFormulaCells,
        aLiveUniqueInventory.mnSeenFormulaCells + aLiveUniqueInventory.mnUnseenFormulaCells);
    CPPUNIT_ASSERT_MESSAGE(
        "full replay live unique supported cells should be a subset of seen cells",
        aLiveUniqueInventory.mnSupportedFormulaCells <= aLiveUniqueInventory.mnSeenFormulaCells);
    CPPUNIT_ASSERT_MESSAGE(
        "core-forced full-legacy replay should reach classic ScInterpreter::Interpret()",
        aCoreForcedSeamDisabledReachabilityStats.mnClassicInterpretCount > 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        "core forcing should prevent formula-group handling on the full-legacy replay lane",
        sal_uInt64(0),
        aCoreForcedSeamDisabledReachabilityStats.mnFormulaGroupHandledCount);
    CPPUNIT_ASSERT_MESSAGE(
        "full replay forced-interpret observe should touch every formula cell in the corpus",
        aForcedDirectInventory.mnFormulaCells == nFormulaCellCount);
    CPPUNIT_ASSERT_MESSAGE(
        "full replay forced-interpret observe should now surface promoted-family support",
        promotedFunctionSupportedCount(aForcedInterpretStats) > 0);
    CPPUNIT_ASSERT_MESSAGE(
        "full replay forced-direct inventory should see at least one formula cell",
        aForcedDirectInventory.mnSeenFormulaCells > 0);
    CPPUNIT_ASSERT_MESSAGE(
        "full replay forced-direct inventory should support at least one formula cell",
        aForcedDirectInventory.mnSupportedFormulaCells > 0);
    CPPUNIT_ASSERT_EQUAL(
        aForcedDirectInventory.mnFormulaCells,
        aForcedDirectInventory.mnSeenFormulaCells + aForcedDirectInventory.mnUnseenFormulaCells);
    CPPUNIT_ASSERT_MESSAGE(
        "full replay forced-direct supported cells should be a subset of seen cells",
        aForcedDirectInventory.mnSupportedFormulaCells <= aForcedDirectInventory.mnSeenFormulaCells);
    CPPUNIT_ASSERT_MESSAGE("supported InterpretTail corpus probe should visit at least one cell",
        nProbeFormulaCount > 0);
    CPPUNIT_ASSERT_EQUAL(nProbeFormulaCount, nLiveAuthoritativeProbeFormulaCount);
    CPPUNIT_ASSERT_MESSAGE(
        "supported InterpretTail corpus probe should either record authoritative usage or be "
        "fully classified as imported host-truth artifacts",
        aProbeStats.mnAuthoritativeCount > 0
            || nProbeHostTruthArtifactFormulaCount == nProbeFormulaCount);
    CPPUNIT_ASSERT_MESSAGE(
        "live authoritative-match probe should record at least one authoritative match",
        aLiveAuthoritativeProbeStats.mnAuthoritativeCount > 0);
    CPPUNIT_ASSERT_MESSAGE(
        "live-target filtered probe plus imported host-truth artifact count should partition the probe surface",
        nLiveTargetProbeFormulaCount + nProbeHostTruthArtifactFormulaCount == nProbeFormulaCount);
    CPPUNIT_ASSERT_MESSAGE(
        "live-target filtering should identify imported host-truth artifacts on the replay probe",
        nProbeHostTruthArtifactFormulaCount > 0);
    CPPUNIT_ASSERT_EQUAL(nProbeFormulaCount, aReplayEligibilityInventory.mnPromotedFormulaCells);
    CPPUNIT_ASSERT_MESSAGE("supported InterpretTail corpus probe should attempt at least one promoted family",
        aProbeStats.maFunctionAuthoritativeCount[static_cast<std::size_t>(FunctionKind::Value)]
                + aProbeStats.maFunctionFallbackCount[static_cast<std::size_t>(FunctionKind::Value)]
                + aProbeStats.maFunctionAuthoritativeCount[static_cast<std::size_t>(FunctionKind::Match)]
                + aProbeStats.maFunctionFallbackCount[static_cast<std::size_t>(FunctionKind::Match)]
                + aProbeStats.maFunctionAuthoritativeCount[static_cast<std::size_t>(FunctionKind::Lookup)]
                + aProbeStats.maFunctionFallbackCount[static_cast<std::size_t>(FunctionKind::Lookup)]
            > 0);
    CPPUNIT_ASSERT_MESSAGE(
        "replay eligibility inventory should record direct live reach for promoted replay cells",
        aReplayEligibilityInventory.mnDirectSeen > 0);
}

} // namespace

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
