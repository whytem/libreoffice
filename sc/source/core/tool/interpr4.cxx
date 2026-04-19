/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <config_features.h>

#include <interpre.hxx>

#include <sal/log.hxx>
#include <o3tl/safeint.hxx>
#include <rtl/math.hxx>
#include <sfx2/app.hxx>
#include <sfx2/objsh.hxx>
#include <basic/sbmeth.hxx>
#include <basic/sbmod.hxx>
#include <basic/sbstar.hxx>
#include <basic/sbx.hxx>
#include <basic/sbxobj.hxx>
#include <basic/sbuno.hxx>
#include <osl/thread.h>
#include <atomic>
#include <spreadsheetengine/detail/ExecutionContext.hxx>
#include <svl/numformat.hxx>
#include <svl/zforlist.hxx>
#include <svl/sharedstringpool.hxx>
#include <editeng/langitem.hxx>
#include <unotools/textsearch.hxx>
#include <unotools/charclass.hxx>
#include <rtl/character.hxx>
#include <rtl/ustring.hxx>
#include <unicode/uchar.h>
#include <unicode/regex.h>
#include <i18nlangtag/mslangid.hxx>
#include <stdlib.h>
#include <string.h>
#include <mutex>

#include <com/sun/star/table/XCellRange.hpp>
#include <com/sun/star/script/XInvocation.hpp>
#include <com/sun/star/sheet/XSheetCellRange.hpp>

#include <global.hxx>
#include <dbdata.hxx>
#include <formulacell.hxx>
#include <callform.hxx>
#include <addincol.hxx>
#include <document.hxx>
#include <dociter.hxx>
#include <docsh.hxx>
#include <docoptio.hxx>
#include <scmatrix.hxx>
#include <adiasync.hxx>
#include <cellsuno.hxx>
#include <optuno.hxx>
#include <rangeseq.hxx>
#include <addinlis.hxx>
#include <jumpmatrix.hxx>
#include <parclass.hxx>
#include <externalrefmgr.hxx>
#include <unitconv.hxx>
#include <formula/FormulaCompiler.hxx>
#include <formula/opcode.hxx>
#include <macromgr.hxx>
#include <doubleref.hxx>
#include <queryparam.hxx>
#include <tokenarray.hxx>
#include <compiler.hxx>
#include <spreadsheetengine/runtime/ConversionRuntime.hxx>
#include <spreadsheetengine/runtime/DateTimeParts.hxx>
#include <spreadsheetengine/runtime/DateTimeWeek.hxx>
#include <spreadsheetengine/runtime/DateTimeWorkday.hxx>
#include <spreadsheetengine/runtime/FinancialRuntime.hxx>
#include <spreadsheetengine/runtime/MathFunctionRuntime.hxx>
#include <spreadsheetengine/runtime/MathBitwise.hxx>
#include <spreadsheetengine/runtime/MathMatrix.hxx>
#include <spreadsheetengine/runtime/MathRounding.hxx>
#include <spreadsheetengine/runtime/MathScalar.hxx>
#include <spreadsheetengine/runtime/MathStatistical.hxx>
#include <spreadsheetengine/runtime/MathTranscendental.hxx>
#include <spreadsheetengine/runtime/RpnControlFlow.hxx>
#include <spreadsheetengine/runtime/RpnCriteria.hxx>
#include <spreadsheetengine/runtime/RpnMatrix.hxx>
#include <spreadsheetengine/runtime/RpnOperators.hxx>
#include <spreadsheetengine/runtime/RpnReference.hxx>
#include <spreadsheetengine/runtime/NumeralConversion.hxx>
#include <spreadsheetengine/api/StringReference.hxx>
#include <spreadsheetengine/compat/libreoffice/ExternalReferenceExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/FormulaInspectionExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/IndirectExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/InterpreterCompatDispatch.hxx>
#include <spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx>
#include <spreadsheetengine/compat/libreoffice/JumpExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/MatrixFrameExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/SwitchExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/TextParsingExecution.hxx>

#include <map>
#include <algorithm>
#include <optional>
#include <basic/basmgr.hxx>
#include <vbahelper/vbaaccesshelper.hxx>
#include <memory>

using namespace com::sun::star;
using namespace formula;
namespace seexternalexec = spreadsheetengine::compat::libreoffice::externalreferenceexecution;
namespace seformulainspect = spreadsheetengine::compat::libreoffice::formulainspection;
namespace seinterpcompatdispatch = spreadsheetengine::compat::libreoffice::interpretercompatdispatch;
namespace sejumpexec = spreadsheetengine::compat::libreoffice::jumpexecution;
namespace selibreoffice = spreadsheetengine::compat::libreoffice;
namespace selogic = spreadsheetengine::api::logic;
namespace seconvert = spreadsheetengine::core::convert;
namespace sedatetime = spreadsheetengine::core::datetime;
namespace sefinance = spreadsheetengine::core::finance;
namespace semath = spreadsheetengine::core::math;
namespace serpn = spreadsheetengine::core::rpn;
namespace serefexec = spreadsheetengine::compat::libreoffice::referenceexecution;
namespace sequery = spreadsheetengine::core::query;
namespace secoercion = spreadsheetengine::core::coercion;
namespace seitee = spreadsheetengine::compat::libreoffice::interprettaileval;
namespace seswitchexec = spreadsheetengine::compat::libreoffice::switchexecution;
namespace setextparseexec = spreadsheetengine::compat::libreoffice::textparsingexecution;
namespace sestringref = spreadsheetengine::api::stringreference;
namespace seindirectexec = spreadsheetengine::compat::libreoffice::indirectexecution;

namespace {

struct UBlockScript {
    UBlockCode from;
    UBlockCode to;
};

const UBlockScript scriptList[] = {
    { UBLOCK_HANGUL_JAMO, UBLOCK_HANGUL_JAMO },
    { UBLOCK_CJK_RADICALS_SUPPLEMENT, UBLOCK_HANGUL_SYLLABLES },
    { UBLOCK_CJK_COMPATIBILITY_IDEOGRAPHS, UBLOCK_CJK_RADICALS_SUPPLEMENT },
    { UBLOCK_IDEOGRAPHIC_DESCRIPTION_CHARACTERS, UBLOCK_CJK_COMPATIBILITY_IDEOGRAPHS },
    { UBLOCK_CJK_COMPATIBILITY_FORMS, UBLOCK_CJK_COMPATIBILITY_FORMS },
    { UBLOCK_HALFWIDTH_AND_FULLWIDTH_FORMS, UBLOCK_HALFWIDTH_AND_FULLWIDTH_FORMS },
    { UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B, UBLOCK_CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT },
    { UBLOCK_CJK_STROKES, UBLOCK_CJK_STROKES }
};

bool IsDBCS(sal_Unicode currentChar)
{
    if ((currentChar == 0x005c || currentChar == 0x20ac)
        && (MsLangId::getConfiguredSystemLanguage() == LANGUAGE_JAPANESE))
        return true;
    sal_uInt16 i;
    UBlockCode block = ublock_getCode(currentChar);
    for (i = 0; i < SAL_N_ELEMENTS(scriptList); i++)
        if (block <= scriptList[i].to)
            break;
    return i < SAL_N_ELEMENTS(scriptList) && block >= scriptList[i].from;
}

sal_Int32 lcl_getLengthB(std::u16string_view str, sal_Int32 nPos)
{
    sal_Int32 index = 0;
    sal_Int32 length = 0;
    while (index < nPos)
    {
        if (IsDBCS(str[index]))
            length += 2;
        else
            length++;
        index++;
    }
    return length;
}

sal_Int32 getLengthB(std::u16string_view str)
{
    if (str.empty())
        return 0;
    return lcl_getLengthB(str, str.size());
}

OUString lcl_RightB(const OUString& rStr, sal_Int32 n)
{
    if (n < getLengthB(rStr))
    {
        OUStringBuffer aBuf(rStr);
        sal_Int32 index = aBuf.getLength();
        while (index-- >= 0)
        {
            if (0 == n)
            {
                aBuf.remove(0, index + 1);
                break;
            }
            if (-1 == n)
            {
                aBuf.remove(0, index + 2);
                aBuf.insert(0, " ");
                break;
            }
            if (IsDBCS(aBuf[index]))
                n -= 2;
            else
                n--;
        }
        return aBuf.makeStringAndClear();
    }
    return rStr;
}

OUString lcl_LeftB(const OUString& rStr, sal_Int32 n)
{
    if (n < getLengthB(rStr))
    {
        OUStringBuffer aBuf(rStr);
        sal_Int32 index = -1;
        while (index++ < aBuf.getLength())
        {
            if (0 == n)
            {
                aBuf.truncate(index);
                break;
            }
            if (-1 == n)
            {
                aBuf.truncate(index - 1);
                aBuf.append(" ");
                break;
            }
            if (IsDBCS(aBuf[index]))
                n -= 2;
            else
                n--;
        }
        return aBuf.makeStringAndClear();
    }
    return rStr;
}

constexpr std::u16string_view TH_0 = u"ศูนย์";
constexpr std::u16string_view TH_1 = u"หนึ่ง";
constexpr std::u16string_view TH_2 = u"สอง";
constexpr std::u16string_view TH_3 = u"สาม";
constexpr std::u16string_view TH_4 = u"สี่";
constexpr std::u16string_view TH_5 = u"ห้า";
constexpr std::u16string_view TH_6 = u"หก";
constexpr std::u16string_view TH_7 = u"เจ็ด";
constexpr std::u16string_view TH_8 = u"แปด";
constexpr std::u16string_view TH_9 = u"เก้า";
constexpr std::u16string_view TH_10 = u"สิบ";
constexpr std::u16string_view TH_11 = u"เอ็ด";
constexpr std::u16string_view TH_20 = u"ยี่";
constexpr std::u16string_view TH_1E2 = u"ร้อย";
constexpr std::u16string_view TH_1E3 = u"พัน";
constexpr std::u16string_view TH_1E4 = u"หมื่น";
constexpr std::u16string_view TH_1E5 = u"แสน";
constexpr std::u16string_view TH_1E6 = u"ล้าน";
constexpr std::u16string_view TH_DOT0 = u"ถ้วน";
constexpr std::u16string_view TH_BAHT = u"บาท";
constexpr std::u16string_view TH_SATANG = u"สตางค์";
constexpr std::u16string_view TH_MINUS = u"ลบ";

void lclAppendDigit(OUStringBuffer& rText, char nDigit)
{
    switch (nDigit)
    {
        case '1': rText.append(TH_1); break;
        case '2': rText.append(TH_2); break;
        case '3': rText.append(TH_3); break;
        case '4': rText.append(TH_4); break;
        case '5': rText.append(TH_5); break;
        case '6': rText.append(TH_6); break;
        case '7': rText.append(TH_7); break;
        case '8': rText.append(TH_8); break;
        case '9': rText.append(TH_9); break;
    }
}

void lclAppendPow10(OUStringBuffer& rText, char nDigit, sal_Int32 nPow10)
{
    lclAppendDigit(rText, nDigit);
    switch (nPow10)
    {
        case 2: rText.append(TH_1E2); break;
        case 3: rText.append(TH_1E3); break;
        case 4: rText.append(TH_1E4); break;
        case 5: rText.append(TH_1E5); break;
    }
}

void lclAppendBlock(OUStringBuffer& rText, std::string_view block)
{
    auto it = block.begin();
    for (size_t pow = block.size() - 1; pow >= 2; --pow)
        if (char ch = *it++; ch != '0')
            lclAppendPow10(rText, ch, pow);

    char ten = block.size() > 1 ? *it++ : '0';
    char one = *it;
    if (ten >= '1')
    {
        if (ten >= '3')
            lclAppendDigit(rText, ten);
        else if (ten == '2')
            rText.append(TH_20);
        rText.append(TH_10);
    }
    if ((ten > '0') && (one == '1'))
        rText.append(TH_11);
    else if (one > '0')
        lclAppendDigit(rText, one);
}

}

#define ADDIN_MAXSTRLEN 256

namespace
{

struct InterpreterDispatchRuntimeStatsStore
{
    std::atomic<sal_uInt64> mnEngineAttemptedCount { 0 };
    std::atomic<sal_uInt64> mnEngineSucceededCount { 0 };
    std::atomic<sal_uInt64> mnEngineDeclinedCount { 0 };
    std::atomic<sal_uInt64> mnRangeEngineAttemptedCount { 0 };
    std::atomic<sal_uInt64> mnRangeEngineSucceededCount { 0 };
    std::atomic<sal_uInt64> mnRangeEngineDeclinedCount { 0 };
    std::atomic<sal_uInt64> mnRangeEngineDeclinedGlobalErrorOrStackCount { 0 };
    std::atomic<sal_uInt64> mnRangeEngineDeclinedNullTokenCount { 0 };
    std::atomic<sal_uInt64> mnRangeEngineDeclinedBuildFailureCount { 0 };
    std::atomic<sal_uInt64> mnControlFlowEngineAttemptedCount { 0 };
    std::atomic<sal_uInt64> mnControlFlowEngineSucceededCount { 0 };
    std::atomic<sal_uInt64> mnControlFlowEngineDeclinedCount { 0 };
    std::atomic<sal_uInt64> mnReferenceEngineAttemptedCount { 0 };
    std::atomic<sal_uInt64> mnReferenceEngineSucceededCount { 0 };
    std::atomic<sal_uInt64> mnReferenceEngineDeclinedCount { 0 };
    std::atomic<sal_uInt64> mnCriteriaEngineAttemptedCount { 0 };
    std::atomic<sal_uInt64> mnCriteriaEngineSucceededCount { 0 };
    std::atomic<sal_uInt64> mnCriteriaEngineDeclinedCount { 0 };
    std::atomic<sal_uInt64> mnMatrixEngineAttemptedCount { 0 };
    std::atomic<sal_uInt64> mnMatrixEngineSucceededCount { 0 };
    std::atomic<sal_uInt64> mnMatrixEngineDeclinedCount { 0 };
    std::mutex maRangeDeclinedSampleMutex;
    std::vector<OUString> maRangeDeclinedFormulaSamples;
};

InterpreterDispatchRuntimeStatsStore& interpreterDispatchRuntimeStatsStore()
{
    static InterpreterDispatchRuntimeStatsStore aStore;
    return aStore;
}

struct InterpreterReachabilityStatsStore
{
    std::atomic<sal_uInt64> mnFormulaCellInterpretCount { 0 };
    std::atomic<sal_uInt64> mnFormulaGroupAttemptCount { 0 };
    std::atomic<sal_uInt64> mnFormulaGroupHandledCount { 0 };
    std::atomic<sal_uInt64> mnInterpretTailCount { 0 };
    std::atomic<sal_uInt64> mnClassicInterpretCount { 0 };
};

InterpreterReachabilityStatsStore& interpreterReachabilityStatsStore()
{
    static InterpreterReachabilityStatsStore aStore;
    return aStore;
}

struct InterpreterClassicOpcodeRuntimeStatsStore
{
    std::atomic<sal_uInt64> mnInterestingOpcodeCount { 0 };
    std::array<std::atomic<sal_uInt64>, SC_OPCODE_LAST_OPCODE_ID + 1> maOpcodeCounts {};
    std::mutex maSampleMutex;
    std::vector<ScInterpreterClassicOpcodeDiagnosticSample> maSamples;
};

InterpreterClassicOpcodeRuntimeStatsStore& interpreterClassicOpcodeRuntimeStatsStore()
{
    static InterpreterClassicOpcodeRuntimeStatsStore aStore;
    return aStore;
}

struct PendingClassicOpcodeSampleContext
{
    OUString maFormulaSource;
    bool mbRecorded = false;
};

std::vector<PendingClassicOpcodeSampleContext>& pendingClassicOpcodeSampleContexts()
{
    thread_local std::vector<PendingClassicOpcodeSampleContext> aContexts;
    return aContexts;
}

void addDispatchRuntimeStat(std::atomic<sal_uInt64>& rTarget, sal_uInt64 nDelta = 1)
{
    rTarget.fetch_add(nDelta, std::memory_order_relaxed);
}

void maybeRecordRangeDispatchDeclineFormulaSample()
{
    auto& rContexts = pendingClassicOpcodeSampleContexts();
    if (rContexts.empty())
        return;

    const OUString& rFormulaSource = rContexts.back().maFormulaSource;
    if (rFormulaSource.isEmpty())
        return;

    auto& rStore = interpreterDispatchRuntimeStatsStore();
    std::lock_guard aGuard(rStore.maRangeDeclinedSampleMutex);
    if (rStore.maRangeDeclinedFormulaSamples.size() >= 16)
        return;
    rStore.maRangeDeclinedFormulaSamples.push_back(rFormulaSource);
}

[[nodiscard]] bool isInterestingClassicOpcode(OpCode eOp)
{
    switch (eOp)
    {
        case ocPush:
        case ocOpen:
        case ocClose:
        case ocSep:
        case ocArrayOpen:
        case ocArrayClose:
        case ocArrayRowSep:
        case ocArrayColSep:
        case ocMissing:
        case ocSpaces:
        case ocWhitespace:
        case ocStringXML:
        case ocStringName:
        case ocSkip:
            return false;
        default:
            return true;
    }
}

void maybeRecordClassicOpcodeDiagnosticSample(OpCode eOp)
{
    auto& rContexts = pendingClassicOpcodeSampleContexts();
    if (rContexts.empty())
        return;

    auto& rTop = rContexts.back();
    if (rTop.mbRecorded)
        return;

    auto& rStore = interpreterClassicOpcodeRuntimeStatsStore();
    std::lock_guard aGuard(rStore.maSampleMutex);
    if (rStore.maSamples.size() < 16)
    {
        rStore.maSamples.push_back(
            { rTop.maFormulaSource, OUString::fromUtf8(OpCodeEnumToString(eOp)) });
    }
    rTop.mbRecorded = true;
}

void addClassicOpcodeRuntimeStat(OpCode eOp, sal_uInt64 nDelta = 1)
{
    auto& rStore = interpreterClassicOpcodeRuntimeStatsStore();
    addDispatchRuntimeStat(rStore.mnInterestingOpcodeCount, nDelta);
    addDispatchRuntimeStat(rStore.maOpcodeCounts[static_cast<std::size_t>(eOp)], nDelta);
    maybeRecordClassicOpcodeDiagnosticSample(eOp);
}

}

thread_local std::unique_ptr<ScTokenStack> ScInterpreter::pGlobalStack;
thread_local bool ScInterpreter::bGlobalStackInUse = false;

void resetScInterpreterDispatchRuntimeStats()
{
    auto& rStore = interpreterDispatchRuntimeStatsStore();
    rStore.mnEngineAttemptedCount.store(0, std::memory_order_relaxed);
    rStore.mnEngineSucceededCount.store(0, std::memory_order_relaxed);
    rStore.mnEngineDeclinedCount.store(0, std::memory_order_relaxed);
    rStore.mnRangeEngineAttemptedCount.store(0, std::memory_order_relaxed);
    rStore.mnRangeEngineSucceededCount.store(0, std::memory_order_relaxed);
    rStore.mnRangeEngineDeclinedCount.store(0, std::memory_order_relaxed);
    rStore.mnRangeEngineDeclinedGlobalErrorOrStackCount.store(0, std::memory_order_relaxed);
    rStore.mnRangeEngineDeclinedNullTokenCount.store(0, std::memory_order_relaxed);
    rStore.mnRangeEngineDeclinedBuildFailureCount.store(0, std::memory_order_relaxed);
    rStore.mnControlFlowEngineAttemptedCount.store(0, std::memory_order_relaxed);
    rStore.mnControlFlowEngineSucceededCount.store(0, std::memory_order_relaxed);
    rStore.mnControlFlowEngineDeclinedCount.store(0, std::memory_order_relaxed);
    rStore.mnReferenceEngineAttemptedCount.store(0, std::memory_order_relaxed);
    rStore.mnReferenceEngineSucceededCount.store(0, std::memory_order_relaxed);
    rStore.mnReferenceEngineDeclinedCount.store(0, std::memory_order_relaxed);
    rStore.mnCriteriaEngineAttemptedCount.store(0, std::memory_order_relaxed);
    rStore.mnCriteriaEngineSucceededCount.store(0, std::memory_order_relaxed);
    rStore.mnCriteriaEngineDeclinedCount.store(0, std::memory_order_relaxed);
    rStore.mnMatrixEngineAttemptedCount.store(0, std::memory_order_relaxed);
    rStore.mnMatrixEngineSucceededCount.store(0, std::memory_order_relaxed);
    rStore.mnMatrixEngineDeclinedCount.store(0, std::memory_order_relaxed);
    {
        std::lock_guard aGuard(rStore.maRangeDeclinedSampleMutex);
        rStore.maRangeDeclinedFormulaSamples.clear();
    }
}

ScInterpreterDispatchRuntimeStatsSnapshot getScInterpreterDispatchRuntimeStatsSnapshot()
{
    auto& rStore = interpreterDispatchRuntimeStatsStore();
    ScInterpreterDispatchRuntimeStatsSnapshot aSnapshot;
    aSnapshot.mnEngineAttemptedCount
        = rStore.mnEngineAttemptedCount.load(std::memory_order_relaxed);
    aSnapshot.mnEngineSucceededCount
        = rStore.mnEngineSucceededCount.load(std::memory_order_relaxed);
    aSnapshot.mnEngineDeclinedCount
        = rStore.mnEngineDeclinedCount.load(std::memory_order_relaxed);
    aSnapshot.mnRangeEngineAttemptedCount
        = rStore.mnRangeEngineAttemptedCount.load(std::memory_order_relaxed);
    aSnapshot.mnRangeEngineSucceededCount
        = rStore.mnRangeEngineSucceededCount.load(std::memory_order_relaxed);
    aSnapshot.mnRangeEngineDeclinedCount
        = rStore.mnRangeEngineDeclinedCount.load(std::memory_order_relaxed);
    aSnapshot.mnRangeEngineDeclinedGlobalErrorOrStackCount
        = rStore.mnRangeEngineDeclinedGlobalErrorOrStackCount.load(
            std::memory_order_relaxed);
    aSnapshot.mnRangeEngineDeclinedNullTokenCount
        = rStore.mnRangeEngineDeclinedNullTokenCount.load(std::memory_order_relaxed);
    aSnapshot.mnRangeEngineDeclinedBuildFailureCount
        = rStore.mnRangeEngineDeclinedBuildFailureCount.load(std::memory_order_relaxed);
    aSnapshot.mnControlFlowEngineAttemptedCount
        = rStore.mnControlFlowEngineAttemptedCount.load(std::memory_order_relaxed);
    aSnapshot.mnControlFlowEngineSucceededCount
        = rStore.mnControlFlowEngineSucceededCount.load(std::memory_order_relaxed);
    aSnapshot.mnControlFlowEngineDeclinedCount
        = rStore.mnControlFlowEngineDeclinedCount.load(std::memory_order_relaxed);
    aSnapshot.mnReferenceEngineAttemptedCount
        = rStore.mnReferenceEngineAttemptedCount.load(std::memory_order_relaxed);
    aSnapshot.mnReferenceEngineSucceededCount
        = rStore.mnReferenceEngineSucceededCount.load(std::memory_order_relaxed);
    aSnapshot.mnReferenceEngineDeclinedCount
        = rStore.mnReferenceEngineDeclinedCount.load(std::memory_order_relaxed);
    aSnapshot.mnCriteriaEngineAttemptedCount
        = rStore.mnCriteriaEngineAttemptedCount.load(std::memory_order_relaxed);
    aSnapshot.mnCriteriaEngineSucceededCount
        = rStore.mnCriteriaEngineSucceededCount.load(std::memory_order_relaxed);
    aSnapshot.mnCriteriaEngineDeclinedCount
        = rStore.mnCriteriaEngineDeclinedCount.load(std::memory_order_relaxed);
    aSnapshot.mnMatrixEngineAttemptedCount
        = rStore.mnMatrixEngineAttemptedCount.load(std::memory_order_relaxed);
    aSnapshot.mnMatrixEngineSucceededCount
        = rStore.mnMatrixEngineSucceededCount.load(std::memory_order_relaxed);
    aSnapshot.mnMatrixEngineDeclinedCount
        = rStore.mnMatrixEngineDeclinedCount.load(std::memory_order_relaxed);
    {
        std::lock_guard aGuard(rStore.maRangeDeclinedSampleMutex);
        aSnapshot.maRangeDeclinedFormulaSamples = rStore.maRangeDeclinedFormulaSamples;
    }
    return aSnapshot;
}

void resetScInterpreterReachabilityStats()
{
    auto& rStore = interpreterReachabilityStatsStore();
    rStore.mnFormulaCellInterpretCount.store(0, std::memory_order_relaxed);
    rStore.mnFormulaGroupAttemptCount.store(0, std::memory_order_relaxed);
    rStore.mnFormulaGroupHandledCount.store(0, std::memory_order_relaxed);
    rStore.mnInterpretTailCount.store(0, std::memory_order_relaxed);
    rStore.mnClassicInterpretCount.store(0, std::memory_order_relaxed);
}

void addScInterpreterReachabilityStat(ScInterpreterReachabilityStat eStat, sal_uInt64 nDelta)
{
    auto& rStore = interpreterReachabilityStatsStore();
    switch (eStat)
    {
        case ScInterpreterReachabilityStat::FormulaCellInterpret:
            addDispatchRuntimeStat(rStore.mnFormulaCellInterpretCount, nDelta);
            return;
        case ScInterpreterReachabilityStat::FormulaGroupAttempt:
            addDispatchRuntimeStat(rStore.mnFormulaGroupAttemptCount, nDelta);
            return;
        case ScInterpreterReachabilityStat::FormulaGroupHandled:
            addDispatchRuntimeStat(rStore.mnFormulaGroupHandledCount, nDelta);
            return;
        case ScInterpreterReachabilityStat::InterpretTail:
            addDispatchRuntimeStat(rStore.mnInterpretTailCount, nDelta);
            return;
        case ScInterpreterReachabilityStat::ClassicInterpret:
            addDispatchRuntimeStat(rStore.mnClassicInterpretCount, nDelta);
            return;
    }
}

ScInterpreterReachabilityStatsSnapshot getScInterpreterReachabilityStatsSnapshot()
{
    const auto& rStore = interpreterReachabilityStatsStore();
    ScInterpreterReachabilityStatsSnapshot aSnapshot;
    aSnapshot.mnFormulaCellInterpretCount
        = rStore.mnFormulaCellInterpretCount.load(std::memory_order_relaxed);
    aSnapshot.mnFormulaGroupAttemptCount
        = rStore.mnFormulaGroupAttemptCount.load(std::memory_order_relaxed);
    aSnapshot.mnFormulaGroupHandledCount
        = rStore.mnFormulaGroupHandledCount.load(std::memory_order_relaxed);
    aSnapshot.mnInterpretTailCount
        = rStore.mnInterpretTailCount.load(std::memory_order_relaxed);
    aSnapshot.mnClassicInterpretCount
        = rStore.mnClassicInterpretCount.load(std::memory_order_relaxed);
    return aSnapshot;
}

void resetScInterpreterClassicOpcodeRuntimeStats()
{
    auto& rStore = interpreterClassicOpcodeRuntimeStatsStore();
    rStore.mnInterestingOpcodeCount.store(0, std::memory_order_relaxed);
    for (auto& rCount : rStore.maOpcodeCounts)
        rCount.store(0, std::memory_order_relaxed);
    {
        std::lock_guard aGuard(rStore.maSampleMutex);
        rStore.maSamples.clear();
    }
    pendingClassicOpcodeSampleContexts().clear();
}

ScInterpreterClassicOpcodeRuntimeStatsSnapshot getScInterpreterClassicOpcodeRuntimeStatsSnapshot()
{
    auto& rStore = interpreterClassicOpcodeRuntimeStatsStore();
    ScInterpreterClassicOpcodeRuntimeStatsSnapshot aSnapshot;
    aSnapshot.mnInterestingOpcodeCount
        = rStore.mnInterestingOpcodeCount.load(std::memory_order_relaxed);
    for (std::size_t nIndex = 0; nIndex < aSnapshot.maOpcodeCounts.size(); ++nIndex)
    {
        aSnapshot.maOpcodeCounts[nIndex]
            = rStore.maOpcodeCounts[nIndex].load(std::memory_order_relaxed);
    }
    {
        std::lock_guard aGuard(rStore.maSampleMutex);
        aSnapshot.maSamples = rStore.maSamples;
    }
    return aSnapshot;
}

void pushScInterpreterClassicOpcodeFormulaContext(const OUString& rFormulaSource)
{
    pendingClassicOpcodeSampleContexts().push_back({ rFormulaSource, false });
}

void popScInterpreterClassicOpcodeFormulaContext()
{
    auto& rContexts = pendingClassicOpcodeSampleContexts();
    if (!rContexts.empty())
        rContexts.pop_back();
}

// document access functions

void ScInterpreter::ReplaceCell( ScAddress& rPos )
{
    size_t ListSize = mrDoc.m_TableOpList.size();
    for ( size_t i = 0; i < ListSize; ++i )
    {
        ScInterpreterTableOpParams *const pTOp = mrDoc.m_TableOpList[ i ];
        if ( rPos == pTOp->aOld1 )
        {
            rPos = pTOp->aNew1;
            return ;
        }
        else if ( rPos == pTOp->aOld2 )
        {
            rPos = pTOp->aNew2;
            return ;
        }
    }
}

bool ScInterpreter::IsTableOpInRange( const ScRange& rRange )
{
    if ( rRange.aStart == rRange.aEnd )
        return false;   // not considered to be a range in TableOp sense

    // we can't replace a single cell in a range
    size_t ListSize = mrDoc.m_TableOpList.size();
    for ( size_t i = 0; i < ListSize; ++i )
    {
        ScInterpreterTableOpParams *const pTOp = mrDoc.m_TableOpList[ i ];
        if ( rRange.Contains( pTOp->aOld1 ) )
            return true;
        if ( rRange.Contains( pTOp->aOld2 ) )
            return true;
    }
    return false;
}

sal_uInt32 ScInterpreter::GetCellNumberFormat( const ScAddress& rPos, const ScRefCellValue& rCell )
{
    sal_uInt32 nFormat;
    FormulaError nErr;
    if (rCell.isEmpty())
    {
        nFormat = mrDoc.GetNumberFormat( mrContext, rPos );
        nErr = FormulaError::NONE;
    }
    else
    {
        if (rCell.getType() == CELLTYPE_FORMULA)
            nErr = rCell.getFormula()->GetErrCode();
        else
            nErr = FormulaError::NONE;
        nFormat = mrDoc.GetNumberFormat( mrContext, rPos );
    }

    SetError(nErr);
    return nFormat;
}

/// Only ValueCell, formula cells already store the result rounded.
double ScInterpreter::GetValueCellValue( const ScAddress& rPos, double fOrig )
{
    if ( bCalcAsShown && fOrig != 0.0 )
    {
        sal_uInt32 nFormat = mrDoc.GetNumberFormat( mrContext, rPos );
        fOrig = mrDoc.RoundValueAsShown( fOrig, nFormat, &mrContext );
    }
    return fOrig;
}

FormulaError ScInterpreter::GetCellErrCode( const ScRefCellValue& rCell )
{
    return rCell.getType() == CELLTYPE_FORMULA ? rCell.getFormula()->GetErrCode() : FormulaError::NONE;
}

double ScInterpreter::ConvertStringToValue( const OUString& rStr )
{
    FormulaError nError = FormulaError::NONE;
    double fValue = ScGlobal::ConvertStringToValue( rStr, maCalcConfig, nError, mnStringNoValueError,
            mrContext, nCurFmtType);
    if (nError != FormulaError::NONE)
        SetError(nError);
    return fValue;
}

double ScInterpreter::ConvertStringToValue( const OUString& rStr, FormulaError& rError, SvNumFormatType& rCurFmtType )
{
    return ScGlobal::ConvertStringToValue( rStr, maCalcConfig, rError, mnStringNoValueError, mrContext, rCurFmtType);
}

double ScInterpreter::GetCellValue( const ScAddress& rPos, const ScRefCellValue& rCell )
{
    FormulaError nErr = nGlobalError;
    nGlobalError = FormulaError::NONE;
    double nVal = GetCellValueOrZero(rPos, rCell);
    // Propagate previous error, if any; nGlobalError==CellNoValue is not an
    // error here, preserve previous error or non-error.
    if (nErr != FormulaError::NONE || nGlobalError == FormulaError::CellNoValue)
        nGlobalError = nErr;
    return nVal;
}

double ScInterpreter::GetCellValueOrZero( const ScAddress& rPos, const ScRefCellValue& rCell )
{
    double fValue = 0.0;

    CellType eType = rCell.getType();
    switch (eType)
    {
        case CELLTYPE_FORMULA:
        {
            ScFormulaCell* pFCell = rCell.getFormula();
            FormulaError nErr = pFCell->GetErrCode();
            if( nErr == FormulaError::NONE )
            {
                if (pFCell->IsValue())
                {
                    fValue = pFCell->GetValue();
                    mrDoc.GetNumberFormatInfo( mrContext, nCurFmtType, nCurFmtIndex,
                        rPos );
                }
                else
                {
                    fValue = ConvertStringToValue(pFCell->GetString().getString());
                }
            }
            else
            {
                fValue = 0.0;
                SetError(nErr);
            }
        }
        break;
        case CELLTYPE_VALUE:
        {
            fValue = rCell.getDouble();
            nCurFmtIndex = mrDoc.GetNumberFormat( mrContext, rPos );
            nCurFmtType = mrContext.NFGetType(nCurFmtIndex);
            if ( bCalcAsShown && fValue != 0.0 )
                fValue = mrDoc.RoundValueAsShown( fValue, nCurFmtIndex, &mrContext );
        }
        break;
        case  CELLTYPE_STRING:
        case  CELLTYPE_EDIT:
        {
            // SUM(A1:A2) differs from A1+A2. No good. But people insist on
            // it ... #i5658#
            OUString aStr = rCell.getString(mrDoc);
            fValue = ConvertStringToValue( aStr );
        }
        break;
        case CELLTYPE_NONE:
            fValue = 0.0;       // empty or broadcaster cell
        break;
    }

    return fValue;
}

void ScInterpreter::GetCellString( svl::SharedString& rStr, const ScRefCellValue& rCell )
{
    FormulaError nErr = FormulaError::NONE;

    switch (rCell.getType())
    {
        case CELLTYPE_STRING:
        case CELLTYPE_EDIT:
            rStr = rCell.getSharedString(mrDoc, mrStrPool);
        break;
        case CELLTYPE_FORMULA:
        {
            ScFormulaCell* pFCell = rCell.getFormula();
            nErr = pFCell->GetErrCode();
            if (pFCell->IsValue())
            {
                rStr = GetStringFromDouble( pFCell->GetValue() );
            }
            else
                rStr = pFCell->GetString();
        }
        break;
        case CELLTYPE_VALUE:
        {
            rStr = GetStringFromDouble( rCell.getDouble() );
        }
        break;
        default:
            rStr = svl::SharedString::getEmptyString();
        break;
    }

    SetError(nErr);
}

bool ScInterpreter::CreateDoubleArr(SCCOL nCol1, SCROW nRow1, SCTAB nTab1,
                            SCCOL nCol2, SCROW nRow2, SCTAB nTab2, sal_uInt8* pCellArr)
{

    // Old Add-Ins are hard limited to sal_uInt16 values.
    static_assert(MAXCOLCOUNT <= SAL_MAX_UINT16 && MAXCOLCOUNT_JUMBO <= SAL_MAX_UINT16,
        "Add check for columns > SAL_MAX_UINT16!");
    if (nRow1 > SAL_MAX_UINT16 || nRow2 > SAL_MAX_UINT16)
        return false;

    sal_uInt16 nCount = 0;
    sal_uInt16* p = reinterpret_cast<sal_uInt16*>(pCellArr);
    *p++ = static_cast<sal_uInt16>(nCol1);
    *p++ = static_cast<sal_uInt16>(nRow1);
    *p++ = static_cast<sal_uInt16>(nTab1);
    *p++ = static_cast<sal_uInt16>(nCol2);
    *p++ = static_cast<sal_uInt16>(nRow2);
    *p++ = static_cast<sal_uInt16>(nTab2);
    sal_uInt16* pCount = p;
    *p++ = 0;
    sal_uInt16 nPos = 14;
    SCTAB nTab = nTab1;
    ScAddress aAdr;
    while (nTab <= nTab2)
    {
        aAdr.SetTab( nTab );
        SCROW nRow = nRow1;
        while (nRow <= nRow2)
        {
            aAdr.SetRow( nRow );
            SCCOL nCol = nCol1;
            while (nCol <= nCol2)
            {
                aAdr.SetCol( nCol );

                ScRefCellValue aCell(mrDoc, aAdr);
                if (!aCell.isEmpty())
                {
                    FormulaError  nErr = FormulaError::NONE;
                    double  nVal = 0.0;
                    bool    bOk = true;
                    switch (aCell.getType())
                    {
                        case CELLTYPE_VALUE :
                            nVal = GetValueCellValue(aAdr, aCell.getDouble());
                            break;
                        case CELLTYPE_FORMULA :
                            if (aCell.getFormula()->IsValue())
                            {
                                nErr = aCell.getFormula()->GetErrCode();
                                nVal = aCell.getFormula()->GetValue();
                            }
                            else
                                bOk = false;
                            break;
                        default :
                            bOk = false;
                            break;
                    }
                    if (bOk)
                    {
                        if ((nPos + (4 * sizeof(sal_uInt16)) + sizeof(double)) > MAXARRSIZE)
                            return false;
                        *p++ = static_cast<sal_uInt16>(nCol);
                        *p++ = static_cast<sal_uInt16>(nRow);
                        *p++ = static_cast<sal_uInt16>(nTab);
                        *p++ = static_cast<sal_uInt16>(nErr);
                        memcpy( p, &nVal, sizeof(double));
                        nPos += 8 + sizeof(double);
                        p = reinterpret_cast<sal_uInt16*>( pCellArr + nPos );
                        nCount++;
                    }
                }
                nCol++;
            }
            nRow++;
        }
        nTab++;
    }
    *pCount = nCount;
    return true;
}

bool ScInterpreter::CreateStringArr(SCCOL nCol1, SCROW nRow1, SCTAB nTab1,
                                    SCCOL nCol2, SCROW nRow2, SCTAB nTab2,
                                    sal_uInt8* pCellArr)
{

    // Old Add-Ins are hard limited to sal_uInt16 values.
    static_assert(MAXCOLCOUNT <= SAL_MAX_UINT16 && MAXCOLCOUNT_JUMBO <= SAL_MAX_UINT16,
        "Add check for columns > SAL_MAX_UINT16!");
    if (nRow1 > SAL_MAX_UINT16 || nRow2 > SAL_MAX_UINT16)
        return false;

    sal_uInt16 nCount = 0;
    sal_uInt16* p = reinterpret_cast<sal_uInt16*>(pCellArr);
    *p++ = static_cast<sal_uInt16>(nCol1);
    *p++ = static_cast<sal_uInt16>(nRow1);
    *p++ = static_cast<sal_uInt16>(nTab1);
    *p++ = static_cast<sal_uInt16>(nCol2);
    *p++ = static_cast<sal_uInt16>(nRow2);
    *p++ = static_cast<sal_uInt16>(nTab2);
    sal_uInt16* pCount = p;
    *p++ = 0;
    sal_uInt16 nPos = 14;
    SCTAB nTab = nTab1;
    while (nTab <= nTab2)
    {
        SCROW nRow = nRow1;
        while (nRow <= nRow2)
        {
            SCCOL nCol = nCol1;
            while (nCol <= nCol2)
            {
                ScRefCellValue aCell(mrDoc, ScAddress(nCol, nRow, nTab));
                if (!aCell.isEmpty())
                {
                    OUString  aStr;
                    FormulaError  nErr = FormulaError::NONE;
                    bool    bOk = true;
                    switch (aCell.getType())
                    {
                        case CELLTYPE_STRING:
                        case CELLTYPE_EDIT:
                            aStr = aCell.getString(mrDoc);
                            break;
                        case CELLTYPE_FORMULA:
                            if (!aCell.getFormula()->IsValue())
                            {
                                nErr = aCell.getFormula()->GetErrCode();
                                aStr = aCell.getFormula()->GetString().getString();
                            }
                            else
                                bOk = false;
                            break;
                        default :
                            bOk = false;
                            break;
                    }
                    if (bOk)
                    {
                        OString aTmp(OUStringToOString(aStr,
                            osl_getThreadTextEncoding()));
                        // Old Add-Ins are limited to sal_uInt16 string
                        // lengths, and room for pad byte check.
                        if ( aTmp.getLength() > SAL_MAX_UINT16 - 2 )
                            return false;
                        // Append a 0-pad-byte if string length is odd
                        // MUST be sal_uInt16
                        sal_uInt16 nStrLen = static_cast<sal_uInt16>(aTmp.getLength());
                        sal_uInt16 nLen = ( nStrLen + 2 ) & ~1;

                        if ((static_cast<sal_uLong>(nPos) + (5 * sizeof(sal_uInt16)) + nLen) > MAXARRSIZE)
                            return false;
                        *p++ = static_cast<sal_uInt16>(nCol);
                        *p++ = static_cast<sal_uInt16>(nRow);
                        *p++ = static_cast<sal_uInt16>(nTab);
                        *p++ = static_cast<sal_uInt16>(nErr);
                        *p++ = nLen;
                        memcpy( p, aTmp.getStr(), nStrLen + 1);
                        nPos += 10 + nStrLen + 1;
                        sal_uInt8* q = pCellArr + nPos;
                        if( (nStrLen & 1) == 0 )
                        {
                            *q++ = 0;
                            nPos++;
                        }
                        p = reinterpret_cast<sal_uInt16*>( pCellArr + nPos );
                        nCount++;
                    }
                }
                nCol++;
            }
            nRow++;
        }
        nTab++;
    }
    *pCount = nCount;
    return true;
}

bool ScInterpreter::CreateCellArr(SCCOL nCol1, SCROW nRow1, SCTAB nTab1,
                                  SCCOL nCol2, SCROW nRow2, SCTAB nTab2,
                                  sal_uInt8* pCellArr)
{

    // Old Add-Ins are hard limited to sal_uInt16 values.
    static_assert(MAXCOLCOUNT <= SAL_MAX_UINT16 && MAXCOLCOUNT_JUMBO <= SAL_MAX_UINT16,
        "Add check for columns > SAL_MAX_UINT16!");
    if (nRow1 > SAL_MAX_UINT16 || nRow2 > SAL_MAX_UINT16)
        return false;

    sal_uInt16 nCount = 0;
    sal_uInt16* p = reinterpret_cast<sal_uInt16*>(pCellArr);
    *p++ = static_cast<sal_uInt16>(nCol1);
    *p++ = static_cast<sal_uInt16>(nRow1);
    *p++ = static_cast<sal_uInt16>(nTab1);
    *p++ = static_cast<sal_uInt16>(nCol2);
    *p++ = static_cast<sal_uInt16>(nRow2);
    *p++ = static_cast<sal_uInt16>(nTab2);
    sal_uInt16* pCount = p;
    *p++ = 0;
    sal_uInt16 nPos = 14;
    SCTAB nTab = nTab1;
    ScAddress aAdr;
    while (nTab <= nTab2)
    {
        aAdr.SetTab( nTab );
        SCROW nRow = nRow1;
        while (nRow <= nRow2)
        {
            aAdr.SetRow( nRow );
            SCCOL nCol = nCol1;
            while (nCol <= nCol2)
            {
                aAdr.SetCol( nCol );
                ScRefCellValue aCell(mrDoc, aAdr);
                if (!aCell.isEmpty())
                {
                    FormulaError  nErr = FormulaError::NONE;
                    sal_uInt16  nType = 0; // 0 = number; 1 = string
                    double  nVal = 0.0;
                    OUString  aStr;
                    bool    bOk = true;
                    switch (aCell.getType())
                    {
                        case CELLTYPE_STRING :
                        case CELLTYPE_EDIT :
                            aStr = aCell.getString(mrDoc);
                            nType = 1;
                            break;
                        case CELLTYPE_VALUE :
                            nVal = GetValueCellValue(aAdr, aCell.getDouble());
                            break;
                        case CELLTYPE_FORMULA :
                            nErr = aCell.getFormula()->GetErrCode();
                            if (aCell.getFormula()->IsValue())
                                nVal = aCell.getFormula()->GetValue();
                            else
                                aStr = aCell.getFormula()->GetString().getString();
                            break;
                        default :
                            bOk = false;
                            break;
                    }
                    if (bOk)
                    {
                        if ((nPos + (5 * sizeof(sal_uInt16))) > MAXARRSIZE)
                            return false;
                        *p++ = static_cast<sal_uInt16>(nCol);
                        *p++ = static_cast<sal_uInt16>(nRow);
                        *p++ = static_cast<sal_uInt16>(nTab);
                        *p++ = static_cast<sal_uInt16>(nErr);
                        *p++ = nType;
                        nPos += 10;
                        if (nType == 0)
                        {
                            if ((nPos + sizeof(double)) > MAXARRSIZE)
                                return false;
                            memcpy( p, &nVal, sizeof(double));
                            nPos += sizeof(double);
                        }
                        else
                        {
                            OString aTmp(OUStringToOString(aStr,
                                osl_getThreadTextEncoding()));
                            // Old Add-Ins are limited to sal_uInt16 string
                            // lengths, and room for pad byte check.
                            if ( aTmp.getLength() > SAL_MAX_UINT16 - 2 )
                                return false;
                            // Append a 0-pad-byte if string length is odd
                            // MUST be sal_uInt16
                            sal_uInt16 nStrLen = static_cast<sal_uInt16>(aTmp.getLength());
                            sal_uInt16 nLen = ( nStrLen + 2 ) & ~1;
                            if ( (static_cast<sal_uLong>(nPos) + 2 + nLen) > MAXARRSIZE)
                                return false;
                            *p++ = nLen;
                            memcpy( p, aTmp.getStr(), nStrLen + 1);
                            nPos += 2 + nStrLen + 1;
                            sal_uInt8* q = pCellArr + nPos;
                            if( (nStrLen & 1) == 0 )
                            {
                                *q++ = 0;
                                nPos++;
                            }
                        }
                        nCount++;
                        p = reinterpret_cast<sal_uInt16*>( pCellArr + nPos );
                    }
                }
                nCol++;
            }
            nRow++;
        }
        nTab++;
    }
    *pCount = nCount;
    return true;
}

// Stack operations

// Also releases a TempToken if appropriate.

void ScInterpreter::PushWithoutError( const FormulaToken& r )
{
    if ( sp >= MAXSTACK )
        SetError( FormulaError::StackOverflow );
    else
    {
        r.IncRef();
        if( sp >= maxsp )
            maxsp = sp + 1;
        else
            pStack[ sp ]->DecRef();
        pStack[ sp ] = &r;
        ++sp;
    }
}

void ScInterpreter::Push( const FormulaToken& r )
{
    if ( sp >= MAXSTACK )
        SetError( FormulaError::StackOverflow );
    else
    {
        if (nGlobalError != FormulaError::NONE)
        {
            if (r.GetType() == svError)
                PushWithoutError( r);
            else
                PushTempTokenWithoutError( new FormulaErrorToken( nGlobalError));
        }
        else
            PushWithoutError( r);
    }
}

void ScInterpreter::PushTempToken( FormulaToken* p )
{
    if ( sp >= MAXSTACK )
    {
        SetError( FormulaError::StackOverflow );
        // p may be a dangling pointer hereafter!
        p->DeleteIfZeroRef();
    }
    else
    {
        if (nGlobalError != FormulaError::NONE)
        {
            if (p->GetType() == svError)
            {
                p->SetError( nGlobalError);
                PushTempTokenWithoutError( p);
            }
            else
            {
                // p may be a dangling pointer hereafter!
                p->DeleteIfZeroRef();
                PushTempTokenWithoutError( new FormulaErrorToken( nGlobalError));
            }
        }
        else
            PushTempTokenWithoutError( p);
    }
}

void ScInterpreter::PushTempTokenWithoutError( const FormulaToken* p )
{
    p->IncRef();
    if ( sp >= MAXSTACK )
    {
        SetError( FormulaError::StackOverflow );
        // p may be a dangling pointer hereafter!
        p->DecRef();
    }
    else
    {
        if( sp >= maxsp )
            maxsp = sp + 1;
        else
            pStack[ sp ]->DecRef();
        pStack[ sp ] = p;
        ++sp;
    }
}

void ScInterpreter::PushTokenRef( const formula::FormulaConstTokenRef& x )
{
    if ( sp >= MAXSTACK )
    {
        SetError( FormulaError::StackOverflow );
    }
    else
    {
        if (nGlobalError != FormulaError::NONE)
        {
            if (x->GetType() == svError && x->GetError() == nGlobalError)
                PushTempTokenWithoutError( x.get());
            else
                PushTempTokenWithoutError( new FormulaErrorToken( nGlobalError));
        }
        else
            PushTempTokenWithoutError( x.get());
    }
}

void ScInterpreter::PushCellResultToken( bool bDisplayEmptyAsString,
        const ScAddress & rAddress, SvNumFormatType * pRetTypeExpr, sal_uInt32 * pRetIndexExpr, bool bFinalResult )
{
    ScRefCellValue aCell(mrDoc, rAddress);
    if (aCell.hasEmptyValue())
    {
        bool bInherited = (aCell.getType() == CELLTYPE_FORMULA);
        if (pRetTypeExpr && pRetIndexExpr)
            mrDoc.GetNumberFormatInfo(mrContext, *pRetTypeExpr, *pRetIndexExpr, rAddress);
        PushTempToken( new ScEmptyCellToken( bInherited, bDisplayEmptyAsString));
        return;
    }

    FormulaError nErr = FormulaError::NONE;
    if (aCell.getType() == CELLTYPE_FORMULA)
        nErr = aCell.getFormula()->GetErrCode();

    if (nErr != FormulaError::NONE)
    {
        PushError( nErr);
        if (pRetTypeExpr)
            *pRetTypeExpr = SvNumFormatType::UNDEFINED;
        if (pRetIndexExpr)
            *pRetIndexExpr = 0;
    }
    else if (aCell.hasString())
    {
        svl::SharedString aRes;
        GetCellString( aRes, aCell);
        PushString( aRes);
        if (pRetTypeExpr)
            *pRetTypeExpr = SvNumFormatType::TEXT;
        if (pRetIndexExpr)
            *pRetIndexExpr = 0;
    }
    else
    {
        double fVal = GetCellValue(rAddress, aCell);
        if (bFinalResult)
        {
            TreatDoubleError( fVal);
            if (!IfErrorPushError())
                PushTempTokenWithoutError( CreateFormulaDoubleToken( fVal));
        }
        else
        {
            PushDouble( fVal);
        }
        if (pRetTypeExpr)
            *pRetTypeExpr = nCurFmtType;
        if (pRetIndexExpr)
            *pRetIndexExpr = nCurFmtIndex;
    }
}

// Simply throw away TOS.

void ScInterpreter::Pop()
{
    if( sp )
        sp--;
    else
        SetError(FormulaError::UnknownStackVariable);
}

// Simply throw away TOS and set error code, used with ocIsError et al.

void ScInterpreter::PopError()
{
    if( sp )
    {
        sp--;
        if (pStack[sp]->GetType() == svError)
            nGlobalError = pStack[sp]->GetError();
    }
    else
        SetError(FormulaError::UnknownStackVariable);
}

FormulaConstTokenRef ScInterpreter::PopToken()
{
    if (sp)
    {
        sp--;
        const FormulaToken* p = pStack[ sp ];
        if (p->GetType() == svError)
            nGlobalError = p->GetError();
        return p;
    }
    else
        SetError(FormulaError::UnknownStackVariable);
    return nullptr;
}

double ScInterpreter::PopDouble()
{
    nCurFmtType = SvNumFormatType::NUMBER;
    nCurFmtIndex = 0;
    if( sp )
    {
        --sp;
        const FormulaToken* p = pStack[ sp ];
        switch (p->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svDouble:
                {
                    SvNumFormatType nType = static_cast<SvNumFormatType>(p->GetDoubleType());
                    if (nType != SvNumFormatType::ALL && nType != SvNumFormatType::UNDEFINED)
                        nCurFmtType = nType;
                    return p->GetDouble();
                }
            case svEmptyCell:
            case svMissing:
                return 0.0;
            default:
                SetError( FormulaError::IllegalArgument);
        }
    }
    else
        SetError( FormulaError::UnknownStackVariable);
    return 0.0;
}

const svl::SharedString & ScInterpreter::PopString()
{
    nCurFmtType = SvNumFormatType::TEXT;
    nCurFmtIndex = 0;
    if( sp )
    {
        --sp;
        const FormulaToken* p = pStack[ sp ];
        switch (p->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svString:
            case svStringName:
                return p->GetString();
            case svEmptyCell:
            case svMissing:
                return svl::SharedString::getEmptyString();
            default:
                SetError( FormulaError::IllegalArgument);
        }
    }
    else
        SetError( FormulaError::UnknownStackVariable);

    return svl::SharedString::getEmptyString();
}

void ScInterpreter::ValidateRef( const ScSingleRefData & rRef )
{
    SCCOL nCol;
    SCROW nRow;
    SCTAB nTab;
    SingleRefToVars( rRef, nCol, nRow, nTab);
}

void ScInterpreter::ValidateRef( const ScComplexRefData & rRef )
{
    ValidateRef( rRef.Ref1);
    ValidateRef( rRef.Ref2);
}

void ScInterpreter::ValidateRef( const ScRefList & rRefList )
{
    for (const auto& rRef : rRefList)
    {
        ValidateRef( rRef);
    }
}

void ScInterpreter::SingleRefToVars( const ScSingleRefData & rRef,
        SCCOL & rCol, SCROW & rRow, SCTAB & rTab )
{
    if ( rRef.IsColRel() )
        rCol = aPos.Col() + rRef.Col();
    else
        rCol = rRef.Col();

    if ( rRef.IsRowRel() )
        rRow = aPos.Row() + rRef.Row();
    else
        rRow = rRef.Row();

    if ( rRef.IsTabRel() )
        rTab = aPos.Tab() + rRef.Tab();
    else
        rTab = rRef.Tab();

    if( !mrDoc.ValidCol( rCol) || rRef.IsColDeleted() )
    {
        SetError( FormulaError::NoRef );
        rCol = 0;
    }
    if( !mrDoc.ValidRow( rRow) || rRef.IsRowDeleted() )
    {
        SetError( FormulaError::NoRef );
        rRow = 0;
    }
    if( !ValidTab( rTab, mrDoc.GetTableCount() - 1) || rRef.IsTabDeleted() )
    {
        SetError( FormulaError::NoRef );
        rTab = 0;
    }
}

void ScInterpreter::PopSingleRef(SCCOL& rCol, SCROW &rRow, SCTAB& rTab)
{
    ScAddress aAddr(rCol, rRow, rTab);
    PopSingleRef(aAddr);
    rCol = aAddr.Col();
    rRow = aAddr.Row();
    rTab = aAddr.Tab();
}

void ScInterpreter::PopSingleRef( ScAddress& rAdr )
{
    if( sp )
    {
        --sp;
        const FormulaToken* p = pStack[ sp ];
        switch (p->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svSingleRef:
                {
                    const ScSingleRefData* pRefData = p->GetSingleRef();
                    if (pRefData->IsDeleted())
                    {
                        SetError( FormulaError::NoRef);
                        break;
                    }

                    SCCOL nCol;
                    SCROW nRow;
                    SCTAB nTab;
                    SingleRefToVars( *pRefData, nCol, nRow, nTab);
                    rAdr.Set( nCol, nRow, nTab );
                    if (!mrDoc.m_TableOpList.empty())
                        ReplaceCell( rAdr );
                }
                break;
            default:
                SetError( FormulaError::IllegalParameter);
        }
    }
    else
        SetError( FormulaError::UnknownStackVariable);
}

void ScInterpreter::DoubleRefToVars( const formula::FormulaToken* p,
        SCCOL& rCol1, SCROW &rRow1, SCTAB& rTab1,
        SCCOL& rCol2, SCROW &rRow2, SCTAB& rTab2 )
{
    const ScComplexRefData& rCRef = *p->GetDoubleRef();
    SingleRefToVars( rCRef.Ref1, rCol1, rRow1, rTab1);
    SingleRefToVars( rCRef.Ref2, rCol2, rRow2, rTab2);
    PutInOrder(rCol1, rCol2);
    PutInOrder(rRow1, rRow2);
    PutInOrder(rTab1, rTab2);
    if (!mrDoc.m_TableOpList.empty())
    {
        ScRange aRange( rCol1, rRow1, rTab1, rCol2, rRow2, rTab2 );
        if ( IsTableOpInRange( aRange ) )
            SetError( FormulaError::IllegalParameter );
    }
}

ScDBRangeBase* ScInterpreter::PopDBDoubleRef()
{
    StackVar eType = GetStackType();
    switch (eType)
    {
        case svUnknown:
            SetError(FormulaError::UnknownStackVariable);
        break;
        case svError:
            PopError();
        break;
        case svDoubleRef:
        {
            SCCOL nCol1, nCol2;
            SCROW nRow1, nRow2;
            SCTAB nTab1, nTab2;
            PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
            if (nGlobalError != FormulaError::NONE)
                break;
            return new ScDBInternalRange(&mrDoc,
                ScRange(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2));
        }
        case svMatrix:
        case svExternalDoubleRef:
        {
            ScMatrixRef pMat;
            if (eType == svMatrix)
                pMat = PopMatrix();
            else
                PopExternalDoubleRef(pMat);
            if (nGlobalError != FormulaError::NONE)
                break;
            return new ScDBExternalRange(&mrDoc, std::move(pMat));
        }
        default:
            SetError( FormulaError::IllegalParameter);
    }

    return nullptr;
}

void ScInterpreter::PopDoubleRef(SCCOL& rCol1, SCROW &rRow1, SCTAB& rTab1,
                                 SCCOL& rCol2, SCROW &rRow2, SCTAB& rTab2)
{
    if( sp )
    {
        --sp;
        const FormulaToken* p = pStack[ sp ];
        switch (p->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svDoubleRef:
                DoubleRefToVars( p, rCol1, rRow1, rTab1, rCol2, rRow2, rTab2);
                break;
            default:
                SetError( FormulaError::IllegalParameter);
        }
    }
    else
        SetError( FormulaError::UnknownStackVariable);
}

void ScInterpreter::DoubleRefToRange( const ScComplexRefData & rCRef,
        ScRange & rRange, bool bDontCheckForTableOp )
{
    SCCOL nCol;
    SCROW nRow;
    SCTAB nTab;
    SingleRefToVars( rCRef.Ref1, nCol, nRow, nTab);
    rRange.aStart.Set( nCol, nRow, nTab );
    SingleRefToVars( rCRef.Ref2, nCol, nRow, nTab);
    rRange.aEnd.Set( nCol, nRow, nTab );
    rRange.PutInOrder();
    if (!mrDoc.m_TableOpList.empty() && !bDontCheckForTableOp)
    {
        if ( IsTableOpInRange( rRange ) )
            SetError( FormulaError::IllegalParameter );
    }
}

void ScInterpreter::PopDoubleRef( ScRange & rRange, short & rParam, size_t & rRefInList )
{
    if (sp)
    {
        const formula::FormulaToken* pToken = pStack[ sp-1 ];
        switch (pToken->GetType())
        {
            case svError:
                nGlobalError = pToken->GetError();
                break;
            case svDoubleRef:
            {
                --sp;
                const ScComplexRefData* pRefData = pToken->GetDoubleRef();
                if (pRefData->IsDeleted())
                {
                    SetError( FormulaError::NoRef);
                    break;
                }
                DoubleRefToRange( *pRefData, rRange);
                break;
            }
            case svRefList:
                {
                    const ScRefList* pList = pToken->GetRefList();
                    if (rRefInList < pList->size())
                    {
                        DoubleRefToRange( (*pList)[rRefInList], rRange);
                        if (++rRefInList < pList->size())
                            ++rParam;
                        else
                        {
                            --sp;
                            rRefInList = 0;
                        }
                    }
                    else
                    {
                        --sp;
                        rRefInList = 0;
                        SetError( FormulaError::IllegalParameter);
                    }
                }
                break;
            default:
                SetError( FormulaError::IllegalParameter);
        }
    }
    else
        SetError( FormulaError::UnknownStackVariable);
}

void ScInterpreter::PopDoubleRef( ScRange& rRange, bool bDontCheckForTableOp )
{
    if( sp )
    {
        --sp;
        const FormulaToken* p = pStack[ sp ];
        switch (p->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svDoubleRef:
                DoubleRefToRange( *p->GetDoubleRef(), rRange, bDontCheckForTableOp);
                break;
            default:
                SetError( FormulaError::IllegalParameter);
        }
    }
    else
        SetError( FormulaError::UnknownStackVariable);
}

const ScComplexRefData* ScInterpreter::GetStackDoubleRef(size_t rRefInList)
{
    if( sp )
    {
        const FormulaToken* p = pStack[ sp - 1 ];
        switch (p->GetType())
        {
            case svDoubleRef:
                return p->GetDoubleRef();
            case svRefList:
            {
                const ScRefList* pList = p->GetRefList();
                if (rRefInList < pList->size())
                    return &(*pList)[rRefInList];
                break;
            }
            default:
                break;
        }
    }
    return nullptr;
}

void ScInterpreter::PopExternalSingleRef(sal_uInt16& rFileId, OUString& rTabName, ScSingleRefData& rRef)
{
    if (!sp)
    {
        SetError(FormulaError::UnknownStackVariable);
        return;
    }

    --sp;
    const FormulaToken* p = pStack[sp];
    StackVar eType = p->GetType();

    if (eType == svError)
    {
        nGlobalError = p->GetError();
        return;
    }

    if (eType != svExternalSingleRef)
    {
        SetError( FormulaError::IllegalParameter);
        return;
    }

    rFileId = p->GetIndex();
    rTabName = p->GetString().getString();
    rRef = *p->GetSingleRef();
}

void ScInterpreter::PopExternalSingleRef(ScExternalRefCache::TokenRef& rToken, ScExternalRefCache::CellFormat* pFmt)
{
    sal_uInt16 nFileId;
    OUString aTabName;
    ScSingleRefData aData;
    PopExternalSingleRef(nFileId, aTabName, aData, rToken, pFmt);
}

void ScInterpreter::PopExternalSingleRef(
    sal_uInt16& rFileId, OUString& rTabName, ScSingleRefData& rRef,
    ScExternalRefCache::TokenRef& rToken, ScExternalRefCache::CellFormat* pFmt)
{
    PopExternalSingleRef(rFileId, rTabName, rRef);
    if (nGlobalError != FormulaError::NONE)
        return;

    const auto aFetch = seexternalexec::fetchExternalSingleRef(mrDoc, aPos, rFileId, rTabName, rRef);
    if (aFetch.meError != FormulaError::NONE)
    {
        SetError(aFetch.meError);
        return;
    }

    rToken = aFetch.mxToken;
    if (pFmt)
        *pFmt = aFetch.maFormat;
}

void ScInterpreter::PopExternalDoubleRef(sal_uInt16& rFileId, OUString& rTabName, ScComplexRefData& rRef)
{
    if (!sp)
    {
        SetError(FormulaError::UnknownStackVariable);
        return;
    }

    --sp;
    const FormulaToken* p = pStack[sp];
    StackVar eType = p->GetType();

    if (eType == svError)
    {
        nGlobalError = p->GetError();
        return;
    }

    if (eType != svExternalDoubleRef)
    {
        SetError( FormulaError::IllegalParameter);
        return;
    }

    rFileId = p->GetIndex();
    rTabName = p->GetString().getString();
    rRef = *p->GetDoubleRef();
}

void ScInterpreter::PopExternalDoubleRef(ScExternalRefCache::TokenArrayRef& rArray)
{
    // Host-only by design: this path still crosses the document/session
    // external-reference cache and returns Calc-owned token containers.
    sal_uInt16 nFileId;
    OUString aTabName;
    ScComplexRefData aData;
    PopExternalDoubleRef(nFileId, aTabName, aData);
    if (nGlobalError != FormulaError::NONE)
        return;

    GetExternalDoubleRef(nFileId, aTabName, aData, rArray);
    if (nGlobalError != FormulaError::NONE)
        return;
}

void ScInterpreter::PopExternalDoubleRef(ScMatrixRef& rMat)
{
    ScExternalRefCache::TokenArrayRef pArray;
    PopExternalDoubleRef(pArray);
    if (nGlobalError != FormulaError::NONE)
        return;

    const auto aProjection = seexternalexec::projectExternalDoubleRefMatrix(pArray);
    if (aProjection.meError != FormulaError::NONE)
        SetError(aProjection.meError);
    else
    {
        rMat = aProjection.mxMatrix;
    }
}

void ScInterpreter::GetExternalDoubleRef(
    sal_uInt16 nFileId, const OUString& rTabName, const ScComplexRefData& rData, ScExternalRefCache::TokenArrayRef& rArray)
{
    // Kept in Calc intentionally because the cache lookup and returned token
    // arrays are still owned by the host interpreter/document layer.
    const auto aFetch = seexternalexec::fetchExternalDoubleRef(mrDoc, aPos, nFileId, rTabName, rData);
    if (aFetch.meError != FormulaError::NONE)
    {
        SetError(aFetch.meError);
        return;
    }

    rArray = aFetch.mxArray;
}

bool ScInterpreter::PopDoubleRefOrSingleRef( ScAddress& rAdr )
{
    switch ( GetStackType() )
    {
        case svDoubleRef :
        {
            ScRange aRange;
            PopDoubleRef( aRange, true );
            return DoubleRefToPosSingleRef( aRange, rAdr );
        }
        case svSingleRef :
        {
            PopSingleRef( rAdr );
            return true;
        }
        default:
            PopError();
            SetError( FormulaError::NoRef );
    }
    return false;
}

void ScInterpreter::PopDoubleRefPushMatrix()
{
    if ( GetStackType() == svDoubleRef )
    {
        ScMatrixRef pMat = GetMatrix();
        if ( pMat )
            PushMatrix( pMat );
        else
            PushIllegalParameter();
    }
    else
        SetError( FormulaError::NoRef );
}

void ScInterpreter::PopRefListPushMatrixOrRef()
{
    if ( GetStackType() == svRefList )
    {
        FormulaConstTokenRef xTok = pStack[sp-1];
        const std::vector<ScComplexRefData>* pv = xTok->GetRefList();
        if (pv)
        {
            const size_t nEntries = pv->size();
            const auto aPlan = serefexec::planReferenceListMaterialization(
                nEntries, bMatrixFormula, serefexec::allSingleCellReferences(*pv));
            if (!aPlan)
            {
                SetError(selibreoffice::toFormulaError(aPlan.meError));
                return;
            }

            if (aPlan.maValue.meKind
                == spreadsheetengine::api::reference::ReferenceListMaterializationKind::SingleReference)
            {
                --sp;
                PushTempTokenWithoutError(new ScDoubleRefToken(mrDoc.GetSheetLimits(), (*pv)[0]));
            }
            else if (aPlan.maValue.meKind
                     == spreadsheetengine::api::reference::ReferenceListMaterializationKind::ColumnVector)
            {
                const auto aMatrix
                    = serefexec::materializeReferenceListColumnVector(mrDoc, aPos, *pv);
                if (!aMatrix)
                {
                    SetError(selibreoffice::toFormulaError(aMatrix.meError));
                    return;
                }
                --sp;
                PushMatrix(aMatrix.maValue);
            }
        }
        // else: keep token on stack, something will handle the error
    }
    else
        SetError( FormulaError::NoRef );
}

void ScInterpreter::ConvertMatrixJumpConditionToMatrix()
{
    StackVar eStackType = GetStackType();
    if (eStackType == svUnknown)
        return;     // can't do anything, some caller will catch that
    if (eStackType == svMatrix)
        return;     // already matrix, nothing to do

    if (!spreadsheetengine::compat::libreoffice::matrixframeexecution::
            shouldConvertJumpConditionToMatrix(eStackType, GetStackType(2)))
        return;     // always convert svDoubleRef, others only in JumpMatrix context

    GetTokenMatrixMap();    // make sure it exists, create if not.
    ScMatrixRef pMat = GetMatrix();
    if ( pMat )
        PushMatrix( pMat );
    else
        PushIllegalParameter();
}

bool ScInterpreter::ConvertMatrixParameters()
{
    sal_uInt16 nParams = pCur->GetParamCount();
    SAL_WARN_IF( nParams > sp, "sc.core", "ConvertMatrixParameters: stack/param count mismatch:  eOp: "
            << static_cast<int>(pCur->GetOpCode()) << "  sp: " << sp << "  nParams: " << nParams);
    assert(nParams <= sp);
    SCSIZE nJumpCols = 0, nJumpRows = 0;
    for ( sal_uInt16 i=1; i <= nParams && i <= sp; ++i )
    {
        const FormulaToken* p = pStack[ sp - i ];
        if ( p->GetOpCode() != ocPush && p->GetOpCode() != ocMissing)
        {
            assert(!"ConvertMatrixParameters: not a push");
        }
        else
        {
            switch ( p->GetType() )
            {
                case svDouble:
                case svString:
                case svStringName:
                case svSingleRef:
                case svExternalSingleRef:
                case svMissing:
                case svError:
                case svEmptyCell:
                    // nothing to do
                break;
                case svMatrix:
                {
                    if (spreadsheetengine::compat::libreoffice::matrixframeexecution::
                            shouldTrackValueParameterDimensions(
                                ScParameterClassification::GetParameterType(pCur, nParams - i)))
                    {   // only if single value expected
                        ScConstMatrixRef pMat = p->GetMatrix();
                        if ( !pMat )
                            SetError( FormulaError::UnknownVariable);
                        else
                        {
                            SCSIZE nCols, nRows;
                            pMat->GetDimensions( nCols, nRows);
                            if ( nJumpCols < nCols )
                                nJumpCols = nCols;
                            if ( nJumpRows < nRows )
                                nJumpRows = nRows;
                        }
                    }
                }
                break;
                case svDoubleRef:
                {
                    formula::ParamClass eType = ScParameterClassification::GetParameterType( pCur, nParams - i);
                    if (spreadsheetengine::compat::libreoffice::matrixframeexecution::
                            shouldConvertDoubleRefParameter(eType, IsInArrayContext()))
                    {
                        SCCOL nCol1, nCol2;
                        SCROW nRow1, nRow2;
                        SCTAB nTab1, nTab2;
                        DoubleRefToVars( p, nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                        // Make sure the map exists, created if not.
                        GetTokenMatrixMap();
                        ScMatrixRef pMat = CreateMatrixFromDoubleRef( p,
                                nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                        if (pMat)
                        {
                            if (spreadsheetengine::compat::libreoffice::matrixframeexecution::
                                    shouldTrackValueParameterDimensions(eType))
                            {   // only if single value expected
                                if ( nJumpCols < o3tl::make_unsigned(nCol2 - nCol1 + 1) )
                                    nJumpCols = static_cast<SCSIZE>(nCol2 - nCol1 + 1);
                                if ( nJumpRows < o3tl::make_unsigned(nRow2 - nRow1 + 1) )
                                    nJumpRows = static_cast<SCSIZE>(nRow2 - nRow1 + 1);
                            }
                            formula::FormulaToken* pNew = new ScMatrixToken( std::move(pMat) );
                            pNew->IncRef();
                            pStack[ sp - i ] = pNew;
                            p->DecRef();    // p may be dead now!
                        }
                    }
                }
                break;
                case svExternalDoubleRef:
                {
                    formula::ParamClass eType = ScParameterClassification::GetParameterType( pCur, nParams - i);
                    if (spreadsheetengine::compat::libreoffice::matrixframeexecution::
                            shouldConvertExternalDoubleRefParameter(eType))
                    {
                        sal_uInt16 nFileId = p->GetIndex();
                        OUString aTabName = p->GetString().getString();
                        const ScComplexRefData& rRef = *p->GetDoubleRef();
                        ScExternalRefCache::TokenArrayRef pArray;
                        GetExternalDoubleRef(nFileId, aTabName, rRef, pArray);
                        if (nGlobalError != FormulaError::NONE || !pArray)
                            break;
                        formula::FormulaToken* pTemp = pArray->FirstToken();
                        if (!pTemp)
                            break;

                        ScMatrixRef pMat = pTemp->GetMatrix();
                        if (pMat)
                        {
                            if (spreadsheetengine::compat::libreoffice::matrixframeexecution::
                                    shouldTrackValueParameterDimensions(eType))
                            {   // only if single value expected
                                SCSIZE nC, nR;
                                pMat->GetDimensions( nC, nR);
                                if (nJumpCols < nC)
                                    nJumpCols = nC;
                                if (nJumpRows < nR)
                                    nJumpRows = nR;
                            }
                            formula::FormulaToken* pNew = new ScMatrixToken( std::move(pMat) );
                            pNew->IncRef();
                            pStack[ sp - i ] = pNew;
                            p->DecRef();    // p may be dead now!
                        }
                    }
                }
                break;
                case svRefList:
                {
                    formula::ParamClass eType = ScParameterClassification::GetParameterType( pCur, nParams - i);
                    if (!spreadsheetengine::compat::libreoffice::matrixframeexecution::
                            allowsReferenceListParameter(eType))
                    {
                        // can't convert to matrix
                        SetError( FormulaError::NoRef);
                    }
                    // else: the consuming function has to decide if and how to
                    // handle a reference list argument in array context.
                }
                break;
                default:
                    assert(!"ConvertMatrixParameters: unknown parameter type");
            }
        }
    }
    if( nJumpCols && nJumpRows )
    {
        short nPC = aCode.GetPC();
        short nStart = nPC - 1;     // restart on current code (-1)
        short nNext = nPC;          // next instruction after subroutine
        short nStop = nPC + 1;      // stop subroutine before reaching that
        FormulaConstTokenRef xNew;
        ScTokenMatrixMap::const_iterator aMapIter;
        if ((aMapIter = maTokenMatrixMap.find( pCur)) != maTokenMatrixMap.end())
            xNew = (*aMapIter).second;
        else
        {
            std::shared_ptr<ScJumpMatrix> pJumpMat;
            try
            {
                pJumpMat = std::make_shared<ScJumpMatrix>( pCur->GetOpCode(), nJumpCols, nJumpRows);
            }
            catch (const std::bad_alloc&)
            {
                SAL_WARN("sc.core", "std::bad_alloc in ScJumpMatrix ctor with " << nJumpCols << " columns and " << nJumpRows << " rows");
                return false;
            }
            pJumpMat->SetAllJumps( 1.0, nStart, nNext, nStop);
            // pop parameters and store in ScJumpMatrix, push in JumpMatrix()
            ScTokenVec aParams(nParams);
            for ( sal_uInt16 i=1; i <= nParams && sp > 0; ++i )
            {
                const FormulaToken* p = pStack[ --sp ];
                p->IncRef();
                // store in reverse order such that a push may simply iterate
                aParams[ nParams - i ] = p;
            }
            pJumpMat->SetJumpParameters( std::move(aParams) );
            xNew = new ScJumpMatrixToken( std::move(pJumpMat) );
            GetTokenMatrixMap().emplace(pCur, xNew);
        }
        PushTempTokenWithoutError( xNew.get());
        // set continuation point of path for main code line
        aCode.Jump( nNext, nNext);
        return true;
    }
    return false;
}

ScMatrixRef ScInterpreter::PopMatrix()
{
    if( sp )
    {
        --sp;
        const FormulaToken* p = pStack[ sp ];
        switch (p->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svMatrix:
                {
                    // ScMatrix itself maintains an im/mutable flag that should
                    // be obeyed where necessary... so we can return ScMatrixRef
                    // here instead of ScConstMatrixRef.
                    ScMatrix* pMat = const_cast<FormulaToken*>(p)->GetMatrix();
                    if ( pMat )
                        pMat->SetErrorInterpreter( this);
                    else
                        SetError( FormulaError::UnknownVariable);
                    return pMat;
                }
            default:
                SetError( FormulaError::IllegalParameter);
        }
    }
    else
        SetError( FormulaError::UnknownStackVariable);
    return nullptr;
}

sc::RangeMatrix ScInterpreter::PopRangeMatrix()
{
    sc::RangeMatrix aRet;
    if (sp)
    {
        switch (pStack[sp-1]->GetType())
        {
            case svMatrix:
            {
                --sp;
                const FormulaToken* p = pStack[sp];
                aRet.mpMat = const_cast<FormulaToken*>(p)->GetMatrix();
                if (aRet.mpMat)
                {
                    aRet.mpMat->SetErrorInterpreter(this);
                    if (p->GetByte() == MATRIX_TOKEN_HAS_RANGE)
                    {
                        const ScComplexRefData& rRef = *p->GetDoubleRef();
                        if (!rRef.Ref1.IsColRel() && !rRef.Ref1.IsRowRel() && !rRef.Ref2.IsColRel() && !rRef.Ref2.IsRowRel())
                        {
                            aRet.mnCol1 = rRef.Ref1.Col();
                            aRet.mnRow1 = rRef.Ref1.Row();
                            aRet.mnTab1 = rRef.Ref1.Tab();
                            aRet.mnCol2 = rRef.Ref2.Col();
                            aRet.mnRow2 = rRef.Ref2.Row();
                            aRet.mnTab2 = rRef.Ref2.Tab();
                        }
                    }
                }
                else
                    SetError( FormulaError::UnknownVariable);
            }
            break;
            default:
                aRet.mpMat = PopMatrix();
        }
    }
    return aRet;
}

void ScInterpreter::QueryMatrixType(const ScMatrixRef& xMat, SvNumFormatType& rRetTypeExpr, sal_uInt32& rRetIndexExpr)
{
    if (xMat)
    {
        SCSIZE nCols, nRows;
        xMat->GetDimensions(nCols, nRows);
        ScMatrixValue nMatVal = xMat->Get(0, 0);
        ScMatValType nMatValType = nMatVal.nType;
        if (ScMatrix::IsNonValueType( nMatValType))
        {
            if ( xMat->IsEmptyPath( 0, 0))
            {   // result of empty FALSE jump path
                FormulaTokenRef xRes = CreateFormulaDoubleToken( 0.0);
                PushTempToken( new ScMatrixFormulaCellToken(nCols, nRows, xMat, xRes.get()));
                rRetTypeExpr = SvNumFormatType::LOGICAL;
            }
            else if ( xMat->IsEmptyResult( 0, 0))
            {   // empty formula result
                FormulaTokenRef xRes = new ScEmptyCellToken( true, true);   // inherited, display empty
                PushTempToken( new ScMatrixFormulaCellToken(nCols, nRows, xMat, xRes.get()));
            }
            else if ( xMat->IsEmpty( 0, 0))
            {   // empty or empty cell
                FormulaTokenRef xRes = new ScEmptyCellToken( false, true);  // not inherited, display empty
                PushTempToken( new ScMatrixFormulaCellToken(nCols, nRows, xMat, xRes.get()));
            }
            else
            {
                FormulaTokenRef xRes = new FormulaStringToken( nMatVal.GetString() );
                PushTempToken( new ScMatrixFormulaCellToken(nCols, nRows, xMat, xRes.get()));
                rRetTypeExpr = SvNumFormatType::TEXT;
            }
        }
        else
        {
            FormulaError nErr = GetDoubleErrorValue( nMatVal.fVal);
            FormulaTokenRef xRes;
            if (nErr != FormulaError::NONE)
                xRes = new FormulaErrorToken( nErr);
            else
                xRes = CreateFormulaDoubleToken( nMatVal.fVal);
            PushTempToken( new ScMatrixFormulaCellToken(nCols, nRows, xMat, xRes.get()));
            if ( rRetTypeExpr != SvNumFormatType::LOGICAL )
                rRetTypeExpr = SvNumFormatType::NUMBER;
        }
        rRetIndexExpr = 0;
        xMat->SetErrorInterpreter( nullptr);
    }
    else
        SetError( FormulaError::UnknownStackVariable);
}

formula::FormulaToken* ScInterpreter::CreateFormulaDoubleToken( double fVal, SvNumFormatType nFmt )
{
    assert( mrContext.maTokens.size() == TOKEN_CACHE_SIZE );

    // Find a spare token
    if (auto p = spreadsheetengine::core::execution::findReusableCachedToken(
            mrContext.maTokens, [](formula::FormulaTypedDoubleToken* pToken) {
                return pToken->GetRef() == 1;
            }))
    {
        p->SetDouble(fVal);
        p->SetDoubleType( static_cast<sal_Int16>(nFmt) );
        return p;
    }

    // Allocate a new token
    auto p = new FormulaTypedDoubleToken( fVal, static_cast<sal_Int16>(nFmt) );
    p->SetRefCntPolicy(RefCntPolicy::UnsafeRef);
    spreadsheetengine::core::execution::replaceCachedToken(
        mrContext.maTokens, mrContext.mnTokenCachePos, p,
        [](formula::FormulaTypedDoubleToken* pToken) { pToken->DecRef(); },
        [](formula::FormulaTypedDoubleToken* pToken) { pToken->IncRef(); });
    return p;
}

formula::FormulaToken* ScInterpreter::CreateDoubleOrTypedToken( double fVal )
{
    // NumberFormat::NUMBER is the default untyped double.
    if (nFuncFmtType != SvNumFormatType::ALL && nFuncFmtType != SvNumFormatType::NUMBER &&
            nFuncFmtType != SvNumFormatType::UNDEFINED)
        return CreateFormulaDoubleToken( fVal, nFuncFmtType);
    else
        return CreateFormulaDoubleToken( fVal);
}

void ScInterpreter::PushDouble(double nVal)
{
    TreatDoubleError( nVal );
    if (!IfErrorPushError())
        PushTempTokenWithoutError( CreateDoubleOrTypedToken( nVal));
}

void ScInterpreter::PushInt(int nVal)
{
    if (!IfErrorPushError())
        PushTempTokenWithoutError( CreateDoubleOrTypedToken( nVal));
}

void ScInterpreter::PushStringBuffer( const sal_Unicode* pString )
{
    if ( pString )
    {
        svl::SharedString aSS = mrDoc.GetSharedStringPool().intern(OUString(pString));
        PushString(aSS);
    }
    else
        PushString(svl::SharedString::getEmptyString());
}

void ScInterpreter::PushString( const OUString& rStr )
{
    PushString(mrDoc.GetSharedStringPool().intern(rStr));
}

void ScInterpreter::PushString( const svl::SharedString& rString )
{
    if (!IfErrorPushError())
        PushTempTokenWithoutError( new FormulaStringToken( rString ) );
}

void ScInterpreter::PushSingleRef(SCCOL nCol, SCROW nRow, SCTAB nTab)
{
    if (!IfErrorPushError())
    {
        ScSingleRefData aRef;
        aRef.InitAddress(ScAddress(nCol,nRow,nTab));
        PushTempTokenWithoutError( new ScSingleRefToken( mrDoc.GetSheetLimits(), aRef ) );
    }
}

void ScInterpreter::PushDoubleRef(SCCOL nCol1, SCROW nRow1, SCTAB nTab1,
                                  SCCOL nCol2, SCROW nRow2, SCTAB nTab2)
{
    if (!IfErrorPushError())
    {
        ScComplexRefData aRef;
        aRef.InitRange(ScRange(nCol1,nRow1,nTab1,nCol2,nRow2,nTab2));
        PushTempTokenWithoutError( new ScDoubleRefToken( mrDoc.GetSheetLimits(), aRef ) );
    }
}

void ScInterpreter::PushExternalSingleRef(
    sal_uInt16 nFileId, const OUString& rTabName, SCCOL nCol, SCROW nRow, SCTAB nTab)
{
    if (!IfErrorPushError())
    {
        ScSingleRefData aRef;
        aRef.InitAddress(ScAddress(nCol,nRow,nTab));
        PushTempTokenWithoutError( new ScExternalSingleRefToken(nFileId,
                    mrDoc.GetSharedStringPool().intern( rTabName), aRef)) ;
    }
}

void ScInterpreter::PushExternalDoubleRef(
    sal_uInt16 nFileId, const OUString& rTabName,
    SCCOL nCol1, SCROW nRow1, SCTAB nTab1, SCCOL nCol2, SCROW nRow2, SCTAB nTab2)
{
    if (!IfErrorPushError())
    {
        ScComplexRefData aRef;
        aRef.InitRange(ScRange(nCol1,nRow1,nTab1,nCol2,nRow2,nTab2));
        PushTempTokenWithoutError( new ScExternalDoubleRefToken(nFileId,
                    mrDoc.GetSharedStringPool().intern( rTabName), aRef) );
    }
}

void ScInterpreter::PushSingleRef( const ScRefAddress& rRef )
{
    if (!IfErrorPushError())
    {
        ScSingleRefData aRef;
        aRef.InitFromRefAddress( mrDoc, rRef, aPos);
        PushTempTokenWithoutError( new ScSingleRefToken( mrDoc.GetSheetLimits(), aRef ) );
    }
}

void ScInterpreter::PushDoubleRef( const ScRefAddress& rRef1, const ScRefAddress& rRef2 )
{
    if (!IfErrorPushError())
    {
        ScComplexRefData aRef;
        aRef.InitFromRefAddresses( mrDoc, rRef1, rRef2, aPos);
        PushTempTokenWithoutError( new ScDoubleRefToken( mrDoc.GetSheetLimits(), aRef ) );
    }
}

void ScInterpreter::PushMatrix( const sc::RangeMatrix& rMat )
{
    if (!rMat.isRangeValid())
    {
        // Just push the matrix part only.
        PushMatrix(rMat.mpMat);
        return;
    }

    rMat.mpMat->SetErrorInterpreter(nullptr);
    nGlobalError = FormulaError::NONE;
    PushTempTokenWithoutError(new ScMatrixRangeToken(rMat));
}

void ScInterpreter::PushMatrix(const ScMatrixRef& pMat)
{
    pMat->SetErrorInterpreter( nullptr);
    // No   if (!IfErrorPushError())   because ScMatrix stores errors itself,
    // but with notifying ScInterpreter via nGlobalError, substituting it would
    // mean to inherit the error on all array elements in all following
    // operations.
    nGlobalError = FormulaError::NONE;
    PushTempTokenWithoutError( new ScMatrixToken( pMat ) );
}

void ScInterpreter::PushError( FormulaError nError )
{
    SetError( nError );     // only sets error if not already set
    PushTempTokenWithoutError( new FormulaErrorToken( nGlobalError));
}

void ScInterpreter::PushParameterExpected()
{
    PushError( FormulaError::ParameterExpected);
}

void ScInterpreter::PushIllegalParameter()
{
    PushError( FormulaError::IllegalParameter);
}

void ScInterpreter::PushIllegalArgument()
{
    PushError( FormulaError::IllegalArgument);
}

void ScInterpreter::PushNA()
{
    PushError( FormulaError::NotAvailable);
}

void ScInterpreter::PushNoValue()
{
    PushError( FormulaError::NoValue);
}

bool ScInterpreter::IsMissing() const
{
    return sp && pStack[sp - 1]->GetType() == svMissing;
}

StackVar ScInterpreter::GetRawStackType()
{
    if( sp )
    {
        return pStack[sp - 1]->GetType();
    }
    else
    {
        SetError(FormulaError::UnknownStackVariable);
        return svUnknown;
    }
}

StackVar ScInterpreter::GetStackType()
{
    switch (StackVar eRes = GetRawStackType())
    {
        case svMissing:
        case svEmptyCell:
            return svDouble; // default!
        default:
            return eRes;
    }
}

StackVar ScInterpreter::GetStackType( sal_uInt8 nParam )
{
    StackVar eRes;
    if( sp > nParam-1 )
    {
        eRes = pStack[sp - nParam]->GetType();
        if( eRes == svMissing || eRes == svEmptyCell )
            eRes = svDouble;    // default!
    }
    else
        eRes = svUnknown;
    return eRes;
}

void ScInterpreter::ReverseStack( sal_uInt8 nParamCount )
{
    //reverse order of parameter stack
    assert( sp >= nParamCount && " less stack elements than parameters");
    sal_uInt16 nStackParams = std::min<sal_uInt16>( sp, nParamCount);
    std::reverse( pStack+(sp-nStackParams), pStack+sp );
}

bool ScInterpreter::DoubleRefToPosSingleRef( const ScRange& rRange, ScAddress& rAdr )
{
    if ( pJumpMatrix )
    {
        SCSIZE nC = 0;
        SCSIZE nR = 0;
        pJumpMatrix->GetPos( nC, nR);
        const auto aSelection = serefexec::selectScalarReferenceCell(
            rRange, aPos,
            spreadsheetengine::api::MatrixCoordinate { static_cast<spreadsheetengine::api::MatrixSize>(nC),
                static_cast<spreadsheetengine::api::MatrixSize>(nR) });
        if (!aSelection)
        {
            SetError(selibreoffice::toFormulaError(aSelection.meError));
            return false;
        }
        rAdr = aSelection.maValue;
        return true;
    }

    const auto aSelection = serefexec::selectScalarReferenceCell(rRange, aPos);
    if (!aSelection)
    {
        SetError(selibreoffice::toFormulaError(aSelection.meError));
        return false;
    }
    rAdr = aSelection.maValue;
    return true;
}

double ScInterpreter::GetDoubleFromMatrix(const ScMatrixRef& pMat)
{
    if (!pMat)
        return 0.0;

    if ( !pJumpMatrix )
    {
        double fVal = pMat->GetDoubleWithStringConversion( 0, 0);
        FormulaError nErr = GetDoubleErrorValue( fVal);
        if (nErr != FormulaError::NONE)
        {
            // Do not propagate the coded double error, but set nGlobalError in
            // case the matrix did not have an error interpreter set.
            SetError( nErr);
            fVal = 0.0;
        }
        return fVal;
    }

    SCSIZE nCols, nRows, nC, nR;
    pMat->GetDimensions( nCols, nRows);
    pJumpMatrix->GetPos( nC, nR);
    // Use vector replication for single row/column arrays.
    if ( (nC < nCols || nCols == 1) && (nR < nRows || nRows == 1) )
    {
        double fVal = pMat->GetDoubleWithStringConversion( nC, nR);
        FormulaError nErr = GetDoubleErrorValue( fVal);
        if (nErr != FormulaError::NONE)
        {
            // Do not propagate the coded double error, but set nGlobalError in
            // case the matrix did not have an error interpreter set.
            SetError( nErr);
            fVal = 0.0;
        }
        return fVal;
    }

    SetError( FormulaError::NoValue);
    return 0.0;
}

double ScInterpreter::GetDouble()
{
    double nVal;
    switch( GetRawStackType() )
    {
        case svDouble:
            nVal = PopDouble();
        break;
        case svString:
            nVal = ConvertStringToValue( PopString().getString());
        break;
        case svSingleRef:
        {
            ScAddress aAdr;
            PopSingleRef( aAdr );
            ScRefCellValue aCell(mrDoc, aAdr);
            nVal = GetCellValue(aAdr, aCell);
        }
        break;
        case svDoubleRef:
        {   // generate position dependent SingleRef
            ScRange aRange;
            PopDoubleRef( aRange );
            ScAddress aAdr;
            if ( nGlobalError == FormulaError::NONE && DoubleRefToPosSingleRef( aRange, aAdr ) )
            {
                ScRefCellValue aCell(mrDoc, aAdr);
                nVal = GetCellValue(aAdr, aCell);
            }
            else
                nVal = 0.0;
        }
        break;
        case svExternalSingleRef:
        {
            ScExternalRefCache::TokenRef pToken;
            PopExternalSingleRef(pToken);
            if (nGlobalError != FormulaError::NONE)
            {
                nVal = 0.0;
                break;
            }

            if (pToken->GetType() == svDouble || pToken->GetType() == svEmptyCell)
                nVal = pToken->GetDouble();
            else
                nVal = ConvertStringToValue( pToken->GetString().getString());
        }
        break;
        case svExternalDoubleRef:
        {
            ScMatrixRef pMat;
            PopExternalDoubleRef(pMat);
            if (nGlobalError != FormulaError::NONE)
            {
                nVal = 0.0;
                break;
            }

            nVal = GetDoubleFromMatrix(pMat);
        }
        break;
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            nVal = GetDoubleFromMatrix(pMat);
        }
        break;
        case svError:
            PopError();
            nVal = 0.0;
        break;
        case svEmptyCell:
        case svMissing:
            Pop();
            nVal = 0.0;
        break;
        default:
            PopError();
            SetError( FormulaError::IllegalParameter);
            nVal = 0.0;
    }
    if ( nFuncFmtType == nCurFmtType )
        nFuncFmtIndex = nCurFmtIndex;
    return nVal;
}

double ScInterpreter::GetDoubleWithDefault(double nDefault)
{
    if (!IsMissing())
        return GetDouble();
    Pop();
    return nDefault;
}

bool ScInterpreter::GetBoolWithDefault(bool bDefault)
{
    return GetDoubleWithDefault(bDefault ? 1.0 : 0.0) != 0.0;
}

template <typename Int>
    requires std::is_integral_v<Int>
Int ScInterpreter::double_to(double fVal)
{
    if (!std::isfinite(fVal))
    {
        SetError( GetDoubleErrorValue( fVal));
        return std::numeric_limits<Int>::max();
    }
    if (fVal > 0.0)
    {
        fVal = rtl::math::approxFloor( fVal);
        if (fVal > std::numeric_limits<Int>::max())
        {
            SetError( FormulaError::IllegalArgument);
            return std::numeric_limits<Int>::max();
        }
    }
    else if (fVal < 0.0)
    {
        fVal = rtl::math::approxCeil( fVal);
        if (fVal < std::numeric_limits<Int>::min())
        {
            SetError( FormulaError::IllegalArgument);
            return std::numeric_limits<Int>::max();
        }
    }
    return static_cast<Int>(fVal);
}

sal_Int32 ScInterpreter::double_to_int32(double fVal)
{
    return double_to<sal_Int32>(fVal);
}

sal_Int32 ScInterpreter::GetInt32()
{
    return double_to_int32(GetDouble());
}

sal_Int32 ScInterpreter::GetInt32WithDefault( sal_Int32 nDefault )
{
    return double_to_int32(GetDoubleWithDefault(nDefault));
}

sal_Int32 ScInterpreter::GetFloor32()
{
    double fVal = GetDouble();
    if (!std::isfinite(fVal))
    {
        SetError( GetDoubleErrorValue( fVal));
        return SAL_MAX_INT32;
    }
    fVal = rtl::math::approxFloor( fVal);
    if (fVal < SAL_MIN_INT32 || SAL_MAX_INT32 < fVal)
    {
        SetError( FormulaError::IllegalArgument);
        return SAL_MAX_INT32;
    }
    return static_cast<sal_Int32>(fVal);
}

sal_Int16 ScInterpreter::GetInt16()
{
    return double_to<sal_Int16>(GetDouble());
}

sal_Int16 ScInterpreter::GetInt16WithDefault(sal_Int16 nDefault)
{
    return double_to<sal_Int16>(GetDoubleWithDefault(nDefault));
}

sal_uInt32 ScInterpreter::GetUInt32()
{
    return double_to<sal_uInt32>(GetDouble());
}

bool ScInterpreter::GetDoubleOrString( double& rDouble, svl::SharedString& rString )
{
    bool bDouble = true;
    switch( GetRawStackType() )
    {
        case svDouble:
            rDouble = PopDouble();
        break;
        case svString:
            rString = PopString();
            bDouble = false;
        break;
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if (!PopDoubleRefOrSingleRef( aAdr))
            {
                rDouble = 0.0;
                return true;    // caller needs to check nGlobalError
            }
            ScRefCellValue aCell( mrDoc, aAdr);
            if (aCell.hasNumeric())
            {
                rDouble = GetCellValue( aAdr, aCell);
            }
            else
            {
                GetCellString( rString, aCell);
                bDouble = false;
            }
        }
        break;
        case svExternalSingleRef:
        case svExternalDoubleRef:
        case svMatrix:
        {
            ScMatValType nType = GetDoubleOrStringFromMatrix( rDouble, rString);
            bDouble = ScMatrix::IsValueType( nType);
        }
        break;
        case svError:
            PopError();
            rDouble = 0.0;
        break;
        case svEmptyCell:
        case svMissing:
            Pop();
            rDouble = 0.0;
        break;
        default:
            PopError();
            SetError( FormulaError::IllegalParameter);
            rDouble = 0.0;
    }
    if ( nFuncFmtType == nCurFmtType )
        nFuncFmtIndex = nCurFmtIndex;
    return bDouble;
}

svl::SharedString ScInterpreter::GetString()
{
    switch (GetRawStackType())
    {
        case svError:
            PopError();
            return svl::SharedString::getEmptyString();
        case svMissing:
        case svEmptyCell:
            Pop();
            return svl::SharedString::getEmptyString();
        case svDouble:
        {
            return GetStringFromDouble( PopDouble() );
        }
        case svString:
        case svStringName:
            return PopString();
        case svSingleRef:
        {
            ScAddress aAdr;
            PopSingleRef( aAdr );
            if (nGlobalError == FormulaError::NONE)
            {
                ScRefCellValue aCell(mrDoc, aAdr);
                svl::SharedString aSS;
                GetCellString(aSS, aCell);
                return aSS;
            }
            else
                return svl::SharedString::getEmptyString();
        }
        case svDoubleRef:
        {   // generate position dependent SingleRef
            ScRange aRange;
            PopDoubleRef( aRange );
            ScAddress aAdr;
            if ( nGlobalError == FormulaError::NONE && DoubleRefToPosSingleRef( aRange, aAdr ) )
            {
                ScRefCellValue aCell(mrDoc, aAdr);
                svl::SharedString aSS;
                GetCellString(aSS, aCell);
                return aSS;
            }
            else
                return svl::SharedString::getEmptyString();
        }
        case svExternalSingleRef:
        {
            ScExternalRefCache::TokenRef pToken;
            PopExternalSingleRef(pToken);
            if (nGlobalError != FormulaError::NONE)
                return svl::SharedString::getEmptyString();

            if (pToken->GetType() == svDouble)
            {
                return GetStringFromDouble( pToken->GetDouble() );
            }
            else // svString or svEmpty
                return pToken->GetString();
        }
        case svExternalDoubleRef:
        {
            ScMatrixRef pMat;
            PopExternalDoubleRef(pMat);
            return GetStringFromMatrix(pMat);
        }
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            return GetStringFromMatrix(pMat);
        }
        break;
        default:
            PopError();
            SetError( FormulaError::IllegalArgument);
    }
    return svl::SharedString::getEmptyString();
}

svl::SharedString ScInterpreter::GetStringFromMatrix(const ScMatrixRef& pMat)
{
    if ( !pMat )
        ;   // nothing
    else if ( !pJumpMatrix )
    {
        return pMat->GetString( mrContext, 0, 0);
    }
    else
    {
        SCSIZE nCols, nRows, nC, nR;
        pMat->GetDimensions( nCols, nRows);
        pJumpMatrix->GetPos( nC, nR);
        // Use vector replication for single row/column arrays.
        if ( (nC < nCols || nCols == 1) && (nR < nRows || nRows == 1) )
            return pMat->GetString( mrContext, nC, nR);

        SetError( FormulaError::NoValue);
    }
    return svl::SharedString::getEmptyString();
}

ScMatValType ScInterpreter::GetDoubleOrStringFromMatrix(
    double& rDouble, svl::SharedString& rString )
{

    rDouble = 0.0;
    rString = svl::SharedString::getEmptyString();
    ScMatValType nMatValType = ScMatValType::Empty;

    ScMatrixRef pMat;
    StackVar eType = GetStackType();
    if (eType == svExternalDoubleRef || eType == svExternalSingleRef || eType == svMatrix)
    {
        pMat = GetMatrix();
    }
    else
    {
        PopError();
        SetError( FormulaError::IllegalParameter);
        return nMatValType;
    }

    ScMatrixValue nMatVal;
    if (!pMat)
    {
        // nothing
    }
    else if (!pJumpMatrix)
    {
        nMatVal = pMat->Get(0, 0);
        nMatValType = nMatVal.nType;
    }
    else
    {
        SCSIZE nCols, nRows, nC, nR;
        pMat->GetDimensions( nCols, nRows);
        pJumpMatrix->GetPos( nC, nR);
        // Use vector replication for single row/column arrays.
        if ( (nC < nCols || nCols == 1) && (nR < nRows || nRows == 1) )
        {
            nMatVal = pMat->Get( nC, nR);
            nMatValType = nMatVal.nType;
        }
        else
            SetError( FormulaError::NoValue);
    }

    if (ScMatrix::IsValueType( nMatValType))
    {
        rDouble = nMatVal.fVal;
        FormulaError nError = nMatVal.GetError();
        if (nError != FormulaError::NONE)
            SetError( nError);
    }
    else
    {
        rString = nMatVal.GetString();
    }

    return nMatValType;
}

svl::SharedString ScInterpreter::GetStringFromDouble( double fVal )
{
    sal_uLong nIndex = mrContext.NFGetStandardFormat(
                        SvNumFormatType::NUMBER,
                        ScGlobal::eLnge);
    return mrStrPool.intern(mrContext.NFGetInputLineString(fVal, nIndex));
}

void ScInterpreter::ScDBGet()
{
    bool bMissingField = false;
    std::unique_ptr<ScDBQueryParamBase> pQueryParam( GetDBParams(bMissingField) );
    if (!pQueryParam)
    {
        // Failed to create query param.
        PushIllegalParameter();
        return;
    }

    pQueryParam->mbSkipString = false;
    ScDBQueryDataIterator aValIter(mrDoc, mrContext, std::move(pQueryParam));
    ScDBQueryDataIterator::Value aValue;
    if (!aValIter.GetFirst(aValue) || aValue.mnError != FormulaError::NONE)
    {
        // No match found.
        PushNoValue();
        return;
    }

    ScDBQueryDataIterator::Value aValNext;
    if (aValIter.GetNext(aValNext) && aValNext.mnError == FormulaError::NONE)
    {
        // There should be only one unique match.
        PushIllegalArgument();
        return;
    }

    if (aValue.mbIsNumber)
        PushDouble(aValue.mfValue);
    else
        PushString(aValue.maString);
}

void ScInterpreter::ScExternal()
{
    sal_uInt8 nParamCount = GetByte();
    OUString aUnoName;
    OUString aFuncName( pCur->GetExternal().toAsciiUpperCase());    // programmatic name
    LegacyFuncData* pLegacyFuncData = ScGlobal::GetLegacyFuncCollection()->findByName(aFuncName);
    if (pLegacyFuncData)
    {
        // Old binary non-UNO add-in function.
        // NOTE: parameter count is 1-based with the 0th "parameter" being the
        // return value, included in pLegacyFuncDatat->GetParamCount()
        if (nParamCount < MAXFUNCPARAM && nParamCount == pLegacyFuncData->GetParamCount() - 1)
        {
            ParamType   eParamType[MAXFUNCPARAM];
            void*       ppParam[MAXFUNCPARAM];
            double      nVal[MAXFUNCPARAM];
            char*       pStr[MAXFUNCPARAM];
            sal_uInt8*  pCellArr[MAXFUNCPARAM];
            short       i;

            for (i = 0; i < MAXFUNCPARAM; i++)
            {
                eParamType[i] = pLegacyFuncData->GetParamType(i);
                ppParam[i] = nullptr;
                nVal[i] = 0.0;
                pStr[i] = nullptr;
                pCellArr[i] = nullptr;
            }

            for (i = nParamCount; (i > 0) && (nGlobalError == FormulaError::NONE); i--)
            {
                if (IsMissing())
                {
                    // Old binary Add-In can't distinguish between missing
                    // omitted argument and 0 (or any other value). Force
                    // error.
                    SetError( FormulaError::ParameterExpected);
                    break;  // for
                }
                switch (eParamType[i])
                {
                    case ParamType::PTR_DOUBLE :
                        {
                            nVal[i-1] = GetDouble();
                            ppParam[i] = &nVal[i-1];
                        }
                        break;
                    case ParamType::PTR_STRING :
                        {
                            OString aStr(OUStringToOString(GetString().getString(),
                                osl_getThreadTextEncoding()));
                            if ( aStr.getLength() >= ADDIN_MAXSTRLEN )
                                SetError( FormulaError::StringOverflow );
                            else
                            {
                                pStr[i-1] = new char[ADDIN_MAXSTRLEN];
                                strncpy( pStr[i-1], aStr.getStr(), ADDIN_MAXSTRLEN );
                                pStr[i-1][ADDIN_MAXSTRLEN-1] = 0;
                                ppParam[i] = pStr[i-1];
                            }
                        }
                        break;
                    case ParamType::PTR_DOUBLE_ARR :
                        {
                            SCCOL nCol1;
                            SCROW nRow1;
                            SCTAB nTab1;
                            SCCOL nCol2;
                            SCROW nRow2;
                            SCTAB nTab2;
                            PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                            pCellArr[i-1] = new sal_uInt8[MAXARRSIZE];
                            if (!CreateDoubleArr(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2, pCellArr[i-1]))
                                SetError(FormulaError::CodeOverflow);
                            else
                                ppParam[i] = pCellArr[i-1];
                        }
                        break;
                    case ParamType::PTR_STRING_ARR :
                        {
                            SCCOL nCol1;
                            SCROW nRow1;
                            SCTAB nTab1;
                            SCCOL nCol2;
                            SCROW nRow2;
                            SCTAB nTab2;
                            PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                            pCellArr[i-1] = new sal_uInt8[MAXARRSIZE];
                            if (!CreateStringArr(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2, pCellArr[i-1]))
                                SetError(FormulaError::CodeOverflow);
                            else
                                ppParam[i] = pCellArr[i-1];
                        }
                        break;
                    case ParamType::PTR_CELL_ARR :
                        {
                            SCCOL nCol1;
                            SCROW nRow1;
                            SCTAB nTab1;
                            SCCOL nCol2;
                            SCROW nRow2;
                            SCTAB nTab2;
                            PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                            pCellArr[i-1] = new sal_uInt8[MAXARRSIZE];
                            if (!CreateCellArr(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2, pCellArr[i-1]))
                                SetError(FormulaError::CodeOverflow);
                            else
                                ppParam[i] = pCellArr[i-1];
                        }
                        break;
                    default :
                        SetError(FormulaError::IllegalParameter);
                        break;
                }
            }
            while ( i-- )
                Pop();      // In case of error (otherwise i==0) pop all parameters

            if (nGlobalError == FormulaError::NONE)
            {
                if ( pLegacyFuncData->GetAsyncType() == ParamType::NONE )
                {
                    switch ( eParamType[0] )
                    {
                        case ParamType::PTR_DOUBLE :
                        {
                            double nErg = 0.0;
                            ppParam[0] = &nErg;
                            pLegacyFuncData->Call(ppParam);
                            PushDouble(nErg);
                        }
                        break;
                        case ParamType::PTR_STRING :
                        {
                            std::unique_ptr<char[]> pcErg(new char[ADDIN_MAXSTRLEN]);
                            ppParam[0] = pcErg.get();
                            pLegacyFuncData->Call(ppParam);
                            OUString aUni( pcErg.get(), strlen(pcErg.get()), osl_getThreadTextEncoding() );
                            PushString( aUni );
                        }
                        break;
                        default:
                            PushError( FormulaError::UnknownState );
                    }
                }
                else
                {
                    // enable asyncs after loading
                    pArr->AddRecalcMode( ScRecalcMode::ONLOAD_LENIENT );
                    // assure identical handler with identical call?
                    double nErg = 0.0;
                    ppParam[0] = &nErg;
                    pLegacyFuncData->Call(ppParam);
                    sal_uLong nHandle = sal_uLong( nErg );
                    if ( nHandle >= 65536 )
                    {
                        ScAddInAsync* pAs = ScAddInAsync::Get( nHandle );
                        if ( !pAs )
                        {
                            pAs = new ScAddInAsync(nHandle, pLegacyFuncData, &mrDoc);
                            pMyFormulaCell->StartListening( *pAs );
                        }
                        else
                        {
                            pMyFormulaCell->StartListening( *pAs );
                            if ( !pAs->HasDocument( &mrDoc ) )
                                pAs->AddDocument( &mrDoc );
                        }
                        if ( pAs->IsValid() )
                        {
                            switch ( pAs->GetType() )
                            {
                                case ParamType::PTR_DOUBLE :
                                    PushDouble( pAs->GetValue() );
                                    break;
                                case ParamType::PTR_STRING :
                                    PushString( pAs->GetString() );
                                    break;
                                default:
                                    PushError( FormulaError::UnknownState );
                            }
                        }
                        else
                            PushNA();
                    }
                    else
                        PushNoValue();
                }
            }

            for (i = 0; i < MAXFUNCPARAM; i++)
            {
                delete[] pStr[i];
                delete[] pCellArr[i];
            }
        }
        else
        {
            while( nParamCount-- > 0)
                PopError();
            PushIllegalParameter();
        }
    }
    else if ( !( aUnoName = ScGlobal::GetAddInCollection()->FindFunction(aFuncName, false) ).isEmpty()  )
    {
        //  bLocalFirst=false in FindFunction, cFunc should be the stored
        //  internal name

        ScUnoAddInCall aCall( mrDoc, *ScGlobal::GetAddInCollection(), aUnoName, nParamCount );

        if ( !aCall.ValidParamCount() )
            SetError( FormulaError::IllegalParameter );

        if ( aCall.NeedsCaller() && GetError() == FormulaError::NONE )
        {
            ScDocShell* pShell = mrDoc.GetDocumentShell();
            if (pShell)
                aCall.SetCallerFromObjectShell( pShell );
            else
            {
                // use temporary model object (without document) to supply options
                aCall.SetCaller( static_cast<beans::XPropertySet*>(
                                    new ScDocOptionsObj( mrDoc.GetDocOptions() ) ) );
            }
        }

        short nPar = nParamCount;
        while ( nPar > 0 && GetError() == FormulaError::NONE )
        {
            --nPar;     // 0 .. (nParamCount-1)

            uno::Any aParam;
            if (IsMissing())
            {
                // Add-In has to explicitly handle an omitted empty missing
                // argument, do not default to anything like GetDouble() would
                // do (e.g. 0).
                Pop();
                aCall.SetParam( nPar, aParam );
                continue;   // while
            }

            StackVar nStackType = GetStackType();
            ScAddInArgumentType eType = aCall.GetArgType( nPar );
            switch (eType)
            {
                case SC_ADDINARG_INTEGER:
                    {
                        sal_Int32 nVal = GetInt32();
                        if (nGlobalError == FormulaError::NONE)
                            aParam <<= nVal;
                    }
                    break;

                case SC_ADDINARG_DOUBLE:
                    aParam <<= GetDouble();
                    break;

                case SC_ADDINARG_STRING:
                    aParam <<= GetString().getString();
                    break;

                case SC_ADDINARG_INTEGER_ARRAY:
                    switch( nStackType )
                    {
                        case svDouble:
                        case svString:
                        case svSingleRef:
                            {
                                sal_Int32 nVal = GetInt32();
                                if (nGlobalError == FormulaError::NONE)
                                {
                                    uno::Sequence<sal_Int32> aInner( &nVal, 1 );
                                    uno::Sequence< uno::Sequence<sal_Int32> > aOuter( &aInner, 1 );
                                    aParam <<= aOuter;
                                }
                            }
                            break;
                        case svDoubleRef:
                            {
                                ScRange aRange;
                                PopDoubleRef( aRange );
                                if (!ScRangeToSequence::FillLongArray( aParam, mrDoc, aRange ))
                                    SetError(FormulaError::IllegalParameter);
                            }
                            break;
                        case svMatrix:
                            if (!ScRangeToSequence::FillLongArray( aParam, PopMatrix().get() ))
                                SetError(FormulaError::IllegalParameter);
                            break;
                        default:
                            PopError();
                            SetError(FormulaError::IllegalParameter);
                    }
                    break;

                case SC_ADDINARG_DOUBLE_ARRAY:
                    switch( nStackType )
                    {
                        case svDouble:
                        case svString:
                        case svSingleRef:
                            {
                                double fVal = GetDouble();
                                uno::Sequence<double> aInner( &fVal, 1 );
                                uno::Sequence< uno::Sequence<double> > aOuter( &aInner, 1 );
                                aParam <<= aOuter;
                            }
                            break;
                        case svDoubleRef:
                            {
                                ScRange aRange;
                                PopDoubleRef( aRange );
                                if (!ScRangeToSequence::FillDoubleArray( aParam, mrDoc, aRange ))
                                    SetError(FormulaError::IllegalParameter);
                            }
                            break;
                        case svMatrix:
                            if (!ScRangeToSequence::FillDoubleArray( aParam, PopMatrix().get() ))
                                SetError(FormulaError::IllegalParameter);
                            break;
                        default:
                            PopError();
                            SetError(FormulaError::IllegalParameter);
                    }
                    break;

                case SC_ADDINARG_STRING_ARRAY:
                    switch( nStackType )
                    {
                        case svDouble:
                        case svString:
                        case svSingleRef:
                            {
                                OUString aString = GetString().getString();
                                uno::Sequence<OUString> aInner( &aString, 1 );
                                uno::Sequence< uno::Sequence<OUString> > aOuter( &aInner, 1 );
                                aParam <<= aOuter;
                            }
                            break;
                        case svDoubleRef:
                            {
                                ScRange aRange;
                                PopDoubleRef( aRange );
                                if (!ScRangeToSequence::FillStringArray( aParam, mrDoc, aRange ))
                                    SetError(FormulaError::IllegalParameter);
                            }
                            break;
                        case svMatrix:
                            if (!ScRangeToSequence::FillStringArray( aParam, PopMatrix().get(), mrContext ))
                                SetError(FormulaError::IllegalParameter);
                            break;
                        default:
                            PopError();
                            SetError(FormulaError::IllegalParameter);
                    }
                    break;

                case SC_ADDINARG_MIXED_ARRAY:
                    switch( nStackType )
                    {
                        case svDouble:
                        case svString:
                        case svSingleRef:
                            {
                                uno::Any aElem;
                                if ( nStackType == svDouble )
                                    aElem <<= GetDouble();
                                else if ( nStackType == svString )
                                    aElem <<= GetString().getString();
                                else
                                {
                                    ScAddress aAdr;
                                    if ( PopDoubleRefOrSingleRef( aAdr ) )
                                    {
                                        ScRefCellValue aCell(mrDoc, aAdr);
                                        if (aCell.hasString())
                                        {
                                            svl::SharedString aStr;
                                            GetCellString(aStr, aCell);
                                            aElem <<= aStr.getString();
                                        }
                                        else
                                            aElem <<= GetCellValue(aAdr, aCell);
                                    }
                                }
                                uno::Sequence<uno::Any> aInner( &aElem, 1 );
                                uno::Sequence< uno::Sequence<uno::Any> > aOuter( &aInner, 1 );
                                aParam <<= aOuter;
                            }
                            break;
                        case svDoubleRef:
                            {
                                ScRange aRange;
                                PopDoubleRef( aRange );
                                if (!ScRangeToSequence::FillMixedArray( aParam, mrDoc, aRange ))
                                    SetError(FormulaError::IllegalParameter);
                            }
                            break;
                        case svMatrix:
                            if (!ScRangeToSequence::FillMixedArray( aParam, PopMatrix().get() ))
                                SetError(FormulaError::IllegalParameter);
                            break;
                        default:
                            PopError();
                            SetError(FormulaError::IllegalParameter);
                    }
                    break;

                case SC_ADDINARG_VALUE_OR_ARRAY:
                    switch( nStackType )
                    {
                        case svDouble:
                            aParam <<= GetDouble();
                            break;
                        case svString:
                            aParam <<= GetString().getString();
                            break;
                        case svSingleRef:
                            {
                                ScAddress aAdr;
                                if ( PopDoubleRefOrSingleRef( aAdr ) )
                                {
                                    ScRefCellValue aCell(mrDoc, aAdr);
                                    if (aCell.hasString())
                                    {
                                        svl::SharedString aStr;
                                        GetCellString(aStr, aCell);
                                        aParam <<= aStr.getString();
                                    }
                                    else
                                        aParam <<= GetCellValue(aAdr, aCell);
                                }
                            }
                            break;
                        case svDoubleRef:
                            {
                                ScRange aRange;
                                PopDoubleRef( aRange );
                                if (!ScRangeToSequence::FillMixedArray( aParam, mrDoc, aRange ))
                                    SetError(FormulaError::IllegalParameter);
                            }
                            break;
                        case svMatrix:
                            if (!ScRangeToSequence::FillMixedArray( aParam, PopMatrix().get() ))
                                SetError(FormulaError::IllegalParameter);
                            break;
                        default:
                            PopError();
                            SetError(FormulaError::IllegalParameter);
                    }
                    break;

                case SC_ADDINARG_CELLRANGE:
                    switch( nStackType )
                    {
                        case svSingleRef:
                            {
                                ScAddress aAdr;
                                PopSingleRef( aAdr );
                                ScRange aRange( aAdr );
                                uno::Reference<table::XCellRange> xObj =
                                        ScCellRangeObj::CreateRangeFromDoc( mrDoc, aRange );
                                if (xObj.is())
                                    aParam <<= xObj;
                                else
                                    SetError(FormulaError::IllegalParameter);
                            }
                            break;
                        case svDoubleRef:
                            {
                                ScRange aRange;
                                PopDoubleRef( aRange );
                                uno::Reference<table::XCellRange> xObj =
                                        ScCellRangeObj::CreateRangeFromDoc( mrDoc, aRange );
                                if (xObj.is())
                                {
                                    aParam <<= xObj;
                                }
                                else
                                {
                                    SetError(FormulaError::IllegalParameter);
                                }
                            }
                            break;
                        default:
                            PopError();
                            SetError(FormulaError::IllegalParameter);
                    }
                    break;

                default:
                    PopError();
                    SetError(FormulaError::IllegalParameter);
            }
            aCall.SetParam( nPar, aParam );
        }

        while (nPar-- > 0)
        {
            Pop();                  // in case of error, remove remaining args
        }
        if ( GetError() == FormulaError::NONE )
        {
            aCall.ExecuteCall();

            if ( aCall.HasVarRes() )                        // handle async functions
            {
                pArr->AddRecalcMode( ScRecalcMode::ONLOAD_LENIENT );
                uno::Reference<sheet::XVolatileResult> xRes = aCall.GetVarRes();
                ScAddInListener* pLis = ScAddInListener::Get( xRes );
                // In case there is no pMyFormulaCell, i.e. while interpreting
                // temporarily from within the Function Wizard, try to obtain a
                // valid result from an existing listener for that volatile, or
                // create a new and hope for an immediate result. If none
                // available that should lead to a void result and thus #N/A.
                bool bTemporaryListener = false;
                if ( !pLis )
                {
                    pLis = ScAddInListener::CreateListener( xRes, &mrDoc );
                    if (pMyFormulaCell)
                        pMyFormulaCell->StartListening( *pLis );
                    else
                        bTemporaryListener = true;
                }
                else if (pMyFormulaCell)
                {
                    pMyFormulaCell->StartListening( *pLis );
                    if ( !pLis->HasDocument( &mrDoc ) )
                    {
                        pLis->AddDocument( &mrDoc );
                    }
                }

                aCall.SetResult( pLis->GetResult() );       // use result from async

                if (bTemporaryListener)
                {
                    try
                    {
                        // EventObject can be any, not evaluated by
                        // ScAddInListener::disposing()
                        css::lang::EventObject aEvent;
                        pLis->disposing(aEvent);    // pLis is dead hereafter
                    }
                    catch (const uno::Exception&)
                    {
                    }
                }
            }

            if ( aCall.GetErrCode() != FormulaError::NONE )
            {
                PushError( aCall.GetErrCode() );
            }
            else if ( aCall.HasMatrix() )
            {
                PushMatrix( aCall.GetMatrix() );
            }
            else if ( aCall.HasString() )
            {
                PushString( aCall.GetString() );
            }
            else
            {
                PushDouble( aCall.GetValue() );
            }
        }
        else                // error...
            PushError( GetError());
    }
    else
    {
        while( nParamCount-- > 0)
        {
            PopError();
        }
        PushError( FormulaError::NoAddin );
    }
}

void ScInterpreter::ScMissing()
{
    if ( aCode.IsEndOfPath() )
        PushTempToken( new ScEmptyCellToken( false, false ) );
    else
        PushTempToken( new FormulaMissingToken );
}

#if HAVE_FEATURE_SCRIPTING

static uno::Any lcl_getSheetModule( const uno::Reference<table::XCellRange>& xCellRange, const ScDocument* pDok )
{
    uno::Reference< sheet::XSheetCellRange > xSheetRange( xCellRange, uno::UNO_QUERY_THROW );
    uno::Reference< beans::XPropertySet > xProps( xSheetRange->getSpreadsheet(), uno::UNO_QUERY_THROW );
    OUString sCodeName;
    xProps->getPropertyValue(u"CodeName"_ustr) >>= sCodeName;
    // #TODO #FIXME ideally we should 'throw' here if we don't get a valid parent, but... it is possible
    // to create a module ( and use 'Option VBASupport 1' ) for a calc document, in this scenario there
    // are *NO* special document module objects ( of course being able to switch between vba/non vba mode at
    // the document in the future could fix this, especially IF the switching of the vba mode takes care to
    // create the special document module objects if they don't exist.
    BasicManager* pBasMgr = pDok->GetDocumentShell()->GetBasicManager();

    uno::Reference< uno::XInterface > xIf;
    if ( pBasMgr && !pBasMgr->GetName().isEmpty() )
    {
        OUString sProj( u"Standard"_ustr );
        if ( !pDok->GetDocumentShell()->GetBasicManager()->GetName().isEmpty() )
        {
            sProj = pDok->GetDocumentShell()->GetBasicManager()->GetName();
        }
        StarBASIC* pBasic = pDok->GetDocumentShell()->GetBasicManager()->GetLib( sProj );
        if ( pBasic )
        {
            SbModule* pMod = pBasic->FindModule( sCodeName );
            if ( pMod )
            {
                xIf = pMod->GetUnoModule();
            }
        }
    }
    return uno::Any( xIf );
}

static bool lcl_setVBARange( const ScRange& aRange, const ScDocument& rDok, SbxVariable* pPar )
{
    bool bOk = false;
    try
    {
        uno::Reference< uno::XInterface > xVBARange;
        uno::Reference<table::XCellRange> xCellRange = ScCellRangeObj::CreateRangeFromDoc( rDok, aRange );
        uno::Sequence< uno::Any > aArgs{ lcl_getSheetModule( xCellRange, &rDok ),
                                         uno::Any(xCellRange) };
        xVBARange = ooo::vba::createVBAUnoAPIServiceWithArgs( rDok.GetDocumentShell(), "ooo.vba.excel.Range", aArgs );
        if ( xVBARange.is() )
        {
            SbxObjectRef aObj = GetSbUnoObject( u"A-Range"_ustr, uno::Any( xVBARange ) );
            SetSbUnoObjectDfltPropName( aObj.get() );
            bOk = pPar->PutObject( aObj.get() );
        }
    }
    catch( uno::Exception& )
    {
    }
    return bOk;
}

static bool lcl_isNumericResult( double& fVal, const SbxVariable* pVar )
{
    switch (pVar->GetType())
    {
        case SbxINTEGER:
        case SbxLONG:
        case SbxSINGLE:
        case SbxDOUBLE:
        case SbxCURRENCY:
        case SbxDATE:
        case SbxUSHORT:
        case SbxULONG:
        case SbxINT:
        case SbxUINT:
        case SbxSALINT64:
        case SbxSALUINT64:
        case SbxDECIMAL:
            fVal = pVar->GetDouble();
            return true;
        case SbxBOOL:
            fVal = (pVar->GetBool() ? 1.0 : 0.0);
            return true;
        default:
            ;   // nothing
    }
    return false;
}

#endif

void ScInterpreter::ScMacro()
{

#if !HAVE_FEATURE_SCRIPTING
    PushNoValue();      // without DocShell no CallBasic
    return;
#else
    SbxBase::ResetError();

    sal_uInt8 nParamCount = GetByte();
    OUString aMacro( pCur->GetExternal() );

    ScDocShell* pDocSh = mrDoc.GetDocumentShell();
    if ( !pDocSh )
    {
        PushNoValue();      // without DocShell no CallBasic
        return;
    }

    //  no security queue beforehand (just CheckMacroWarn), moved to  CallBasic

    //  If the  Dok was loaded during a Basic-Calls,
    //  is the  Sbx-object created(?)
//  pDocSh->GetSbxObject();

    //  search function with the name,
    //  then assemble  SfxObjectShell::CallBasic from aBasicStr, aMacroStr

    StarBASIC* pRoot;

    try
    {
        pRoot = pDocSh->GetBasic();
    }
    catch (...)
    {
        pRoot = nullptr;
    }

    SbxVariable* pVar = pRoot ? pRoot->Find(aMacro, SbxClassType::Method) : nullptr;
    if( !pVar || pVar->GetType() == SbxVOID )
    {
        PushError( FormulaError::NoMacro );
        return;
    }
    SbMethod* pMethod = dynamic_cast<SbMethod*>(pVar);
    if( !pMethod )
    {
        PushError( FormulaError::NoMacro );
        return;
    }

    bool bVolatileMacro = false;

    SbModule* pModule = pMethod->GetModule();
    bool bUseVBAObjects = pModule->IsVBASupport();
    SbxObject* pObject = pModule->GetParent();
    assert(pObject);
    OSL_ENSURE(dynamic_cast<const StarBASIC *>(pObject) != nullptr, "No Basic found!");
    OUString aMacroStr = pObject->GetName() + "." + pModule->GetName() + "." + pMethod->GetName();
    OUString aBasicStr;
    if (pRoot && bUseVBAObjects)
    {
        // just here to make sure the VBA objects when we run the macro during ODF import
        pRoot->getVBAGlobals();
    }
    if (pObject->GetParent())
    {
        aBasicStr = pObject->GetParent()->GetName();    // document BASIC
    }
    else
    {
        aBasicStr = SfxGetpApp()->GetName();            // application BASIC
    }
    //  assemble a parameter array

    SbxArrayRef refPar = new SbxArray;
    bool bOk = true;
    for( sal_uInt32 i = nParamCount; i && bOk ; i-- )
    {
        SbxVariable* pPar = refPar->Get(i);
        switch( GetStackType() )
        {
            case svDouble:
                pPar->PutDouble( GetDouble() );
            break;
            case svString:
                pPar->PutString( GetString().getString() );
            break;
            case svExternalSingleRef:
            {
                ScExternalRefCache::TokenRef pToken;
                PopExternalSingleRef(pToken);
                if (nGlobalError != FormulaError::NONE)
                    bOk = false;
                else
                {
                    if ( pToken->GetType() == svString )
                        pPar->PutString( pToken->GetString().getString() );
                    else if ( pToken->GetType() == svDouble )
                        pPar->PutDouble( pToken->GetDouble() );
                    else
                    {
                        SetError( FormulaError::IllegalArgument );
                        bOk = false;
                    }
                }
            }
            break;
            case svSingleRef:
            {
                ScAddress aAdr;
                PopSingleRef( aAdr );
                if ( bUseVBAObjects )
                {
                    ScRange aRange( aAdr );
                    bOk = lcl_setVBARange( aRange, mrDoc, pPar );
                }
                else
                {
                    bOk = SetSbxVariable( pPar, aAdr );
                }
            }
            break;
            case svDoubleRef:
            {
                SCCOL nCol1;
                SCROW nRow1;
                SCTAB nTab1;
                SCCOL nCol2;
                SCROW nRow2;
                SCTAB nTab2;
                PopDoubleRef( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2 );
                if( nTab1 != nTab2 )
                {
                    SetError( FormulaError::IllegalParameter );
                    bOk = false;
                }
                else
                {
                    if ( bUseVBAObjects )
                    {
                        ScRange aRange( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2 );
                        bOk = lcl_setVBARange( aRange, mrDoc, pPar );
                    }
                    else
                    {
                        SbxDimArrayRef refArray = new SbxDimArray;
                        refArray->AddDim(1, nRow2 - nRow1 + 1);
                        refArray->AddDim(1, nCol2 - nCol1 + 1);
                        ScAddress aAdr( nCol1, nRow1, nTab1 );
                        for( SCROW nRow = nRow1; bOk && nRow <= nRow2; nRow++ )
                        {
                            aAdr.SetRow( nRow );
                            sal_Int32 nIdx[ 2 ];
                            nIdx[ 0 ] = nRow-nRow1+1;
                            for( SCCOL nCol = nCol1; bOk && nCol <= nCol2; nCol++ )
                            {
                                aAdr.SetCol( nCol );
                                nIdx[ 1 ] = nCol-nCol1+1;
                                SbxVariable* p = refArray->Get(nIdx);
                                bOk = SetSbxVariable( p, aAdr );
                            }
                        }
                        pPar->PutObject( refArray.get() );
                    }
                }
            }
            break;
            case svExternalDoubleRef:
            case svMatrix:
            {
                ScMatrixRef pMat = GetMatrix();
                SCSIZE nC, nR;
                if (pMat && nGlobalError == FormulaError::NONE)
                {
                    pMat->GetDimensions(nC, nR);
                    SbxDimArrayRef refArray = new SbxDimArray;
                    refArray->AddDim(1, static_cast<sal_Int32>(nR));
                    refArray->AddDim(1, static_cast<sal_Int32>(nC));
                    for( SCSIZE nMatRow = 0; nMatRow < nR; nMatRow++ )
                    {
                        sal_Int32 nIdx[ 2 ];
                        nIdx[ 0 ] = static_cast<sal_Int32>(nMatRow+1);
                        for( SCSIZE nMatCol = 0; nMatCol < nC; nMatCol++ )
                        {
                            nIdx[ 1 ] = static_cast<sal_Int32>(nMatCol+1);
                            SbxVariable* p = refArray->Get(nIdx);
                            if (pMat->IsStringOrEmpty(nMatCol, nMatRow))
                            {
                                p->PutString( pMat->GetString(nMatCol, nMatRow).getString() );
                            }
                            else
                            {
                                p->PutDouble( pMat->GetDouble(nMatCol, nMatRow));
                            }
                        }
                    }
                    pPar->PutObject( refArray.get() );
                }
                else
                {
                    SetError( FormulaError::IllegalParameter );
                }
            }
            break;
            default:
                SetError( FormulaError::IllegalParameter );
                bOk = false;
        }
    }
    if( bOk )
    {
        mrDoc.LockTable( aPos.Tab() );
        SbxVariableRef refRes = new SbxVariable;
        mrDoc.IncMacroInterpretLevel();
        ErrCode eRet = pDocSh->CallBasic( aMacroStr, aBasicStr, refPar.get(), refRes.get() );
        mrDoc.DecMacroInterpretLevel();
        mrDoc.UnlockTable( aPos.Tab() );

        ScMacroManager* pMacroMgr = mrDoc.GetMacroManager();
        if (pMacroMgr)
        {
            bVolatileMacro = pMacroMgr->GetUserFuncVolatile( pMethod->GetName() );
            pMacroMgr->AddDependentCell(pModule->GetName(), pMyFormulaCell);
        }

        double fVal;
        SbxDataType eResType = refRes->GetType();
        if( SbxBase::GetError() )
        {
            SetError( FormulaError::NoValue);
        }
        if ( eRet != ERRCODE_NONE )
        {
            PushNoValue();
        }
        else if (lcl_isNumericResult( fVal, refRes.get()))
        {
            switch (eResType)
            {
                case SbxDATE:
                    nFuncFmtType = SvNumFormatType::DATE;
                break;
                case SbxBOOL:
                    nFuncFmtType = SvNumFormatType::LOGICAL;
                break;
                // Do not add SbxCURRENCY, we don't know which currency.
                default:
                    ;   // nothing
            }
            PushDouble( fVal );
        }
        else if ( eResType & SbxARRAY )
        {
            SbxBase* pElemObj = refRes->GetObject();
            SbxDimArray* pDimArray = dynamic_cast<SbxDimArray*>(pElemObj);
            sal_Int32 nDim = pDimArray ? pDimArray->GetDims() : 0;
            if ( 1 <= nDim && nDim <= 2 )
            {
                sal_Int32 nCs, nCe, nRs;
                SCSIZE nC, nR;
                SCCOL nColIdx;
                SCROW nRowIdx;
                if ( nDim == 1 )
                {   // array( cols )  one line, several columns
                    pDimArray->GetDim(1, nCs, nCe);
                    nC = static_cast<SCSIZE>(nCe - nCs + 1);
                    nRs = 0;
                    nR = 1;
                    nColIdx = 0;
                    nRowIdx = 1;
                }
                else
                {   // array( rows, cols )
                    sal_Int32 nRe;
                    pDimArray->GetDim(1, nRs, nRe);
                    nR = static_cast<SCSIZE>(nRe - nRs + 1);
                    pDimArray->GetDim(2, nCs, nCe);
                    nC = static_cast<SCSIZE>(nCe - nCs + 1);
                    nColIdx = 1;
                    nRowIdx = 0;
                }
                ScMatrixRef pMat = GetNewMat( nC, nR, /*bEmpty*/true);
                if ( pMat )
                {
                    SbxVariable* pV;
                    for ( SCSIZE j=0; j < nR; j++ )
                    {
                        sal_Int32 nIdx[ 2 ];
                        //  in one-dimensional array( cols )  nIdx[1]
                        // from SbxDimArray::Get is ignored
                        nIdx[ nRowIdx ] = nRs + static_cast<sal_Int32>(j);
                        for ( SCSIZE i=0; i < nC; i++ )
                        {
                            nIdx[ nColIdx ] = nCs + static_cast<sal_Int32>(i);
                            pV = pDimArray->Get(nIdx);
                            if ( lcl_isNumericResult( fVal, pV) )
                            {
                                pMat->PutDouble( fVal, i, j );
                            }
                            else
                            {
                                pMat->PutString(mrStrPool.intern(pV->GetOUString()), i, j);
                            }
                        }
                    }
                    PushMatrix( pMat );
                }
                else
                {
                    PushIllegalArgument();
                }
            }
            else
            {
                PushNoValue();
            }
        }
        else
        {
            PushString( refRes->GetOUString() );
        }
    }

    if (bVolatileMacro && meVolatileType == NOT_VOLATILE)
        meVolatileType = VOLATILE_MACRO;
#endif
}

#if HAVE_FEATURE_SCRIPTING

bool ScInterpreter::SetSbxVariable( SbxVariable* pVar, const ScAddress& rPos )
{
    bool bOk = true;
    ScRefCellValue aCell(mrDoc, rPos);
    if (!aCell.isEmpty())
    {
        FormulaError nErr;
        double nVal;
        switch (aCell.getType())
        {
            case CELLTYPE_VALUE :
                nVal = GetValueCellValue(rPos, aCell.getDouble());
                pVar->PutDouble( nVal );
            break;
            case CELLTYPE_STRING :
            case CELLTYPE_EDIT :
                pVar->PutString(aCell.getString(mrDoc));
            break;
            case CELLTYPE_FORMULA :
                nErr = aCell.getFormula()->GetErrCode();
                if( nErr == FormulaError::NONE )
                {
                    if (aCell.getFormula()->IsValue())
                    {
                        nVal = aCell.getFormula()->GetValue();
                        pVar->PutDouble( nVal );
                    }
                    else
                        pVar->PutString(aCell.getFormula()->GetString().getString());
                }
                else
                {
                    SetError( nErr );
                    bOk = false;
                }
                break;
            default :
                pVar->PutEmpty();
        }
    }
    else
        pVar->PutEmpty();

    return bOk;
}

#endif

void ScInterpreter::ScTableOp()
{
    sal_uInt8 nParamCount = GetByte();
    if (nParamCount != 3 && nParamCount != 5)
    {
        PushIllegalParameter();
        return;
    }
    ScInterpreterTableOpParams aTableOp;
    if (nParamCount == 5)
    {
        PopSingleRef( aTableOp.aNew2 );
        PopSingleRef( aTableOp.aOld2 );
    }
    PopSingleRef( aTableOp.aNew1 );
    PopSingleRef( aTableOp.aOld1 );
    PopSingleRef( aTableOp.aFormulaPos );

    aTableOp.bValid = true;
    mrDoc.m_TableOpList.push_back(&aTableOp);
    mrDoc.IncInterpreterTableOpLevel();

    bool bReuseLastParams = (mrDoc.aLastTableOpParams == aTableOp);
    if ( bReuseLastParams )
    {
        aTableOp.aNotifiedFormulaPos = mrDoc.aLastTableOpParams.aNotifiedFormulaPos;
        aTableOp.bRefresh = true;
        for ( const auto& rPos : aTableOp.aNotifiedFormulaPos )
        {   // emulate broadcast and indirectly collect cell pointers
            ScRefCellValue aCell(mrDoc, rPos);
            if (aCell.getType() == CELLTYPE_FORMULA)
                aCell.getFormula()->SetTableOpDirty();
        }
    }
    else
    {   // broadcast and indirectly collect cell pointers and positions
        mrDoc.SetTableOpDirty( ScRange(aTableOp.aOld1) );
        if ( nParamCount == 5 )
            mrDoc.SetTableOpDirty( ScRange(aTableOp.aOld2) );
    }
    aTableOp.bCollectNotifications = false;

    ScRefCellValue aCell(mrDoc, aTableOp.aFormulaPos);
    if (aCell.getType() == CELLTYPE_FORMULA)
        aCell.getFormula()->SetDirtyVar();
    if (aCell.hasNumeric())
    {
        PushDouble(GetCellValue(aTableOp.aFormulaPos, aCell));
    }
    else
    {
        svl::SharedString aCellString;
        GetCellString(aCellString, aCell);
        PushString( aCellString );
    }

    auto const itr =
        ::std::find(mrDoc.m_TableOpList.begin(), mrDoc.m_TableOpList.end(), &aTableOp);
    if (itr != mrDoc.m_TableOpList.end())
    {
        mrDoc.m_TableOpList.erase(itr);
    }

    // set dirty again once more to be able to recalculate original
    for ( const auto& pCell : aTableOp.aNotifiedFormulaCells )
    {
        pCell->SetTableOpDirty();
    }

    // save these params for next incarnation
    if ( !bReuseLastParams )
        mrDoc.aLastTableOpParams = aTableOp;

    if (aCell.getType() == CELLTYPE_FORMULA)
    {
        aCell.getFormula()->SetDirtyVar();
        aCell.getFormula()->GetErrCode();     // recalculate original
    }

    // Reset all dirty flags so next incarnation does really collect all cell
    // pointers during notifications and not just non-dirty ones, which may
    // happen if a formula cell is used by more than one TableOp block.
    for ( const auto& pCell : aTableOp.aNotifiedFormulaCells )
    {
        pCell->ResetTableOpDirtyVar();
    }

    mrDoc.DecInterpreterTableOpLevel();
}

void ScInterpreter::ScDBArea()
{
    ScDBData* pDBData = mrDoc.GetDBCollection()->getNamedDBs().findByIndex(pCur->GetIndex());
    if (pDBData)
    {
        ScComplexRefData aRefData;
        aRefData.InitFlags();
        ScRange aRange;
        pDBData->GetArea(aRange);
        aRange.aEnd.SetTab(aRange.aStart.Tab());
        aRefData.SetRange(mrDoc.GetSheetLimits(), aRange, aPos);
        PushTempToken( new ScDoubleRefToken( mrDoc.GetSheetLimits(), aRefData ) );
    }
    else
        PushError( FormulaError::NoName);
}

void ScInterpreter::ScColRowNameAuto()
{
    ScComplexRefData aRefData( *pCur->GetDoubleRef() );
    ScRange aAbs = aRefData.toAbs(mrDoc, aPos);
    if (!mrDoc.ValidRange(aAbs))
    {
        PushError( FormulaError::NoRef );
        return;
    }

    SCCOL nStartCol;
    SCROW nStartRow;

    // maybe remember limit by using defined ColRowNameRange
    SCCOL nCol2 = aAbs.aEnd.Col();
    SCROW nRow2 = aAbs.aEnd.Row();
    // DataArea of the first cell
    nStartCol = aAbs.aStart.Col();
    nStartRow = aAbs.aStart.Row();
    aAbs.aEnd = aAbs.aStart; // Shrink to the top-left cell.

    {
        // Expand to the data area. Only modify the end position.
        SCCOL nDACol1 = aAbs.aStart.Col(), nDACol2 = aAbs.aEnd.Col();
        SCROW nDARow1 = aAbs.aStart.Row(), nDARow2 = aAbs.aEnd.Row();
        mrDoc.GetDataArea(aAbs.aStart.Tab(), nDACol1, nDARow1, nDACol2, nDARow2, true, false);
        aAbs.aEnd.SetCol(nDACol2);
        aAbs.aEnd.SetRow(nDARow2);
    }

    // corresponds with ScCompiler::GetToken
    if ( aRefData.Ref1.IsColRel() )
    {   // ColName
        aAbs.aEnd.SetCol(nStartCol);
        // maybe get previous limit by using defined ColRowNameRange
        if (aAbs.aEnd.Row() > nRow2)
            aAbs.aEnd.SetRow(nRow2);
        if ( aPos.Col() == nStartCol )
        {
            SCROW nMyRow = aPos.Row();
            if ( nStartRow <= nMyRow && nMyRow <= aAbs.aEnd.Row())
            {   //Formula in the same column and within the range
                if ( nMyRow == nStartRow )
                {   // take the rest under the name
                    nStartRow++;
                    if ( nStartRow > mrDoc.MaxRow() )
                        nStartRow = mrDoc.MaxRow();
                    aAbs.aStart.SetRow(nStartRow);
                }
                else
                {   // below the name to the formula cell
                    aAbs.aEnd.SetRow(nMyRow - 1);
                }
            }
        }
    }
    else
    {   // RowName
        aAbs.aEnd.SetRow(nStartRow);
        // maybe get previous limit by using defined ColRowNameRange
        if (aAbs.aEnd.Col() > nCol2)
            aAbs.aEnd.SetCol(nCol2);
        if ( aPos.Row() == nStartRow )
        {
            SCCOL nMyCol = aPos.Col();
            if (nStartCol <= nMyCol && nMyCol <= aAbs.aEnd.Col())
            {   //Formula in the same column and within the range
                if ( nMyCol == nStartCol )
                {    // take the rest under the name
                    nStartCol++;
                    if ( nStartCol > mrDoc.MaxCol() )
                        nStartCol = mrDoc.MaxCol();
                    aAbs.aStart.SetCol(nStartCol);
                }
                else
                {   // below the name to the formula cell
                    aAbs.aEnd.SetCol(nMyCol - 1);
                }
            }
        }
    }
    aRefData.SetRange(mrDoc.GetSheetLimits(), aAbs, aPos);
    PushTempToken( new ScDoubleRefToken( mrDoc.GetSheetLimits(), aRefData ) );
}

// --- internals ------------------------------------------------------------

void ScInterpreter::ScTTT()
{   // temporary test, testing functions etc.
    sal_uInt8 nParamCount = GetByte();
    // do something, count down nParamCount with Pops!

    // clean up Stack
    while ( nParamCount-- > 0)
        Pop();
    PushError(FormulaError::NoValue);
}

ScInterpreter::ScInterpreter( ScFormulaCell* pCell, ScDocument& rDoc, ScInterpreterContext& rContext,
        const ScAddress& rPos, ScTokenArray& r, bool bForGroupThreading )
    : aCode(r)
    , aPos(rPos)
    , pArr(&r)
    , mrContext(rContext)
    , mrDoc(rDoc)
    , mpLinkManager(rDoc.GetLinkManager())
    , mrStrPool(rDoc.GetSharedStringPool())
    , pJumpMatrix(nullptr)
    , pMyFormulaCell(pCell)
    , pCur(nullptr)
    , nGlobalError(FormulaError::NONE)
    , sp(0)
    , maxsp(0)
    , nFuncFmtIndex(0)
    , nCurFmtIndex(0)
    , nRetFmtIndex(0)
    , nFuncFmtType(SvNumFormatType::ALL)
    , nCurFmtType(SvNumFormatType::ALL)
    , nRetFmtType(SvNumFormatType::ALL)
    , mnStringNoValueError(FormulaError::NoValue)
    , mnSubTotalFlags(SubtotalFlags::NONE)
    , cPar(0)
    , bCalcAsShown(rDoc.GetDocOptions().IsCalcAsShown())
    , meVolatileType(r.IsRecalcModeAlways() ? VOLATILE : NOT_VOLATILE)
{
    MergeCalcConfig();

    if(pMyFormulaCell)
    {
        ScMatrixMode cMatFlag = pMyFormulaCell->GetMatrixFlag();
        bMatrixFormula = ( cMatFlag == ScMatrixMode::Formula );
    }
    else
        bMatrixFormula = false;

    // Let's not use the global stack while formula-group-threading.
    // as it complicates its life-cycle mgmt since for threading formula-groups,
    // ScInterpreter is preallocated (in main thread) for each worker thread.
    if (!bGlobalStackInUse && !bForGroupThreading)
    {
        bGlobalStackInUse = true;
        if (!pGlobalStack)
            pGlobalStack.reset(new ScTokenStack);
        pStackObj = pGlobalStack.get();
    }
    else
    {
        pStackObj = new ScTokenStack;
    }
    pStack = pStackObj->pPointer;
}

ScInterpreter::~ScInterpreter()
{
    if ( pStackObj == pGlobalStack.get() )
        bGlobalStackInUse = false;
    else
        delete pStackObj;
}

void ScInterpreter::Init( ScFormulaCell* pCell, const ScAddress& rPos, ScTokenArray& rTokArray )
{
    aCode.ReInit(rTokArray);
    aPos = rPos;
    pArr = &rTokArray;
    pJumpMatrix = nullptr;
    DropTokenCaches();
    pMyFormulaCell = pCell;
    pCur = nullptr;
    nGlobalError = FormulaError::NONE;
    sp = 0;
    maxsp = 0;
    nFuncFmtIndex = 0;
    nCurFmtIndex = 0;
    nRetFmtIndex = 0;
    nFuncFmtType = SvNumFormatType::ALL;
    nCurFmtType = SvNumFormatType::ALL;
    nRetFmtType = SvNumFormatType::ALL;
    mnStringNoValueError = FormulaError::NoValue;
    mnSubTotalFlags = SubtotalFlags::NONE;
    cPar = 0;
}

void ScInterpreter::DropTokenCaches()
{
    xResult = nullptr;
    maTokenMatrixMap.clear();
}

ScCalcConfig& ScInterpreter::GetOrCreateGlobalConfig()
{
    if (!mpGlobalConfig)
        mpGlobalConfig = new ScCalcConfig();
    return *mpGlobalConfig;
}

void ScInterpreter::SetGlobalConfig(const ScCalcConfig& rConfig)
{
    GetOrCreateGlobalConfig() = rConfig;
}

const ScCalcConfig& ScInterpreter::GetGlobalConfig()
{
    return GetOrCreateGlobalConfig();
}

void ScInterpreter::MergeCalcConfig()
{
    maCalcConfig = GetOrCreateGlobalConfig();
    maCalcConfig.MergeDocumentSpecific( mrDoc.GetCalcConfig());
}

void ScInterpreter::GlobalExit()
{
    OSL_ENSURE(!bGlobalStackInUse, "who is still using the TokenStack?");
    pGlobalStack.reset();
}

namespace {

double applyImplicitIntersection(const sc::RangeMatrix& rMat, const ScAddress& rPos)
{
    if (rMat.mnRow1 <= rPos.Row() && rPos.Row() <= rMat.mnRow2 && rMat.mnCol1 == rMat.mnCol2)
    {
        SCROW nOffset = rPos.Row() - rMat.mnRow1;
        return rMat.mpMat->GetDouble(0, nOffset);
    }

    if (rMat.mnCol1 <= rPos.Col() && rPos.Col() <= rMat.mnCol2 && rMat.mnRow1 == rMat.mnRow2)
    {
        SCROW nOffset = rPos.Col() - rMat.mnCol1;
        return rMat.mpMat->GetDouble(nOffset, 0);
    }

    return std::numeric_limits<double>::quiet_NaN();
}

// Test for Functions that evaluate an error code and directly set nGlobalError to 0
bool IsErrFunc(OpCode oc)
{
    switch (oc)
    {
        case ocCount :
        case ocCount2 :
        case ocErrorType :
        case ocIsEmpty :
        case ocIsErr :
        case ocIsError :
        case ocIsFormula :
        case ocIsLogical :
        case ocIsNA :
        case ocIsNonString :
        case ocIsRef :
        case ocIsString :
        case ocIsValue :
        case ocN :
        case ocType :
        case ocIfError :
        case ocIfNA :
        case ocErrorType_ODF :
        case ocAggregate:       // may ignore errors depending on option
        case ocIfs_MS:
        case ocSwitch_MS:
        case ocXLookup:
            return true;
        default:
            return false;
    }
}

} //namespace

StackVar ScInterpreter::Interpret()
{
    addScInterpreterReachabilityStat(ScInterpreterReachabilityStat::ClassicInterpret);
    SvNumFormatType nRetTypeExpr = SvNumFormatType::UNDEFINED;
    sal_uInt32 nRetIndexExpr = 0;
    sal_uInt16 nErrorFunction = 0;
    sal_uInt16 nErrorFunctionCount = 0;
    std::vector<sal_uInt16> aErrorFunctionStack;
    sal_uInt16 nStackBase;

    nGlobalError = FormulaError::NONE;
    nStackBase = sp = maxsp = 0;
    nRetFmtType = SvNumFormatType::UNDEFINED;
    nFuncFmtType = SvNumFormatType::UNDEFINED;
    nFuncFmtIndex = nCurFmtIndex = nRetFmtIndex = 0;
    xResult = nullptr;
    pJumpMatrix = nullptr;
    mnSubTotalFlags = SubtotalFlags::NONE;
    ScTokenMatrixMap::const_iterator aTokenMatrixMapIter;

    // Once upon a time we used to have FP exceptions on, and there was a
    // Windows printer driver that kept switching off exceptions, so we had to
    // switch them back on again every time. Who knows if there isn't a driver
    // that keeps switching exceptions on, now that we run with exceptions off,
    // so reassure exceptions are really off.
    SAL_MATH_FPEXCEPTIONS_OFF();

    OpCode eOp = ocNone;
    aCode.Reset();
    for (;;)
    {
        pCur = aCode.Next();
        if (!pCur || (nGlobalError != FormulaError::NONE && nErrorFunction > nErrorFunctionCount) )
            break;
        eOp = pCur->GetOpCode();
        if (isInterestingClassicOpcode(eOp))
            addClassicOpcodeRuntimeStat(eOp);
        cPar = pCur->GetByte();
        if ( eOp == ocPush )
        {
            // RPN code push without error
            PushWithoutError( *pCur );
            nCurFmtType = SvNumFormatType::UNDEFINED;
        }
        else
        {
            const bool bIsOpCodeJumpCommand = FormulaCompiler::IsOpCodeJumpCommand(eOp);
            if (!bIsOpCodeJumpCommand &&
               ((aTokenMatrixMapIter = maTokenMatrixMap.find( pCur)) !=
                maTokenMatrixMap.end()) &&
               (*aTokenMatrixMapIter).second->GetType() != svJumpMatrix)
            {
                // Path already calculated, reuse result.
                const sal_uInt8 nParamCount = pCur->GetParamCount();
                if (sp >= nParamCount)
                    nStackBase = sp - nParamCount;
                else
                {
                    SAL_WARN("sc.core", "Stack anomaly with calculated path at "
                            << aPos.Tab() << "," << aPos.Col() << "," << aPos.Row()
                            << "  " << aPos.Format(
                                ScRefFlags::VALID | ScRefFlags::FORCE_DOC | ScRefFlags::TAB_3D, &mrDoc)
                            << "  eOp: " << static_cast<int>(eOp)
                            << "  params: " << static_cast<int>(nParamCount)
                            << "  nStackBase: " << nStackBase << "  sp: " << sp);
                    nStackBase = sp;
                    assert(!"underflow");
                }
                sp = nStackBase;
                PushTokenRef( (*aTokenMatrixMapIter).second);
            }
            else
            {
                // previous expression determines the current number format
                nCurFmtType = nRetTypeExpr;
                nCurFmtIndex = nRetIndexExpr;
                // default function's format, others are set if needed
                nFuncFmtType = SvNumFormatType::NUMBER;
                nFuncFmtIndex = 0;

                if (bIsOpCodeJumpCommand)
                    nStackBase = sp;        // don't mess around with the jumps
                else
                {
                    // Convert parameters to matrix if in array/matrix formula and
                    // parameters of function indicate doing so. Create JumpMatrix
                    // if necessary.
                    if ( MatrixParameterConversion() )
                    {
                        eOp = ocNone;       // JumpMatrix created
                        nStackBase = sp;
                    }
                    else
                    {
                        const sal_uInt8 nParamCount = pCur->GetParamCount();
                        if (sp >= nParamCount)
                            nStackBase = sp - nParamCount;
                        else
                        {
                            SAL_WARN("sc.core", "Stack anomaly at " << aPos.Tab() << "," << aPos.Col() << "," << aPos.Row()
                                    << "  " << aPos.Format(
                                        ScRefFlags::VALID | ScRefFlags::FORCE_DOC | ScRefFlags::TAB_3D, &mrDoc)
                                    << "  eOp: " << static_cast<int>(eOp)
                                    << "  params: " << static_cast<int>(nParamCount)
                                    << "  nStackBase: " << nStackBase << "  sp: " << sp);
                            nStackBase = sp;
                            assert(!"underflow");
                        }
                    }
                }

                namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;
                const auto warnIfLegacyDispatchReached = [&](const char* pRouteLabel,
                                                            std::u16string_view rFunctionName,
                                                            auto aClassifier,
                                                            const char* pFailureMessage,
                                                            bool bRequireNonArrayContext = false) {
                    if (!pMyFormulaCell || pMyFormulaCell->IsIterCell()
                        || pMyFormulaCell->GetMatrixFlag() != ScMatrixMode::NONE
                        || pMyFormulaCell->IsHyperLinkCell()
                        || mrDoc.IsThreadedGroupCalcInProgress()
                        || (bRequireNonArrayContext && IsInArrayContext()))
                    {
                        return;
                    }

                    const OUString aFormulaSource
                        = pMyFormulaCell->GetFormula(FormulaGrammar::GRAM_ODFF, &mrContext);
                    const std::u16string_view aFormulaView(aFormulaSource.getStr(),
                        aFormulaSource.getLength());
                    if (!aClassifier(aFormulaView))
                        return;

                    SAL_WARN("sc.core",
                        pRouteLabel << " "
                                    << OUString(rFunctionName.data(), rFunctionName.size())
                                    << " reached ScInterpreter for " << aFormulaSource);
                    OSL_FAIL(pFailureMessage);
                };
                const auto warnIfLegacyScalarRootReached = [&](std::u16string_view rLabel) {
                    warnIfLegacyDispatchReached(
                        "family-local default-on", rLabel,
                        [](std::u16string_view rFormula) {
                            return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                        },
                        "family-local default-on scalar root reached ScInterpreter");
                };
                const auto warnIfLegacyDefaultOnReached =
                    [&](std::u16string_view rLabel, const char* pFailureMessage) {
                        warnIfLegacyDispatchReached(
                            "family-local default-on", rLabel,
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            pFailureMessage);
                    };
                const auto warnIfLegacyNumericAggregateReached =
                    [&](std::u16string_view rLabel) {
                        warnIfLegacyDefaultOnReached(
                            rLabel,
                            "family-local default-on numeric aggregate reached ScInterpreter");
                    };
                const auto warnIfLegacyStatisticalAggregateReached =
                    [&](std::u16string_view rLabel) {
                        warnIfLegacyDefaultOnReached(
                            rLabel,
                            "family-local default-on statistical aggregate reached "
                            "ScInterpreter");
                    };
                const auto warnIfLegacyGrowthProjectionReached =
                    [&](std::u16string_view rLabel) {
                        warnIfLegacyDefaultOnReached(
                            rLabel,
                            "family-local default-on growth projection reached ScInterpreter");
                    };
                const auto toCalcMathFormulaError =
                    [](spreadsheetengine::api::Error eError) {
                        if (eError == spreadsheetengine::api::Error::Domain)
                            return FormulaError::IllegalArgument;
                        return selibreoffice::toFormulaError(eError);
                    };
                const auto warnIfLegacyStatisticalDistributionReached =
                    [&](std::u16string_view rLabel) {
                        warnIfLegacyDefaultOnReached(
                            rLabel,
                            "family-local default-on statistical distribution reached "
                            "ScInterpreter");
                    };
                const auto pushValueResult = [&](const auto& rResult) {
                    if (!rResult)
                    {
                        PushError(selibreoffice::toFormulaError(rResult.meError));
                        return;
                    }
                    PushDouble(rResult.maValue);
                };
                const auto pushCalcMathValueResult = [&](const auto& rResult) {
                    if (!rResult)
                    {
                        PushError(toCalcMathFormulaError(rResult.meError));
                        return;
                    }
                    PushDouble(rResult.maValue);
                };
                // The first operator pilot only accepts plain scalar tokens.
                // Reference and matrix operands still fall back to the legacy
                // interpreter so we do not guess at broadcast or reference
                // resolution semantics.
                const auto tryBuildEngineScalarBinaryOperand
                    = [&](const FormulaToken& rToken,
                          serpn::BinaryScalarOperator eOperator)
                    -> std::optional<serpn::RpnValue> {
                    switch (rToken.GetType())
                    {
                        case svDouble:
                        {
                            const auto eType
                                = static_cast<SvNumFormatType>(rToken.GetDoubleType());
                            // Arithmetic must preserve Calc's format-sensitive
                            // semantics, so the engine path only accepts plain
                            // numeric/logical doubles here. Comparisons do not
                            // depend on number-format propagation, so they may
                            // accept wider scalar shapes later without changing
                            // this arithmetic contract.
                            if (!serpn::isComparisonOperator(eOperator))
                            {
                                switch (eType)
                                {
                                    case SvNumFormatType::ALL:
                                    case SvNumFormatType::UNDEFINED:
                                    case SvNumFormatType::NUMBER:
                                    case SvNumFormatType::LOGICAL:
                                        break;
                                    default:
                                        return std::nullopt;
                                }
                            }

                            if (serpn::isConcatenationOperator(eOperator))
                                return std::nullopt;

                            return serpn::RpnValue::number(rToken.GetDouble());
                        }
                        case svString:
                        case svStringName:
                            return serpn::RpnValue::text(rToken.GetString().getString());
                        case svMissing:
                        case svEmptyCell:
                            return serpn::RpnValue::empty();
                        case svError:
                            return serpn::RpnValue::error(
                                selibreoffice::toApiError(rToken.GetError()));
                        default:
                            return std::nullopt;
                    }
                };
                const auto tryBuildEngineScalarReferenceOperand
                    = [&](const FormulaToken& rToken,
                          serpn::BinaryScalarOperator eOperator)
                    -> std::optional<serpn::RpnValue> {
                    if (mrDoc.m_TableOpList.empty() == false || pJumpMatrix)
                        return std::nullopt;

                    const FormulaError nSavedError = nGlobalError;
                    const SvNumFormatType eSavedCurFmtType = nCurFmtType;
                    const sal_uInt32 nSavedCurFmtIndex = nCurFmtIndex;
                    const auto restoreInterpreterState = [&]() {
                        nGlobalError = nSavedError;
                        nCurFmtType = eSavedCurFmtType;
                        nCurFmtIndex = nSavedCurFmtIndex;
                    };
                    const auto makeErrorResult = [&](FormulaError eError) {
                        restoreInterpreterState();
                        return std::optional<serpn::RpnValue>(
                            serpn::RpnValue::error(selibreoffice::toApiError(eError)));
                    };
                    const auto buildFromCellAddress = [&](const ScAddress& rAddress)
                        -> std::optional<serpn::RpnValue> {
                        ScRefCellValue aCell(mrDoc, rAddress);
                        const FormulaError eCellError = GetCellErrCode(aCell);
                        if (eCellError != FormulaError::NONE)
                            return makeErrorResult(eCellError);

                        switch (aCell.getType())
                        {
                            case CELLTYPE_NONE:
                                restoreInterpreterState();
                                return serpn::RpnValue::empty();
                            case CELLTYPE_VALUE:
                                if (serpn::isConcatenationOperator(eOperator))
                                {
                                    restoreInterpreterState();
                                    return std::nullopt;
                                }
                                {
                                    const double fValue = GetCellValue(rAddress, aCell);
                                    if (nGlobalError != FormulaError::NONE)
                                        return makeErrorResult(nGlobalError);
                                    restoreInterpreterState();
                                    return serpn::RpnValue::number(fValue);
                                }
                            case CELLTYPE_STRING:
                            case CELLTYPE_EDIT:
                                if (!serpn::isConcatenationOperator(eOperator))
                                {
                                    restoreInterpreterState();
                                    return std::nullopt;
                                }
                                {
                                    svl::SharedString aString;
                                    GetCellString(aString, aCell);
                                    if (nGlobalError != FormulaError::NONE)
                                        return makeErrorResult(nGlobalError);
                                    restoreInterpreterState();
                                    return serpn::RpnValue::text(aString.getString());
                                }
                            case CELLTYPE_FORMULA:
                                if (aCell.getFormula()->IsValue())
                                {
                                    if (serpn::isConcatenationOperator(eOperator))
                                    {
                                        restoreInterpreterState();
                                        return std::nullopt;
                                    }
                                    const double fValue = GetCellValue(rAddress, aCell);
                                    if (nGlobalError != FormulaError::NONE)
                                        return makeErrorResult(nGlobalError);
                                    restoreInterpreterState();
                                    return serpn::RpnValue::number(fValue);
                                }

                                if (!serpn::isConcatenationOperator(eOperator))
                                {
                                    restoreInterpreterState();
                                    return std::nullopt;
                                }
                                {
                                    svl::SharedString aString;
                                    GetCellString(aString, aCell);
                                    if (nGlobalError != FormulaError::NONE)
                                        return makeErrorResult(nGlobalError);
                                    restoreInterpreterState();
                                    return serpn::RpnValue::text(aString.getString());
                                }
                        }

                        restoreInterpreterState();
                        return std::nullopt;
                    };

                    nGlobalError = FormulaError::NONE;
                    switch (rToken.GetType())
                    {
                        case svSingleRef:
                        {
                            const ScSingleRefData* pRefData = rToken.GetSingleRef();
                            if (pRefData->IsDeleted())
                                return makeErrorResult(FormulaError::NoRef);

                            SCCOL nCol = 0;
                            SCROW nRow = 0;
                            SCTAB nTab = 0;
                            SingleRefToVars(*pRefData, nCol, nRow, nTab);
                            if (nGlobalError != FormulaError::NONE)
                                return makeErrorResult(nGlobalError);

                            return buildFromCellAddress(ScAddress(nCol, nRow, nTab));
                        }
                        case svDoubleRef:
                        {
                            ScRange aRange;
                            DoubleRefToRange(*rToken.GetDoubleRef(), aRange);
                            if (nGlobalError != FormulaError::NONE)
                                return makeErrorResult(nGlobalError);

                            ScAddress aAddress;
                            if (!DoubleRefToPosSingleRef(aRange, aAddress))
                            {
                                if (nGlobalError != FormulaError::NONE)
                                    return makeErrorResult(nGlobalError);
                                restoreInterpreterState();
                                return std::nullopt;
                            }

                            return buildFromCellAddress(aAddress);
                        }
                        default:
                            restoreInterpreterState();
                            return std::nullopt;
                    }
                };
                const auto tryPushEngineScalarBinaryOp
                    = [&](serpn::BinaryScalarOperator eOperator) {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore().mnEngineAttemptedCount);
                    if (sp < 2)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore().mnEngineDeclinedCount);
                        return false;
                    }

                    const FormulaToken* pRight = pStack[sp - 1];
                    const FormulaToken* pLeft = pStack[sp - 2];
                    if (!pLeft || !pRight)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore().mnEngineDeclinedCount);
                        return false;
                    }

                    if (serpn::isComparisonOperator(eOperator)
                        && ((pLeft->GetType() == svString || pLeft->GetType() == svStringName)
                            && (pRight->GetType() == svString || pRight->GetType() == svStringName)))
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore().mnEngineDeclinedCount);
                        return false;
                    }

                    auto oLeft = tryBuildEngineScalarBinaryOperand(*pLeft, eOperator);
                    if (!oLeft)
                        oLeft = tryBuildEngineScalarReferenceOperand(*pLeft, eOperator);
                    auto oRight = tryBuildEngineScalarBinaryOperand(*pRight, eOperator);
                    if (!oRight)
                        oRight = tryBuildEngineScalarReferenceOperand(*pRight, eOperator);
                    if (!oLeft || !oRight)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore().mnEngineDeclinedCount);
                        return false;
                    }

                    const auto aResult
                        = serpn::evaluateBinaryScalarOperator(eOperator, *oLeft, *oRight);
                    if (aResult.meReadiness != serpn::RpnCoercionReadiness::Ready)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore().mnEngineDeclinedCount);
                        return false;
                    }

                    sp -= 2;
                    nGlobalError = FormulaError::NONE;
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore().mnEngineSucceededCount);

                    if (!aResult)
                    {
                        PushError(selibreoffice::toFormulaError(aResult.meError));
                        return true;
                    }

                    switch (aResult.maValue.meKind)
                    {
                        case serpn::RpnValueKind::Number:
                            nFuncFmtType = SvNumFormatType::NUMBER;
                            PushDouble(aResult.maValue.maScalar.mfNumber);
                            return true;
                        case serpn::RpnValueKind::Boolean:
                            nFuncFmtType = SvNumFormatType::LOGICAL;
                            PushInt(aResult.maValue.maScalar.mfNumber != 0.0);
                            return true;
                        case serpn::RpnValueKind::String:
                            PushString(selibreoffice::toLibreOfficeString(
                                aResult.maValue.maScalar.maString));
                            return true;
                        case serpn::RpnValueKind::Empty:
                            PushString(OUString());
                            return true;
                        case serpn::RpnValueKind::Error:
                            PushError(selibreoffice::toFormulaError(
                                aResult.maValue.maScalar.meError));
                            return true;
                        case serpn::RpnValueKind::Reference:
                        case serpn::RpnValueKind::Matrix:
                            return false;
                    }

                    return false;
                };
                const auto tryPushEngineBadLiteralError = [&]() {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore().mnEngineAttemptedCount);
                    if (!pMyFormulaCell || !pArr)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore().mnEngineDeclinedCount);
                        return false;
                    }

                    const OUString aFormulaSource
                        = pMyFormulaCell->GetFormula(FormulaGrammar::GRAM_ODFF, &mrContext);
                    if (aFormulaSource.isEmpty())
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore().mnEngineDeclinedCount);
                        return false;
                    }

                    const auto aAttempt = setaileval::tryEvaluateFormula(
                        mrDoc, mrContext, aPos,
                        std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength()),
                        mrDoc.GetCalcConfig().mbEmptyStringAsZero, pArr);
                    if (!aAttempt.mbSupported
                        || aAttempt.maResult.meType
                               != spreadsheetengine::api::formulavalue::ValueType::Error)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore().mnEngineDeclinedCount);
                        return false;
                    }

                    nGlobalError = FormulaError::NONE;
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore().mnEngineSucceededCount);
                    PushError(selibreoffice::toFormulaError(aAttempt.maResult.meError));
                    return true;
                };
                const auto tryPushEngineBadLiteralRangeOpcode = [&]() {
                    if (!pMyFormulaCell || !pArr)
                        return false;

                    const OUString aFormulaSource
                        = pMyFormulaCell->GetFormula(FormulaGrammar::GRAM_ODFF, &mrContext);
                    if (aFormulaSource.isEmpty())
                        return false;

                    if (!setaileval::isRootErrorLiteralFormula(
                            std::u16string_view(aFormulaSource.getStr(),
                                aFormulaSource.getLength())))
                    {
                        return false;
                    }

                    return tryPushEngineBadLiteralError();
                };
                const auto tryPushEngineRangeReference = [&]() {
                    auto& rDispatchStats = interpreterDispatchRuntimeStatsStore();
                    addDispatchRuntimeStat(rDispatchStats.mnEngineAttemptedCount);
                    addDispatchRuntimeStat(rDispatchStats.mnRangeEngineAttemptedCount);
                    if (nGlobalError != FormulaError::NONE || sp < 2)
                    {
                        addDispatchRuntimeStat(rDispatchStats.mnEngineDeclinedCount);
                        addDispatchRuntimeStat(rDispatchStats.mnRangeEngineDeclinedCount);
                        addDispatchRuntimeStat(
                            rDispatchStats.mnRangeEngineDeclinedGlobalErrorOrStackCount);
                        maybeRecordRangeDispatchDeclineFormulaSample();
                        return false;
                    }

                    const FormulaToken* pRight = pStack[sp - 1];
                    const FormulaToken* pLeft = pStack[sp - 2];
                    if (!pLeft || !pRight)
                    {
                        addDispatchRuntimeStat(rDispatchStats.mnEngineDeclinedCount);
                        addDispatchRuntimeStat(rDispatchStats.mnRangeEngineDeclinedCount);
                        addDispatchRuntimeStat(
                            rDispatchStats.mnRangeEngineDeclinedNullTokenCount);
                        maybeRecordRangeDispatchDeclineFormulaSample();
                        return false;
                    }

                    FormulaTokenRef xRangeResult
                        = seinterpcompatdispatch::Dispatcher::rangeReferenceToken(
                            *this, *pLeft, *pRight);
                    if (!xRangeResult)
                    {
                        addDispatchRuntimeStat(rDispatchStats.mnEngineDeclinedCount);
                        addDispatchRuntimeStat(rDispatchStats.mnRangeEngineDeclinedCount);
                        addDispatchRuntimeStat(
                            rDispatchStats.mnRangeEngineDeclinedBuildFailureCount);
                        maybeRecordRangeDispatchDeclineFormulaSample();
                        return false;
                    }

                    sp -= 2;
                    nGlobalError = FormulaError::NONE;
                    addDispatchRuntimeStat(rDispatchStats.mnEngineSucceededCount);
                    addDispatchRuntimeStat(rDispatchStats.mnRangeEngineSucceededCount);
                    PushTokenRef(xRangeResult);
                    return true;
                };
                const auto pushLegacyGcdOrLcm = [&](std::u16string_view rLabel, bool bLcm) {
                    warnIfLegacyDefaultOnReached(
                        rLabel, "family-local default-on math scalar reached ScInterpreter");
                    short nParamCount = GetByte();
                    if (!MustHaveParamCountMin(nParamCount, 1))
                        return;

                    double fx;
                    double fy = bLcm ? 1.0 : 0.0;
                    ScRange aRange;
                    size_t nRefInList = 0;
                    const auto aAccumulate = [&](double fInput) -> bool {
                        fx = ::rtl::math::approxFloor(fInput);
                        if (fx < 0.0)
                        {
                            PushIllegalArgument();
                            return false;
                        }
                        if (bLcm)
                        {
                            if (fx == 0.0 || fy == 0.0)
                                fy = 0.0;
                            else
                                fy = fx * fy / ScGetGCD(fx, fy);
                        }
                        else
                            fy = ScGetGCD(fx, fy);
                        return true;
                    };

                    while (nGlobalError == FormulaError::NONE && nParamCount-- > 0)
                    {
                        switch (GetStackType())
                        {
                            case svDouble:
                            case svString:
                            case svSingleRef:
                                if (!aAccumulate(GetDouble()))
                                    return;
                                break;
                            case svDoubleRef:
                            case svRefList:
                            {
                                FormulaError nErr = FormulaError::NONE;
                                PopDoubleRef(aRange, nParamCount, nRefInList);
                                double nCellVal;
                                ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags);
                                if (aValIter.GetFirst(nCellVal, nErr))
                                {
                                    do
                                    {
                                        if (!aAccumulate(nCellVal))
                                            return;
                                    } while (nErr == FormulaError::NONE
                                             && aValIter.GetNext(nCellVal, nErr));
                                }
                                SetError(nErr);
                            }
                            break;
                            case svMatrix:
                            case svExternalSingleRef:
                            case svExternalDoubleRef:
                            {
                                ScMatrixRef pMat = GetMatrix();
                                if (pMat)
                                {
                                    SCSIZE nC;
                                    SCSIZE nR;
                                    pMat->GetDimensions(nC, nR);
                                    if (nC == 0 || nR == 0)
                                        SetError(FormulaError::IllegalArgument);
                                    else
                                    {
                                        double nVal = bLcm ? pMat->GetLcm() : pMat->GetGcd();
                                        if (bLcm)
                                            fy = (nVal * fy) / ScGetGCD(nVal, fy);
                                        else
                                            fy = ScGetGCD(nVal, fy);
                                    }
                                }
                            }
                            break;
                            default:
                                SetError(FormulaError::IllegalParameter);
                                break;
                        }
                    }
                    PushDouble(fy);
                };
                const auto pushLegacyCombin = [&](std::u16string_view rLabel,
                                                  bool bAllowRepetition) {
                    warnIfLegacyDefaultOnReached(
                        rLabel, "family-local default-on math scalar reached ScInterpreter");
                    if (MustHaveParamCount(GetByte(), 2))
                    {
                        const double k = GetDouble();
                        const double n = GetDouble();
                        const auto aResult = semath::evaluateCombinValue(
                            n, k, bAllowRepetition);
                        if (!aResult)
                        {
                            PushError(selibreoffice::toFormulaError(aResult.meError));
                            return;
                        }
                        PushDouble(aResult.maValue);
                    }
                };
                const auto pushLegacyColor = [&]() {
                    warnIfLegacyDefaultOnReached(
                        u"COLOR", "family-local default-on math scalar reached ScInterpreter");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 3, 4))
                        return;

                    double nAlpha = 0;
                    if (nParamCount == 4)
                        nAlpha = rtl::math::approxFloor(GetDouble());
                    if (nAlpha < 0 || nAlpha > 255)
                    {
                        PushIllegalArgument();
                        return;
                    }

                    double nBlue = rtl::math::approxFloor(GetDouble());
                    if (nBlue < 0 || nBlue > 255)
                    {
                        PushIllegalArgument();
                        return;
                    }

                    double nGreen = rtl::math::approxFloor(GetDouble());
                    if (nGreen < 0 || nGreen > 255)
                    {
                        PushIllegalArgument();
                        return;
                    }

                    double nRed = rtl::math::approxFloor(GetDouble());
                    if (nRed < 0 || nRed > 255)
                    {
                        PushIllegalArgument();
                        return;
                    }

                    PushDouble(256 * 256 * 256 * nAlpha + 256 * 256 * nRed + 256 * nGreen
                               + nBlue);
                };
                const auto pushLegacyRawSubtract = [&]() {
                    warnIfLegacyDefaultOnReached(
                        u"RAWSUBTRACT",
                        "family-local default-on math scalar reached ScInterpreter");
                    short nParamCount = GetByte();
                    if (!MustHaveParamCountMin(nParamCount, 2))
                        return;

                    ReverseStack(nParamCount);
                    double fRes = GetDouble();
                    while (nGlobalError == FormulaError::NONE && --nParamCount > 0)
                        fRes -= GetDouble();
                    while (nParamCount-- > 0)
                        PopError();
                    PushDouble(fRes);
                };
                const auto pushLegacyDateOrTimeValue =
                    [&](const char* pFunctionName, SvNumFormatType eFormatType,
                        auto aEvaluator) {
                        warnIfLegacyDispatchReached(
                            "literal-only hard-routed", OUString::createFromAscii(pFunctionName),
                            [](std::u16string_view rFormula) {
                                return setaileval::isHardRoutedFormula(rFormula);
                            },
                            "literal-only hard-routed text parsing slice reached ScInterpreter");

                        const OUString aInputString = GetString().getString();
                        const auto aResult = aEvaluator(aInputString);
                        if (aResult)
                        {
                            nFuncFmtType = eFormatType;
                            PushDouble(aResult.maValue);
                        }
                        else
                            PushIllegalArgument();
                    };
                const auto warnIfLegacyDateFamilyReached = [&](std::u16string_view rFunctionName) {
                    warnIfLegacyDispatchReached(
                        "family-local default-on", rFunctionName,
                        [](std::u16string_view rFormula) {
                            return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                        },
                        "family-local default-on date family reached ScInterpreter");
                };
                const auto warnIfLegacyRateFamilyReached = [&](std::u16string_view rFunctionName) {
                    warnIfLegacyDispatchReached(
                        "family-local default-on", rFunctionName,
                        [](std::u16string_view rFormula) {
                            return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                        },
                        "family-local default-on financial rate family reached ScInterpreter");
                };
                const auto pushLegacyNpv = [&]() {
                    warnIfLegacyRateFamilyReached(u"NPV");
                    nFuncFmtType = SvNumFormatType::CURRENCY;
                    short nParamCount = GetByte();
                    if (!MustHaveParamCountMin(nParamCount, 2))
                        return;

                    KahanSum fVal = 0.0;
                    ReverseStack(nParamCount);
                    if (nGlobalError == FormulaError::NONE)
                    {
                        double fCount = 1.0;
                        const double fRate = GetDouble();
                        --nParamCount;
                        size_t nRefInList = 0;
                        ScRange aRange;
                        while (nParamCount-- > 0)
                        {
                            switch (GetStackType())
                            {
                                case svDouble:
                                {
                                    fVal += GetDouble() / pow(1.0 + fRate, fCount);
                                    fCount++;
                                }
                                break;
                                case svSingleRef:
                                {
                                    ScAddress aAdr;
                                    PopSingleRef(aAdr);
                                    ScRefCellValue aCell(mrDoc, aAdr);
                                    if (!aCell.hasEmptyValue() && aCell.hasNumeric())
                                    {
                                        const double fCellVal = GetCellValue(aAdr, aCell);
                                        fVal += fCellVal / pow(1.0 + fRate, fCount);
                                        fCount++;
                                    }
                                }
                                break;
                                case svDoubleRef:
                                case svRefList:
                                {
                                    FormulaError nErr = FormulaError::NONE;
                                    double fCellVal = 0.0;
                                    PopDoubleRef(aRange, nParamCount, nRefInList);
                                    ScHorizontalValueIterator aValIter(mrDoc, aRange);
                                    while ((nErr == FormulaError::NONE)
                                           && aValIter.GetNext(fCellVal, nErr))
                                    {
                                        fVal += fCellVal / pow(1.0 + fRate, fCount);
                                        fCount++;
                                    }
                                    if (nErr != FormulaError::NONE)
                                        SetError(nErr);
                                }
                                break;
                                case svMatrix:
                                case svExternalSingleRef:
                                case svExternalDoubleRef:
                                {
                                    ScMatrixRef pMat = GetMatrix();
                                    if (pMat)
                                    {
                                        SCSIZE nC = 0;
                                        SCSIZE nR = 0;
                                        pMat->GetDimensions(nC, nR);
                                        if (nC == 0 || nR == 0)
                                        {
                                            PushIllegalArgument();
                                            return;
                                        }

                                        for (SCSIZE j = 0; j < nC; j++)
                                        {
                                            for (SCSIZE k = 0; k < nR; ++k)
                                            {
                                                if (!pMat->IsValue(j, k))
                                                {
                                                    PushIllegalArgument();
                                                    return;
                                                }
                                                const double fX = pMat->GetDouble(j, k);
                                                fVal += fX / pow(1.0 + fRate, fCount);
                                                fCount++;
                                            }
                                        }
                                    }
                                }
                                break;
                                default:
                                    SetError(FormulaError::IllegalParameter);
                                    break;
                            }
                        }
                    }
                    PushDouble(fVal.get());
                };
                const auto pushLegacyIrr = [&]() {
                    warnIfLegacyRateFamilyReached(u"IRR");
                    nFuncFmtType = SvNumFormatType::PERCENT;
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 1, 2))
                        return;

                    constexpr double fEpsilon = 1.0E-7;
                    const double fEstimated = nParamCount == 2 ? GetDouble() : 0.1;
                    double fEps = 1.0;
                    double x = fEstimated == -1.0 ? 0.1 : fEstimated;
                    double fValue = 0.0;

                    ScRange aRange;
                    ScMatrixRef pMat;
                    SCSIZE nC = 0;
                    SCSIZE nR = 0;
                    bool bIsMatrix = false;
                    switch (GetStackType())
                    {
                        case svDoubleRef:
                            PopDoubleRef(aRange);
                            break;
                        case svMatrix:
                        case svExternalSingleRef:
                        case svExternalDoubleRef:
                            pMat = GetMatrix();
                            if (pMat)
                            {
                                pMat->GetDimensions(nC, nR);
                                if (nC == 0 || nR == 0)
                                {
                                    PushIllegalParameter();
                                    return;
                                }
                                bIsMatrix = true;
                            }
                            else
                            {
                                PushIllegalParameter();
                                return;
                            }
                            break;
                        default:
                            PushIllegalParameter();
                            return;
                    }

                    constexpr sal_uInt16 nIterationsMax = 20;
                    sal_uInt16 nItCount = 0;
                    FormulaError nIterError = FormulaError::NONE;
                    while (fEps > fEpsilon && nItCount < nIterationsMax
                           && nGlobalError == FormulaError::NONE)
                    {
                        KahanSum fNom = 0.0;
                        KahanSum fDenom = 0.0;
                        double fCount = 0.0;
                        if (bIsMatrix)
                        {
                            for (SCSIZE j = 0; j < nC && nGlobalError == FormulaError::NONE; j++)
                            {
                                for (SCSIZE k = 0; k < nR; k++)
                                {
                                    if (!pMat->IsValue(j, k))
                                        continue;
                                    fValue = pMat->GetDouble(j, k);
                                    if (nGlobalError != FormulaError::NONE)
                                        break;

                                    fNom += fValue / pow(1.0 + x, fCount);
                                    fDenom += -fCount * fValue / pow(1.0 + x, fCount + 1.0);
                                    fCount++;
                                }
                            }
                        }
                        else
                        {
                            ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags);
                            bool bLoop = aValIter.GetFirst(fValue, nIterError);
                            while (bLoop && nIterError == FormulaError::NONE)
                            {
                                fNom += fValue / pow(1.0 + x, fCount);
                                fDenom += -fCount * fValue / pow(1.0 + x, fCount + 1.0);
                                fCount++;
                                bLoop = aValIter.GetNext(fValue, nIterError);
                            }
                            SetError(nIterError);
                        }
                        const double xNew
                            = x - o3tl::div_allow_zero(fNom.get(), fDenom.get());
                        nItCount++;
                        fEps = std::abs(xNew - x);
                        x = xNew;
                    }
                    if (fEstimated == 0.0 && std::abs(x) < fEpsilon)
                        x = 0.0;
                    if (fEps < fEpsilon)
                        PushDouble(x);
                    else
                        PushError(FormulaError::NoConvergence);
                };
                const auto pushLegacyMirr = [&]() {
                    warnIfLegacyRateFamilyReached(u"MIRR");
                    nFuncFmtType = SvNumFormatType::PERCENT;
                    if (!MustHaveParamCount(GetByte(), 3))
                        return;

                    const double fRate1_reinvest = GetDouble() + 1;
                    const double fRate1_invest = GetDouble() + 1;

                    ScRange aRange;
                    ScMatrixRef pMat;
                    SCSIZE nC = 0;
                    SCSIZE nR = 0;
                    bool bIsMatrix = false;
                    switch (GetStackType())
                    {
                        case svDoubleRef:
                            PopDoubleRef(aRange);
                            break;
                        case svMatrix:
                        case svExternalSingleRef:
                        case svExternalDoubleRef:
                            pMat = GetMatrix();
                            if (pMat)
                            {
                                pMat->GetDimensions(nC, nR);
                                if (nC == 0 || nR == 0)
                                    SetError(FormulaError::IllegalArgument);
                                bIsMatrix = true;
                            }
                            else
                                SetError(FormulaError::IllegalArgument);
                            break;
                        default:
                            SetError(FormulaError::IllegalParameter);
                            break;
                    }

                    if (nGlobalError != FormulaError::NONE)
                    {
                        PushError(nGlobalError);
                        return;
                    }

                    KahanSum fNPV_reinvest = 0.0;
                    double fPow_reinvest = 1.0;
                    KahanSum fNPV_invest = 0.0;
                    double fPow_invest = 1.0;
                    sal_uLong nCount = 0;
                    bool bHasPosValue = false;
                    bool bHasNegValue = false;

                    if (bIsMatrix)
                    {
                        for (SCSIZE j = 0; j < nC; j++)
                        {
                            for (SCSIZE k = 0; k < nR; ++k)
                            {
                                if (!pMat->IsValue(j, k))
                                    continue;
                                const double fX = pMat->GetDouble(j, k);
                                if (nGlobalError != FormulaError::NONE)
                                    break;

                                if (fX > 0.0)
                                {
                                    bHasPosValue = true;
                                    fNPV_reinvest += fX * fPow_reinvest;
                                }
                                else if (fX < 0.0)
                                {
                                    bHasNegValue = true;
                                    fNPV_invest += fX * fPow_invest;
                                }
                                fPow_reinvest /= fRate1_reinvest;
                                fPow_invest /= fRate1_invest;
                                nCount++;
                            }
                        }
                    }
                    else
                    {
                        ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags);
                        double fCellValue = 0.0;
                        FormulaError nIterError = FormulaError::NONE;

                        bool bLoop = aValIter.GetFirst(fCellValue, nIterError);
                        while (bLoop)
                        {
                            if (fCellValue > 0.0)
                            {
                                bHasPosValue = true;
                                fNPV_reinvest += fCellValue * fPow_reinvest;
                            }
                            else if (fCellValue < 0.0)
                            {
                                bHasNegValue = true;
                                fNPV_invest += fCellValue * fPow_invest;
                            }
                            fPow_reinvest /= fRate1_reinvest;
                            fPow_invest /= fRate1_invest;
                            nCount++;

                            bLoop = aValIter.GetNext(fCellValue, nIterError);
                        }

                        if (nIterError != FormulaError::NONE)
                            SetError(nIterError);
                    }

                    if (!(bHasPosValue && bHasNegValue))
                        SetError(FormulaError::IllegalArgument);

                    if (nGlobalError != FormulaError::NONE)
                    {
                        PushError(nGlobalError);
                        return;
                    }

                    double fResult = -o3tl::div_allow_zero(
                        fNPV_reinvest.get(), fNPV_invest.get());
                    fResult *= pow(fRate1_reinvest, static_cast<double>(nCount - 1));
                    fResult = pow(fResult, div(1.0, (nCount - 1)));
                    PushDouble(fResult - 1.0);
                };
                const auto pushLegacyBitwise = [&](std::u16string_view rFunctionName,
                                                   auto aOperator) {
                    warnIfLegacyDispatchReached(
                        "family-local default-on", rFunctionName,
                        [](std::u16string_view rFormula) {
                            return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                        },
                        "family-local default-on bitwise slice reached ScInterpreter");

                    if (!MustHaveParamCount(GetByte(), 2))
                        return;

                    const double fRight = GetDouble();
                    const double fLeft = GetDouble();
                    if (std::optional<double> fResult = aOperator(fLeft, fRight))
                        PushDouble(*fResult);
                    else
                        PushIllegalArgument();
                };
                const auto pushLegacyFormulaText = [&]() {
                    warnIfLegacyDispatchReached(
                        "family-local default-on", u"FORMULA",
                        [](std::u16string_view rFormula) {
                            return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                        },
                        "family-local default-on FORMULA reached ScInterpreter", true);

                    OUString aFormula;
                    switch (GetStackType())
                    {
                        case svDoubleRef:
                            if (IsInArrayContext())
                            {
                                SCCOL nCol1, nCol2;
                                SCROW nRow1, nRow2;
                                SCTAB nTab1, nTab2;
                                PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                                if (nGlobalError != FormulaError::NONE)
                                    break;

                                if (nTab1 != nTab2)
                                {
                                    SetError(FormulaError::IllegalArgument);
                                    break;
                                }

                                const auto aMatrixResult = seformulainspect::buildFormulaTextMatrix(
                                    mrDoc, mrContext,
                                    ScRange(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2), mrStrPool,
                                    [this](SCSIZE nColumns, SCSIZE nRows) {
                                        return GetNewMat(nColumns, nRows, true);
                                    });
                                if (aMatrixResult.meFailure
                                    == seformulainspect::MatrixInspectionFailure::IllegalArgument)
                                {
                                    SetError(FormulaError::IllegalArgument);
                                    break;
                                }
                                if (aMatrixResult.meFailure
                                    == seformulainspect::MatrixInspectionFailure::MatrixSize)
                                {
                                    break;
                                }

                                PushMatrix(aMatrixResult.mpMatrix);
                                return;
                            }
                            [[fallthrough]];
                        case svSingleRef:
                        {
                            ScAddress aAdr;
                            if (!PopDoubleRefOrSingleRef(aAdr))
                                break;

                            const auto aFormulaText
                                = seformulainspect::formulaTextForCell(mrDoc, mrContext, aAdr);
                            if (!aFormulaText)
                                SetError(selibreoffice::toFormulaError(aFormulaText.meError));
                            else
                                aFormula = aFormulaText.maValue;
                        }
                        break;
                        default:
                            PopError();
                            SetError(FormulaError::NotAvailable);
                    }

                    PushString(aFormula);
                };
                const auto warnTextUtilityDispatch = [&](std::u16string_view rFunctionName) {
                    warnIfLegacyDispatchReached(
                        "family-local default-on", rFunctionName,
                        [](std::u16string_view rFormula) {
                            return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                        },
                        "family-local default-on text utility reached ScInterpreter");
                };
                const auto pushLegacyUnaryTextTransform =
                    [&](std::u16string_view rFunctionName, auto aTransform) {
                        warnTextUtilityDispatch(rFunctionName);
                        PushString(aTransform(GetString().getString()));
                    };
                const auto pushLegacyLen = [&]() {
                    warnTextUtilityDispatch(u"LEN");
                    PushDouble(selibreoffice::countCodePoints(GetString().getString()));
                };
                const auto pushLegacyT = [&]() {
                    warnTextUtilityDispatch(u"T");
                    switch (GetStackType())
                    {
                        case svDoubleRef:
                        case svSingleRef:
                        {
                            ScAddress aAdr;
                            if (!PopDoubleRefOrSingleRef(aAdr))
                            {
                                PushInt(0);
                                return;
                            }
                            bool bValue = false;
                            ScRefCellValue aCell(mrDoc, aAdr);
                            if (GetCellErrCode(aCell) == FormulaError::NONE)
                            {
                                switch (aCell.getType())
                                {
                                    case CELLTYPE_VALUE:
                                        bValue = true;
                                        break;
                                    case CELLTYPE_FORMULA:
                                        bValue = aCell.getFormula()->IsValue();
                                        break;
                                    default:
                                        break;
                                }
                            }
                            if (bValue)
                                PushString(OUString());
                            else
                            {
                                svl::SharedString aStr;
                                GetCellString(aStr, aCell);
                                PushString(aStr);
                            }
                        }
                        break;
                        case svMatrix:
                        case svExternalSingleRef:
                        case svExternalDoubleRef:
                        {
                            double fVal;
                            svl::SharedString aStr;
                            ScMatValType nMatValType = GetDoubleOrStringFromMatrix(fVal, aStr);
                            if (ScMatrix::IsValueType(nMatValType))
                                PushString(svl::SharedString::getEmptyString());
                            else
                                PushString(aStr);
                        }
                        break;
                        case svDouble:
                        {
                            PopError();
                            PushString(OUString());
                        }
                        break;
                        case svString:
                            break;
                        default:
                            PushError(FormulaError::UnknownOpCode);
                    }
                };
                const auto pushLegacyClean = [&]() {
                    warnTextUtilityDispatch(u"CLEAN");
                    PushString(selibreoffice::cleanPrintable(GetString().getString()));
                };
                const auto pushLegacyCode = [&]() {
                    warnTextUtilityDispatch(u"CODE");
                    PushInt(selibreoffice::codeFromText(GetString().getString()));
                };
                const auto pushLegacyChar = [&]() {
                    warnTextUtilityDispatch(u"CHAR");
                    if (auto aStr = selibreoffice::charFromValue(GetDouble()))
                        PushString(*aStr);
                    else
                        PushIllegalArgument();
                };
                const auto pushLegacyJisAsc = [&](std::u16string_view rFunctionName,
                                                  auto aTransform) {
                    warnTextUtilityDispatch(rFunctionName);
                    if (MustHaveParamCount(GetByte(), 1))
                        PushString(aTransform(GetString().getString()));
                };
                const auto pushLegacyUnicode = [&]() {
                    warnTextUtilityDispatch(u"UNICODE");
                    if (MustHaveParamCount(GetByte(), 1))
                    {
                        if (std::optional<double> fValue
                            = selibreoffice::unicodeFromText(GetString().getString()))
                        {
                            PushDouble(*fValue);
                        }
                        else
                            PushIllegalParameter();
                    }
                };
                const auto pushLegacyUnichar = [&]() {
                    warnTextUtilityDispatch(u"UNICHAR");
                    if (MustHaveParamCount(GetByte(), 1))
                    {
                        sal_uInt32 nCodePoint = GetUInt32();
                        if (nGlobalError != FormulaError::NONE)
                            PushIllegalArgument();
                        else if (auto aStr = selibreoffice::unicharFromCodePoint(nCodePoint))
                            PushString(*aStr);
                        else
                            PushIllegalArgument();
                    }
                };
                const auto pushLegacyTextBeforeAfter = [&](bool bBefore) {
                    warnTextUtilityDispatch(bBefore ? u"TEXTBEFORE" : u"TEXTAFTER");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 1, 6))
                        return;

                    std::optional<svl::SharedString> aIfNotFound;
                    if (nParamCount == 6)
                        aIfNotFound = GetString();

                    bool bMatchEnd = false;
                    if (nParamCount >= 5)
                    {
                        if (!IsMissing())
                            bMatchEnd = GetBool();
                        else
                            Pop();
                    }

                    bool bMatchMode = false;
                    if (nParamCount >= 4)
                    {
                        if (!IsMissing())
                            bMatchMode = GetBool();
                        else
                            Pop();
                    }

                    sal_Int32 nInstanceNum(1);
                    if (nParamCount >= 3)
                    {
                        if (!IsMissing())
                            nInstanceNum = GetInt32WithDefault(1);
                        else
                            Pop();
                    }

                    if (nInstanceNum == 0)
                    {
                        PushError(FormulaError::NotAvailable);
                        return;
                    }

                    std::vector<svl::SharedString> aDelimiters;
                    if (nParamCount >= 2)
                    {
                        switch (GetStackType())
                        {
                            case svSingleRef:
                            case svDoubleRef:
                            case svMatrix:
                            case svExternalSingleRef:
                            case svExternalDoubleRef:
                            {
                                ScMatrixRef pMatSource = GetMatrix();
                                if (!pMatSource)
                                {
                                    PushIllegalParameter();
                                    return;
                                }

                                SCSIZE nsC = 0;
                                SCSIZE nsR = 0;
                                pMatSource->GetDimensions(nsC, nsR);
                                for (SCSIZE i = 0; i < nsC; ++i)
                                {
                                    for (SCSIZE j = 0; j < nsR; ++j)
                                        aDelimiters.push_back(pMatSource->GetString(i, j));
                                }
                            }
                            break;
                            default:
                                aDelimiters.push_back(GetString());
                        }
                    }

                    svl::SharedString sText = GetString();
                    if (sText.isEmpty())
                    {
                        PushIllegalParameter();
                        return;
                    }

                    std::vector<sal_Int32> aDelimiterPositions;
                    if (bMatchEnd && !bBefore)
                        aDelimiterPositions.push_back(0);

                    OUString sStr(sText.getString());
                    const sal_Int32 nLength(sStr.getLength());
                    sal_Int32 nStart(0);
                    while (nStart < nLength)
                    {
                        sal_Int32 nIndex = nLength;
                        sal_Int32 nDelLength(0);
                        bool bFound = false;

                        for (auto& rDelimiter : aDelimiters)
                        {
                            if (rDelimiter.isEmpty())
                                continue;

                            OUString sDelimiter = rDelimiter.getString();
                            sal_Int32 nDelimiterIndex = bMatchMode
                                                            ? ScGlobal::getCharClass()
                                                                  .lowercase(sStr)
                                                                  .indexOf(
                                                                      ScGlobal::getCharClass()
                                                                          .lowercase(sDelimiter),
                                                                      nStart)
                                                            : sStr.indexOf(sDelimiter, nStart);

                            if (nDelimiterIndex != -1 && nDelimiterIndex < nIndex)
                            {
                                bFound = true;
                                nDelLength = sDelimiter.getLength();
                                nIndex = nDelimiterIndex;
                            }
                        }

                        if (bFound)
                        {
                            aDelimiterPositions.push_back(
                                bBefore ? nIndex : nIndex + nDelLength);
                        }

                        nStart = nIndex + nDelLength;
                    }

                    if (bMatchEnd && bBefore)
                        aDelimiterPositions.push_back(nLength);

                    const sal_Int32 nSize(aDelimiterPositions.size());
                    if (nSize == 0 || std::abs(nInstanceNum) > nSize)
                    {
                        if (aIfNotFound.has_value())
                            PushString(aIfNotFound.value());
                        else
                            PushError(FormulaError::NotAvailable);
                        return;
                    }

                    if (nInstanceNum < 0)
                        nInstanceNum = nSize + nInstanceNum + 1;

                    const sal_Int32 nDelimiterPos(aDelimiterPositions[nInstanceNum - 1]);
                    if (bBefore)
                        PushString(sStr.copy(0, nDelimiterPos));
                    else
                        PushString(sStr.copy(nDelimiterPos, nLength - nDelimiterPos));
                };
                const auto pushLegacyTrim = [&]() {
                    warnTextUtilityDispatch(u"TRIM");
                    PushString(selibreoffice::trimRepeatedSpaces(GetString().getString()));
                };
                const auto pushLegacyValue = [&]() {
                    warnTextUtilityDispatch(u"VALUE");

                    OUString aInputString;
                    double fVal;

                    switch (GetRawStackType())
                    {
                        case svMissing:
                        case svEmptyCell:
                            Pop();
                            PushInt(0);
                            return;
                        case svDouble:
                            return;
                        case svSingleRef:
                        case svDoubleRef:
                        {
                            ScAddress aAdr;
                            if (!PopDoubleRefOrSingleRef(aAdr))
                            {
                                PushInt(0);
                                return;
                            }
                            ScRefCellValue aCell(mrDoc, aAdr);
                            if (aCell.hasString())
                            {
                                svl::SharedString aSS;
                                GetCellString(aSS, aCell);
                                aInputString = aSS.getString();
                            }
                            else if (aCell.hasNumeric())
                            {
                                PushDouble(GetCellValue(aAdr, aCell));
                                return;
                            }
                            else
                            {
                                PushDouble(0.0);
                                return;
                            }
                        }
                        break;
                        case svMatrix:
                        {
                            svl::SharedString aSS;
                            ScMatValType nType = GetDoubleOrStringFromMatrix(fVal, aSS);
                            aInputString = aSS.getString();
                            switch (nType)
                            {
                                case ScMatValType::Empty:
                                    fVal = 0.0;
                                    [[fallthrough]];
                                case ScMatValType::Value:
                                case ScMatValType::Boolean:
                                    PushDouble(fVal);
                                    return;
                                case ScMatValType::String:
                                    break;
                                default:
                                    PushIllegalArgument();
                            }
                        }
                        break;
                        default:
                            aInputString = GetString().getString();
                            break;
                    }

                    const auto aResult
                        = setextparseexec::evaluateValue(mrDoc, mrContext, aInputString);
                    if (aResult)
                        PushDouble(aResult.maValue);
                    else
                        PushIllegalArgument();
                };
                const auto pushLegacyNumberValue = [&]() {
                    warnTextUtilityDispatch(u"NUMBERVALUE");

                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 1, 3))
                        return;

                    std::optional<OUString> oGroupSeparator;
                    std::optional<OUString> oDecimalSeparator;
                    if (nParamCount == 3)
                        oGroupSeparator = GetString().getString();
                    if (nParamCount >= 2)
                        oDecimalSeparator = GetString().getString();

                    if (GetStackType() == svDouble)
                        return;

                    OUString aInputString = GetString().getString();
                    if (nGlobalError != FormulaError::NONE)
                    {
                        PushError(nGlobalError);
                        return;
                    }

                    const auto aResult = setextparseexec::evaluateNumberValue(
                        mrDoc, mrContext, aInputString, oDecimalSeparator, oGroupSeparator,
                        maCalcConfig.mbEmptyStringAsZero);
                    if (aResult)
                    {
                        PushDouble(aResult.maValue);
                        return;
                    }

                    switch (aResult.meError)
                    {
                        case spreadsheetengine::api::Error::IllegalArgument:
                            PushIllegalArgument();
                            return;
                        case spreadsheetengine::api::Error::NoValue:
                            PushNoValue();
                            return;
                        default:
                            PushIllegalArgument();
                            return;
                    }
                };
                const auto pushLegacyCurrency = [&]() {
                    warnTextUtilityDispatch(u"DOLLAR");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 1, 2))
                        return;

                    OUString aStr;
                    double fDec;
                    if (nParamCount == 2)
                    {
                        fDec = ::rtl::math::approxFloor(GetDouble());
                        if (fDec < -15.0 || fDec > 15.0)
                        {
                            PushIllegalArgument();
                            return;
                        }
                    }
                    else
                        fDec = 2.0;
                    double fVal = GetDouble();
                    double fFac = fDec != 0.0 ? pow(double(10), fDec) : 1.0;
                    if (fVal < 0.0)
                        fVal = ceil(fVal * fFac - 0.5) / fFac;
                    else
                        fVal = floor(fVal * fFac + 0.5) / fFac;
                    const Color* pColor = nullptr;
                    if (fDec < 0.0)
                        fDec = 0.0;
                    sal_uLong nIndex = mrContext.NFGetStandardFormat(
                        SvNumFormatType::CURRENCY, ScGlobal::eLnge);
                    if (static_cast<sal_uInt16>(fDec) != mrContext.NFGetFormatPrecision(nIndex))
                    {
                        OUString sFormatString = mrContext.NFGenerateFormat(
                            nIndex, ScGlobal::eLnge, true, false,
                            static_cast<sal_uInt16>(fDec));
                        if (!mrContext.NFGetPreviewString(
                                sFormatString, fVal, aStr, &pColor, ScGlobal::eLnge))
                            SetError(FormulaError::IllegalArgument);
                    }
                    else
                        mrContext.NFGetOutputString(fVal, nIndex, aStr, &pColor);
                    PushString(aStr);
                };
                const auto pushLegacyReplace = [&]() {
                    warnTextUtilityDispatch(u"REPLACE");
                    if (!MustHaveParamCount(GetByte(), 4))
                        return;

                    OUString aNewStr = GetString().getString();
                    sal_Int32 nCount = GetStringPositionArgument();
                    sal_Int32 nPos = GetStringPositionArgument();
                    OUString aOldStr = GetString().getString();
                    if (nPos < 1 || nCount < 0)
                        PushIllegalArgument();
                    else
                    {
                        sal_Int32 nLen = aOldStr.getLength();
                        if (nPos > nLen + 1)
                            nPos = nLen + 1;
                        if (nCount > nLen - nPos + 1)
                            nCount = nLen - nPos + 1;
                        sal_Int32 nIdx = 0;
                        sal_Int32 nCnt = 0;
                        while (nIdx < nLen && nPos > nCnt + 1)
                        {
                            aOldStr.iterateCodePoints(&nIdx);
                            ++nCnt;
                        }
                        sal_Int32 nStart = nIdx;
                        while (nIdx < nLen && nPos + nCount - 1 > nCnt)
                        {
                            aOldStr.iterateCodePoints(&nIdx);
                            ++nCnt;
                        }
                        if (CheckStringResultLen(aOldStr, aNewStr.getLength() - (nIdx - nStart)))
                            aOldStr = aOldStr.replaceAt(nStart, nIdx - nStart, aNewStr);
                        PushString(aOldStr);
                    }
                };
                const auto pushLegacyFixed = [&]() {
                    warnTextUtilityDispatch(u"FIXED");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 1, 3))
                        return;

                    OUString aStr;
                    double fDec;
                    bool bThousand;
                    if (nParamCount == 3)
                        bThousand = !GetBool();
                    else
                        bThousand = true;
                    if (nParamCount >= 2)
                    {
                        fDec = ::rtl::math::approxFloor(GetDoubleWithDefault(2.0));
                        if (fDec < -15.0 || fDec > 15.0)
                        {
                            PushIllegalArgument();
                            return;
                        }
                    }
                    else
                        fDec = 2.0;
                    double fVal = GetDouble();
                    double fFac = fDec != 0.0 ? pow(double(10), fDec) : 1.0;
                    if (fVal < 0.0)
                        fVal = ceil(fVal * fFac - 0.5) / fFac;
                    else
                        fVal = floor(fVal * fFac + 0.5) / fFac;
                    const Color* pColor = nullptr;
                    if (fDec < 0.0)
                        fDec = 0.0;
                    sal_uLong nIndex = mrContext.NFGetStandardFormat(
                        SvNumFormatType::NUMBER, ScGlobal::eLnge);
                    OUString sFormatString = mrContext.NFGenerateFormat(
                        nIndex, ScGlobal::eLnge, bThousand, false,
                        static_cast<sal_uInt16>(fDec));
                    if (!mrContext.NFGetPreviewString(
                            sFormatString, fVal, aStr, &pColor, ScGlobal::eLnge))
                        PushIllegalArgument();
                    else
                        PushString(aStr);
                };
                const auto pushLegacyFind = [&]() {
                    warnTextUtilityDispatch(u"FIND");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 2, 3))
                        return;

                    sal_Int32 nCnt = nParamCount == 3 ? GetDouble() : 1;
                    OUString sStr = GetString().getString();
                    if (nCnt < 1 || nCnt > sStr.getLength())
                        PushNoValue();
                    else
                    {
                        sal_Int32 nPos = sStr.indexOf(GetString().getString(), nCnt - 1);
                        if (nPos == -1)
                            PushNoValue();
                        else
                        {
                            sal_Int32 nIdx = 0;
                            nCnt = 0;
                            while (nIdx < nPos)
                            {
                                sStr.iterateCodePoints(&nIdx);
                                ++nCnt;
                            }
                            PushDouble(static_cast<double>(nCnt + 1));
                        }
                    }
                };
                const auto pushLegacyMid = [&]() {
                    warnTextUtilityDispatch(u"MID");
                    if (!MustHaveParamCount(GetByte(), 3))
                        return;

                    const sal_Int32 nSubLen = GetStringPositionArgument();
                    const sal_Int32 nStart = GetStringPositionArgument();
                    OUString aStr = GetString().getString();
                    if (nStart < 1 || nSubLen < 0)
                        PushIllegalArgument();
                    else if (nStart > kScInterpreterMaxStrLen || nSubLen > kScInterpreterMaxStrLen)
                        PushError(FormulaError::StringOverflow);
                    else
                    {
                        sal_Int32 nLen = aStr.getLength();
                        sal_Int32 nIdx = 0;
                        sal_Int32 nCnt = 0;
                        while (nIdx < nLen && nStart - 1 > nCnt)
                        {
                            aStr.iterateCodePoints(&nIdx);
                            ++nCnt;
                        }
                        sal_Int32 nIdx0 = nIdx;
                        while (nIdx < nLen && nStart + nSubLen - 1 > nCnt)
                        {
                            aStr.iterateCodePoints(&nIdx);
                            ++nCnt;
                        }
                        PushString(aStr.copy(nIdx0, nIdx - nIdx0));
                    }
                };
                const auto pushLegacyText = [&]() {
                    warnTextUtilityDispatch(u"TEXT");
                    if (!MustHaveParamCount(GetByte(), 2))
                        return;

                    OUString sFormatString = GetString().getString();
                    svl::SharedString aStr;
                    bool bString = false;
                    double fVal = 0.0;
                    switch (GetStackType())
                    {
                        case svError:
                            PopError();
                            break;
                        case svDouble:
                            fVal = PopDouble();
                            break;
                        default:
                        {
                            FormulaConstTokenRef xTok(PopToken());
                            if (nGlobalError == FormulaError::NONE)
                            {
                                PushTokenRef(xTok);
                                FormulaError nSErr = mnStringNoValueError;
                                mnStringNoValueError = FormulaError::NotNumericString;
                                fVal = GetDouble();
                                mnStringNoValueError = nSErr;
                                if (nGlobalError == FormulaError::NotNumericString)
                                {
                                    nGlobalError = FormulaError::NONE;
                                    PushTokenRef(xTok);
                                    aStr = GetString();
                                    bString = true;
                                }
                            }
                        }
                    }
                    if (nGlobalError != FormulaError::NONE)
                        PushError(nGlobalError);
                    else if (sFormatString.isEmpty())
                    {
                        if (bString)
                            PushString(aStr);
                        else
                            PushString(OUString());
                    }
                    else
                    {
                        OUString aResult;
                        const Color* pColor = nullptr;
                        LanguageType eCellLang;
                        const ScPatternAttr* pPattern
                            = mrDoc.GetPattern(aPos.Col(), aPos.Row(), aPos.Tab());
                        if (pPattern)
                            eCellLang = pPattern->GetItem(ATTR_LANGUAGE_FORMAT).GetValue();
                        else
                            eCellLang = ScGlobal::eLnge;
                        if (bString)
                        {
                            if (!mrContext.NFGetPreviewString(
                                    sFormatString, aStr.getString(), aResult, &pColor, eCellLang))
                                PushIllegalArgument();
                            else
                                PushString(aResult);
                        }
                        else
                        {
                            if (!mrContext.NFGetPreviewStringGuess(
                                    sFormatString, fVal, aResult, &pColor, eCellLang))
                                PushIllegalArgument();
                            else
                                PushString(aResult);
                        }
                    }
                };
                const auto pushLegacySubstitute = [&]() {
                    warnTextUtilityDispatch(u"SUBSTITUTE");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 3, 4))
                        return;

                    sal_Int32 nCnt;
                    if (nParamCount == 4)
                    {
                        nCnt = GetStringPositionArgument();
                        if (nCnt < 1)
                        {
                            PushIllegalArgument();
                            return;
                        }
                    }
                    else
                        nCnt = 0;
                    OUString sNewStr = GetString().getString();
                    OUString sOldStr = GetString().getString();
                    OUString sStr = GetString().getString();
                    sal_Int32 nPos = 0;
                    sal_Int32 nCount = 0;
                    std::optional<OUStringBuffer> oResult;
                    for (sal_Int32 nEnd = sStr.indexOf(sOldStr); nEnd >= 0;
                         nEnd = sStr.indexOf(sOldStr, nEnd))
                    {
                        if (nCnt == 0 || ++nCount == nCnt)
                        {
                            if (!oResult)
                                oResult.emplace(
                                    sStr.getLength() + sNewStr.getLength() - sOldStr.getLength());
                            oResult->append(sStr.subView(nPos, nEnd - nPos));
                            if (!CheckStringResultLen(*oResult, sNewStr.getLength()))
                                return PushError(GetError());
                            oResult->append(sNewStr);
                            nPos = nEnd + sOldStr.getLength();
                            if (nCnt > 0)
                                break;
                        }
                        nEnd += sOldStr.getLength();
                    }
                    if (oResult)
                        oResult->append(sStr.subView(nPos, sStr.getLength() - nPos));
                    PushString(oResult ? oResult->makeStringAndClear() : sStr);
                };
                const auto pushLegacyRept = [&]() {
                    warnTextUtilityDispatch(u"REPT");
                    if (!MustHaveParamCount(GetByte(), 2))
                        return;

                    sal_Int32 nCnt = GetStringPositionArgument();
                    OUString aStr = GetString().getString();
                    if (nCnt < 0)
                        PushIllegalArgument();
                    else if (static_cast<double>(nCnt) * aStr.getLength() > kScInterpreterMaxStrLen)
                        PushError(FormulaError::StringOverflow);
                    else if (nCnt == 0)
                        PushString(OUString());
                    else
                    {
                        const sal_Int32 nLen = aStr.getLength();
                        OUStringBuffer aRes(nCnt * nLen);
                        while (nCnt--)
                            aRes.append(aStr);
                        PushString(aRes.makeStringAndClear());
                    }
                };
                const auto pushLegacyConcat = [&]() {
                    warnTextUtilityDispatch(u"CONCATENATE");
                    sal_uInt8 nParamCount = GetByte();
                    ReverseStack(nParamCount);

                    OUStringBuffer aRes;
                    while (nParamCount-- > 0)
                    {
                        OUString aStr = GetString().getString();
                        if (CheckStringResultLen(aRes, aStr.getLength()))
                            aRes.append(aStr);
                        else
                            break;
                    }
                    PushString(aRes.makeStringAndClear());
                };
                const auto pushLegacySearch = [&]() {
                    warnTextUtilityDispatch(u"SEARCH");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 2, 3))
                        return;

                    sal_Int32 nStart;
                    if (nParamCount == 3)
                    {
                        nStart = GetStringPositionArgument();
                        if (nStart < 1)
                        {
                            PushIllegalArgument();
                            return;
                        }
                    }
                    else
                        nStart = 1;
                    OUString sStr = GetString().getString();
                    OUString SearchStr = GetString().getString();
                    sal_Int32 nPos = nStart - 1;
                    sal_Int32 nEndPos = sStr.getLength();
                    if (nPos >= nEndPos)
                        PushNoValue();
                    else
                    {
                        utl::SearchParam::SearchType eSearchType
                            = DetectSearchType(SearchStr, mrDoc);
                        utl::SearchParam sPar(SearchStr, eSearchType, false, '~', false);
                        utl::TextSearch sT(sPar, ScGlobal::getCharClass());
                        bool bBool = sT.SearchForward(sStr, &nPos, &nEndPos);
                        if (!bBool)
                            PushNoValue();
                        else
                        {
                            sal_Int32 nIdx = 0;
                            sal_Int32 nCnt = 0;
                            while (nIdx < nPos)
                            {
                                sStr.iterateCodePoints(&nIdx);
                                ++nCnt;
                            }
                            PushDouble(static_cast<double>(nCnt + 1));
                        }
                    }
                };
                const auto pushLegacyRegex = [&]() {
                    warnTextUtilityDispatch(u"REGEX");
                    const sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 2, 4))
                        return;

                    bool bGlobalReplacement = false;
                    sal_Int32 nOccurrence = 1;
                    if (nParamCount == 4)
                    {
                        double fOccurrence;
                        svl::SharedString aFlagsString;
                        bool bDouble;
                        if (!IsMissing())
                            bDouble = GetDoubleOrString(fOccurrence, aFlagsString);
                        else
                        {
                            PopError();
                            bDouble = true;
                            fOccurrence = nOccurrence;
                        }
                        if (nGlobalError != FormulaError::NONE)
                        {
                            PushError(nGlobalError);
                            return;
                        }
                        if (bDouble)
                        {
                            if (!CheckStringPositionArgument(fOccurrence))
                            {
                                PushError(FormulaError::IllegalArgument);
                                return;
                            }
                            nOccurrence = static_cast<sal_Int32>(fOccurrence);
                        }
                        else
                        {
                            const OUString& aFlags(aFlagsString.getString());
                            if (aFlags.getLength() > 1)
                            {
                                PushIllegalArgument();
                                return;
                            }
                            if (aFlags.getLength() == 1)
                            {
                                if (aFlags.indexOf('g') >= 0)
                                    bGlobalReplacement = true;
                                else
                                {
                                    PushIllegalArgument();
                                    return;
                                }
                            }
                        }
                    }

                    bool bReplacement = false;
                    OUString aReplacement;
                    if (nParamCount >= 3)
                    {
                        if (IsMissing() || nOccurrence == 0)
                            PopError();
                        else
                        {
                            aReplacement = GetString().getString();
                            bReplacement = true;
                        }
                    }

                    const OUString aExpression = GetString().getString();
                    const OUString aText = GetString().getString();

                    if (nGlobalError != FormulaError::NONE)
                    {
                        PushError(nGlobalError);
                        return;
                    }
                    if (nOccurrence == 0)
                    {
                        PushString(aText);
                        return;
                    }

                    const icu::UnicodeString aIcuExpression(
                        false, reinterpret_cast<const UChar*>(aExpression.getStr()),
                        aExpression.getLength());
                    UErrorCode status = U_ZERO_ERROR;
                    icu::RegexMatcher aRegexMatcher(aIcuExpression, 0, status);
                    if (U_FAILURE(status))
                    {
                        PushIllegalArgument();
                        return;
                    }
                    aRegexMatcher.setTimeLimit(23 * 1000, status);

                    const icu::UnicodeString aIcuText(
                        false, reinterpret_cast<const UChar*>(aText.getStr()),
                        aText.getLength());
                    aRegexMatcher.reset(aIcuText);

                    if (!bReplacement)
                    {
                        sal_Int32 nCount = 0;
                        while (aRegexMatcher.find(status) && U_SUCCESS(status)
                               && ++nCount < nOccurrence)
                            ;
                        if (U_FAILURE(status))
                        {
                            PushIllegalArgument();
                            return;
                        }
                        if (nCount != nOccurrence)
                        {
                            PushError(FormulaError::NotAvailable);
                            return;
                        }
                        icu::UnicodeString aMatch(aRegexMatcher.group(status));
                        if (U_FAILURE(status))
                        {
                            PushIllegalArgument();
                            return;
                        }
                        PushString(OUString(
                            reinterpret_cast<const sal_Unicode*>(aMatch.getBuffer()),
                            aMatch.length()));
                        return;
                    }

                    const icu::UnicodeString aIcuReplacement(
                        false, reinterpret_cast<const UChar*>(aReplacement.getStr()),
                        aReplacement.getLength());
                    icu::UnicodeString aReplaced;
                    if (bGlobalReplacement)
                        aReplaced = aRegexMatcher.replaceAll(aIcuReplacement, status);
                    else if (nOccurrence == 1)
                        aReplaced = aRegexMatcher.replaceFirst(aIcuReplacement, status);
                    else
                    {
                        sal_Int32 nCount = 0;
                        while (aRegexMatcher.find(status) && U_SUCCESS(status))
                        {
                            if (++nCount == nOccurrence)
                            {
                                aRegexMatcher.appendReplacement(
                                    aReplaced, aIcuReplacement, status);
                                break;
                            }
                        }
                        aRegexMatcher.appendTail(aReplaced);
                    }
                    if (U_FAILURE(status))
                    {
                        PushIllegalArgument();
                        return;
                    }
                    PushString(OUString(
                        reinterpret_cast<const sal_Unicode*>(aReplaced.getBuffer()),
                        aReplaced.length()));
                };
                const auto pushLegacyLenB = [&]() {
                    warnTextUtilityDispatch(u"LENB");
                    PushDouble(getLengthB(GetString().getString()));
                };
                const auto pushLegacyRightB = [&]() {
                    warnTextUtilityDispatch(u"RIGHTB");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 1, 2))
                        return;
                    sal_Int32 n = 1;
                    if (nParamCount == 2)
                    {
                        n = GetStringPositionArgument();
                        if (n < 0)
                        {
                            PushIllegalArgument();
                            return;
                        }
                    }
                    PushString(lcl_RightB(GetString().getString(), n));
                };
                const auto pushLegacyLeftB = [&]() {
                    warnTextUtilityDispatch(u"LEFTB");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 1, 2))
                        return;
                    sal_Int32 n = 1;
                    if (nParamCount == 2)
                    {
                        n = GetStringPositionArgument();
                        if (n < 0)
                        {
                            PushIllegalArgument();
                            return;
                        }
                    }
                    PushString(lcl_LeftB(GetString().getString(), n));
                };
                const auto pushLegacyMidB = [&]() {
                    warnTextUtilityDispatch(u"MIDB");
                    if (!MustHaveParamCount(GetByte(), 3))
                        return;
                    const sal_Int32 nCount = GetStringPositionArgument();
                    const sal_Int32 nStart = GetStringPositionArgument();
                    OUString aStr = GetString().getString();
                    if (nStart < 1 || nCount < 0)
                        PushIllegalArgument();
                    else
                    {
                        aStr = lcl_LeftB(aStr, nStart + nCount - 1);
                        sal_Int32 nCnt = getLengthB(aStr) - nStart + 1;
                        PushString(lcl_RightB(aStr, std::max<sal_Int32>(nCnt, 0)));
                    }
                };
                const auto pushLegacyReplaceB = [&]() {
                    warnTextUtilityDispatch(u"REPLACEB");
                    if (!MustHaveParamCount(GetByte(), 4))
                        return;
                    OUString aNewStr = GetString().getString();
                    const sal_Int32 nCount = GetStringPositionArgument();
                    const sal_Int32 nPos = GetStringPositionArgument();
                    OUString aOldStr = GetString().getString();
                    int nLen = getLengthB(aOldStr);
                    if (nPos < 1.0 || nPos > nLen || nCount < 0.0 || nPos + nCount - 1 > nLen)
                        PushIllegalArgument();
                    else
                    {
                        OUString aStr1 = lcl_LeftB(aOldStr, nPos - 1);
                        OUString aStr3 = lcl_RightB(aOldStr, nLen - nPos - nCount + 1);
                        PushString(aStr1 + aNewStr + aStr3);
                    }
                };
                const auto pushLegacyFindB = [&]() {
                    warnTextUtilityDispatch(u"FINDB");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 2, 3))
                        return;
                    sal_Int32 nStart = nParamCount == 3 ? GetStringPositionArgument() : 1;
                    OUString aStr = GetString().getString();
                    int nLen = getLengthB(aStr);
                    OUString asStr = GetString().getString();
                    int nsLen = getLengthB(asStr);
                    if (nStart < 1 || nStart > nLen - nsLen + 1)
                        PushIllegalArgument();
                    else
                    {
                        OUString aBuf = lcl_RightB(aStr, nLen - nStart + 1);
                        sal_Int32 nPos = aBuf.indexOf(asStr, 0);
                        if (nPos == -1)
                            PushNoValue();
                        else
                        {
                            int nBytePos = lcl_getLengthB(aBuf, nPos);
                            PushDouble(nBytePos + nStart);
                        }
                    }
                };
                const auto pushLegacySearchB = [&]() {
                    warnTextUtilityDispatch(u"SEARCHB");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 2, 3))
                        return;

                    sal_Int32 nStart;
                    if (nParamCount == 3)
                    {
                        nStart = GetStringPositionArgument();
                        if (nStart < 1)
                        {
                            PushIllegalArgument();
                            return;
                        }
                    }
                    else
                        nStart = 1;
                    OUString aStr = GetString().getString();
                    sal_Int32 nLen = getLengthB(aStr);
                    OUString asStr = GetString().getString();
                    sal_Int32 nsLen = nStart - 1;
                    if (nsLen >= nLen)
                        PushNoValue();
                    else
                    {
                        OUString aSubStr(lcl_RightB(aStr, nLen - nStart + 1));
                        sal_Int32 nPos = 0;
                        sal_Int32 nEndPos = aSubStr.getLength();
                        utl::SearchParam::SearchType eSearchType
                            = DetectSearchType(asStr, mrDoc);
                        utl::SearchParam sPar(asStr, eSearchType, false, '~', false);
                        utl::TextSearch sT(sPar, ScGlobal::getCharClass());
                        if (!sT.SearchForward(aSubStr, &nPos, &nEndPos))
                            PushNoValue();
                        else
                        {
                            int nBytePos = lcl_getLengthB(aSubStr, nPos);
                            PushDouble(nBytePos + nStart);
                        }
                    }
                };
                const auto pushLegacyEncodeUrl = [&]() {
                    warnTextUtilityDispatch(u"ENCODEURL");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 1))
                        return;

                    OUString aStr = GetString().getString();
                    if (aStr.isEmpty())
                    {
                        PushError(FormulaError::NoValue);
                        return;
                    }

                    OString aUtf8Str(aStr.toUtf8());
                    const sal_Int32 nLen = aUtf8Str.getLength();
                    OStringBuffer aUrlBuf(nLen);
                    for (int i = 0; i < nLen; i++)
                    {
                        char c = aUtf8Str[i];
                        if (rtl::isAsciiAlphanumeric(static_cast<unsigned char>(c)) || c == '-'
                            || c == '_')
                        {
                            aUrlBuf.append(c);
                        }
                        else
                        {
                            aUrlBuf.append('%');
                            auto convertedChar = OString::number(
                                                     static_cast<unsigned char>(c), 16)
                                                     .toAsciiUpperCase();
                            if (convertedChar.length == 1)
                                aUrlBuf.append("0");
                            aUrlBuf.append(convertedChar);
                        }
                    }
                    PushString(OUString::fromUtf8(aUrlBuf));
                };
                const auto pushLegacyTextJoinMs = [&]() {
                    warnTextUtilityDispatch(u"TEXTJOIN");
                    short nParamCount = GetByte();
                    if (!MustHaveParamCountMin(nParamCount, 3))
                        return;

                    ReverseStack(nParamCount);

                    std::vector<OUString> aDelimiters;
                    size_t nRefInList = 0;
                    switch (GetStackType())
                    {
                        case svString:
                        case svDouble:
                            aDelimiters.push_back(GetString().getString());
                            break;
                        case svSingleRef:
                        {
                            ScAddress aAdr;
                            PopSingleRef(aAdr);
                            if (nGlobalError != FormulaError::NONE)
                                break;
                            ScRefCellValue aCell(mrDoc, aAdr);
                            if (aCell.hasEmptyValue())
                                aDelimiters.emplace_back("");
                            else
                            {
                                svl::SharedString aSS;
                                GetCellString(aSS, aCell);
                                aDelimiters.push_back(aSS.getString());
                            }
                        }
                        break;
                        case svDoubleRef:
                        case svRefList:
                        {
                            ScRange aRange;
                            PopDoubleRef(aRange, nParamCount, nRefInList);
                            if (nGlobalError != FormulaError::NONE)
                                break;
                            SCCOL nCol1, nCol2;
                            SCROW nRow1, nRow2;
                            SCTAB nTab1, nTab2;
                            aRange.GetVars(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                            if (nTab1 != nTab2)
                            {
                                SetError(FormulaError::IllegalParameter);
                                break;
                            }
                            PutInOrder(nRow1, nRow2);
                            PutInOrder(nCol1, nCol2);
                            ScAddress aAdr;
                            aAdr.SetTab(nTab1);
                            for (SCROW nRow = nRow1; nRow <= nRow2; nRow++)
                            {
                                for (SCCOL nCol = nCol1; nCol <= nCol2; nCol++)
                                {
                                    aAdr.SetRow(nRow);
                                    aAdr.SetCol(nCol);
                                    ScRefCellValue aCell(mrDoc, aAdr);
                                    if (aCell.hasEmptyValue())
                                        aDelimiters.emplace_back("");
                                    else
                                    {
                                        svl::SharedString aSS;
                                        GetCellString(aSS, aCell);
                                        aDelimiters.push_back(aSS.getString());
                                    }
                                }
                            }
                        }
                        break;
                        case svMatrix:
                        case svExternalSingleRef:
                        case svExternalDoubleRef:
                        {
                            ScMatrixRef pMat = GetMatrix();
                            if (pMat)
                            {
                                SCSIZE nC, nR;
                                pMat->GetDimensions(nC, nR);
                                if (nC == 0 || nR == 0)
                                    SetError(FormulaError::IllegalArgument);
                                else
                                {
                                    for (SCSIZE k = 0; k < nR; ++k)
                                        for (SCSIZE j = 0; j < nC; ++j)
                                            aDelimiters.push_back(pMat->GetString(j, k).getString());
                                }
                            }
                        }
                        break;
                        default:
                            PushIllegalArgument();
                            return;
                    }

                    bool bSkipEmpty = GetBool();
                    if (nGlobalError != FormulaError::NONE)
                    {
                        PushError(nGlobalError);
                        return;
                    }

                    OUStringBuffer aResBuf;
                    if (nGlobalError == FormulaError::NONE && nParamCount > 2)
                    {
                        std::vector<bool> aResArray;
                        nParamCount -= 2;
                        while (nParamCount-- > 0)
                        {
                            switch (GetStackType())
                            {
                                case svString:
                                case svDouble:
                                {
                                    OUString aStr = GetString().getString();
                                    if (!aStr.isEmpty() || !bSkipEmpty)
                                    {
                                        if (!aResBuf.isEmpty())
                                            aResBuf.append(aDelimiters[aResArray.size() % aDelimiters.size()]);
                                        if (CheckStringResultLen(aResBuf, aStr.getLength()))
                                            aResBuf.append(aStr);
                                    }
                                    aResArray.push_back(true);
                                }
                                break;
                                case svSingleRef:
                                {
                                    ScAddress aAdr;
                                    PopSingleRef(aAdr);
                                    if (nGlobalError != FormulaError::NONE)
                                        break;
                                    ScRefCellValue aCell(mrDoc, aAdr);
                                    if (!aCell.isEmpty() || !bSkipEmpty)
                                    {
                                        svl::SharedString aSS;
                                        GetCellString(aSS, aCell);
                                        OUString aStr = aSS.getString();
                                        if (!aResBuf.isEmpty())
                                            aResBuf.append(aDelimiters[aResArray.size() % aDelimiters.size()]);
                                        if (CheckStringResultLen(aResBuf, aStr.getLength()))
                                            aResBuf.append(aStr);
                                    }
                                    aResArray.push_back(true);
                                }
                                break;
                                case svDoubleRef:
                                case svRefList:
                                {
                                    ScRange aRange;
                                    PopDoubleRef(aRange, nParamCount, nRefInList);
                                    if (nGlobalError != FormulaError::NONE)
                                        break;
                                    SCCOL nCol1, nCol2;
                                    SCROW nRow1, nRow2;
                                    SCTAB nTab1, nTab2;
                                    aRange.GetVars(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                                    if (nTab1 != nTab2)
                                    {
                                        SetError(FormulaError::IllegalParameter);
                                        break;
                                    }
                                    PutInOrder(nRow1, nRow2);
                                    PutInOrder(nCol1, nCol2);
                                    ScAddress aAdr;
                                    aAdr.SetTab(nTab1);
                                    for (SCROW nRow = nRow1; nRow <= nRow2; nRow++)
                                    {
                                        for (SCCOL nCol = nCol1; nCol <= nCol2; nCol++)
                                        {
                                            aAdr.SetRow(nRow);
                                            aAdr.SetCol(nCol);
                                            ScRefCellValue aCell(mrDoc, aAdr);
                                            if (!aCell.isEmpty() || !bSkipEmpty)
                                            {
                                                svl::SharedString aSS;
                                                GetCellString(aSS, aCell);
                                                OUString aStr = aSS.getString();
                                                if (!aResBuf.isEmpty())
                                                    aResBuf.append(aDelimiters[aResArray.size() % aDelimiters.size()]);
                                                if (CheckStringResultLen(aResBuf, aStr.getLength()))
                                                    aResBuf.append(aStr);
                                            }
                                            aResArray.push_back(true);
                                        }
                                    }
                                }
                                break;
                                case svMatrix:
                                case svExternalSingleRef:
                                case svExternalDoubleRef:
                                {
                                    ScMatrixRef pMat = GetMatrix();
                                    if (pMat)
                                    {
                                        SCSIZE nC, nR;
                                        pMat->GetDimensions(nC, nR);
                                        if (nC == 0 || nR == 0)
                                            SetError(FormulaError::IllegalArgument);
                                        else
                                        {
                                            for (SCSIZE k = 0; k < nR; ++k)
                                            {
                                                for (SCSIZE j = 0; j < nC; ++j)
                                                {
                                                    OUString aStr = pMat->GetString(j, k).getString();
                                                    if (!aStr.isEmpty() || !bSkipEmpty)
                                                    {
                                                        if (!aResBuf.isEmpty())
                                                            aResBuf.append(aDelimiters[aResArray.size() % aDelimiters.size()]);
                                                        if (CheckStringResultLen(aResBuf, aStr.getLength()))
                                                            aResBuf.append(aStr);
                                                    }
                                                    aResArray.push_back(true);
                                                }
                                            }
                                        }
                                    }
                                }
                                break;
                                default:
                                    PushIllegalArgument();
                                    return;
                            }
                        }
                    }
                    PushString(aResBuf.makeStringAndClear());
                };
                const auto pushLegacyBahtText = [&]() {
                    warnTextUtilityDispatch(u"BAHTTEXT");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 1))
                        return;
                    double fValue = GetDouble();
                    if (nGlobalError != FormulaError::NONE)
                    {
                        PushError(nGlobalError);
                        return;
                    }
                    OString number = rtl::math::doubleToString(
                        std::abs(fValue), rtl_math_StringFormat_F, 2, '.');
                    const sal_Int32 dotPos = number.getLength() - 3;
                    std::string_view sBaht(number.subView(0, dotPos)),
                        sSatang(number.subView(dotPos + 1));
                    bool noBaht = sBaht == "0", noSatang = sSatang == "00";
                    if (noBaht && noSatang)
                        return PushString(OUString::Concat(TH_0) + TH_BAHT + TH_DOT0);

                    OUStringBuffer aText;
                    if (fValue < 0.0)
                        aText.append(TH_MINUS);
                    if (!noBaht)
                    {
                        size_t blocksize = sBaht.size() % 6;
                        if (blocksize == 0)
                            blocksize = 6;
                        while (!sBaht.empty())
                        {
                            lclAppendBlock(aText, sBaht.substr(0, blocksize));
                            sBaht.remove_prefix(blocksize);
                            blocksize = 6;
                            if (!sBaht.empty())
                                aText.append(TH_1E6);
                        }
                        aText.append(TH_BAHT);
                    }
                    if (noSatang)
                        aText.append(TH_DOT0);
                    else
                    {
                        lclAppendBlock(aText, sSatang);
                        aText.append(TH_SATANG);
                    }
                    PushString(aText.makeStringAndClear());
                };
                const auto pushLegacyExact = [&]() {
                    warnTextUtilityDispatch(u"EXACT");
                    nFuncFmtType = SvNumFormatType::LOGICAL;
                    if (MustHaveParamCount(GetByte(), 2))
                    {
                        svl::SharedString s1 = GetString();
                        svl::SharedString s2 = GetString();
                        PushInt(int(s1 == s2));
                    }
                };
                const auto pushLegacyLeftRight = [&](bool bRight) {
                    warnTextUtilityDispatch(bRight ? u"RIGHT" : u"LEFT");
                    sal_uInt8 nParamCount = GetByte();
                    if (!MustHaveParamCount(nParamCount, 1, 2))
                        return;

                    sal_Int32 n = 1;
                    if (nParamCount == 2)
                    {
                        n = GetStringPositionArgument();
                        if (n < 0)
                        {
                            PushIllegalArgument();
                            return;
                        }
                    }

                    OUString aStr = GetString().getString();
                    if (!bRight)
                    {
                        sal_Int32 nIdx = 0;
                        sal_Int32 nCnt = 0;
                        while (nIdx < aStr.getLength() && n > nCnt++)
                            aStr.iterateCodePoints(&nIdx);
                        PushString(aStr.copy(0, nIdx));
                        return;
                    }

                    const sal_Int32 nLen = aStr.getLength();
                    if (nLen <= n)
                    {
                        PushString(aStr);
                        return;
                    }

                    sal_Int32 nIdx = nLen;
                    sal_Int32 nCnt = 0;
                    while (nIdx > 0 && n > nCnt)
                    {
                        aStr.iterateCodePoints(&nIdx, -1);
                        ++nCnt;
                    }
                    PushString(aStr.copy(nIdx, nLen - nIdx));
                };
                const auto pushLegacyConcatMs = [&]() {
                    warnTextUtilityDispatch(u"CONCAT");
                    OUStringBuffer aResBuf;
                    short nParamCount = GetByte();
                    ReverseStack(nParamCount);

                    size_t nRefInList = 0;
                    while (nParamCount-- > 0 && nGlobalError == FormulaError::NONE)
                    {
                        switch (GetStackType())
                        {
                            case svString:
                            case svDouble:
                            {
                                OUString aStr = GetString().getString();
                                if (CheckStringResultLen(aResBuf, aStr.getLength()))
                                    aResBuf.append(aStr);
                            }
                            break;
                            case svSingleRef:
                            {
                                ScAddress aAdr;
                                PopSingleRef(aAdr);
                                if (nGlobalError != FormulaError::NONE)
                                    break;
                                ScRefCellValue aCell(mrDoc, aAdr);
                                if (!aCell.hasEmptyValue())
                                {
                                    svl::SharedString aSS;
                                    GetCellString(aSS, aCell);
                                    const OUString& rStr = aSS.getString();
                                    if (CheckStringResultLen(aResBuf, rStr.getLength()))
                                        aResBuf.append(rStr);
                                }
                            }
                            break;
                            case svDoubleRef:
                            case svRefList:
                            {
                                ScRange aRange;
                                PopDoubleRef(aRange, nParamCount, nRefInList);
                                if (nGlobalError != FormulaError::NONE)
                                    break;
                                SCCOL nCol1, nCol2;
                                SCROW nRow1, nRow2;
                                SCTAB nTab1, nTab2;
                                aRange.GetVars(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                                if (nTab1 != nTab2)
                                {
                                    SetError(FormulaError::IllegalParameter);
                                    break;
                                }
                                PutInOrder(nRow1, nRow2);
                                PutInOrder(nCol1, nCol2);
                                ScAddress aAdr;
                                aAdr.SetTab(nTab1);
                                for (SCROW nRow = nRow1; nRow <= nRow2; ++nRow)
                                {
                                    for (SCCOL nCol = nCol1; nCol <= nCol2; ++nCol)
                                    {
                                        aAdr.SetRow(nRow);
                                        aAdr.SetCol(nCol);
                                        ScRefCellValue aCell(mrDoc, aAdr);
                                        if (!aCell.hasEmptyValue())
                                        {
                                            svl::SharedString aSS;
                                            GetCellString(aSS, aCell);
                                            const OUString& rStr = aSS.getString();
                                            if (CheckStringResultLen(aResBuf, rStr.getLength()))
                                                aResBuf.append(rStr);
                                        }
                                    }
                                }
                            }
                            break;
                            case svMatrix:
                            case svExternalSingleRef:
                            case svExternalDoubleRef:
                            {
                                ScMatrixRef pMat = GetMatrix();
                                if (!pMat)
                                    break;

                                SCSIZE nC = 0;
                                SCSIZE nR = 0;
                                pMat->GetDimensions(nC, nR);
                                if (nC == 0 || nR == 0)
                                {
                                    SetError(FormulaError::IllegalArgument);
                                    break;
                                }

                                for (SCSIZE k = 0; k < nR; ++k)
                                {
                                    for (SCSIZE j = 0; j < nC; ++j)
                                    {
                                        if (pMat->IsStringOrEmpty(j, k))
                                        {
                                            OUString aStr = pMat->GetString(j, k).getString();
                                            if (CheckStringResultLen(aResBuf, aStr.getLength()))
                                                aResBuf.append(aStr);
                                        }
                                        else if (pMat->IsValue(j, k))
                                        {
                                            OUString aStr
                                                = pMat->GetString(mrContext, j, k).getString();
                                            if (CheckStringResultLen(aResBuf, aStr.getLength()))
                                                aResBuf.append(aStr);
                                        }
                                    }
                                }
                            }
                            break;
                            default:
                                PopError();
                                SetError(FormulaError::IllegalArgument);
                                break;
                        }
                    }
                    PushString(aResBuf.makeStringAndClear());
                };
                const auto warnInformationPredicateDispatch
                    = [&](std::u16string_view rFunctionName) {
                          warnIfLegacyDispatchReached(
                              "family-local default-on", rFunctionName,
                              [](std::u16string_view rFormula) {
                                  return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                              },
                              "family-local default-on information predicate reached "
                              "ScInterpreter");
                      };
                const auto pushLegacyIsEmpty = [&]() {
                    warnInformationPredicateDispatch(u"ISBLANK");
                    short nRes = 0;
                    nFuncFmtType = SvNumFormatType::LOGICAL;
                    switch (GetRawStackType())
                    {
                        case svEmptyCell:
                        {
                            FormulaConstTokenRef p = PopToken();
                            if (!static_cast<const ScEmptyCellToken*>(p.get())->IsInherited())
                                nRes = 1;
                        }
                        break;
                        case svDoubleRef:
                        case svSingleRef:
                        {
                            ScAddress aAdr;
                            if (!PopDoubleRefOrSingleRef(aAdr))
                                break;
                            ScRefCellValue aCell(mrDoc, aAdr);
                            if (aCell.getType() == CELLTYPE_NONE)
                                nRes = 1;
                        }
                        break;
                        case svExternalSingleRef:
                        case svExternalDoubleRef:
                        case svMatrix:
                        {
                            ScMatrixRef pMat = GetMatrix();
                            if (!pMat)
                                break;
                            if (!pJumpMatrix)
                                nRes = pMat->IsEmptyCell(0, 0) ? 1 : 0;
                            else
                            {
                                SCSIZE nCols, nRows, nC, nR;
                                pMat->GetDimensions(nCols, nRows);
                                pJumpMatrix->GetPos(nC, nR);
                                if (nC < nCols && nR < nRows)
                                    nRes = pMat->IsEmptyCell(nC, nR) ? 1 : 0;
                            }
                        }
                        break;
                        default:
                            Pop();
                    }
                    nGlobalError = FormulaError::NONE;
                    PushInt(nRes);
                };
                const auto pushLegacyIsString = [&](bool bInvert) {
                    warnInformationPredicateDispatch(bInvert ? u"ISNONTEXT" : u"ISTEXT");
                    PushInt(int(bInvert ? !IsString() : IsString()));
                };
                const auto pushLegacyIsLogical = [&]() {
                    warnInformationPredicateDispatch(u"ISLOGICAL");
                    bool bRes = false;
                    switch (GetStackType())
                    {
                        case svDoubleRef:
                        case svSingleRef:
                        {
                            ScAddress aAdr;
                            if (!PopDoubleRefOrSingleRef(aAdr))
                                break;

                            ScRefCellValue aCell(mrDoc, aAdr);
                            if (GetCellErrCode(aCell) == FormulaError::NONE && aCell.hasNumeric())
                            {
                                const sal_uInt32 nFormat = GetCellNumberFormat(aAdr, aCell);
                                bRes = (mrContext.NFGetType(nFormat) == SvNumFormatType::LOGICAL);
                            }
                        }
                        break;
                        case svMatrix:
                        {
                            double fVal;
                            svl::SharedString aStr;
                            const ScMatValType nMatValType = GetDoubleOrStringFromMatrix(fVal, aStr);
                            bRes = (nMatValType == ScMatValType::Boolean);
                        }
                        break;
                        default:
                            PopError();
                            if (nGlobalError == FormulaError::NONE)
                                bRes = (nCurFmtType == SvNumFormatType::LOGICAL);
                    }
                    nCurFmtType = nFuncFmtType = SvNumFormatType::LOGICAL;
                    nGlobalError = FormulaError::NONE;
                    PushInt(int(bRes));
                };
                const auto pushLegacyIsRef = [&]() {
                    warnInformationPredicateDispatch(u"ISREF");
                    nFuncFmtType = SvNumFormatType::LOGICAL;
                    bool bRes = false;
                    switch (GetStackType())
                    {
                        case svSingleRef:
                        {
                            ScAddress aAdr;
                            PopSingleRef(aAdr);
                            if (nGlobalError == FormulaError::NONE)
                                bRes = true;
                        }
                        break;
                        case svDoubleRef:
                        {
                            ScRange aRange;
                            PopDoubleRef(aRange);
                            if (nGlobalError == FormulaError::NONE)
                                bRes = true;
                        }
                        break;
                        case svRefList:
                        {
                            FormulaConstTokenRef x = PopToken();
                            if (nGlobalError == FormulaError::NONE)
                                bRes = !x->GetRefList()->empty();
                        }
                        break;
                        case svExternalSingleRef:
                        {
                            ScExternalRefCache::TokenRef pToken;
                            PopExternalSingleRef(pToken);
                            if (nGlobalError == FormulaError::NONE)
                                bRes = true;
                        }
                        break;
                        case svExternalDoubleRef:
                        {
                            ScExternalRefCache::TokenArrayRef pArray;
                            PopExternalDoubleRef(pArray);
                            if (nGlobalError == FormulaError::NONE)
                                bRes = true;
                        }
                        break;
                        default:
                            Pop();
                    }
                    nGlobalError = FormulaError::NONE;
                    PushInt(int(bRes));
                };
                const auto pushLegacyIsValue = [&]() {
                    warnInformationPredicateDispatch(u"ISNUMBER");
                    nFuncFmtType = SvNumFormatType::LOGICAL;
                    bool bRes = false;
                    switch (GetRawStackType())
                    {
                        case svDouble:
                            Pop();
                            bRes = true;
                            break;
                        case svDoubleRef:
                        case svSingleRef:
                        {
                            ScAddress aAdr;
                            if (!PopDoubleRefOrSingleRef(aAdr))
                                break;
                            ScRefCellValue aCell(mrDoc, aAdr);
                            if (GetCellErrCode(aCell) == FormulaError::NONE)
                            {
                                switch (aCell.getType())
                                {
                                    case CELLTYPE_VALUE:
                                        bRes = true;
                                        break;
                                    case CELLTYPE_FORMULA:
                                        bRes = (aCell.getFormula()->IsValue()
                                                && !aCell.getFormula()->IsEmpty());
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                        break;
                        case svExternalSingleRef:
                        {
                            ScExternalRefCache::TokenRef pToken;
                            PopExternalSingleRef(pToken);
                            if (nGlobalError == FormulaError::NONE && pToken->GetType() == svDouble)
                                bRes = true;
                        }
                        break;
                        case svExternalDoubleRef:
                        case svMatrix:
                        {
                            ScMatrixRef pMat = GetMatrix();
                            if (!pMat)
                                break;
                            if (!pJumpMatrix)
                            {
                                if (pMat->GetErrorIfNotString(0, 0) == FormulaError::NONE)
                                    bRes = pMat->IsValue(0, 0);
                            }
                            else
                            {
                                SCSIZE nCols, nRows, nC, nR;
                                pMat->GetDimensions(nCols, nRows);
                                pJumpMatrix->GetPos(nC, nR);
                                if (nC < nCols && nR < nRows
                                    && pMat->GetErrorIfNotString(nC, nR) == FormulaError::NONE)
                                {
                                    bRes = pMat->IsValue(nC, nR);
                                }
                            }
                        }
                        break;
                        default:
                            Pop();
                    }
                    nGlobalError = FormulaError::NONE;
                    PushInt(int(bRes));
                };
                const auto pushLegacyIsFormula = [&]() {
                    warnInformationPredicateDispatch(u"ISFORMULA");
                    nFuncFmtType = SvNumFormatType::LOGICAL;
                    bool bRes = false;
                    switch (GetStackType())
                    {
                        case svDoubleRef:
                            if (IsInArrayContext())
                            {
                                SCCOL nCol1, nCol2;
                                SCROW nRow1, nRow2;
                                SCTAB nTab1, nTab2;
                                PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                                if (nGlobalError != FormulaError::NONE)
                                {
                                    PushError(nGlobalError);
                                    return;
                                }
                                if (nTab1 != nTab2)
                                {
                                    PushIllegalArgument();
                                    return;
                                }

                                const auto aMatrixResult = seformulainspect::buildIsFormulaMatrix(
                                    mrDoc, mrContext,
                                    ScRange(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2),
                                    [this](SCSIZE nColumns, SCSIZE nRows) {
                                        return GetNewMat(nColumns, nRows, true);
                                    });
                                if (aMatrixResult.meFailure
                                    == seformulainspect::MatrixInspectionFailure::IllegalArgument)
                                {
                                    PushIllegalArgument();
                                    return;
                                }
                                if (aMatrixResult.meFailure
                                    == seformulainspect::MatrixInspectionFailure::MatrixSize)
                                {
                                    PushError(FormulaError::MatrixSize);
                                    return;
                                }

                                PushMatrix(aMatrixResult.mpMatrix);
                                return;
                            }
                            [[fallthrough]];
                        case svSingleRef:
                        {
                            ScAddress aAdr;
                            if (!PopDoubleRefOrSingleRef(aAdr))
                                break;
                            bRes = seformulainspect::isFormulaCell(mrDoc, mrContext, aAdr);
                        }
                        break;
                        default:
                            Pop();
                    }
                    nGlobalError = FormulaError::NONE;
                    PushInt(int(bRes));
                };
                const auto pushLegacyIsNA = [&]() {
                    warnInformationPredicateDispatch(u"ISNA");
                    nFuncFmtType = SvNumFormatType::LOGICAL;
                    bool bRes = false;
                    switch (GetStackType())
                    {
                        case svDoubleRef:
                        case svSingleRef:
                        {
                            ScAddress aAdr;
                            const bool bOk = PopDoubleRefOrSingleRef(aAdr);
                            if (nGlobalError == FormulaError::NotAvailable)
                                bRes = true;
                            else if (bOk)
                            {
                                ScRefCellValue aCell(mrDoc, aAdr);
                                bRes = (GetCellErrCode(aCell) == FormulaError::NotAvailable);
                            }
                        }
                        break;
                        case svExternalSingleRef:
                        {
                            ScExternalRefCache::TokenRef pToken;
                            PopExternalSingleRef(pToken);
                            if (nGlobalError == FormulaError::NotAvailable
                                || (pToken && pToken->GetType() == svError
                                    && pToken->GetError() == FormulaError::NotAvailable))
                            {
                                bRes = true;
                            }
                        }
                        break;
                        case svExternalDoubleRef:
                        case svMatrix:
                        {
                            ScMatrixRef pMat = GetMatrix();
                            if (!pMat)
                                break;
                            if (!pJumpMatrix)
                                bRes = (pMat->GetErrorIfNotString(0, 0)
                                        == FormulaError::NotAvailable);
                            else
                            {
                                SCSIZE nCols, nRows, nC, nR;
                                pMat->GetDimensions(nCols, nRows);
                                pJumpMatrix->GetPos(nC, nR);
                                if (nC < nCols && nR < nRows)
                                {
                                    bRes = (pMat->GetErrorIfNotString(nC, nR)
                                            == FormulaError::NotAvailable);
                                }
                            }
                        }
                        break;
                        default:
                            PopError();
                            if (nGlobalError == FormulaError::NotAvailable)
                                bRes = true;
                    }
                    nGlobalError = FormulaError::NONE;
                    PushInt(int(bRes));
                };
                const auto pushLegacyIsErrLike = [&](std::u16string_view rFunctionName,
                                                     bool bTreatNAAsError) {
                    warnInformationPredicateDispatch(rFunctionName);
                    nFuncFmtType = SvNumFormatType::LOGICAL;
                    bool bRes = false;
                    switch (GetStackType())
                    {
                        case svDoubleRef:
                        case svSingleRef:
                        {
                            ScAddress aAdr;
                            const bool bOk = PopDoubleRefOrSingleRef(aAdr);
                            if (!bOk || (nGlobalError != FormulaError::NONE
                                         && (bTreatNAAsError
                                                 || nGlobalError != FormulaError::NotAvailable)))
                            {
                                bRes = true;
                            }
                            else
                            {
                                ScRefCellValue aCell(mrDoc, aAdr);
                                const FormulaError nErr = GetCellErrCode(aCell);
                                bRes = bTreatNAAsError ? (nErr != FormulaError::NONE)
                                                       : (nErr != FormulaError::NONE
                                                          && nErr != FormulaError::NotAvailable);
                            }
                        }
                        break;
                        case svExternalSingleRef:
                        {
                            ScExternalRefCache::TokenRef pToken;
                            PopExternalSingleRef(pToken);
                            if (bTreatNAAsError)
                            {
                                bRes = (nGlobalError != FormulaError::NONE
                                        || pToken->GetType() == svError);
                            }
                            else if ((nGlobalError != FormulaError::NONE
                                      && nGlobalError != FormulaError::NotAvailable)
                                     || !pToken
                                     || (pToken->GetType() == svError
                                         && pToken->GetError() != FormulaError::NotAvailable))
                            {
                                bRes = true;
                            }
                        }
                        break;
                        case svExternalDoubleRef:
                        case svMatrix:
                        {
                            ScMatrixRef pMat = GetMatrix();
                            if (nGlobalError != FormulaError::NONE || !pMat)
                            {
                                bRes = bTreatNAAsError
                                           ? (nGlobalError != FormulaError::NONE || !pMat)
                                           : ((nGlobalError != FormulaError::NONE
                                               && nGlobalError != FormulaError::NotAvailable)
                                              || !pMat);
                            }
                            else
                            {
                                const auto getErr = [&](SCSIZE nC, SCSIZE nR) {
                                    return pMat->GetErrorIfNotString(nC, nR);
                                };
                                FormulaError nErr = FormulaError::NONE;
                                if (!pJumpMatrix)
                                    nErr = getErr(0, 0);
                                else
                                {
                                    SCSIZE nCols, nRows, nC, nR;
                                    pMat->GetDimensions(nCols, nRows);
                                    pJumpMatrix->GetPos(nC, nR);
                                    if (nC < nCols && nR < nRows)
                                        nErr = getErr(nC, nR);
                                }
                                bRes = bTreatNAAsError ? (nErr != FormulaError::NONE)
                                                       : (nErr != FormulaError::NONE
                                                          && nErr != FormulaError::NotAvailable);
                            }
                        }
                        break;
                        default:
                            PopError();
                            if (bTreatNAAsError)
                                bRes = (nGlobalError != FormulaError::NONE);
                            else if (nGlobalError != FormulaError::NONE
                                     && nGlobalError != FormulaError::NotAvailable)
                            {
                                bRes = true;
                            }
                    }
                    nGlobalError = FormulaError::NONE;
                    PushInt(int(bRes));
                };
                const auto pushLegacyIsEvenOdd = [&](bool bOdd) {
                    warnInformationPredicateDispatch(bOdd ? u"ISODD" : u"ISEVEN");
                    PushInt(int(bOdd ? !IsEven() : IsEven()));
                };
                const auto warnLogicalDispatch = [&](std::u16string_view rFunctionName) {
                    warnIfLegacyDispatchReached(
                        "family-local default-on", rFunctionName,
                        [](std::u16string_view rFormula) {
                            return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                        },
                        "family-local default-on logical slice reached ScInterpreter");
                };
                const auto pushLegacyLogicalFold = [&](std::u16string_view rFunctionName,
                                                       spreadsheetengine::compat::libreoffice::
                                                           interpreterdispatch::LogicalFoldMode
                                                               eMode) {
                    warnLogicalDispatch(rFunctionName);
                    ScLogicalFoldOp(eMode);
                };
                const auto pushLegacyNot = [&]() {
                    warnLogicalDispatch(u"NOT");
                    nFuncFmtType = SvNumFormatType::LOGICAL;
                    ScUnaryMatrixOrScalarOp(
                        spreadsheetengine::compat::libreoffice::interpreterdispatch::
                            UnaryMatrixScalarMode::LogicalNot);
                };
                const auto warnConditionalDispatch = [&](std::u16string_view rFunctionName) {
                    warnIfLegacyDispatchReached(
                        "family-local default-on", rFunctionName,
                        [](std::u16string_view rFormula) {
                            return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                        },
                        "family-local default-on conditional slice reached ScInterpreter");
                };
                // Batch 1 first admission: scalar IF condition through the
                // engine-native planIfBranch. Reference and matrix operands
                // defer to the legacy ScIfJump path, which still owns the
                // matrix-frame JumpMatrix protocol until Batch 4 lands.
                const auto tryPlanEngineIfJump = [&]() -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnControlFlowEngineAttemptedCount);

                    const short* pJump = pCur->GetJump();
                    const short nJumpCount = pJump[0];
                    if (!sp || nJumpCount < 2 || nJumpCount > 3)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }

                    const FormulaToken* pConditionToken = pStack[sp - 1];
                    if (!pConditionToken)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }

                    std::optional<serpn::RpnValue> oCondition;
                    switch (pConditionToken->GetType())
                    {
                        case svDouble:
                            oCondition = serpn::RpnValue::number(pConditionToken->GetDouble());
                            break;
                        case svError:
                            oCondition = serpn::RpnValue::error(
                                selibreoffice::toApiError(pConditionToken->GetError()));
                            break;
                        case svEmptyCell:
                        case svMissing:
                            oCondition = serpn::RpnValue::empty();
                            break;
                        default:
                            // Reference, matrix, external-ref, string,
                            // jump-matrix tokens all defer to legacy so we
                            // do not guess at coercion or matrix-broadcast
                            // semantics here.
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnControlFlowEngineDeclinedCount);
                            return false;
                    }

                    const std::optional<std::size_t> oThenSlot
                        = nJumpCount >= 2 ? std::optional<std::size_t>(0) : std::nullopt;
                    const std::optional<std::size_t> oElseSlot
                        = nJumpCount == 3 ? std::optional<std::size_t>(1) : std::nullopt;

                    const auto aPlan = serpn::planIfBranch(*oCondition, oThenSlot, oElseSlot);
                    if (aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }
                    if (!aPlan)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }

                    Pop();
                    nGlobalError = FormulaError::NONE;

                    switch (aPlan.maValue.meDirective)
                    {
                        case serpn::BranchDirective::TakeSlot:
                            aCode.Jump(
                                pJump[aPlan.maValue.mnSlot + 1], pJump[nJumpCount]);
                            break;
                        case serpn::BranchDirective::PropagateError:
                            PushError(selibreoffice::toFormulaError(aPlan.maValue.meError));
                            aCode.Jump(pJump[nJumpCount], pJump[nJumpCount]);
                            break;
                        case serpn::BranchDirective::ReturnSyntheticBoolean:
                            nFuncFmtType = SvNumFormatType::LOGICAL;
                            PushInt(aPlan.maValue.mbSyntheticBool ? 1 : 0);
                            aCode.Jump(pJump[nJumpCount], pJump[nJumpCount]);
                            break;
                        default:
                            // ReturnNotAvailable / ReturnParameterExpected /
                            // KeepPrimaryValue / EvaluateAlternate are not
                            // produced by planIfBranch; treat defensively as
                            // decline so legacy re-runs unchanged.
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnControlFlowEngineDeclinedCount);
                            return false;
                    }

                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnControlFlowEngineSucceededCount);
                    return true;
                };
                // Batch 2 second admission: span-count COLUMNS / ROWS /
                // SHEETS. Covers single-argument svSingleRef and svDoubleRef;
                // anything else (multi-arg, matrices, external refs) defers
                // to legacy.
                const auto tryPlanEngineSpanCount
                    = [&](serpn::SpanCountKind eKind) -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    if (nParamCount != 1 || !sp)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }
                    const FormulaToken* pTop = pStack[sp - 1];
                    if (!pTop)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    spreadsheetengine::api::ResolvedReference aResolved;
                    if (pTop->GetType() == svSingleRef)
                    {
                        ScAddress aAdr;
                        PopSingleRef(aAdr);
                        if (nGlobalError != FormulaError::NONE)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnReferenceEngineDeclinedCount);
                            return false;
                        }
                        aResolved.maRange.maStart = {
                            static_cast<spreadsheetengine::api::SheetId>(aAdr.Tab()),
                            static_cast<spreadsheetengine::api::ColumnIndex>(aAdr.Col()),
                            static_cast<spreadsheetengine::api::RowIndex>(aAdr.Row())
                        };
                        aResolved.maRange.maEnd = aResolved.maRange.maStart;
                    }
                    else if (pTop->GetType() == svDoubleRef)
                    {
                        SCCOL nCol1, nCol2;
                        SCROW nRow1, nRow2;
                        SCTAB nTab1, nTab2;
                        PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                        if (nGlobalError != FormulaError::NONE)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnReferenceEngineDeclinedCount);
                            return false;
                        }
                        aResolved.maRange.maStart = {
                            static_cast<spreadsheetengine::api::SheetId>(nTab1),
                            static_cast<spreadsheetengine::api::ColumnIndex>(nCol1),
                            static_cast<spreadsheetengine::api::RowIndex>(nRow1)
                        };
                        aResolved.maRange.maEnd = {
                            static_cast<spreadsheetengine::api::SheetId>(nTab2),
                            static_cast<spreadsheetengine::api::ColumnIndex>(nCol2),
                            static_cast<spreadsheetengine::api::RowIndex>(nRow2)
                        };
                    }
                    else
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    const auto aPlan = serpn::planSpanCount(
                        serpn::RpnValue::reference(aResolved), eKind);
                    if (!aPlan
                        || aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineSucceededCount);
                    PushDouble(aPlan.maValue);
                    return true;
                };

                // Batch 3 admissions: COUNTIF / SUMIF / AVERAGEIF route
                // through planSingleCriterionAggregate when the criteria
                // range and (for SUMIF/AVERAGEIF) target range are scalar
                // svDoubleRefs, the criterion is a scalar svDouble or
                // svString, and all ranges live on a single sheet.
                // External refs, matrices, RefList, and multi-sheet
                // operands defer to legacy.
                const auto buildCriteriaRangeInput
                    = [&](const FormulaToken* pRangeTok,
                          sequery::CriteriaAggregateInput& rInput) -> bool {
                    if (!pRangeTok || pRangeTok->GetType() != svDoubleRef)
                        return false;
                    const ScComplexRefData& rRef = *pRangeTok->GetDoubleRef();
                    const ScRange aAbs = rRef.toAbs(mrDoc, aPos);
                    if (aAbs.aStart.Tab() != aAbs.aEnd.Tab())
                        return false;
                    rInput.mbScalar = false;
                    rInput.maReference.maRange.maStart = {
                        static_cast<spreadsheetengine::api::SheetId>(aAbs.aStart.Tab()),
                        static_cast<spreadsheetengine::api::ColumnIndex>(aAbs.aStart.Col()),
                        static_cast<spreadsheetengine::api::RowIndex>(aAbs.aStart.Row())
                    };
                    rInput.maReference.maRange.maEnd = {
                        static_cast<spreadsheetengine::api::SheetId>(aAbs.aEnd.Tab()),
                        static_cast<spreadsheetengine::api::ColumnIndex>(aAbs.aEnd.Col()),
                        static_cast<spreadsheetengine::api::RowIndex>(aAbs.aEnd.Row())
                    };
                    rInput.mnColumns
                        = static_cast<spreadsheetengine::api::MatrixSize>(
                            aAbs.aEnd.Col() - aAbs.aStart.Col() + 1);
                    rInput.mnRows
                        = static_cast<spreadsheetengine::api::MatrixSize>(
                            aAbs.aEnd.Row() - aAbs.aStart.Row() + 1);
                    return true;
                };

                const auto tryPlanEngineSingleCriterionAggregate
                    = [&](sequery::CriteriaAggregateKind eKind,
                          bool bWithTargetRange) -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnCriteriaEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    const sal_uInt8 nExpectedCount
                        = bWithTargetRange ? 3 : 2;
                    // SUMIF / AVERAGEIF also accept 2-arg form (aggregate
                    // over the criteria range itself).
                    const bool bHasTargetArg
                        = bWithTargetRange && nParamCount == 3;
                    if ((nParamCount != nExpectedCount && nParamCount != 2)
                        || sp < nParamCount)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    const FormulaToken* pTargetTok
                        = bHasTargetArg ? pStack[sp - 1] : nullptr;
                    const FormulaToken* pCriterionTok
                        = pStack[sp - (bHasTargetArg ? 2 : 1)];
                    const FormulaToken* pCriteriaRangeTok
                        = pStack[sp - nParamCount];

                    if (!pCriterionTok || !pCriteriaRangeTok
                        || pCriteriaRangeTok->GetType() != svDoubleRef
                        || (bHasTargetArg && pTargetTok
                            && pTargetTok->GetType() != svDoubleRef))
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    std::optional<serpn::RpnValue> oCriterion;
                    switch (pCriterionTok->GetType())
                    {
                        case svDouble:
                            oCriterion = serpn::RpnValue::number(
                                pCriterionTok->GetDouble());
                            break;
                        case svString:
                            oCriterion = serpn::RpnValue::text(
                                pCriterionTok->GetString().getString());
                            break;
                        default:
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnCriteriaEngineDeclinedCount);
                            return false;
                    }

                    const auto aPredicateResult = serpn::buildCriteriaPredicate(
                        *oCriterion, sedatetime::parseStandaloneNumberText,
                        secoercion::parseAsciiDouble);
                    if (!aPredicateResult
                        || aPredicateResult.meReadiness
                               != serpn::RpnCoercionReadiness::Ready)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    sequery::CriteriaAggregateInput aCriteriaInput;
                    if (!buildCriteriaRangeInput(pCriteriaRangeTok, aCriteriaInput))
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    std::optional<sequery::CriteriaAggregateInput> oTargetInput;
                    if (bHasTargetArg)
                    {
                        sequery::CriteriaAggregateInput aTargetInput;
                        if (!buildCriteriaRangeInput(pTargetTok, aTargetInput))
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnCriteriaEngineDeclinedCount);
                            return false;
                        }
                        oTargetInput = aTargetInput;
                    }

                    serpn::SingleCriterionAggregateRequest aRequest;
                    aRequest.maCriteriaRange = aCriteriaInput;
                    aRequest.maPredicate = aPredicateResult.maValue;
                    aRequest.moTargetRange = oTargetInput;
                    aRequest.meKind = eKind;
                    aRequest.meSearchType
                        = seitee::detail::searchTypeFromDocument(mrDoc);
                    aRequest.mbMatchWholeCell = true;

                    const seitee::detail::CriteriaAggregateMaterializer aMaterializer(
                        mrDoc, mrContext);
                    const auto aResult = serpn::planSingleCriterionAggregate(
                        aMaterializer, aRequest);
                    if (!aResult
                        || aResult.meReadiness != serpn::RpnCoercionReadiness::Ready)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    // Commit: drop operands and push result.
                    sp -= nParamCount;
                    nGlobalError = FormulaError::NONE;
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnCriteriaEngineSucceededCount);

                    switch (aResult.maValue.meKind)
                    {
                        case spreadsheetengine::api::CellValueKind::Number:
                            PushDouble(aResult.maValue.mfNumber);
                            break;
                        case spreadsheetengine::api::CellValueKind::Error:
                            PushError(selibreoffice::toFormulaError(
                                aResult.maValue.meError));
                            break;
                        default:
                            // Numeric aggregate produced a non-number;
                            // treat as error.
                            PushError(FormulaError::IllegalArgument);
                            break;
                    }
                    return true;
                };

                const auto tryPlanEngineCountIf = [&]() -> bool {
                    return tryPlanEngineSingleCriterionAggregate(
                        sequery::CriteriaAggregateKind::Count, false);
                };

                // Batch 4 matrix admissions. The pure-scalar-input
                // constructors (MUNIT / MSEQUENCE) demonstrate the
                // RpnMatrix substrate end-to-end without host-side
                // range materialization; TRANSPOSE additionally
                // exercises the matrix-consuming path by accepting an
                // in-memory svMatrix token. Reference-consuming matrix
                // opcodes (MDETERM / MINVERSE / MMULT / the SUMPRODUCT
                // family) still need the full reference-to-matrix
                // materialization contract and are deferred.
                const auto convertMatrixOperandToMatrixRef
                    = [&](const serpn::MatrixOperand& rOperand) -> ScMatrixRef {
                    if (rOperand.isEmpty())
                        return nullptr;
                    const SCSIZE nCols
                        = static_cast<SCSIZE>(rOperand.maDimensions.mnColumns);
                    const SCSIZE nRows
                        = static_cast<SCSIZE>(rOperand.maDimensions.mnRows);
                    if (!ScMatrix::IsSizeAllocatable(nCols, nRows))
                        return nullptr;
                    ScMatrixRef pResult = GetNewMat(nCols, nRows, /*bEmpty*/true);
                    if (!pResult)
                        return nullptr;
                    for (SCSIZE r = 0; r < nRows; ++r)
                    {
                        for (SCSIZE c = 0; c < nCols; ++c)
                        {
                            const auto& rCell
                                = rOperand.maValues[static_cast<std::size_t>(r) * nCols + c];
                            switch (rCell.meKind)
                            {
                                case spreadsheetengine::api::CellValueKind::Number:
                                case spreadsheetengine::api::CellValueKind::Boolean:
                                    pResult->PutDouble(rCell.mfNumber, c, r);
                                    break;
                                case spreadsheetengine::api::CellValueKind::Text:
                                    pResult->PutString(
                                        mrDoc.GetSharedStringPool().intern(
                                            selibreoffice::toLibreOfficeString(rCell.maString)),
                                        c, r);
                                    break;
                                case spreadsheetengine::api::CellValueKind::Error:
                                    pResult->PutError(
                                        selibreoffice::toFormulaError(rCell.meError), c, r);
                                    break;
                                case spreadsheetengine::api::CellValueKind::Empty:
                                    pResult->PutEmpty(c, r);
                                    break;
                            }
                        }
                    }
                    return pResult;
                };

                const auto convertMatrixRefToMatrixOperand
                    = [&](const ScMatrix& rMat) -> std::optional<serpn::MatrixOperand> {
                    SCSIZE nCols = 0;
                    SCSIZE nRows = 0;
                    rMat.GetDimensions(nCols, nRows);
                    if (nCols == 0 || nRows == 0)
                        return std::nullopt;
                    serpn::MatrixOperand aOperand;
                    aOperand.maDimensions.mnColumns
                        = static_cast<spreadsheetengine::api::MatrixSize>(nCols);
                    aOperand.maDimensions.mnRows
                        = static_cast<spreadsheetengine::api::MatrixSize>(nRows);
                    aOperand.maValues.reserve(static_cast<std::size_t>(nRows) * nCols);
                    for (SCSIZE r = 0; r < nRows; ++r)
                    {
                        for (SCSIZE c = 0; c < nCols; ++c)
                        {
                            if (rMat.IsStringOrEmpty(c, r))
                            {
                                if (rMat.IsEmpty(c, r))
                                {
                                    aOperand.maValues.push_back(
                                        spreadsheetengine::api::CellValue::empty());
                                }
                                else
                                {
                                    aOperand.maValues.push_back(
                                        spreadsheetengine::api::CellValue::text(
                                            selibreoffice::toApiString(
                                                rMat.GetString(c, r).getString())));
                                }
                                continue;
                            }
                            const FormulaError eErr = rMat.GetErrorIfNotString(c, r);
                            if (eErr != FormulaError::NONE)
                            {
                                aOperand.maValues.push_back(
                                    spreadsheetengine::api::CellValue::error(
                                        selibreoffice::toApiError(eErr)));
                                continue;
                            }
                            const double fValue = rMat.GetDouble(c, r);
                            if (rMat.IsBoolean(c, r))
                            {
                                aOperand.maValues.push_back(
                                    spreadsheetengine::api::CellValue::boolean(fValue != 0.0));
                            }
                            else
                            {
                                aOperand.maValues.push_back(
                                    spreadsheetengine::api::CellValue::number(fValue));
                            }
                        }
                    }
                    return aOperand;
                };

                const auto tryPlanEngineIdentityMatrix = [&]() -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnMatrixEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    if (nParamCount != 1 || !sp)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }
                    const FormulaToken* pDimTok = pStack[sp - 1];
                    if (!pDimTok || pDimTok->GetType() != svDouble)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }
                    const double fDim = pDimTok->GetDouble();
                    if (fDim < 1.0 || fDim > 65535.0)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }
                    const auto aPlan
                        = serpn::planIdentityMatrix(static_cast<std::size_t>(fDim));
                    if (!aPlan
                        || aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }
                    ScMatrixRef pMat = convertMatrixOperandToMatrixRef(aPlan.maValue);
                    if (!pMat)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }
                    sp -= 1;
                    nGlobalError = FormulaError::NONE;
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnMatrixEngineSucceededCount);
                    PushMatrix(pMat);
                    return true;
                };

                const auto tryPlanEngineSequenceMatrix = [&]() -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnMatrixEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    if (nParamCount < 1 || nParamCount > 4 || sp < nParamCount)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }

                    // Scope fence: only admit when every argument is a
                    // scalar svDouble so the full planning path is
                    // host-free. Missing / defaulted args still reach
                    // here as svMissing, which we decline.
                    for (sal_uInt8 i = 1; i <= nParamCount; ++i)
                    {
                        const FormulaToken* pTok = pStack[sp - i];
                        if (!pTok || pTok->GetType() != svDouble)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnMatrixEngineDeclinedCount);
                            return false;
                        }
                    }

                    // Arguments pushed in order (rows, cols, start, step);
                    // stack top is the last pushed.
                    double fStep = 1.0;
                    double fStart = 1.0;
                    sal_Int32 nColumns = 1;
                    sal_Int32 nRows = 1;
                    std::size_t nIdx = 0;
                    if (nParamCount >= 4)
                        fStep = pStack[sp - (++nIdx)]->GetDouble();
                    if (nParamCount >= 3)
                        fStart = pStack[sp - (++nIdx)]->GetDouble();
                    if (nParamCount >= 2)
                    {
                        nColumns
                            = static_cast<sal_Int32>(pStack[sp - (++nIdx)]->GetDouble());
                        if (nColumns < 1)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnMatrixEngineDeclinedCount);
                            return false;
                        }
                    }
                    nRows = static_cast<sal_Int32>(pStack[sp - (++nIdx)]->GetDouble());
                    if (nRows < 1)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }

                    const auto aPlan = serpn::planSequenceMatrix(
                        static_cast<std::size_t>(nRows),
                        static_cast<std::size_t>(nColumns), fStart, fStep);
                    if (!aPlan
                        || aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }
                    ScMatrixRef pMat = convertMatrixOperandToMatrixRef(aPlan.maValue);
                    if (!pMat)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }
                    sp -= nParamCount;
                    nGlobalError = FormulaError::NONE;
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnMatrixEngineSucceededCount);
                    PushMatrix(pMat);
                    return true;
                };

                const auto tryPlanEngineTranspose = [&]() -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnMatrixEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    if (nParamCount != 1 || !sp)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }
                    // Scope fence: admit only in-memory matrix
                    // operands. Range tokens (svDoubleRef / svSingleRef
                    // / svRefList) still need the reference-to-matrix
                    // materialization contract and defer to legacy.
                    const FormulaToken* pTok = pStack[sp - 1];
                    if (!pTok || pTok->GetType() != svMatrix)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }
                    ScMatrix* pSourceMat = const_cast<FormulaToken*>(pTok)->GetMatrix();
                    if (!pSourceMat)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }
                    auto oOperand = convertMatrixRefToMatrixOperand(*pSourceMat);
                    if (!oOperand)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }
                    const auto aPlan = serpn::planTranspose(*oOperand);
                    if (!aPlan
                        || aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }
                    ScMatrixRef pResult = convertMatrixOperandToMatrixRef(aPlan.maValue);
                    if (!pResult)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnMatrixEngineDeclinedCount);
                        return false;
                    }
                    sp -= 1;
                    nGlobalError = FormulaError::NONE;
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnMatrixEngineSucceededCount);
                    PushMatrix(pResult);
                    return true;
                };

                // Batch 3 DB family admission (DSUM / DCOUNT / DAVERAGE
                // / DMAX / DMIN). The 3-arg shape is
                // (database_range, field, criteria_range). Scope fence:
                // both ranges must be single-sheet svDoubleRef; field
                // must be a scalar svDouble (1-based column index) or
                // svString (matching a database header); criteria range
                // must have a header row plus exactly one data row
                // (multi-row OR criteria defer to legacy).
                const auto tryPlanEngineDatabaseAggregate
                    = [&](sequery::CriteriaAggregateKind eKind) -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnCriteriaEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    if (nParamCount != 3 || sp < 3)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    const FormulaToken* pCriteriaTok = pStack[sp - 1];
                    const FormulaToken* pFieldTok = pStack[sp - 2];
                    const FormulaToken* pDbTok = pStack[sp - 3];

                    if (!pDbTok || !pFieldTok || !pCriteriaTok
                        || pDbTok->GetType() != svDoubleRef
                        || pCriteriaTok->GetType() != svDoubleRef)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    const ScRange aDbRange
                        = pDbTok->GetDoubleRef()->toAbs(mrDoc, aPos);
                    const ScRange aCriteriaRange
                        = pCriteriaTok->GetDoubleRef()->toAbs(mrDoc, aPos);
                    if (aDbRange.aStart.Tab() != aDbRange.aEnd.Tab()
                        || aCriteriaRange.aStart.Tab() != aCriteriaRange.aEnd.Tab())
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    const SCROW nDbRowCount
                        = aDbRange.aEnd.Row() - aDbRange.aStart.Row() + 1;
                    const SCROW nCriteriaRowCount
                        = aCriteriaRange.aEnd.Row() - aCriteriaRange.aStart.Row() + 1;
                    // Need header + at least one data row on both sides;
                    // and (scope fence) exactly one criteria data row.
                    if (nDbRowCount < 2 || nCriteriaRowCount != 2)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    // Field resolution.
                    std::optional<SCCOL> oFieldColumn;
                    if (pFieldTok->GetType() == svDouble)
                    {
                        const double fIdx = pFieldTok->GetDouble();
                        const SCCOL nDbCols
                            = aDbRange.aEnd.Col() - aDbRange.aStart.Col() + 1;
                        if (fIdx < 1.0
                            || fIdx > static_cast<double>(nDbCols))
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnCriteriaEngineDeclinedCount);
                            return false;
                        }
                        oFieldColumn = aDbRange.aStart.Col()
                                       + static_cast<SCCOL>(fIdx) - 1;
                    }
                    else if (pFieldTok->GetType() == svString)
                    {
                        const OUString aFieldName
                            = pFieldTok->GetString().getString();
                        for (SCCOL nCol = aDbRange.aStart.Col();
                             nCol <= aDbRange.aEnd.Col(); ++nCol)
                        {
                            const ScAddress aHdr(
                                nCol, aDbRange.aStart.Row(), aDbRange.aStart.Tab());
                            const auto aHdrVal = seitee::detail::readMaterializedHostCellValue(
                                mrDoc, mrContext, aHdr);
                            if (aHdrVal.meKind
                                    == spreadsheetengine::api::CellValueKind::Text
                                && aHdrVal.maString
                                       == spreadsheetengine::api::StringView(
                                           aFieldName.getStr(),
                                           aFieldName.getLength()))
                            {
                                oFieldColumn = nCol;
                                break;
                            }
                        }
                    }
                    // DCOUNT may have missing field (count all matching
                    // rows regardless of field). Defer that case.
                    if (!oFieldColumn)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    // Walk criteria columns. For each criteria column
                    // where the data cell is non-empty, find the matching
                    // database column by header text.
                    std::vector<sequery::CriteriaAggregateInput> aRanges;
                    std::vector<sequery::CriteriaPredicate> aPredicates;
                    const SCROW nCriteriaDataRow
                        = aCriteriaRange.aStart.Row() + 1;
                    for (SCCOL nCritCol = aCriteriaRange.aStart.Col();
                         nCritCol <= aCriteriaRange.aEnd.Col(); ++nCritCol)
                    {
                        const ScAddress aCritDataAddr(
                            nCritCol, nCriteriaDataRow, aCriteriaRange.aStart.Tab());
                        const auto aCritDataVal = seitee::detail::readMaterializedHostCellValue(
                            mrDoc, mrContext, aCritDataAddr);
                        if (aCritDataVal.meKind
                            == spreadsheetengine::api::CellValueKind::Empty)
                        {
                            continue;
                        }

                        const ScAddress aCritHdrAddr(
                            nCritCol, aCriteriaRange.aStart.Row(),
                            aCriteriaRange.aStart.Tab());
                        const auto aCritHdrVal = seitee::detail::readMaterializedHostCellValue(
                            mrDoc, mrContext, aCritHdrAddr);
                        if (aCritHdrVal.meKind
                            != spreadsheetengine::api::CellValueKind::Text)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnCriteriaEngineDeclinedCount);
                            return false;
                        }

                        // Find matching database column.
                        std::optional<SCCOL> oMatchedCol;
                        for (SCCOL nDbCol = aDbRange.aStart.Col();
                             nDbCol <= aDbRange.aEnd.Col(); ++nDbCol)
                        {
                            const ScAddress aDbHdrAddr(
                                nDbCol, aDbRange.aStart.Row(),
                                aDbRange.aStart.Tab());
                            const auto aDbHdrVal
                                = seitee::detail::readMaterializedHostCellValue(
                                    mrDoc, mrContext, aDbHdrAddr);
                            if (aDbHdrVal.meKind
                                    == spreadsheetengine::api::CellValueKind::Text
                                && aDbHdrVal.maString == aCritHdrVal.maString)
                            {
                                oMatchedCol = nDbCol;
                                break;
                            }
                        }
                        if (!oMatchedCol)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnCriteriaEngineDeclinedCount);
                            return false;
                        }

                        // Build predicate from criterion value.
                        serpn::RpnValue aCriterion;
                        switch (aCritDataVal.meKind)
                        {
                            case spreadsheetengine::api::CellValueKind::Number:
                                aCriterion = serpn::RpnValue::number(
                                    aCritDataVal.mfNumber);
                                break;
                            case spreadsheetengine::api::CellValueKind::Text:
                                aCriterion = serpn::RpnValue::text(
                                    aCritDataVal.maString);
                                break;
                            case spreadsheetengine::api::CellValueKind::Boolean:
                                aCriterion = serpn::RpnValue::boolean(
                                    aCritDataVal.mfNumber != 0.0);
                                break;
                            default:
                                addDispatchRuntimeStat(
                                    interpreterDispatchRuntimeStatsStore()
                                        .mnCriteriaEngineDeclinedCount);
                                return false;
                        }
                        const auto aPredicate = serpn::buildCriteriaPredicate(
                            aCriterion,
                            sedatetime::parseStandaloneNumberText,
                            secoercion::parseAsciiDouble);
                        if (!aPredicate
                            || aPredicate.meReadiness
                                   != serpn::RpnCoercionReadiness::Ready)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnCriteriaEngineDeclinedCount);
                            return false;
                        }

                        // Build CriteriaAggregateInput spanning the
                        // matched database column's data rows.
                        sequery::CriteriaAggregateInput aRangeInput;
                        aRangeInput.mbScalar = false;
                        aRangeInput.maReference.maRange.maStart = {
                            static_cast<spreadsheetengine::api::SheetId>(
                                aDbRange.aStart.Tab()),
                            static_cast<spreadsheetengine::api::ColumnIndex>(
                                *oMatchedCol),
                            static_cast<spreadsheetengine::api::RowIndex>(
                                aDbRange.aStart.Row() + 1)
                        };
                        aRangeInput.maReference.maRange.maEnd = {
                            static_cast<spreadsheetengine::api::SheetId>(
                                aDbRange.aStart.Tab()),
                            static_cast<spreadsheetengine::api::ColumnIndex>(
                                *oMatchedCol),
                            static_cast<spreadsheetengine::api::RowIndex>(
                                aDbRange.aEnd.Row())
                        };
                        aRangeInput.mnColumns = 1;
                        aRangeInput.mnRows
                            = static_cast<spreadsheetengine::api::MatrixSize>(
                                nDbRowCount - 1);

                        aRanges.push_back(aRangeInput);
                        aPredicates.push_back(aPredicate.maValue);
                    }

                    if (aRanges.empty())
                    {
                        // No criteria → all rows match. Bridge by
                        // treating the first database column as criterion
                        // range with a "non-empty" predicate equivalent.
                        // Decline for now; legacy handles this fine.
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    // Target range = field column's data rows.
                    sequery::CriteriaAggregateInput aTargetInput;
                    aTargetInput.mbScalar = false;
                    aTargetInput.maReference.maRange.maStart = {
                        static_cast<spreadsheetengine::api::SheetId>(
                            aDbRange.aStart.Tab()),
                        static_cast<spreadsheetengine::api::ColumnIndex>(
                            *oFieldColumn),
                        static_cast<spreadsheetengine::api::RowIndex>(
                            aDbRange.aStart.Row() + 1)
                    };
                    aTargetInput.maReference.maRange.maEnd = {
                        static_cast<spreadsheetengine::api::SheetId>(
                            aDbRange.aStart.Tab()),
                        static_cast<spreadsheetengine::api::ColumnIndex>(
                            *oFieldColumn),
                        static_cast<spreadsheetengine::api::RowIndex>(
                            aDbRange.aEnd.Row())
                    };
                    aTargetInput.mnColumns = 1;
                    aTargetInput.mnRows
                        = static_cast<spreadsheetengine::api::MatrixSize>(
                            nDbRowCount - 1);

                    serpn::MultiCriterionAggregateRequest aRequest;
                    aRequest.maCriteriaRanges = std::move(aRanges);
                    aRequest.maCriteria = std::move(aPredicates);
                    aRequest.moTargetRange = aTargetInput;
                    aRequest.meKind = eKind;
                    aRequest.meSearchType
                        = seitee::detail::searchTypeFromDocument(mrDoc);
                    aRequest.mbMatchWholeCell = true;

                    const seitee::detail::CriteriaAggregateMaterializer aMaterializer(
                        mrDoc, mrContext);
                    const auto aResult = serpn::planMultiCriterionAggregate(
                        aMaterializer, aRequest);
                    if (!aResult
                        || aResult.meReadiness
                               != serpn::RpnCoercionReadiness::Ready)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    sp -= nParamCount;
                    nGlobalError = FormulaError::NONE;
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnCriteriaEngineSucceededCount);

                    switch (aResult.maValue.meKind)
                    {
                        case spreadsheetengine::api::CellValueKind::Number:
                            PushDouble(aResult.maValue.mfNumber);
                            break;
                        case spreadsheetengine::api::CellValueKind::Error:
                            PushError(selibreoffice::toFormulaError(
                                aResult.maValue.meError));
                            break;
                        default:
                            PushError(FormulaError::IllegalArgument);
                            break;
                    }
                    return true;
                };

                // Batch 3 multi-criterion admissions: COUNTIFS, SUMIFS,
                // AVERAGEIFS, MINIFS_MS, MAXIFS_MS. All take N parallel
                // (criteria_range, criterion) pairs plus an optional
                // aggregation target at the bottom. The admission
                // accepts svDoubleRef ranges on a single sheet and
                // scalar svDouble or svString criteria. External refs,
                // matrices, RefList, multi-sheet, and non-scalar
                // criteria defer to legacy.
                const auto tryPlanEngineMultiCriterionAggregate
                    = [&](sequery::CriteriaAggregateKind eKind,
                          bool bWithTargetRange) -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnCriteriaEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    // COUNTIFS: even, ≥2. SUMIFS/AVERAGEIFS/MINIFS/MAXIFS:
                    // odd, ≥3 (target_range at the bottom).
                    if (bWithTargetRange)
                    {
                        if (nParamCount < 3 || (nParamCount % 2 != 1))
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnCriteriaEngineDeclinedCount);
                            return false;
                        }
                    }
                    else
                    {
                        if (nParamCount < 2 || (nParamCount % 2 != 0))
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnCriteriaEngineDeclinedCount);
                            return false;
                        }
                    }
                    if (sp < nParamCount)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    const std::size_t nPairs
                        = bWithTargetRange
                              ? static_cast<std::size_t>((nParamCount - 1) / 2)
                              : static_cast<std::size_t>(nParamCount / 2);

                    // Validate all tokens and build the inputs /
                    // predicates ahead of committing the sp decrement.
                    std::vector<sequery::CriteriaAggregateInput> aRanges;
                    std::vector<sequery::CriteriaPredicate> aCriteria;
                    aRanges.reserve(nPairs);
                    aCriteria.reserve(nPairs);

                    // Pairs are [crit_range_1, crit_1, crit_range_2,
                    // crit_2, ...] from the BOTTOM. The last pair's
                    // criterion is at sp-1, its range at sp-2.
                    const std::size_t nPairsBase
                        = bWithTargetRange
                              ? static_cast<std::size_t>(sp - nParamCount + 1)
                              : static_cast<std::size_t>(sp - nParamCount);

                    for (std::size_t i = 0; i < nPairs; ++i)
                    {
                        const FormulaToken* pRangeTok
                            = pStack[nPairsBase + 2 * i];
                        const FormulaToken* pCriterionTok
                            = pStack[nPairsBase + 2 * i + 1];
                        if (!pRangeTok || !pCriterionTok)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnCriteriaEngineDeclinedCount);
                            return false;
                        }

                        sequery::CriteriaAggregateInput aInput;
                        if (!buildCriteriaRangeInput(pRangeTok, aInput))
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnCriteriaEngineDeclinedCount);
                            return false;
                        }

                        std::optional<serpn::RpnValue> oCriterion;
                        switch (pCriterionTok->GetType())
                        {
                            case svDouble:
                                oCriterion = serpn::RpnValue::number(
                                    pCriterionTok->GetDouble());
                                break;
                            case svString:
                                oCriterion = serpn::RpnValue::text(
                                    pCriterionTok->GetString().getString());
                                break;
                            default:
                                addDispatchRuntimeStat(
                                    interpreterDispatchRuntimeStatsStore()
                                        .mnCriteriaEngineDeclinedCount);
                                return false;
                        }
                        const auto aPredicate = serpn::buildCriteriaPredicate(
                            *oCriterion,
                            sedatetime::parseStandaloneNumberText,
                            secoercion::parseAsciiDouble);
                        if (!aPredicate
                            || aPredicate.meReadiness
                                   != serpn::RpnCoercionReadiness::Ready)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnCriteriaEngineDeclinedCount);
                            return false;
                        }

                        aRanges.push_back(aInput);
                        aCriteria.push_back(aPredicate.maValue);
                    }

                    std::optional<sequery::CriteriaAggregateInput> oTarget;
                    if (bWithTargetRange)
                    {
                        const FormulaToken* pTargetTok
                            = pStack[sp - nParamCount];
                        sequery::CriteriaAggregateInput aTarget;
                        if (!buildCriteriaRangeInput(pTargetTok, aTarget))
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnCriteriaEngineDeclinedCount);
                            return false;
                        }
                        oTarget = aTarget;
                    }

                    serpn::MultiCriterionAggregateRequest aRequest;
                    aRequest.maCriteriaRanges = std::move(aRanges);
                    aRequest.maCriteria = std::move(aCriteria);
                    aRequest.moTargetRange = oTarget;
                    aRequest.meKind = eKind;
                    aRequest.meSearchType
                        = seitee::detail::searchTypeFromDocument(mrDoc);
                    aRequest.mbMatchWholeCell = true;

                    const seitee::detail::CriteriaAggregateMaterializer aMaterializer(
                        mrDoc, mrContext);
                    const auto aResult = serpn::planMultiCriterionAggregate(
                        aMaterializer, aRequest);
                    if (!aResult
                        || aResult.meReadiness
                               != serpn::RpnCoercionReadiness::Ready)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnCriteriaEngineDeclinedCount);
                        return false;
                    }

                    sp -= nParamCount;
                    nGlobalError = FormulaError::NONE;
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnCriteriaEngineSucceededCount);

                    switch (aResult.maValue.meKind)
                    {
                        case spreadsheetengine::api::CellValueKind::Number:
                            PushDouble(aResult.maValue.mfNumber);
                            break;
                        case spreadsheetengine::api::CellValueKind::Error:
                            PushError(selibreoffice::toFormulaError(
                                aResult.maValue.meError));
                            break;
                        default:
                            PushError(FormulaError::IllegalArgument);
                            break;
                    }
                    return true;
                };
                const auto tryPlanEngineSumIf = [&]() -> bool {
                    return tryPlanEngineSingleCriterionAggregate(
                        sequery::CriteriaAggregateKind::Sum, true);
                };
                const auto tryPlanEngineAverageIf = [&]() -> bool {
                    return tryPlanEngineSingleCriterionAggregate(
                        sequery::CriteriaAggregateKind::Average, true);
                };

                // Batch 2 sixth admission: ADDRESS with the narrow
                // 2-argument form (row, col). Uses default A1 convention
                // and absolute mode 1. Any other parameter combination
                // (3-5 args with abs mode, style flag, sheet token) or
                // non-scalar arguments defer to legacy ScAddressFunc.
                const auto tryPlanEngineIndirect = [&]() -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    if (nParamCount < 1 || nParamCount > 2 || sp < nParamCount)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    // Scope fence: reference text must already be an
                    // svString token (no live coercion); optional A1/R1C1
                    // flag must be a scalar svDouble.
                    const FormulaToken* pFlagTok
                        = (nParamCount == 2) ? pStack[sp - 1] : nullptr;
                    const FormulaToken* pTextTok
                        = pStack[sp - nParamCount];
                    if (!pTextTok || pTextTok->GetType() != svString
                        || (pFlagTok && pFlagTok->GetType() != svDouble))
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    const bool bForceR1C1
                        = (pFlagTok != nullptr && pFlagTok->GetDouble() == 0.0);
                    const auto aSyntaxPolicy
                        = sestringref::resolveIndirectAddressSyntaxPolicy(
                            selibreoffice::toApiAddressConvention(
                                maCalcConfig.meStringRefAddressSyntax),
                            selibreoffice::toApiAddressConvention(
                                mrDoc.GetAddressConvention()),
                            maCalcConfig.meStringRefAddressSyntax
                                == FormulaGrammar::CONV_A1_XL_A1,
                            bForceR1C1);
                    const FormulaGrammar::AddressConvention eConv
                        = selibreoffice::toLibreOfficeAddressConvention(
                            aSyntaxPolicy.mePrimary);
                    const bool bTryXlA1
                        = aSyntaxPolicy.moFallback
                          == spreadsheetengine::api::AddressConvention::XlA1;

                    const svl::SharedString sRefStr = pTextTok->GetString();
                    if (sRefStr.getString().isEmpty())
                    {
                        sp -= nParamCount;
                        nGlobalError = FormulaError::NONE;
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineSucceededCount);
                        PushError(FormulaError::NoRef);
                        return true;
                    }

                    const auto oResolved = seindirectexec::resolveIndirectReference(
                        mrDoc, aPos, sRefStr, eConv, bTryXlA1);
                    if (!oResolved)
                    {
                        sp -= nParamCount;
                        nGlobalError = FormulaError::NONE;
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineSucceededCount);
                        PushError(FormulaError::NoRef);
                        return true;
                    }

                    sp -= nParamCount;
                    nGlobalError = FormulaError::NONE;
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineSucceededCount);
                    switch (oResolved->meKind)
                    {
                        case seindirectexec::IndirectExecutionResult::Kind::SingleRef:
                            PushSingleRef(oResolved->maRef1);
                            break;
                        case seindirectexec::IndirectExecutionResult::Kind::DoubleRef:
                            PushDoubleRef(oResolved->maRef1, oResolved->maRef2);
                            break;
                        case seindirectexec::IndirectExecutionResult::Kind::ExternalSingleRef:
                            PushExternalSingleRef(
                                oResolved->mnFileId, oResolved->maTabName,
                                oResolved->maRef1.Col(),
                                oResolved->maRef1.Row(),
                                oResolved->maRef1.Tab());
                            break;
                        case seindirectexec::IndirectExecutionResult::Kind::ExternalDoubleRef:
                            PushExternalDoubleRef(
                                oResolved->mnFileId, oResolved->maTabName,
                                oResolved->maRef1.Col(),
                                oResolved->maRef1.Row(),
                                oResolved->maRef1.Tab(),
                                oResolved->maRef2.Col(),
                                oResolved->maRef2.Row(),
                                oResolved->maRef2.Tab());
                            break;
                        case seindirectexec::IndirectExecutionResult::Kind::Token:
                            PushTokenRef(oResolved->mxToken);
                            break;
                    }
                    return true;
                };

                const auto tryPlanEngineAddress = [&]() -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    if (nParamCount != 2 || sp < 2)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }
                    const FormulaToken* pColTok = pStack[sp - 1];
                    const FormulaToken* pRowTok = pStack[sp - 2];
                    if (!pColTok || !pRowTok
                        || pColTok->GetType() != svDouble
                        || pRowTok->GetType() != svDouble)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    const sal_Int32 nCol
                        = static_cast<sal_Int32>(pColTok->GetDouble());
                    const sal_Int32 nRow
                        = static_cast<sal_Int32>(pRowTok->GetDouble());

                    // 1-based row/col validation.
                    if (nCol < 1 || nRow < 1
                        || !mrDoc.ValidCol(static_cast<SCCOL>(nCol - 1))
                        || !mrDoc.ValidRow(static_cast<SCROW>(nRow - 1)))
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    serefexec::AddressFunctionRequest aRequest;
                    aRequest.mnRow
                        = static_cast<spreadsheetengine::api::RowIndex>(nRow - 1);
                    aRequest.mnColumn
                        = static_cast<spreadsheetengine::api::ColumnIndex>(nCol - 1);
                    aRequest.mnAbsMode = 1;
                    aRequest.mbA1Style = true;
                    aRequest.meConvention = mrDoc.GetAddressConvention();

                    const auto aFormatted
                        = serefexec::formatAddressFunctionResult(aRequest);
                    if (!aFormatted)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    sp -= 2;
                    nGlobalError = FormulaError::NONE;
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineSucceededCount);
                    PushString(aFormatted.maValue);
                    return true;
                };

                // Batch 2 fifth admission: INDEX scalar-reference selection.
                // Covers INDEX(ref, row) and INDEX(ref, row, col) where:
                //   - base is svSingleRef or svDoubleRef (no matrix, no
                //     external, no RefList — those stay on legacy)
                //   - row is a positive scalar double
                //   - col is a positive scalar double (if present)
                // Matrix-returning forms (row=0 or col=0) and external /
                // matrix / RefList bases defer to legacy.
                const auto tryPlanEngineIndex = [&]() -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    if (nParamCount < 2 || nParamCount > 3)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }
                    if (sp < nParamCount)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    const FormulaToken* pColTok
                        = (nParamCount == 3) ? pStack[sp - 1] : nullptr;
                    const FormulaToken* pRowTok
                        = pStack[sp - (nParamCount == 3 ? 2 : 1)];
                    const FormulaToken* pBaseTok
                        = pStack[sp - nParamCount];

                    const StackVar eBaseType = pBaseTok ? pBaseTok->GetType() : svUnknown;
                    if ((eBaseType != svSingleRef && eBaseType != svDoubleRef)
                        || !pRowTok || pRowTok->GetType() != svDouble
                        || (pColTok && pColTok->GetType() != svDouble))
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    const double fRow = pRowTok->GetDouble();
                    const double fCol = pColTok ? pColTok->GetDouble() : 0.0;
                    // Scope fence: zero-axis (whole row / column slice)
                    // produces a matrix result; defer to legacy.
                    if (fRow <= 0.0 || (nParamCount == 3 && fCol <= 0.0))
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    // Peek the base range before popping so we can decline
                    // cleanly if anything goes wrong building the plan.
                    spreadsheetengine::api::ResolvedReference aBase;
                    if (eBaseType == svSingleRef)
                    {
                        const ScSingleRefData& rRef = *pBaseTok->GetSingleRef();
                        const ScAddress aAbs = rRef.toAbs(mrDoc, aPos);
                        aBase.maRange.maStart = {
                            static_cast<spreadsheetengine::api::SheetId>(aAbs.Tab()),
                            static_cast<spreadsheetengine::api::ColumnIndex>(aAbs.Col()),
                            static_cast<spreadsheetengine::api::RowIndex>(aAbs.Row())
                        };
                        aBase.maRange.maEnd = aBase.maRange.maStart;
                    }
                    else
                    {
                        const ScComplexRefData& rRef = *pBaseTok->GetDoubleRef();
                        const ScRange aAbs = rRef.toAbs(mrDoc, aPos);
                        aBase.maRange.maStart = {
                            static_cast<spreadsheetengine::api::SheetId>(aAbs.aStart.Tab()),
                            static_cast<spreadsheetengine::api::ColumnIndex>(aAbs.aStart.Col()),
                            static_cast<spreadsheetengine::api::RowIndex>(aAbs.aStart.Row())
                        };
                        aBase.maRange.maEnd = {
                            static_cast<spreadsheetengine::api::SheetId>(aAbs.aEnd.Tab()),
                            static_cast<spreadsheetengine::api::ColumnIndex>(aAbs.aEnd.Col()),
                            static_cast<spreadsheetengine::api::RowIndex>(aAbs.aEnd.Row())
                        };
                    }

                    serpn::IndexProjectionParameters aParams;
                    aParams.mnRowIndex
                        = static_cast<spreadsheetengine::api::RowIndex>(fRow);
                    aParams.mnColumnIndex
                        = (nParamCount == 3)
                              ? static_cast<spreadsheetengine::api::ColumnIndex>(fCol)
                              : 0;
                    aParams.mbColumnArgumentMissing = (nParamCount != 3);
                    aParams.mnParamCount = nParamCount;
                    aParams.mnAreaIndex = 1;
                    aParams.mnAreaCount = 1;

                    const auto aPlan = serpn::projectIndexReference(
                        serpn::RpnValue::reference(aBase), aParams);
                    if (!aPlan
                        || aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    // Only Scalar and KeepSource are scope-fence-safe here.
                    // RowSlice / ColumnSlice need matrix materialization.
                    using SelectionKind
                        = spreadsheetengine::api::reference::IndexSelectionKind;
                    if (aPlan.maValue.meKind != SelectionKind::KeepSource
                        && aPlan.maValue.meKind != SelectionKind::Scalar)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    // Commit: drop the operands and push the selection.
                    sp -= nParamCount;
                    nGlobalError = FormulaError::NONE;
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineSucceededCount);

                    if (aPlan.maValue.meKind == SelectionKind::KeepSource)
                    {
                        // Push the base unchanged.
                        if (eBaseType == svSingleRef)
                        {
                            PushSingleRef(
                                static_cast<SCCOL>(aBase.maRange.maStart.mnColumn),
                                static_cast<SCROW>(aBase.maRange.maStart.mnRow),
                                static_cast<SCTAB>(aBase.maRange.maStart.mnSheet));
                        }
                        else
                        {
                            PushDoubleRef(
                                static_cast<SCCOL>(aBase.maRange.maStart.mnColumn),
                                static_cast<SCROW>(aBase.maRange.maStart.mnRow),
                                static_cast<SCTAB>(aBase.maRange.maStart.mnSheet),
                                static_cast<SCCOL>(aBase.maRange.maEnd.mnColumn),
                                static_cast<SCROW>(aBase.maRange.maEnd.mnRow),
                                static_cast<SCTAB>(aBase.maRange.maEnd.mnSheet));
                        }
                    }
                    else
                    {
                        PushSingleRef(
                            static_cast<SCCOL>(aPlan.maValue.maRange.maStart.mnColumn),
                            static_cast<SCROW>(aPlan.maValue.maRange.maStart.mnRow),
                            static_cast<SCTAB>(aPlan.maValue.maRange.maStart.mnSheet));
                    }
                    return true;
                };

                // Batch 2 fourth admission: OFFSET with scalar int offsets
                // and a single-reference base. Covers 3-argument form only
                // (no new-height / new-width) to keep the first pass
                // narrow. Complex parameter combinations and external refs
                // defer to legacy.
                const auto tryPlanEngineOffset = [&]() -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    // Scope fence: only the three-argument form for now.
                    if (nParamCount != 3 || sp < 3)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    const FormulaToken* pColOffsetTok = pStack[sp - 1];
                    const FormulaToken* pRowOffsetTok = pStack[sp - 2];
                    const FormulaToken* pBaseTok = pStack[sp - 3];
                    if (!pColOffsetTok || !pRowOffsetTok || !pBaseTok
                        || pColOffsetTok->GetType() != svDouble
                        || pRowOffsetTok->GetType() != svDouble
                        || (pBaseTok->GetType() != svSingleRef
                            && pBaseTok->GetType() != svDoubleRef))
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    const std::int64_t nColOffset
                        = static_cast<std::int64_t>(pColOffsetTok->GetDouble());
                    const std::int64_t nRowOffset
                        = static_cast<std::int64_t>(pRowOffsetTok->GetDouble());

                    // Drop the three operands now that we've peeked them.
                    sp -= 3;
                    nGlobalError = FormulaError::NONE;

                    // Build the base ResolvedReference from the popped token.
                    spreadsheetengine::api::ResolvedReference aBase;
                    if (pBaseTok->GetType() == svSingleRef)
                    {
                        const ScSingleRefData& rRef = *pBaseTok->GetSingleRef();
                        const ScAddress aAbs = rRef.toAbs(mrDoc, aPos);
                        aBase.maRange.maStart = {
                            static_cast<spreadsheetengine::api::SheetId>(aAbs.Tab()),
                            static_cast<spreadsheetengine::api::ColumnIndex>(aAbs.Col()),
                            static_cast<spreadsheetengine::api::RowIndex>(aAbs.Row())
                        };
                        aBase.maRange.maEnd = aBase.maRange.maStart;
                    }
                    else
                    {
                        const ScComplexRefData& rRef = *pBaseTok->GetDoubleRef();
                        const ScRange aAbs = rRef.toAbs(mrDoc, aPos);
                        aBase.maRange.maStart = {
                            static_cast<spreadsheetengine::api::SheetId>(aAbs.aStart.Tab()),
                            static_cast<spreadsheetengine::api::ColumnIndex>(aAbs.aStart.Col()),
                            static_cast<spreadsheetengine::api::RowIndex>(aAbs.aStart.Row())
                        };
                        aBase.maRange.maEnd = {
                            static_cast<spreadsheetengine::api::SheetId>(aAbs.aEnd.Tab()),
                            static_cast<spreadsheetengine::api::ColumnIndex>(aAbs.aEnd.Col()),
                            static_cast<spreadsheetengine::api::RowIndex>(aAbs.aEnd.Row())
                        };
                    }

                    serpn::OffsetParameters aParams;
                    aParams.mnRowOffset = nRowOffset;
                    aParams.mnColumnOffset = nColOffset;
                    aParams.mnMaxColumn = mrDoc.MaxCol();
                    aParams.mnMaxRow = mrDoc.MaxRow();

                    const auto aPlan = serpn::planOffset(
                        serpn::RpnValue::reference(aBase), aParams);
                    if (!aPlan
                        || aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        // We already popped the operands; push the error
                        // through the engine path too so the caller doesn't
                        // re-run the legacy on a drained stack.
                        if (!aPlan
                            && aPlan.meError != spreadsheetengine::api::Error::None)
                        {
                            PushError(selibreoffice::toFormulaError(aPlan.meError));
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnReferenceEngineSucceededCount);
                            return true;
                        }
                        return false;
                    }

                    const spreadsheetengine::api::CellRange& rRange = aPlan.maValue;
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineSucceededCount);
                    if (rRange.isSingleCell())
                    {
                        PushSingleRef(
                            static_cast<SCCOL>(rRange.maStart.mnColumn),
                            static_cast<SCROW>(rRange.maStart.mnRow),
                            static_cast<SCTAB>(rRange.maStart.mnSheet));
                    }
                    else
                    {
                        PushDoubleRef(
                            static_cast<SCCOL>(rRange.maStart.mnColumn),
                            static_cast<SCROW>(rRange.maStart.mnRow),
                            static_cast<SCTAB>(rRange.maStart.mnSheet),
                            static_cast<SCCOL>(rRange.maEnd.mnColumn),
                            static_cast<SCROW>(rRange.maEnd.mnRow),
                            static_cast<SCTAB>(rRange.maEnd.mnSheet));
                    }
                    return true;
                };

                // Batch 2 third admission: ocAreas with a single scalar
                // reference always has an area count of 1. svRefList and
                // anything else defer to legacy where iteration over the
                // reference list is canonical.
                const auto tryPlanEngineAreaCount = [&]() -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    if (nParamCount != 1 || !sp)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }
                    const FormulaToken* pTop = pStack[sp - 1];
                    if (!pTop
                        || (pTop->GetType() != svSingleRef
                            && pTop->GetType() != svDoubleRef))
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    // Single scalar reference operand — exactly one area.
                    const auto aPlan = serpn::planAreaCount(1);
                    if (!aPlan)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    Pop();
                    nGlobalError = FormulaError::NONE;
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineSucceededCount);
                    PushDouble(aPlan.maValue);
                    return true;
                };

                // Batch 2 first admission: scalar axis-ordinal
                // COLUMN / ROW / SHEET. Covers no-argument (use aPos) and
                // single-reference argument via planAxisOrdinal.
                // Matrix-context no-arg, external-ref, double-ref, and any
                // other shape defer to the legacy ScColumn / ScRow / ScSheet.
                const auto tryPlanEngineAxisOrdinal
                    = [&](serpn::AxisOrdinalKind eKind) -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    if (nParamCount > 1)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    ScAddress aRefPos;
                    if (nParamCount == 0)
                    {
                        if (bMatrixFormula)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnReferenceEngineDeclinedCount);
                            return false;
                        }
                        aRefPos = aPos;
                    }
                    else
                    {
                        if (!sp || !pStack[sp - 1]
                            || pStack[sp - 1]->GetType() != svSingleRef)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnReferenceEngineDeclinedCount);
                            return false;
                        }
                        PopSingleRef(aRefPos);
                        if (nGlobalError != FormulaError::NONE)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnReferenceEngineDeclinedCount);
                            return false;
                        }
                    }

                    spreadsheetengine::api::ResolvedReference aResolved;
                    aResolved.maRange.maStart = {
                        static_cast<spreadsheetengine::api::SheetId>(aRefPos.Tab()),
                        static_cast<spreadsheetengine::api::ColumnIndex>(aRefPos.Col()),
                        static_cast<spreadsheetengine::api::RowIndex>(aRefPos.Row())
                    };
                    aResolved.maRange.maEnd = aResolved.maRange.maStart;

                    const auto aPlan = serpn::planAxisOrdinal(
                        serpn::RpnValue::reference(aResolved), eKind);
                    if (!aPlan
                        || aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnReferenceEngineDeclinedCount);
                        return false;
                    }

                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnReferenceEngineSucceededCount);
                    PushDouble(aPlan.maValue);
                    return true;
                };
                // Batch 1 second admission: scalar-selector CHOOSE through
                // engine-native planChooseBranch. Matrix selector defers to
                // legacy ScChooseJump which owns the JumpMatrix protocol.
                const auto tryPlanEngineChooseJump = [&]() -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnControlFlowEngineAttemptedCount);

                    const short* pJump = pCur->GetJump();
                    const short nJumpCount = pJump[0];
                    if (!sp || nJumpCount < 1)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }

                    const FormulaToken* pSelectorToken = pStack[sp - 1];
                    if (!pSelectorToken)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }

                    std::optional<serpn::RpnValue> oSelector;
                    switch (pSelectorToken->GetType())
                    {
                        case svDouble:
                            oSelector = serpn::RpnValue::number(
                                pSelectorToken->GetDouble());
                            break;
                        case svError:
                            oSelector = serpn::RpnValue::error(
                                selibreoffice::toApiError(pSelectorToken->GetError()));
                            break;
                        default:
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnControlFlowEngineDeclinedCount);
                            return false;
                    }

                    // planChooseBranch wants the count of branch slots,
                    // which is nJumpCount - 1 (since pJump[nJumpCount] is
                    // the endpoint, not a branch).
                    const auto aPlan = serpn::planChooseBranch(
                        *oSelector,
                        static_cast<std::int16_t>(nJumpCount - 1));
                    if (aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready
                        || !aPlan)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }

                    Pop();
                    nGlobalError = FormulaError::NONE;

                    switch (aPlan.maValue.meDirective)
                    {
                        case serpn::BranchDirective::TakeSlot:
                            // planChooseBranch slot is 1-based (matches
                            // CHOOSE semantics), and pJump[slot] is the
                            // target for branch `slot`.
                            aCode.Jump(
                                pJump[aPlan.maValue.mnSlot], pJump[nJumpCount]);
                            break;
                        case serpn::BranchDirective::PropagateError:
                            PushError(selibreoffice::toFormulaError(
                                aPlan.maValue.meError));
                            aCode.Jump(pJump[nJumpCount], pJump[nJumpCount]);
                            break;
                        default:
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnControlFlowEngineDeclinedCount);
                            return false;
                    }

                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnControlFlowEngineSucceededCount);
                    return true;
                };
                const auto tryPlanEngineIfError = [&](bool bNAonly) -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnControlFlowEngineAttemptedCount);

                    const short* pJump = pCur->GetJump();
                    const short nJumpCount = pJump[0];
                    if (!sp || nJumpCount != 2)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }

                    // Scope fence: only admit when the primary is a simple
                    // scalar token (svDouble / svString / svError / svEmpty
                    // / svMissing). Reference, matrix, external-ref, and
                    // jump-matrix shapes still need the legacy MatrixJump
                    // protocol and defer.
                    const FormulaToken* pPrimary = pStack[sp - 1];
                    if (!pPrimary)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }
                    spreadsheetengine::api::Error ePrimaryError
                        = spreadsheetengine::api::Error::None;
                    switch (pPrimary->GetType())
                    {
                        case svDouble:
                        case svString:
                        case svEmptyCell:
                        case svMissing:
                            // No error state — IFERROR/IFNA keep the value.
                            break;
                        case svError:
                            ePrimaryError = selibreoffice::toApiError(pPrimary->GetError());
                            break;
                        default:
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnControlFlowEngineDeclinedCount);
                            return false;
                    }
                    // Pre-existing global error from a prior popped operand
                    // also matters; only admit when it is None so the engine's
                    // single-error decision matches the legacy path.
                    if (nGlobalError != FormulaError::NONE)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }

                    const auto aPlan
                        = serpn::planIfErrorBranch(ePrimaryError, bNAonly,
                                                   /*nAlternateSlot=*/1);
                    FormulaConstTokenRef xPrimary(pPrimary);
                    Pop();
                    nGlobalError = FormulaError::NONE;

                    switch (aPlan.meDirective)
                    {
                        case serpn::BranchDirective::KeepPrimaryValue:
                            PushTokenRef(xPrimary);
                            aCode.Jump(pJump[nJumpCount], pJump[nJumpCount]);
                            break;
                        case serpn::BranchDirective::EvaluateAlternate:
                            aCode.Jump(pJump[1], pJump[nJumpCount]);
                            break;
                        case serpn::BranchDirective::PropagateError:
                            PushError(selibreoffice::toFormulaError(aPlan.meError));
                            aCode.Jump(pJump[nJumpCount], pJump[nJumpCount]);
                            break;
                        default:
                            // ReturnNotAvailable / ReturnParameterExpected /
                            // TakeSlot / ReturnSyntheticBoolean are not
                            // produced by planIfErrorBranch; treat
                            // defensively as decline so legacy re-runs.
                            PushTokenRef(xPrimary);
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnControlFlowEngineDeclinedCount);
                            return false;
                    }

                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnControlFlowEngineSucceededCount);
                    return true;
                };
                const auto pushLegacyIfJump = [&]() {
                    warnConditionalDispatch(u"IF");
                    ScIfJump();
                };
                const auto pushLegacyIfError = [&](bool bNAonly) {
                    warnConditionalDispatch(bNAonly ? u"IFNA" : u"IFERROR");

                    const short* pJump = pCur->GetJump();
                    short nJumpCount = pJump[0];
                    if (!sp || nJumpCount != 2)
                    {
                        nGlobalError = (sp ? FormulaError::ParameterExpected
                                           : FormulaError::UnknownStackVariable);
                        PushError(nGlobalError);
                        aCode.Jump(pJump[nJumpCount], pJump[nJumpCount]);
                        return;
                    }

                    FormulaConstTokenRef xToken(pStack[sp - 1]);
                    bool bError = false;
                    FormulaError nOldGlobalError = nGlobalError;
                    nGlobalError = FormulaError::NONE;

                    MatrixJumpConditionToMatrix();
                    switch (GetStackType())
                    {
                        default:
                            Pop();
                            if (nOldGlobalError != FormulaError::NONE)
                                nGlobalError = nOldGlobalError;
                            if (nGlobalError != FormulaError::NONE)
                                bError = true;
                            break;
                        case svError:
                            PopError();
                            bError = true;
                            break;
                        case svDoubleRef:
                        case svSingleRef:
                        {
                            ScAddress aAdr;
                            if (!PopDoubleRefOrSingleRef(aAdr))
                                bError = true;
                            else
                            {
                                ScRefCellValue aCell(mrDoc, aAdr);
                                nGlobalError = GetCellErrCode(aCell);
                                if (sejumpexec::matchesIfErrorPolicy(nGlobalError, bNAonly))
                                    bError = true;
                            }
                        }
                        break;
                        case svExternalSingleRef:
                        case svExternalDoubleRef:
                        {
                            double fVal;
                            svl::SharedString aStr;
                            GetDoubleOrStringFromMatrix(fVal, aStr);
                            if (nGlobalError != FormulaError::NONE)
                                bError = true;
                        }
                        break;
                        case svMatrix:
                        {
                            const ScMatrixRef pMat = PopMatrix();
                            if (!pMat
                                || (nGlobalError != FormulaError::NONE
                                    && (!bNAonly || nGlobalError == FormulaError::NotAvailable)))
                            {
                                bError = true;
                                break;
                            }

                            SCSIZE nErrorCol = ::std::numeric_limits<SCSIZE>::max();
                            SCSIZE nErrorRow = ::std::numeric_limits<SCSIZE>::max();
                            SCSIZE nCols, nRows;
                            pMat->GetDimensions(nCols, nRows);
                            if (nCols == 0 || nRows == 0)
                            {
                                bError = true;
                                break;
                            }
                            if (const auto oFirstError
                                = sejumpexec::findFirstIfErrorCoordinate(*pMat, bNAonly))
                            {
                                bError = true;
                                nErrorCol = oFirstError->mnColumn;
                                nErrorRow = oFirstError->mnRow;
                            }
                            if (!bError)
                                break;

                            FormulaConstTokenRef xNew;
                            ScTokenMatrixMap::const_iterator aMapIter;
                            if ((aMapIter = maTokenMatrixMap.find(pCur))
                                != maTokenMatrixMap.end())
                            {
                                xNew = (*aMapIter).second;
                            }
                            else
                            {
                                std::shared_ptr<ScJumpMatrix> pJumpMat(
                                    std::make_shared<ScJumpMatrix>(pCur->GetOpCode(), nCols, nRows));
                                const double fFlagResult
                                    = CreateDoubleError(FormulaError::JumpMatHasResult);
                                pJumpMat->SetAllJumps(
                                    fFlagResult, pJump[nJumpCount], pJump[nJumpCount]);
                                sejumpexec::initializeIfErrorJumpMatrix(
                                    *pMat, *pJumpMat, pJump, nJumpCount, bNAonly,
                                    { nErrorCol, nErrorRow });
                                xNew = new ScJumpMatrixToken(std::move(pJumpMat));
                                GetTokenMatrixMap().emplace(pCur, xNew);
                            }
                            nGlobalError = nOldGlobalError;
                            PushTokenRef(xNew);
                            aCode.Jump(pJump[nJumpCount], pJump[nJumpCount]);
                            return;
                        }
                    }

                    const auto eIfErrorAction = selogic::selectIfErrorAction(
                        nGlobalError == FormulaError::NotAvailable
                            ? spreadsheetengine::api::Error::NotAvailable
                            : (bError ? spreadsheetengine::api::Error::IllegalArgument
                                      : spreadsheetengine::api::Error::None),
                        bNAonly);
                    if (bError && eIfErrorAction == selogic::IfErrorAction::EvaluateAlternate)
                    {
                        nGlobalError = FormulaError::NONE;
                        aCode.Jump(pJump[1], pJump[nJumpCount]);
                    }
                    else
                    {
                        nGlobalError = nOldGlobalError;
                        PushTokenRef(xToken);
                        aCode.Jump(pJump[nJumpCount], pJump[nJumpCount]);
                    }
                };
                const auto tryPlanEngineIfs = [&]() -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnControlFlowEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    if (nParamCount == 0 || (nParamCount % 2) != 0
                        || sp < nParamCount)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }
                    if (nGlobalError != FormulaError::NONE)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }

                    // Conditions sit at the even offsets from the bottom of
                    // the param window: c0 at sp - nParamCount + 0,
                    // c1 at +2, c2 at +4, ... Values follow at +1, +3, +5.
                    // Scope fence: every condition must be a simple scalar
                    // token so the planner can decide locally without host
                    // resolution. Any non-scalar condition declines to the
                    // legacy ReverseStack walk.
                    const std::size_t nPairs
                        = static_cast<std::size_t>(nParamCount) / 2;
                    std::size_t winningPair = nPairs;
                    enum class Outcome
                    {
                        Continue,
                        Selected,
                        ReturnNotAvailable,
                        ReturnParameterExpected,
                        PropagateError
                    };
                    Outcome eOutcome = Outcome::Continue;
                    spreadsheetengine::api::Error ePropagated
                        = spreadsheetengine::api::Error::None;
                    for (std::size_t i = 0; i < nPairs; ++i)
                    {
                        const FormulaToken* pCond
                            = pStack[sp - nParamCount + 2 * i];
                        if (!pCond)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnControlFlowEngineDeclinedCount);
                            return false;
                        }
                        std::optional<serpn::RpnValue> oCond;
                        switch (pCond->GetType())
                        {
                            case svDouble:
                                oCond = serpn::RpnValue::number(pCond->GetDouble());
                                break;
                            case svError:
                                oCond = serpn::RpnValue::error(
                                    selibreoffice::toApiError(pCond->GetError()));
                                break;
                            case svEmptyCell:
                            case svMissing:
                                oCond = serpn::RpnValue::empty();
                                break;
                            default:
                                addDispatchRuntimeStat(
                                    interpreterDispatchRuntimeStatsStore()
                                        .mnControlFlowEngineDeclinedCount);
                                return false;
                        }
                        const std::int16_t nRemainingAfter
                            = static_cast<std::int16_t>(nParamCount - 2 * i - 2);
                        const auto aPlan = serpn::planIfsBranch(
                            *oCond, i, nRemainingAfter);
                        if (aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready
                            || !aPlan)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnControlFlowEngineDeclinedCount);
                            return false;
                        }
                        switch (aPlan.maValue.meDirective)
                        {
                            case serpn::BranchDirective::TakeSlot:
                                winningPair = aPlan.maValue.mnSlot;
                                eOutcome = Outcome::Selected;
                                break;
                            case serpn::BranchDirective::ReturnNotAvailable:
                                eOutcome = Outcome::ReturnNotAvailable;
                                break;
                            case serpn::BranchDirective::ReturnParameterExpected:
                                eOutcome = Outcome::ReturnParameterExpected;
                                break;
                            case serpn::BranchDirective::PropagateError:
                                eOutcome = Outcome::PropagateError;
                                ePropagated = aPlan.maValue.meError;
                                break;
                            default:
                                // SkipCurrentResult: keep walking.
                                break;
                        }
                        if (eOutcome != Outcome::Continue)
                            break;
                    }

                    if (eOutcome == Outcome::Continue)
                    {
                        // Walked all pairs without selection — N/A.
                        eOutcome = Outcome::ReturnNotAvailable;
                    }

                    if (eOutcome == Outcome::Selected)
                    {
                        FormulaConstTokenRef xValue(
                            pStack[sp - nParamCount + 2 * winningPair + 1]);
                        sp -= nParamCount;
                        nGlobalError = FormulaError::NONE;
                        if (xValue)
                            PushTokenRef(xValue);
                        else
                            PushError(FormulaError::UnknownStackVariable);
                    }
                    else
                    {
                        sp -= nParamCount;
                        nGlobalError = FormulaError::NONE;
                        switch (eOutcome)
                        {
                            case Outcome::ReturnNotAvailable:
                                PushNA();
                                break;
                            case Outcome::ReturnParameterExpected:
                                PushParameterExpected();
                                break;
                            case Outcome::PropagateError:
                                PushError(selibreoffice::toFormulaError(ePropagated));
                                break;
                            default:
                                break;
                        }
                    }

                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnControlFlowEngineSucceededCount);
                    return true;
                };
                const auto pushLegacyIfs = [&]() {
                    warnConditionalDispatch(u"IFS");

                    short nParamCount = GetByte();
                    ReverseStack(nParamCount);

                    nGlobalError = FormulaError::NONE;
                    bool bFinished = false;
                    while (nParamCount > 0 && !bFinished && nGlobalError == FormulaError::NONE)
                    {
                        bool bVal = GetBool();
                        nParamCount--;
                        switch (spreadsheetengine::api::logic::evaluateIfsCondition(
                                    bVal, nGlobalError != FormulaError::NONE, nParamCount))
                        {
                            case spreadsheetengine::api::logic::IfsAction::SelectCurrentResult:
                                bFinished = true;
                                break;
                            case spreadsheetengine::api::logic::IfsAction::SkipCurrentResult:
                                Pop();
                                nParamCount--;
                                break;
                            case spreadsheetengine::api::logic::IfsAction::ReturnParameterExpected:
                                PushParameterExpected();
                                return;
                            case spreadsheetengine::api::logic::IfsAction::ReturnNotAvailable:
                                PushNA();
                                return;
                            case spreadsheetengine::api::logic::IfsAction::ReturnNoValue:
                                PushNoValue();
                                return;
                        }
                    }

                    if (nGlobalError != FormulaError::NONE || !bFinished)
                    {
                        if (!bFinished)
                            PushNA();
                        if (nGlobalError != FormulaError::NONE)
                            PushNoValue();
                        return;
                    }

                    FormulaConstTokenRef xToken(PopToken());
                    if (xToken)
                    {
                        while (nParamCount > 1)
                        {
                            Pop();
                            nParamCount--;
                        }
                        PushTokenRef(xToken);
                    }
                    else
                        PushError(FormulaError::UnknownStackVariable);
                };
                const auto tryPlanEngineSwitch = [&]() -> bool {
                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnControlFlowEngineAttemptedCount);

                    const sal_uInt8 nParamCount = pCur->GetByte();
                    if (nParamCount < 3 || sp < nParamCount)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }
                    if (nGlobalError != FormulaError::NONE)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }

                    // Stack window (pre-reverse): selector at
                    // sp - nParamCount, then case0, result0, case1, result1,
                    // ..., last pushed at sp - 1. Even total → trailing
                    // default; odd total → no default.
                    const std::size_t nBase
                        = static_cast<std::size_t>(sp - nParamCount);
                    const bool bHasDefault = (nParamCount % 2) == 0;
                    const std::size_t nPairBytes
                        = nParamCount - 1 - (bHasDefault ? 1 : 0);
                    const std::size_t nPairs = nPairBytes / 2;

                    auto buildScalar
                        = [&](const FormulaToken* pTok) -> std::optional<serpn::RpnValue> {
                        if (!pTok)
                            return std::nullopt;
                        switch (pTok->GetType())
                        {
                            case svDouble:
                                return serpn::RpnValue::number(pTok->GetDouble());
                            case svString:
                                return serpn::RpnValue::text(
                                    selibreoffice::toApiString(
                                        pTok->GetString().getString()));
                            case svError:
                                return serpn::RpnValue::error(
                                    selibreoffice::toApiError(pTok->GetError()));
                            case svEmptyCell:
                            case svMissing:
                                return serpn::RpnValue::empty();
                            default:
                                return std::nullopt;
                        }
                    };

                    const FormulaToken* pSelectorTok = pStack[nBase];
                    auto oSelector = buildScalar(pSelectorTok);
                    if (!oSelector)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }

                    std::vector<serpn::RpnValue> aCaseLabels;
                    aCaseLabels.reserve(nPairs);
                    for (std::size_t i = 0; i < nPairs; ++i)
                    {
                        const FormulaToken* pCaseTok = pStack[nBase + 1 + 2 * i];
                        auto oCase = buildScalar(pCaseTok);
                        if (!oCase)
                        {
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnControlFlowEngineDeclinedCount);
                            return false;
                        }
                        aCaseLabels.push_back(std::move(*oCase));
                    }

                    const std::optional<std::size_t> oDefaultSlot
                        = bHasDefault ? std::optional<std::size_t>(nPairs)
                                      : std::nullopt;
                    const auto aPlan = serpn::planSwitchBranch(
                        *oSelector,
                        std::span<const serpn::RpnValue>(aCaseLabels.data(),
                                                         aCaseLabels.size()),
                        oDefaultSlot);
                    if (aPlan.meReadiness != serpn::RpnCoercionReadiness::Ready
                        || !aPlan)
                    {
                        addDispatchRuntimeStat(
                            interpreterDispatchRuntimeStatsStore()
                                .mnControlFlowEngineDeclinedCount);
                        return false;
                    }

                    switch (aPlan.maValue.meDirective)
                    {
                        case serpn::BranchDirective::TakeSlot:
                        {
                            const std::size_t nSlot = aPlan.maValue.mnSlot;
                            // Pair-result slot k → token at nBase+2+2k.
                            // Default slot k == nPairs → token at sp-1.
                            const std::size_t nResultIndex
                                = (nSlot < nPairs)
                                      ? (nBase + 2 + 2 * nSlot)
                                      : static_cast<std::size_t>(sp - 1);
                            FormulaConstTokenRef xWinning(pStack[nResultIndex]);
                            sp -= nParamCount;
                            nGlobalError = FormulaError::NONE;
                            if (xWinning)
                                PushTokenRef(xWinning);
                            else
                                PushError(FormulaError::UnknownStackVariable);
                            break;
                        }
                        case serpn::BranchDirective::ReturnNotAvailable:
                            sp -= nParamCount;
                            nGlobalError = FormulaError::NONE;
                            PushNA();
                            break;
                        case serpn::BranchDirective::PropagateError:
                            sp -= nParamCount;
                            nGlobalError = FormulaError::NONE;
                            PushError(selibreoffice::toFormulaError(
                                aPlan.maValue.meError));
                            break;
                        default:
                            addDispatchRuntimeStat(
                                interpreterDispatchRuntimeStatsStore()
                                    .mnControlFlowEngineDeclinedCount);
                            return false;
                    }

                    addDispatchRuntimeStat(
                        interpreterDispatchRuntimeStatsStore()
                            .mnControlFlowEngineSucceededCount);
                    return true;
                };
                const auto pushLegacySwitch = [&]() {
                    warnConditionalDispatch(u"COM.MICROSOFT.SWITCH");

                    short nParamCount = GetByte();
                    if (!MustHaveParamCountMin(nParamCount, 3))
                        return;

                    ReverseStack(nParamCount);

                    nGlobalError = FormulaError::NONE;
                    seswitchexec::SwitchValue aReference;
                    switch (GetStackType())
                    {
                        case svDouble:
                            aReference = seswitchexec::makeNumericSwitchValue(GetDouble());
                            break;
                        case svString:
                            aReference = seswitchexec::makeTextSwitchValue(GetString());
                            break;
                        case svSingleRef:
                        case svDoubleRef:
                        {
                            ScAddress aAdr;
                            if (!PopDoubleRefOrSingleRef(aAdr))
                                break;
                            ScRefCellValue aCell(mrDoc, aAdr);
                            if (!(aCell.hasString() || aCell.hasEmptyValue() || aCell.isEmpty()))
                                aReference
                                    = seswitchexec::makeNumericSwitchValue(GetCellValue(aAdr, aCell));
                            else
                            {
                                svl::SharedString aRefStr;
                                GetCellString(aRefStr, aCell);
                                aReference = seswitchexec::makeTextSwitchValue(aRefStr);
                            }
                        }
                        break;
                        case svExternalSingleRef:
                        case svExternalDoubleRef:
                        case svMatrix:
                        {
                            double fRefVal = 0.0;
                            svl::SharedString aRefStr;
                            if (ScMatrix::IsValueType(GetDoubleOrStringFromMatrix(fRefVal, aRefStr)))
                                aReference = seswitchexec::makeNumericSwitchValue(fRefVal);
                            else
                                aReference = seswitchexec::makeTextSwitchValue(aRefStr);
                        }
                        break;
                        default:
                            PopError();
                            PushIllegalArgument();
                            return;
                    }

                    nParamCount--;
                    bool bFinished = false;
                    while (nParamCount > 1 && !bFinished && nGlobalError == FormulaError::NONE)
                    {
                        seswitchexec::SwitchValue aCandidate;
                        if (aReference.mbNumeric)
                            aCandidate = seswitchexec::makeNumericSwitchValue(GetDouble());
                        else
                            aCandidate = seswitchexec::makeTextSwitchValue(GetString());
                        nParamCount--;
                        if ((nGlobalError != FormulaError::NONE && nParamCount < 2)
                            || seswitchexec::matchesSwitchCase(aReference, aCandidate))
                        {
                            bFinished = true;
                        }
                        else
                        {
                            if (nParamCount >= 2)
                            {
                                Pop();
                                nParamCount--;
                                bFinished = (nParamCount == 1);
                            }
                            else
                            {
                                PushNA();
                                return;
                            }
                            nGlobalError = FormulaError::NONE;
                        }
                    }

                    if (nGlobalError != FormulaError::NONE || !bFinished)
                    {
                        if (!bFinished)
                            PushNA();
                        else
                            PushError(nGlobalError);
                        return;
                    }

                    FormulaConstTokenRef xToken(PopToken());
                    if (xToken)
                    {
                        while (nParamCount > 1)
                        {
                            Pop();
                            nParamCount--;
                        }
                        PushTokenRef(xToken);
                    }
                    else
                        PushError(FormulaError::UnknownStackVariable);
                };

                switch( eOp )
                {
                    case ocSep:
                    case ocClose:           // pushed by the compiler
                    case ocMissing          : ScMissing();                  break;
                    case ocMacro            : ScMacro();                    break;
                    case ocDBArea           : ScDBArea();                   break;
                    case ocColRowNameAuto   : ScColRowNameAuto();           break;
                    case ocIf               :
                        if (!tryPlanEngineIfJump())
                            pushLegacyIfJump();
                        break;
                    case ocIfError          :
                        if (!tryPlanEngineIfError(false))
                            pushLegacyIfError(false);
                        break;
                    case ocIfNA             :
                        if (!tryPlanEngineIfError(true))
                            pushLegacyIfError(true);
                        break;
                    case ocChoose           :
                        if (!tryPlanEngineChooseJump())
                            ScChooseJump();
                        break;
                    case ocChooseCols       : ScChooseColsOrRows(true); break;
                    case ocChooseRows       : ScChooseColsOrRows(false); break;
                    case ocAdd              :
                        if (!tryPushEngineScalarBinaryOp(serpn::BinaryScalarOperator::Add))
                        {
                            warnIfLegacyScalarRootReached(u"ADD");
                            CalculateAddSub(false);
                        }
                        break;
                    case ocSub              :
                        if (!tryPushEngineScalarBinaryOp(
                                serpn::BinaryScalarOperator::Subtract))
                        {
                            warnIfLegacyScalarRootReached(u"SUB");
                            CalculateAddSub(true);
                        }
                        break;
                    case ocMul              :
                        if (!tryPushEngineScalarBinaryOp(
                                serpn::BinaryScalarOperator::Multiply))
                            ScMul();
                        break;
                    case ocDiv              :
                        if (!tryPushEngineScalarBinaryOp(serpn::BinaryScalarOperator::Divide))
                            ScDiv();
                        break;
                    case ocAmpersand        :
                        if (!tryPushEngineScalarBinaryOp(serpn::BinaryScalarOperator::Concat))
                            ScAmpersand();
                        break;
                    case ocPow              :
                        if (!tryPushEngineScalarBinaryOp(serpn::BinaryScalarOperator::Power))
                            ScPow();
                        break;
                    case ocEqual            :
                        if (!tryPushEngineScalarBinaryOp(serpn::BinaryScalarOperator::Equal))
                        {
                            warnIfLegacyScalarRootReached(u"EQUAL");
                            ScCompareOp(
                                spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                    ComparisonMode::Equal,
                                SC_EQUAL);
                        }
                        break;
                    case ocNotEqual         :
                        if (!tryPushEngineScalarBinaryOp(
                                serpn::BinaryScalarOperator::NotEqual))
                        {
                            warnIfLegacyScalarRootReached(u"NOT_EQUAL");
                            ScCompareOp(
                                spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                    ComparisonMode::NotEqual,
                                SC_NOT_EQUAL);
                        }
                        break;
                    case ocLess             :
                        if (!tryPushEngineScalarBinaryOp(serpn::BinaryScalarOperator::Less))
                        {
                            warnIfLegacyScalarRootReached(u"LESS");
                            ScCompareOp(
                                spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                    ComparisonMode::Less,
                                SC_LESS);
                        }
                        break;
                    case ocGreater          :
                        if (!tryPushEngineScalarBinaryOp(
                                serpn::BinaryScalarOperator::Greater))
                        {
                            warnIfLegacyScalarRootReached(u"GREATER");
                            ScCompareOp(
                                spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                    ComparisonMode::Greater,
                                SC_GREATER);
                        }
                        break;
                    case ocLessEqual        :
                        if (!tryPushEngineScalarBinaryOp(
                                serpn::BinaryScalarOperator::LessEqual))
                        {
                            warnIfLegacyScalarRootReached(u"LESS_EQUAL");
                            ScCompareOp(
                                spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                    ComparisonMode::LessEqual,
                                SC_LESS_EQUAL);
                        }
                        break;
                    case ocGreaterEqual     :
                        if (!tryPushEngineScalarBinaryOp(
                                serpn::BinaryScalarOperator::GreaterEqual))
                        {
                            warnIfLegacyScalarRootReached(u"GREATER_EQUAL");
                            ScCompareOp(
                                spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                    ComparisonMode::GreaterEqual,
                                SC_GREATER_EQUAL);
                        }
                        break;
                    case ocAnd              :
                        pushLegacyLogicalFold(
                            u"AND", spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                        LogicalFoldMode::And);
                        break;
                    case ocOr               :
                        pushLegacyLogicalFold(
                            u"OR", spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                       LogicalFoldMode::Or);
                        break;
                    case ocXor              :
                        pushLegacyLogicalFold(
                            u"XOR", spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                        LogicalFoldMode::Xor);
                        break;
                    case ocIntersect        : ScIntersect();                break;
                    case ocRange            :
                        if (!tryPushEngineBadLiteralRangeOpcode()
                            && !tryPushEngineRangeReference())
                        {
                            warnIfLegacyDispatchReached(
                                "engine-first root range reference", u"RANGE_REFERENCE",
                                [](std::u16string_view rFormula) {
                                    return setaileval::isRootRangeReferenceFormula(rFormula);
                                },
                                "engine-backed root range reference reached ScInterpreter");
                            ScRangeFunc();
                        }
                        break;
                    case ocUnion            : ScUnionFunc();                break;
                    case ocNot              : pushLegacyNot();              break;
                    case ocNegSub           :
                    case ocNeg              :
                        warnIfLegacyScalarRootReached(u"NEGATE");
                        nFuncFmtType = nCurFmtType;
                        ScUnaryMatrixOrScalarOp(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                UnaryMatrixScalarMode::Negate);
                        break;
                    case ocPercentSign      :
                        warnIfLegacyScalarRootReached(u"PERCENT");
                        nFuncFmtType = SvNumFormatType::PERCENT;
                        PushInt(100);
                        ScSyntheticBinaryOp(ocDiv, &ScInterpreter::ScDiv);
                        break;
                    case ocPi               :
                        warnIfLegacyDefaultOnReached(
                            u"PI", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computePi());
                        break;
                    case ocRandom           : ScRandom();                   break;
                    case ocRandArray        : ScRandArray();                break;
                    case ocRandomNV         : ScRandom();                   break;
                    case ocRandbetweenNV    : ScRandbetween();              break;
                    case ocFilter           : ScFilter();               break;
                    case ocSort             : ScSort();                 break;
                    case ocSortBy           : ScSortBy();               break;
                    case ocDrop             : ScTakeOrDrop(false);      break;
                    case ocExpand           : ScExpand();               break;
                    case ocHStack           : ScHorizontalOrVerticalStack(true); break;
                    case ocVStack           : ScHorizontalOrVerticalStack(false); break;
                    case ocTake             : ScTakeOrDrop(true);       break;
                    case ocTextAfter        : pushLegacyTextBeforeAfter(false); break;
                    case ocTextBefore       : pushLegacyTextBeforeAfter(true);  break;
                    case ocTextSplit        : ScTextSplit();            break;
                    case ocToCol            : ScToColOrRow(true);       break;
                    case ocToRow            : ScToColOrRow(false);      break;
                    case ocUnique           : ScUnique();               break;
                    case ocLet              : ScLet();                  break;
                    case ocWrapCols         : ScWrapColsOrRows(true);   break;
                    case ocWrapRows         : ScWrapColsOrRows(false);  break;
                    case ocTrue             :
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"TRUE()",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on logical constant reached ScInterpreter");
                        nFuncFmtType = SvNumFormatType::LOGICAL;
                        PushInt(1);
                        break;
                    case ocFalse            :
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"FALSE()",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on logical constant reached ScInterpreter");
                        nFuncFmtType = SvNumFormatType::LOGICAL;
                        PushInt(0);
                        break;
                    case ocGetActDate       :
                    {
                        warnIfLegacyDateFamilyReached(u"TODAY");
                        nFuncFmtType = SvNumFormatType::DATE;
                        Date aActDate(Date::SYSTEM);
                        tools::Long nDiff = aActDate - mrContext.NFGetNullDate();
                        PushDouble(static_cast<double>(nDiff));
                    }
                    break;
                    case ocGetActTime       :
                    {
                        warnIfLegacyDateFamilyReached(u"NOW");
                        nFuncFmtType = SvNumFormatType::DATETIME;
                        DateTime aActTime(DateTime::SYSTEM);
                        tools::Long nDiff = aActTime - mrContext.NFGetNullDate();
                        double fTime = aActTime.GetHour()
                                           / static_cast<double>(::tools::Time::hourPerDay)
                                       + aActTime.GetMin()
                                           / static_cast<double>(::tools::Time::minutePerDay)
                                       + aActTime.GetSec()
                                           / static_cast<double>(::tools::Time::secondPerDay)
                                       + aActTime.GetNanoSec()
                                           / static_cast<double>(::tools::Time::nanoSecPerDay);
                        PushDouble(static_cast<double>(nDiff) + fTime);
                    }
                    break;
                    case ocNotAvail         : PushError( FormulaError::NotAvailable); break;
                    case ocDeg              :
                        warnIfLegacyDefaultOnReached(
                            u"DEGREES", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeDegrees(GetDouble()));
                        break;
                    case ocRad              :
                        warnIfLegacyDefaultOnReached(
                            u"RADIANS", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeRadians(GetDouble()));
                        break;
                    case ocSin              :
                        warnIfLegacyDefaultOnReached(
                            u"SIN", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeSin(GetDouble()));
                        break;
                    case ocCos              :
                        warnIfLegacyDefaultOnReached(
                            u"COS", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeCos(GetDouble()));
                        break;
                    case ocTan              :
                        warnIfLegacyDefaultOnReached(
                            u"TAN", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeTan(GetDouble()));
                        break;
                    case ocCot              :
                        warnIfLegacyDefaultOnReached(
                            u"COT", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeCot(GetDouble()));
                        break;
                    case ocArcSin           :
                        warnIfLegacyDefaultOnReached(
                            u"ASIN", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeArcSin(GetDouble()));
                        break;
                    case ocArcCos           :
                        warnIfLegacyDefaultOnReached(
                            u"ACOS", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeArcCos(GetDouble()));
                        break;
                    case ocArcTan           :
                        warnIfLegacyDefaultOnReached(
                            u"ATAN", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeArcTan(GetDouble()));
                        break;
                    case ocArcCot           :
                        warnIfLegacyDefaultOnReached(
                            u"ACOT", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeArcCot(GetDouble()));
                        break;
                    case ocSinHyp           :
                        warnIfLegacyDefaultOnReached(
                            u"SINH", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeSinHyp(GetDouble()));
                        break;
                    case ocCosHyp           :
                        warnIfLegacyDefaultOnReached(
                            u"COSH", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeCosHyp(GetDouble()));
                        break;
                    case ocTanHyp           :
                        warnIfLegacyDefaultOnReached(
                            u"TANH", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeTanHyp(GetDouble()));
                        break;
                    case ocCotHyp           :
                        warnIfLegacyDefaultOnReached(
                            u"COTH", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeCotHyp(GetDouble()));
                        break;
                    case ocArcSinHyp        :
                        warnIfLegacyDefaultOnReached(
                            u"ASINH", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeArcSinHyp(GetDouble()));
                        break;
                    case ocArcCosHyp        :
                        warnIfLegacyDefaultOnReached(
                            u"ACOSH", "family-local default-on math scalar reached ScInterpreter");
                        if (std::optional<double> fResult = semath::computeArcCosHyp(GetDouble()))
                            PushDouble(*fResult);
                        else
                            PushIllegalArgument();
                        break;
                    case ocArcTanHyp        :
                        warnIfLegacyDefaultOnReached(
                            u"ATANH", "family-local default-on math scalar reached ScInterpreter");
                        if (std::optional<double> fResult = semath::computeArcTanHyp(GetDouble()))
                            PushDouble(*fResult);
                        else
                            PushIllegalArgument();
                        break;
                    case ocArcCotHyp        :
                        warnIfLegacyDefaultOnReached(
                            u"ACOTH", "family-local default-on math scalar reached ScInterpreter");
                        if (std::optional<double> fResult = semath::computeArcCotHyp(GetDouble()))
                            PushDouble(*fResult);
                        else
                            PushIllegalArgument();
                        break;
                    case ocCosecant         :
                        warnIfLegacyDefaultOnReached(
                            u"CSC", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeCosecant(GetDouble()));
                        break;
                    case ocSecant           :
                        warnIfLegacyDefaultOnReached(
                            u"SEC", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeSecant(GetDouble()));
                        break;
                    case ocCosecantHyp      :
                        warnIfLegacyDefaultOnReached(
                            u"CSCH", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeCosecantHyp(GetDouble()));
                        break;
                    case ocSecantHyp        :
                        warnIfLegacyDefaultOnReached(
                            u"SECH", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeSecantHyp(GetDouble()));
                        break;
                    case ocExp              :
                        warnIfLegacyDefaultOnReached(
                            u"EXP", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeExp(GetDouble()));
                        break;
                    case ocLn               :
                        warnIfLegacyDefaultOnReached(
                            u"LN", "family-local default-on math scalar reached ScInterpreter");
                        if (std::optional<double> fResult = semath::computeLn(GetDouble()))
                            PushDouble(*fResult);
                        else
                            PushIllegalArgument();
                        break;
                    case ocLog10            :
                        warnIfLegacyDefaultOnReached(
                            u"LOG10", "family-local default-on math scalar reached ScInterpreter");
                        if (std::optional<double> fResult = semath::computeLog10(GetDouble()))
                            PushDouble(*fResult);
                        else
                            PushIllegalArgument();
                        break;
                    case ocSqrt             :
                        warnIfLegacyDefaultOnReached(
                            u"SQRT", "family-local default-on math scalar reached ScInterpreter");
                        if (std::optional<double> fResult = semath::computeSqrt(GetDouble()))
                            PushDouble(*fResult);
                        else
                            PushIllegalArgument();
                        break;
                    case ocFact             :
                        warnIfLegacyStatisticalDistributionReached(u"FACT");
                        pushCalcMathValueResult(semath::evaluateFactorialValue(GetDouble()));
                        break;
                    case ocGetYear          :
                        warnIfLegacyDateFamilyReached(u"YEAR");
                        PushDouble(sedatetime::extractYear(
                            selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), GetFloor32()));
                        break;
                    case ocGetMonth         :
                        warnIfLegacyDateFamilyReached(u"MONTH");
                        PushDouble(sedatetime::extractMonth(
                            selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), GetFloor32()));
                        break;
                    case ocGetDay           :
                        warnIfLegacyDateFamilyReached(u"DAY");
                        if (std::optional<double> fDay = sedatetime::extractDay(
                                selibreoffice::toApiDateParts(mrContext.NFGetNullDate()),
                                GetFloor32()))
                            PushDouble(*fDay);
                        else
                        {
                            SetError(FormulaError::IllegalArgument);
                            PushDouble(HUGE_VAL);
                        }
                        break;
                    case ocGetDayOfWeek     :
                    {
                        warnIfLegacyDateFamilyReached(u"WEEKDAY");
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 1, 2))
                            break;
                        sal_Int16 nFlag = (nParamCount == 2) ? GetInt16() : 1;
                        const auto aResult = sedatetime::computeDayOfWeek(
                            selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), GetFloor32(),
                            nFlag);
                        if (!aResult.mbValid)
                            SetError(FormulaError::IllegalArgument);
                        PushInt(aResult.mnValue);
                    }
                    break;
                    case ocWeek             :
                    {
                        warnIfLegacyDateFamilyReached(u"WEEKNUM");
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 1, 2))
                            break;
                        sal_Int16 nFlag = (nParamCount == 1) ? 1 : GetInt16WithDefault(1);
                        if (std::optional<int> nWeek = sedatetime::computeWeekOfYear(
                                selibreoffice::toApiDateParts(mrContext.NFGetNullDate()),
                                GetFloor32(), nFlag))
                            PushInt(*nWeek);
                        else
                            PushIllegalArgument();
                    }
                    break;
                    case ocIsoWeeknum       :
                        warnIfLegacyDateFamilyReached(u"ISOWEEKNUM");
                        if (!MustHaveParamCount(GetByte(), 1))
                            break;
                        PushInt(sedatetime::computeIsoWeekOfYear(
                            selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), GetFloor32()));
                        break;
                    case ocWeeknumOOo       :
                        warnIfLegacyDateFamilyReached(u"WEEKNUM");
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        PushInt(sedatetime::computeWeeknumOOo(
                            selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), GetFloor32(),
                            GetInt16()));
                        break;
                    case ocEasterSunday     :
                    {
                        warnIfLegacyDateFamilyReached(u"EASTERSUNDAY");
                        nFuncFmtType = SvNumFormatType::DATE;
                        if (!MustHaveParamCount(GetByte(), 1))
                            break;
                        sal_Int16 nYear = GetInt16();
                        if (nGlobalError != FormulaError::NONE)
                        {
                            PushError(nGlobalError);
                            break;
                        }
                        if (nYear < 100)
                            nYear = mrContext.NFExpandTwoDigitYear(nYear);
                        if (std::optional<double> fSerial = sedatetime::computeEasterSundaySerial(
                                selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), nYear))
                            PushDouble(*fSerial);
                        else
                            PushIllegalArgument();
                    }
                    break;
                    case ocNetWorkdays      :
                    case ocNetWorkdays_MS   :
                    {
                        const bool bOOXML_Version = eOp == ocNetWorkdays_MS;
                        warnIfLegacyDateFamilyReached(
                            bOOXML_Version ? u"NETWORKDAYS.INTL" : u"NETWORKDAYS");
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 2, 4))
                            break;

                        std::vector<double> nSortArray;
                        bool bWeekendMask[7];
                        const Date& rNullDate = mrContext.NFGetNullDate();
                        sal_Int32 nNullDate = rNullDate.GetAsNormalizedDays();
                        FormulaError nErr = bOOXML_Version
                                                ? GetWeekendAndHolidayMasks_MS(
                                                      nParamCount, nNullDate, nSortArray,
                                                      bWeekendMask, false)
                                                : GetWeekendAndHolidayMasks(
                                                      nParamCount, nNullDate, nSortArray,
                                                      bWeekendMask);
                        if (nErr != FormulaError::NONE)
                        {
                            PushError(nErr);
                            break;
                        }
                        sal_Int32 nDate2 = GetFloor32();
                        sal_Int32 nDate1 = GetFloor32();
                        if (nGlobalError != FormulaError::NONE
                            || (nDate1 > SAL_MAX_INT32 - nNullDate)
                            || nDate2 > (SAL_MAX_INT32 - nNullDate))
                        {
                            PushIllegalArgument();
                            break;
                        }
                        nDate2 += nNullDate;
                        nDate1 += nNullDate;
                        const auto aHolidaySerials = selibreoffice::toApiDateSerials(nSortArray);
                        PushDouble(static_cast<double>(sedatetime::countWorkdays(
                            nDate1, nDate2, aHolidaySerials,
                            selibreoffice::toApiWeekendMask(bWeekendMask))));
                    }
                    break;
                    case ocWorkday_MS       :
                    {
                        warnIfLegacyDateFamilyReached(u"WORKDAY.INTL");
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 2, 4))
                            break;

                        nFuncFmtType = SvNumFormatType::DATE;
                        std::vector<double> nSortArray;
                        bool bWeekendMask[7];
                        const Date& rNullDate = mrContext.NFGetNullDate();
                        sal_Int32 nNullDate = rNullDate.GetAsNormalizedDays();
                        FormulaError nErr = GetWeekendAndHolidayMasks_MS(
                            nParamCount, nNullDate, nSortArray, bWeekendMask, true);
                        if (nErr != FormulaError::NONE)
                        {
                            PushError(nErr);
                            break;
                        }
                        sal_Int32 nDays = GetFloor32();
                        sal_Int32 nDate = GetFloor32();
                        if (nGlobalError != FormulaError::NONE
                            || (nDate > SAL_MAX_INT32 - nNullDate))
                        {
                            PushIllegalArgument();
                            break;
                        }
                        nDate += nNullDate;
                        if (!nDays)
                        {
                            PushDouble(static_cast<double>(nDate - nNullDate));
                            break;
                        }
                        const auto aHolidaySerials = selibreoffice::toApiDateSerials(nSortArray);
                        PushDouble(static_cast<double>(sedatetime::advanceWorkday(
                                       nDate, nDays, aHolidaySerials,
                                       selibreoffice::toApiWeekendMask(bWeekendMask))
                                   - nNullDate));
                    }
                    break;
                    case ocGetHour          :
                        warnIfLegacyDateFamilyReached(u"HOUR");
                        PushDouble(sedatetime::extractHour(GetDouble()));
                        break;
                    case ocGetMin           :
                        warnIfLegacyDateFamilyReached(u"MINUTE");
                        PushDouble(sedatetime::extractMinute(GetDouble()));
                        break;
                    case ocGetSec           :
                        warnIfLegacyDateFamilyReached(u"SECOND");
                        PushDouble(sedatetime::extractSecond(GetDouble()));
                        break;
                    case ocPlusMinus        :
                        warnIfLegacyScalarRootReached(u"UNARY_PLUS");
                        PushInt(semath::computePlusMinus(GetDouble()));
                        break;
                    case ocAbs              :
                        warnIfLegacyDefaultOnReached(
                            u"ABS", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeAbs(GetDouble()));
                        break;
                    case ocInt              :
                        warnIfLegacyDefaultOnReached(
                            u"INT", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeInt(GetDouble()));
                        break;
                    case ocEven             :
                        warnIfLegacyDefaultOnReached(
                            u"EVEN", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeEven(GetDouble()));
                        break;
                    case ocOdd              :
                        warnIfLegacyDefaultOnReached(
                            u"ODD", "family-local default-on math scalar reached ScInterpreter");
                        PushDouble(semath::computeOdd(GetDouble()));
                        break;
                    case ocPhi              :
                        PushDouble(semath::evaluateNormalDistribution(GetDouble(), 0.0, 1.0, false)
                                       .maValue);
                        break;
                    case ocGauss            :
                        PushDouble(semath::gaussValue(GetDouble()));
                        break;
                    case ocStdNormDist:
                    case ocStdNormDist_MS:
                    {
                        const bool bMicrosoftSyntax = eOp == ocStdNormDist_MS;
                        warnIfLegacyStatisticalDistributionReached(
                            bMicrosoftSyntax ? u"NORM.S.DIST" : u"NORMSDIST");
                        if (!MustHaveParamCount(GetByte(), bMicrosoftSyntax ? 2 : 1))
                            break;

                        bool bCumulative = true;
                        double fX = 0.0;
                        if (bMicrosoftSyntax)
                        {
                            bCumulative = GetBool();
                            fX = GetDouble();
                        }
                        else
                            fX = GetDouble();

                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyStdNormDist(fX, bCumulative));
                    }
                    break;
                    case ocFisher           :
                        warnIfLegacyStatisticalDistributionReached(u"FISHER");
                        pushCalcMathValueResult(semath::fisherTransform(GetDouble()));
                        break;
                    case ocFisherInv        :
                        PushDouble(semath::inverseFisherTransform(GetDouble()));
                        break;
                    case ocIsEmpty          : pushLegacyIsEmpty();          break;
                    case ocIsString         : pushLegacyIsString(false);    break;
                    case ocIsNonString      : pushLegacyIsString(true);     break;
                    case ocIsLogical        : pushLegacyIsLogical();        break;
                    case ocType             : ScType();                 break;
                    case ocCell             : ScCell();                     break;
                    case ocIsRef            : pushLegacyIsRef();            break;
                    case ocIsValue          : pushLegacyIsValue();          break;
                    case ocIsFormula        : pushLegacyIsFormula();        break;
                    case ocFormula          : pushLegacyFormulaText();      break;
                    case ocIsNA             : pushLegacyIsNA();             break;
                    case ocIsErr            : pushLegacyIsErrLike(u"ISERR", false); break;
                    case ocIsError          : pushLegacyIsErrLike(u"ISERROR", true); break;
                    case ocIsEven           : pushLegacyIsEvenOdd(false);   break;
                    case ocIsOdd            : pushLegacyIsEvenOdd(true);    break;
                    case ocN                : ScN();                    break;
                    case ocGetDateValue     :
                        pushLegacyDateOrTimeValue(
                            "DATEVALUE", SvNumFormatType::DATE,
                            [&](const OUString& rInputString) {
                                return setextparseexec::evaluateDateValue(
                                    mrDoc, mrContext, rInputString);
                            });
                        break;
                    case ocGetTimeValue     :
                        pushLegacyDateOrTimeValue(
                            "TIMEVALUE", SvNumFormatType::TIME,
                            [&](const OUString& rInputString) {
                                return setextparseexec::evaluateTimeValue(
                                    mrDoc, mrContext, rInputString);
                            });
                        break;
                    case ocCode             : pushLegacyCode();             break;
                    case ocTrim             : pushLegacyTrim();          break;
                    case ocUpper            :
                        pushLegacyUnaryTextTransform(
                            u"UPPER", [&](const OUString& rText) {
                                return selibreoffice::uppercase(
                                    ScGlobal::getCharClass(), rText);
                            });
                        break;
                    case ocProper           :
                        pushLegacyUnaryTextTransform(
                            u"PROPER", [&](const OUString& rText) {
                                return selibreoffice::propercase(
                                    ScGlobal::getCharClass(), rText);
                            });
                        break;
                    case ocLower            :
                        pushLegacyUnaryTextTransform(
                            u"LOWER", [&](const OUString& rText) {
                                return selibreoffice::lowercase(
                                    ScGlobal::getCharClass(), rText);
                            });
                        break;
                    case ocLen              : pushLegacyLen();              break;
                    case ocT                : pushLegacyT();                break;
                    case ocClean            : pushLegacyClean();            break;
                    case ocValue            : pushLegacyValue();        break;
                    case ocNumberValue      : pushLegacyNumberValue();  break;
                    case ocChar             : pushLegacyChar();             break;
                    case ocArcTan2          :
                        warnIfLegacyDefaultOnReached(
                            u"ATAN2", "family-local default-on math scalar reached ScInterpreter");
                        if (MustHaveParamCount(GetByte(), 2))
                        {
                            double fVal2 = GetDouble();
                            double fVal1 = GetDouble();
                            PushDouble(semath::computeArcTan2(fVal2, fVal1));
                        }
                        break;
                    case ocMod              :
                    {
                        warnIfLegacyDefaultOnReached(
                            u"MOD", "family-local default-on math scalar reached ScInterpreter");
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        double fDenom = GetDouble();
                        if (fDenom == 0.0)
                        {
                            PushError(FormulaError::DivisionByZero);
                            break;
                        }
                        double fNum = GetDouble();
                        if (std::optional<double> fResult = semath::computeMod(fNum, fDenom))
                            PushDouble(*fResult);
                        else
                            PushError(FormulaError::NoValue);
                    }
                    break;
                    case ocPower            :
                        if (MustHaveParamCount(GetByte(), 2))
                            ScPow();
                        break;
                    case ocRound            :
                        warnIfLegacyDefaultOnReached(
                            u"ROUND", "family-local default-on round reached ScInterpreter");
                        RoundNumber(rtl_math_RoundingMode_Corrected);
                        break;
                    case ocRoundSig         :
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"ROUNDSIG",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on ROUNDSIG reached ScInterpreter");
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        {
                            const double fDigits = GetDouble();
                            const double fValue = GetDouble();
                            pushValueResult(semath::evaluateRoundSigValue(fValue, fDigits));
                        }
                        break;
                    case ocRoundUp          :
                        warnIfLegacyDefaultOnReached(
                            u"ROUNDUP", "family-local default-on round reached ScInterpreter");
                        RoundNumber(rtl_math_RoundingMode_Up);
                        break;
                    case ocTrunc            :
                        warnIfLegacyDefaultOnReached(
                            u"TRUNC", "family-local default-on round reached ScInterpreter");
                        RoundNumber(rtl_math_RoundingMode_Down);
                        break;
                    case ocRoundDown        :
                        warnIfLegacyDefaultOnReached(
                            u"ROUNDDOWN", "family-local default-on round reached ScInterpreter");
                        RoundNumber(rtl_math_RoundingMode_Down);
                        break;
                    case ocCeil             :
                    case ocCeil_Math        :
                    {
                        const bool bODFF = eOp == ocCeil;
                        warnIfLegacyDefaultOnReached(
                            bODFF ? u"CEILING" : u"CEILING.MATH",
                            "family-local default-on math scalar reached ScInterpreter");
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 1, 3))
                            break;
                        bool bAbs = nParamCount == 3 && GetBool();
                        double fDec;
                        double fVal;
                        if (nParamCount == 1)
                        {
                            fVal = GetDouble();
                            fDec = (fVal < 0 ? -1 : 1);
                        }
                        else
                        {
                            bool bArgumentMissing = IsMissing();
                            fDec = GetDouble();
                            fVal = GetDouble();
                            if (bArgumentMissing)
                                fDec = (fVal < 0 ? -1 : 1);
                        }
                        if (fVal == 0 || fDec == 0.0)
                            PushInt(0);
                        else if (std::optional<double> fResult
                                 = semath::computeCeiling(fVal, fDec, bAbs, bODFF))
                            PushDouble(*fResult);
                        else
                            PushIllegalArgument();
                    }
                    break;
                    case ocCeil_MS          :
                        warnIfLegacyDefaultOnReached(
                            u"COM.MICROSOFT.CEILING",
                            "family-local default-on math scalar reached ScInterpreter");
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        {
                            double fDec = GetDouble();
                            double fVal = GetDouble();
                            if (std::optional<double> fResult
                                = semath::computeCeilingMs(fVal, fDec))
                                PushDouble(*fResult);
                            else
                                PushIllegalArgument();
                        }
                        break;
                    case ocCeil_Precise     :
                    case ocCeil_ISO         :
                    {
                        warnIfLegacyDefaultOnReached(
                            eOp == ocCeil_Precise ? u"CEILING.PRECISE" : u"ISO.CEILING",
                            "family-local default-on math scalar reached ScInterpreter");
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 1, 2))
                            break;
                        double fDec;
                        double fVal;
                        if (nParamCount == 1)
                        {
                            fVal = GetDouble();
                            fDec = 1.0;
                        }
                        else
                        {
                            fDec = std::abs(GetDoubleWithDefault(1.0));
                            fVal = GetDouble();
                        }
                        if (fDec == 0.0 || fVal == 0.0)
                            PushInt(0);
                        else
                            PushDouble(semath::computeCeilingPrecise(fVal, fDec));
                    }
                    break;
                    case ocFloor            :
                    case ocFloor_Math       :
                    {
                        const bool bODFF = eOp == ocFloor;
                        warnIfLegacyDefaultOnReached(
                            bODFF ? u"FLOOR" : u"FLOOR.MATH",
                            "family-local default-on math scalar reached ScInterpreter");
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 1, 3))
                            break;
                        bool bAbs = (nParamCount == 3 && GetBool());
                        double fDec;
                        double fVal;
                        if (nParamCount == 1)
                        {
                            fVal = GetDouble();
                            fDec = (fVal < 0 ? -1 : 1);
                        }
                        else
                        {
                            bool bArgumentMissing = IsMissing();
                            fDec = GetDouble();
                            fVal = GetDouble();
                            if (bArgumentMissing)
                                fDec = (fVal < 0 ? -1 : 1);
                        }
                        if (fDec == 0.0 || fVal == 0.0)
                            PushInt(0);
                        else if (std::optional<double> fResult
                                 = semath::computeFloor(fVal, fDec, bAbs, bODFF))
                            PushDouble(*fResult);
                        else
                            PushIllegalArgument();
                    }
                    break;
                    case ocFloor_MS         :
                        warnIfLegacyDefaultOnReached(
                            u"COM.MICROSOFT.FLOOR",
                            "family-local default-on math scalar reached ScInterpreter");
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        {
                            double fDec = GetDouble();
                            double fVal = GetDouble();
                            if (std::optional<double> fResult
                                = semath::computeFloorMs(fVal, fDec))
                                PushDouble(*fResult);
                            else
                                PushIllegalArgument();
                        }
                        break;
                    case ocFloor_Precise    :
                        warnIfLegacyDefaultOnReached(
                            u"FLOOR.PRECISE",
                            "family-local default-on math scalar reached ScInterpreter");
                        {
                            sal_uInt8 nParamCount = GetByte();
                            if (!MustHaveParamCount(nParamCount, 1, 2))
                                break;
                            double fDec = nParamCount == 1 ? 1.0 : std::abs(GetDoubleWithDefault(1.0));
                            double fVal = GetDouble();
                            if (fDec == 0.0 || fVal == 0.0)
                                PushInt(0);
                            else
                                PushDouble(semath::computeFloorPrecise(fVal, fDec));
                        }
                        break;
                    case ocSumProduct:
                        warnIfLegacyNumericAggregateReached(u"SUMPRODUCT");
                        seinterpcompatdispatch::Dispatcher::aggregateSumProduct(*this);
                        break;
                    case ocSumSQ:
                        warnIfLegacyNumericAggregateReached(u"SUMSQ");
                        seinterpcompatdispatch::Dispatcher::aggregateSumSq(*this);
                        break;
                    case ocSumX2MY2         : ScSumX2MY2();                 break;
                    case ocSumX2DY2         : ScSumX2DY2();                 break;
                    case ocSumXMY2          : ScSumXMY2();                  break;
                    case ocRawSubtract      : pushLegacyRawSubtract();      break;
                    case ocLog              :
                    {
                        warnIfLegacyDefaultOnReached(
                            u"LOG", "family-local default-on math scalar reached ScInterpreter");
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 1, 2))
                            break;
                        double fBase = nParamCount == 2 ? GetDouble() : 10.0;
                        double fVal = GetDouble();
                        if (std::optional<double> fResult = semath::computeLog(fVal, fBase))
                            PushDouble(*fResult);
                        else
                            PushIllegalArgument();
                    }
                    break;
                    case ocGCD              : pushLegacyGcdOrLcm(u"GCD", false); break;
                    case ocLCM              : pushLegacyGcdOrLcm(u"LCM", true); break;
                    case ocGetDate          :
                        warnIfLegacyDateFamilyReached(u"DATE");
                        nFuncFmtType = SvNumFormatType::DATE;
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        {
                            sal_Int16 nDay = GetInt16();
                            sal_Int16 nMonth = GetInt16();
                            if (IsMissing())
                                SetError(FormulaError::ParameterExpected);
                            sal_Int16 nYear = GetInt16();
                            if (nGlobalError != FormulaError::NONE || nYear < 0)
                                PushIllegalArgument();
                            else
                                PushDouble(GetDateSerial(nYear, nMonth, nDay, false));
                        }
                        break;
                    case ocGetTime          :
                        warnIfLegacyDateFamilyReached(u"TIME");
                        nFuncFmtType = SvNumFormatType::TIME;
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        {
                            double fSec = GetDouble();
                            double fMin = GetDouble();
                            double fHour = GetDouble();
                            if (std::optional<double> fTime
                                = sedatetime::makeTimeSerial(fHour, fMin, fSec))
                                PushDouble(*fTime);
                            else
                                PushIllegalArgument();
                        }
                        break;
                    case ocGetDiffDate      :
                        warnIfLegacyDateFamilyReached(u"DAYS");
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        {
                            double fDate2 = GetDouble();
                            double fDate1 = GetDouble();
                            PushDouble(sedatetime::computeDiffDate(fDate1, fDate2));
                        }
                        break;
                    case ocGetDiffDate360   :
                    {
                        warnIfLegacyDateFamilyReached(u"DAYS360");
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 2, 3))
                            break;
                        bool bFlag = nParamCount == 3 && GetBool();
                        sal_Int32 nDate2 = GetFloor32();
                        sal_Int32 nDate1 = GetFloor32();
                        if (nGlobalError != FormulaError::NONE)
                            PushError(nGlobalError);
                        else
                            PushDouble(sedatetime::computeDiffDate360(
                                selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), nDate1,
                                nDate2, bFlag));
                    }
                    break;
                    case ocGetDateDif       :
                        warnIfLegacyDateFamilyReached(u"DATEDIF");
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        {
                            OUString aInterval = GetString().getString();
                            sal_Int32 nDate2 = GetFloor32();
                            sal_Int32 nDate1 = GetFloor32();
                            if (nGlobalError != FormulaError::NONE)
                            {
                                PushError(nGlobalError);
                                break;
                            }
                            if (std::optional<double> fResult = sedatetime::computeDateDif(
                                    selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), nDate1,
                                    nDate2, selibreoffice::toApiString(aInterval)))
                                PushDouble(*fResult);
                            else
                                PushIllegalArgument();
                        }
                        break;
                    case ocMin:
                        warnIfLegacyStatisticalAggregateReached(u"MIN");
                        seinterpcompatdispatch::Dispatcher::aggregateMin(*this, false);
                        break;
                    case ocMinA:
                        warnIfLegacyStatisticalAggregateReached(u"MINA");
                        seinterpcompatdispatch::Dispatcher::aggregateMin(*this, true);
                        break;
                    case ocMax:
                        warnIfLegacyStatisticalAggregateReached(u"MAX");
                        seinterpcompatdispatch::Dispatcher::aggregateMax(*this, false);
                        break;
                    case ocMaxA:
                        warnIfLegacyStatisticalAggregateReached(u"MAXA");
                        seinterpcompatdispatch::Dispatcher::aggregateMax(*this, true);
                        break;
                    case ocSum:
                        warnIfLegacyNumericAggregateReached(u"SUM");
                        seinterpcompatdispatch::Dispatcher::aggregateSum(*this);
                        break;
                    case ocProduct:
                        warnIfLegacyNumericAggregateReached(u"PRODUCT");
                        seinterpcompatdispatch::Dispatcher::aggregateProduct(*this);
                        break;
                    case ocNPV              : pushLegacyNpv();          break;
                    case ocIRR              : pushLegacyIrr();          break;
                    case ocMIRR             : pushLegacyMirr();         break;
                    case ocISPMT            :
                    {
                        warnIfLegacyRateFamilyReached(u"ISPMT");
                        if (!MustHaveParamCount(GetByte(), 4))
                            break;
                        double fInvest = GetDouble();
                        double fTotal = GetDouble();
                        double fPeriod = GetDouble();
                        double fRate = GetDouble();
                        pushValueResult(sefinance::evaluateInterestSchedulePayment(
                            fRate, fPeriod, fTotal, fInvest));
                    }
                    break;
                    case ocAverage:
                        warnIfLegacyNumericAggregateReached(u"AVERAGE");
                        seinterpcompatdispatch::Dispatcher::aggregateAverage(*this, false);
                        break;
                    case ocAverageA:
                        warnIfLegacyNumericAggregateReached(u"AVERAGEA");
                        seinterpcompatdispatch::Dispatcher::aggregateAverage(*this, true);
                        break;
                    case ocCount            : IterateParameters(ifCOUNT);   break;
                    case ocCount2           : IterateParameters(ifCOUNT2);  break;
                    case ocVar              :
                    case ocVarS:
                        warnIfLegacyStatisticalAggregateReached(u"VAR.S");
                        seinterpcompatdispatch::Dispatcher::aggregateVar(*this, false);
                        break;
                    case ocVarA:
                        warnIfLegacyStatisticalAggregateReached(u"VARA");
                        seinterpcompatdispatch::Dispatcher::aggregateVar(*this, true);
                        break;
                    case ocVarP             :
                    case ocVarP_MS:
                        warnIfLegacyStatisticalAggregateReached(u"VAR.P");
                        seinterpcompatdispatch::Dispatcher::aggregateVarP(*this, false);
                        break;
                    case ocVarPA:
                        warnIfLegacyStatisticalAggregateReached(u"VARPA");
                        seinterpcompatdispatch::Dispatcher::aggregateVarP(*this, true);
                        break;
                    case ocStDev            :
                    case ocStDevS:
                        warnIfLegacyStatisticalAggregateReached(u"STDEV.S");
                        seinterpcompatdispatch::Dispatcher::aggregateStDev(*this, false);
                        break;
                    case ocStDevA:
                        warnIfLegacyStatisticalAggregateReached(u"STDEVA");
                        seinterpcompatdispatch::Dispatcher::aggregateStDev(*this, true);
                        break;
                    case ocStDevP           :
                    case ocStDevP_MS:
                        warnIfLegacyStatisticalAggregateReached(u"STDEV.P");
                        seinterpcompatdispatch::Dispatcher::aggregateStDevP(*this, false);
                        break;
                    case ocStDevPA:
                        warnIfLegacyStatisticalAggregateReached(u"STDEVPA");
                        seinterpcompatdispatch::Dispatcher::aggregateStDevP(*this, true);
                        break;
                    case ocPV               :
                    {
                        warnIfLegacyRateFamilyReached(u"PV");
                        nFuncFmtType = SvNumFormatType::CURRENCY;
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 3, 5))
                            break;
                        bool bPayInAdvance = nParamCount == 5 && GetBool();
                        double fFv = nParamCount >= 4 ? GetDouble() : 0.0;
                        double fPmt = GetDouble();
                        double fNper = GetDouble();
                        double fRate = GetDouble();
                        pushValueResult(sefinance::evaluatePresentValue(
                            fRate, fNper, fPmt, fFv, bPayInAdvance));
                    }
                    break;
                    case ocSYD              :
                    {
                        warnIfLegacyRateFamilyReached(u"SYD");
                        nFuncFmtType = SvNumFormatType::CURRENCY;
                        if (!MustHaveParamCount(GetByte(), 4))
                            break;
                        double fPer = GetDouble();
                        double fLife = GetDouble();
                        double fSalvage = GetDouble();
                        double fCost = GetDouble();
                        pushValueResult(sefinance::evaluateSumOfYearsDepreciation(
                            fCost, fSalvage, fLife, fPer));
                    }
                    break;
                    case ocDDB              :
                    {
                        warnIfLegacyRateFamilyReached(u"DDB");
                        nFuncFmtType = SvNumFormatType::CURRENCY;
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 4, 5))
                            break;
                        double fFactor = nParamCount == 5 ? GetDouble() : 2.0;
                        double fPeriod = GetDouble();
                        double fLife = GetDouble();
                        double fSalvage = GetDouble();
                        double fCost = GetDouble();
                        pushValueResult(sefinance::evaluateDoubleDecliningBalance(
                            fCost, fSalvage, fLife, fPeriod, fFactor));
                    }
                    break;
                    case ocDB               :
                    {
                        warnIfLegacyRateFamilyReached(u"DB");
                        nFuncFmtType = SvNumFormatType::CURRENCY;
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 4, 5))
                            break;
                        double fMonths = nParamCount == 4 ? 12.0 : ::rtl::math::approxFloor(GetDouble());
                        double fPeriod = GetDouble();
                        double fLife = GetDouble();
                        double fSalvage = GetDouble();
                        double fCost = GetDouble();
                        pushValueResult(sefinance::evaluateFixedDecliningBalance(
                            fCost, fSalvage, fLife, fPeriod, fMonths));
                    }
                    break;
                    case ocVBD              :
                    {
                        warnIfLegacyRateFamilyReached(u"VDB");
                        nFuncFmtType = SvNumFormatType::CURRENCY;
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 5, 7))
                            break;
                        bool bNoSwitch = nParamCount == 7 && GetBool();
                        double fFactor = nParamCount >= 6 ? GetDouble() : 2.0;
                        double fEnd = GetDouble();
                        double fStart = GetDouble();
                        double fLife = GetDouble();
                        double fSalvage = GetDouble();
                        double fCost = GetDouble();
                        pushValueResult(sefinance::evaluateVariableDecliningBalance(
                            fCost, fSalvage, fLife, fStart, fEnd, fFactor, bNoSwitch));
                    }
                    break;
                    case ocPDuration        :
                    {
                        warnIfLegacyRateFamilyReached(u"PDURATION");
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        double fFuture = GetDouble();
                        double fPresent = GetDouble();
                        double fRate = GetDouble();
                        pushValueResult(sefinance::evaluatePaybackDuration(
                            fRate, fPresent, fFuture));
                    }
                    break;
                    case ocSLN              :
                    {
                        warnIfLegacyRateFamilyReached(u"SLN");
                        nFuncFmtType = SvNumFormatType::CURRENCY;
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        double fLife = GetDouble();
                        double fSalvage = GetDouble();
                        double fCost = GetDouble();
                        pushValueResult(sefinance::evaluateStraightLineDepreciation(
                            fCost, fSalvage, fLife));
                    }
                    break;
                    case ocPMT              :
                    {
                        warnIfLegacyRateFamilyReached(u"PMT");
                        nFuncFmtType = SvNumFormatType::CURRENCY;
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 3, 5))
                            break;
                        bool bPayInAdvance = nParamCount == 5 && GetBool();
                        double fFv = nParamCount >= 4 ? GetDouble() : 0.0;
                        double fPv = GetDouble();
                        double fNper = GetDouble();
                        double fRate = GetDouble();
                        pushValueResult(sefinance::evaluatePayment(
                            fRate, fNper, fPv, fFv, bPayInAdvance));
                    }
                    break;
                    case ocColumns          :
                        if (!tryPlanEngineSpanCount(serpn::SpanCountKind::Columns))
                            ScColumns();
                        break;
                    case ocRows             :
                        if (!tryPlanEngineSpanCount(serpn::SpanCountKind::Rows))
                            ScRows();
                        break;
                    case ocSheets           :
                        if (!tryPlanEngineSpanCount(serpn::SpanCountKind::Sheets))
                            ScSheets();
                        break;
                    case ocColumn           :
                        if (!tryPlanEngineAxisOrdinal(serpn::AxisOrdinalKind::Column))
                            ScColumn();
                        break;
                    case ocRow              :
                        if (!tryPlanEngineAxisOrdinal(serpn::AxisOrdinalKind::Row))
                            ScRow();
                        break;
                    case ocSheet            :
                        if (!tryPlanEngineAxisOrdinal(serpn::AxisOrdinalKind::Sheet))
                            ScSheet();
                        break;
                    case ocRRI              :
                    {
                        warnIfLegacyRateFamilyReached(u"RRI");
                        nFuncFmtType = SvNumFormatType::PERCENT;
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        double fFutureValue = GetDouble();
                        double fPresentValue = GetDouble();
                        double fNrOfPeriods = GetDouble();
                        pushValueResult(sefinance::evaluateGrowthRateOverPeriods(
                            fNrOfPeriods, fPresentValue, fFutureValue));
                    }
                    break;
                    case ocFV               :
                    {
                        warnIfLegacyRateFamilyReached(u"FV");
                        nFuncFmtType = SvNumFormatType::CURRENCY;
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 3, 5))
                            break;
                        bool bPayInAdvance = nParamCount == 5 && GetBool();
                        double fPv = nParamCount >= 4 ? GetDouble() : 0.0;
                        double fPmt = GetDouble();
                        double fNper = GetDouble();
                        double fRate = GetDouble();
                        pushValueResult(sefinance::evaluateFutureValue(
                            fRate, fNper, fPmt, fPv, bPayInAdvance));
                    }
                    break;
                    case ocNper             :
                    {
                        warnIfLegacyRateFamilyReached(u"NPER");
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 3, 5))
                            break;
                        bool bPayInAdvance = nParamCount == 5 && GetBool();
                        double fFV = nParamCount >= 4 ? GetDouble() : 0.0;
                        double fPV = GetDouble();
                        double fPmt = GetDouble();
                        double fRate = GetDouble();
                        pushValueResult(sefinance::evaluatePeriodsForFutureValue(
                            fRate, fPmt, fPV, fFV, bPayInAdvance));
                    }
                    break;
                    case ocRate             :
                    {
                        warnIfLegacyRateFamilyReached(u"RATE");
                        nFuncFmtType = SvNumFormatType::PERCENT;
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 3, 6))
                            break;
                        double fGuess = nParamCount == 6 ? GetDouble() : 0.1;
                        bool bDefaultGuess = nParamCount != 6;
                        bool bPayType = nParamCount >= 5 && GetBool();
                        double fFv = nParamCount >= 4 ? GetDouble() : 0.0;
                        double fPv = GetDouble();
                        double fPayment = GetDouble();
                        double fNper = GetDouble();
                        if (fNper <= 0.0)
                        {
                            PushIllegalArgument();
                            break;
                        }
                        const semath::FinancialRateResult aResult = semath::solveRate(
                            fNper, fPayment, fPv, fFv, bPayType, fGuess, bDefaultGuess);
                        if (!aResult.mbConverged)
                            SetError(FormulaError::NoConvergence);
                        PushDouble(aResult.mfRate);
                    }
                    break;
                    case ocFilterXML        : ScFilterXML();            break;
                    case ocWebservice       : ScWebservice();           break;
                    case ocEncodeURL        : pushLegacyEncodeUrl();    break;
                    case ocColor            : pushLegacyColor();            break;
                    case ocErf_MS           :
                        if (MustHaveParamCount(GetByte(), 1))
                        {
                            warnIfLegacyStatisticalDistributionReached(u"ERF");
                            pushValueResult(semath::evaluateErrorFunction(GetDouble()));
                        }
                        break;
                    case ocErfc_MS          :
                        if (MustHaveParamCount(GetByte(), 1))
                        {
                            warnIfLegacyStatisticalDistributionReached(u"ERFC");
                            pushValueResult(
                                semath::evaluateComplementaryErrorFunction(GetDouble()));
                        }
                        break;
                    case ocIpmt             :
                    {
                        warnIfLegacyRateFamilyReached(u"IPMT");
                        nFuncFmtType = SvNumFormatType::CURRENCY;
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 4, 6))
                            break;
                        bool bPayInAdvance = nParamCount == 6 && GetBool();
                        double fFv = nParamCount >= 5 ? GetDouble() : 0.0;
                        double fPv = GetDouble();
                        double fNper = GetDouble();
                        double fPer = GetDouble();
                        double fRate = GetDouble();
                        pushValueResult(sefinance::evaluateInterestPayment(
                            fRate, fPer, fNper, fPv, fFv, bPayInAdvance));
                    }
                    break;
                    case ocPpmt             :
                    {
                        warnIfLegacyRateFamilyReached(u"PPMT");
                        nFuncFmtType = SvNumFormatType::CURRENCY;
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 4, 6))
                            break;
                        bool bPayInAdvance = nParamCount == 6 && GetBool();
                        double fFv = nParamCount >= 5 ? GetDouble() : 0.0;
                        double fPv = GetDouble();
                        double fNper = GetDouble();
                        double fPer = GetDouble();
                        double fRate = GetDouble();
                        pushValueResult(sefinance::evaluatePrincipalPayment(
                            fRate, fPer, fNper, fPv, fFv, bPayInAdvance));
                    }
                    break;
                    case ocCumIpmt          :
                    {
                        warnIfLegacyRateFamilyReached(u"CUMIPMT");
                        nFuncFmtType = SvNumFormatType::CURRENCY;
                        if (!MustHaveParamCount(GetByte(), 6))
                            break;
                        double fFlag = GetDoubleWithDefault(-1.0);
                        double fEnd = ::rtl::math::approxFloor(GetDouble());
                        double fStart = ::rtl::math::approxFloor(GetDouble());
                        double fPv = GetDouble();
                        double fNper = GetDouble();
                        double fRate = GetDouble();
                        pushValueResult(sefinance::evaluateCumulativeInterest(
                            fRate, fNper, fPv, fStart, fEnd, static_cast<bool>(fFlag)));
                    }
                    break;
                    case ocCumPrinc         :
                    {
                        warnIfLegacyRateFamilyReached(u"CUMPRINC");
                        nFuncFmtType = SvNumFormatType::CURRENCY;
                        if (!MustHaveParamCount(GetByte(), 6))
                            break;
                        double fFlag = GetDoubleWithDefault(-1.0);
                        double fEnd = ::rtl::math::approxFloor(GetDouble());
                        double fStart = ::rtl::math::approxFloor(GetDouble());
                        double fPv = GetDouble();
                        double fNper = GetDouble();
                        double fRate = GetDouble();
                        pushValueResult(sefinance::evaluateCumulativePrincipal(
                            fRate, fNper, fPv, fStart, fEnd, static_cast<bool>(fFlag)));
                    }
                    break;
                    case ocEffect           :
                    {
                        warnIfLegacyRateFamilyReached(u"EFFECT");
                        nFuncFmtType = SvNumFormatType::PERCENT;
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        double fPeriods = ::rtl::math::approxFloor(GetDouble());
                        double fNominal = GetDouble();
                        pushValueResult(sefinance::evaluateEffectiveAnnualRate(fNominal, fPeriods));
                    }
                    break;
                    case ocNominal          :
                    {
                        warnIfLegacyRateFamilyReached(u"NOMINAL");
                        nFuncFmtType = SvNumFormatType::PERCENT;
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        double fPeriods = ::rtl::math::approxFloor(GetDouble());
                        double fEffective = GetDouble();
                        pushValueResult(sefinance::evaluateNominal(fEffective, fPeriods));
                    }
                    break;
                    case ocSubTotal         : ScSubTotal();                 break;
                    case ocAggregate:
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"AGGREGATE",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on AGGREGATE reached ScInterpreter");
                        seinterpcompatdispatch::Dispatcher::aggregateFunction(*this);
                        break;
                    case ocDBSum            :
                        if (!tryPlanEngineDatabaseAggregate(
                                sequery::CriteriaAggregateKind::Sum))
                            ScDBSum();
                        break;
                    case ocDBCount          : ScDBCount();                  break;
                    case ocDBCount2         : ScDBCount2();                 break;
                    case ocDBAverage        :
                        if (!tryPlanEngineDatabaseAggregate(
                                sequery::CriteriaAggregateKind::Average))
                            ScDBAverage();
                        break;
                    case ocDBGet            : ScDBGet();                    break;
                    case ocDBMax            :
                        if (!tryPlanEngineDatabaseAggregate(
                                sequery::CriteriaAggregateKind::Max))
                            ScDBMax();
                        break;
                    case ocDBMin            :
                        if (!tryPlanEngineDatabaseAggregate(
                                sequery::CriteriaAggregateKind::Min))
                            ScDBMin();
                        break;
                    case ocDBProduct        : ScDBProduct();                break;
                    case ocDBStdDev         : ScDBStdDev();                 break;
                    case ocDBStdDevP        : ScDBStdDevP();                break;
                    case ocDBVar            : ScDBVar();                    break;
                    case ocDBVarP           : ScDBVarP();                   break;
                    case ocIndirect         :
                        if (!tryPlanEngineIndirect())
                            ScIndirect();
                        break;
                    case ocAddress          :
                        if (!tryPlanEngineAddress())
                            ScAddressFunc();
                        break;
                    case ocMatch:
                    {
                        warnIfLegacyDispatchReached(
                            "literal-only hard-routed", u"MATCH",
                            [](std::u16string_view rFormula) {
                                return setaileval::isHardRoutedFormula(rFormula);
                            },
                            "hard-routed MATCH reached ScInterpreter");
                        ScMatchOp(false);
                    }
                    break;
                    case ocXMatch:
                    {
                        warnIfLegacyDispatchReached(
                            "literal-only hard-routed", u"XMATCH",
                            [](std::u16string_view rFormula) {
                                return setaileval::isHardRoutedFormula(rFormula);
                            },
                            "hard-routed XMATCH reached ScInterpreter");
                        ScMatchOp(true);
                    }
                    break;
                    case ocCountEmptyCells  : ScCountEmptyCells();      break;
                    case ocCountIf          :
                        if (!tryPlanEngineCountIf())
                            ScCountIf();
                        break;
                    case ocSumIf            :
                        if (!tryPlanEngineSumIf())
                            IterateParametersIf(ifSUMIF);
                        break;
                    case ocAverageIf        :
                        if (!tryPlanEngineAverageIf())
                            IterateParametersIf(ifAVERAGEIF);
                        break;
                    case ocSumIfs:
                    {
                        if (tryPlanEngineMultiCriterionAggregate(
                                sequery::CriteriaAggregateKind::Sum, true))
                            break;
                        const sal_uInt8 nParamCount = GetByte();
                        if (nParamCount < 3 || (nParamCount % 2 != 1))
                            PushError(FormulaError::ParameterExpected);
                        else
                            IterateParametersIfs(
                                [](const sc::ParamIfsResult& rRes) { return rRes.mfSum.get(); });
                    }
                    break;
                    case ocAverageIfs:
                    {
                        if (tryPlanEngineMultiCriterionAggregate(
                                sequery::CriteriaAggregateKind::Average, true))
                            break;
                        const sal_uInt8 nParamCount = GetByte();
                        if (nParamCount < 3 || (nParamCount % 2 != 1))
                            PushError(FormulaError::ParameterExpected);
                        else
                            IterateParametersIfs([](const sc::ParamIfsResult& rRes) {
                                return sc::div(rRes.mfSum.get(), rRes.mfCount);
                            });
                    }
                    break;
                    case ocCountIfs:
                    {
                        if (tryPlanEngineMultiCriterionAggregate(
                                sequery::CriteriaAggregateKind::Count, false))
                            break;
                        const sal_uInt8 nParamCount = GetByte();
                        if (nParamCount < 2 || (nParamCount % 2 != 0))
                            PushError(FormulaError::ParameterExpected);
                        else
                            IterateParametersIfs(
                                [](const sc::ParamIfsResult& rRes) { return rRes.mfCount; });
                    }
                    break;
                    case ocLookup           : ScLookup();               break;
                    case ocVLookup:
                        warnIfLegacyDispatchReached(
                            "literal-only hard-routed", u"VLOOKUP",
                            [](std::u16string_view rFormula) {
                                return setaileval::isHardRoutedFormula(rFormula);
                            },
                            "hard-routed VLOOKUP reached ScInterpreter");
                        CalculateLookup(false);
                        break;
                    case ocXLookup          : ScXLookup();              break;
                    case ocHLookup:
                        warnIfLegacyDispatchReached(
                            "literal-only hard-routed", u"HLOOKUP",
                            [](std::u16string_view rFormula) {
                                return setaileval::isHardRoutedFormula(rFormula);
                            },
                            "hard-routed HLOOKUP reached ScInterpreter");
                        CalculateLookup(true);
                        break;
                    case ocIndex            :
                        if (!tryPlanEngineIndex())
                            ScIndex();
                        break;
                    case ocMultiArea        : ScMultiArea();                break;
                    case ocOffset           :
                        if (!tryPlanEngineOffset())
                            ScOffset();
                        break;
                    case ocAreas            :
                        if (!tryPlanEngineAreaCount())
                            ScAreas();
                        break;
                    case ocCurrency         : pushLegacyCurrency();     break;
                    case ocReplace          : pushLegacyReplace();      break;
                    case ocFixed            : pushLegacyFixed();        break;
                    case ocFind             : pushLegacyFind();         break;
                    case ocExact            : pushLegacyExact();            break;
                    case ocLeft             : pushLegacyLeftRight(false);   break;
                    case ocRight            : pushLegacyLeftRight(true);    break;
                    case ocSearch           : pushLegacySearch();       break;
                    case ocMid              : pushLegacyMid();          break;
                    case ocText             : pushLegacyText();         break;
                    case ocSubstitute       : pushLegacySubstitute();   break;
                    case ocRegex            : pushLegacyRegex();        break;
                    case ocRept             : pushLegacyRept();         break;
                    case ocConcat           : pushLegacyConcat();       break;
                    case ocConcat_MS        : pushLegacyConcatMs();         break;
                    case ocTextJoin_MS      : pushLegacyTextJoinMs();   break;
                    case ocIfs_MS           :
                        if (!tryPlanEngineIfs())
                            pushLegacyIfs();
                        break;
                    case ocSwitch_MS        :
                        if (!tryPlanEngineSwitch())
                            pushLegacySwitch();
                        break;
                    case ocMinIfs_MS:
                    {
                        if (tryPlanEngineMultiCriterionAggregate(
                                sequery::CriteriaAggregateKind::Min, true))
                            break;
                        const sal_uInt8 nParamCount = GetByte();
                        if (nParamCount < 3 || (nParamCount % 2 != 1))
                            PushError(FormulaError::ParameterExpected);
                        else
                            IterateParametersIfs([](const sc::ParamIfsResult& rRes) {
                                return (rRes.mfMin < std::numeric_limits<double>::max())
                                           ? rRes.mfMin
                                           : 0.0;
                            });
                    }
                    break;
                    case ocMaxIfs_MS:
                    {
                        if (tryPlanEngineMultiCriterionAggregate(
                                sequery::CriteriaAggregateKind::Max, true))
                            break;
                        const sal_uInt8 nParamCount = GetByte();
                        if (nParamCount < 3 || (nParamCount % 2 != 1))
                            PushError(FormulaError::ParameterExpected);
                        else
                            IterateParametersIfs([](const sc::ParamIfsResult& rRes) {
                                return (rRes.mfMax > std::numeric_limits<double>::lowest())
                                           ? rRes.mfMax
                                           : 0.0;
                            });
                    }
                    break;
                    case ocMatValue         : ScMatValue();                 break;
                    case ocMatrixUnit       :
                        if (!tryPlanEngineIdentityMatrix())
                            ScEMat();
                        break;
                    case ocMatDet:
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"MDETERM",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on MDETERM reached ScInterpreter");
                        seinterpcompatdispatch::Dispatcher::matrixDeterminant(*this);
                        break;
                    case ocMatInv           : ScMatInv();                   break;
                    case ocMatMult          : ScMatMult();                  break;
                    case ocMatSequence      :
                        if (!tryPlanEngineSequenceMatrix())
                            ScMatSequence();
                        break;
                    case ocMatTrans         :
                        if (!tryPlanEngineTranspose())
                            ScMatTrans();
                        break;
                    case ocMatRef           : ScMatRef();                   break;
                    case ocB:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"BINOMDIST");
                        const sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 3, 4))
                            break;

                        if (nParamCount == 3)
                        {
                            const double fSuccesses = GetDouble();
                            const double fProbability = GetDouble();
                            const double fTrials = GetDouble();
                            pushCalcMathValueResult(
                                spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                    evaluateLegacyBinomDist(
                                        fSuccesses, fTrials, fProbability));
                            break;
                        }

                        const double fUpper = GetDouble();
                        const double fLower = GetDouble();
                        const double fProbability = GetDouble();
                        const double fTrials = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyBinomRange(
                                    fTrials, fProbability, fLower, fUpper));
                    }
                    break;
                    case ocNormDist:
                    case ocNormDist_MS:
                    {
                        const bool bMicrosoftSyntax = eOp == ocNormDist_MS;
                        warnIfLegacyStatisticalDistributionReached(
                            bMicrosoftSyntax ? u"NORM.DIST" : u"NORMDIST");
                        const sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, bMicrosoftSyntax ? 4 : 3, 4))
                            break;

                        const bool bCumulative = nParamCount != 4 || GetBool();
                        const double fSigma = GetDouble();
                        const double fMean = GetDouble();
                        const double fX = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyNormDist(fX, fMean, fSigma, bCumulative));
                    }
                    break;
                    case ocExpDist:
                    case ocExpDist_MS:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"EXPONDIST");
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        const bool bCumulative = GetDouble() != 0.0;
                        const double fLambda = GetDouble();
                        const double fX = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyExponentialDist(fX, fLambda, bCumulative));
                    }
                    break;
                    case ocBinomDist:
                    case ocBinomDist_MS:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"BINOM.DIST");
                        if (!MustHaveParamCount(GetByte(), 4))
                            break;
                        const bool bCumulative = GetBool();
                        const double fProbability = GetDouble();
                        const double fTrials = GetDouble();
                        const double fSuccesses = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyBinomDistMs(
                                    fSuccesses, fTrials, fProbability, bCumulative));
                    }
                    break;
                    case ocPoissonDist:
                    case ocPoissonDist_MS:
                    {
                        const bool bOdfSyntax = eOp == ocPoissonDist;
                        warnIfLegacyStatisticalDistributionReached(
                            bOdfSyntax ? u"POISSON" : u"POISSON.DIST");
                        const sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, bOdfSyntax ? 2 : 3, 3))
                            break;
                        const bool bCumulative = nParamCount != 3 || GetBool();
                        const double fLambda = GetDouble();
                        const double fX = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyPoissonDist(fX, fLambda, bCumulative));
                    }
                    break;
                    case ocCombin           : pushLegacyCombin(u"COMBIN", false);  break;
                    case ocCombinA          : pushLegacyCombin(u"COMBINA", true);  break;
                    case ocPermut:
                    case ocPermutationA:
                    {
                        const bool bAllowRepetition = eOp == ocPermutationA;
                        warnIfLegacyStatisticalDistributionReached(
                            bAllowRepetition ? u"PERMUTATIONA" : u"PERMUT");
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        const double fK = GetDouble();
                        const double fN = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyPermutation(fN, fK, bAllowRepetition));
                    }
                    break;
                    case ocHypGeomDist:
                    case ocHypGeomDist_MS:
                    {
                        const bool bMicrosoftSyntax = eOp == ocHypGeomDist_MS;
                        warnIfLegacyStatisticalDistributionReached(
                            bMicrosoftSyntax ? u"HYPGEOM.DIST" : u"HYPGEOMDIST");
                        const sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, bMicrosoftSyntax ? 5 : 4, 5))
                            break;
                        const bool bCumulative = nParamCount == 5 && GetBool();
                        const double fPopulationSize = GetDouble();
                        const double fPopulationSuccesses = GetDouble();
                        const double fSampleSuccesses = GetDouble();
                        const double fX = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyHypGeomDist(
                                    fX, fSampleSuccesses, fPopulationSuccesses,
                                    fPopulationSize, bCumulative));
                    }
                    break;
                    case ocLogNormDist:
                    case ocLogNormDist_MS:
                    {
                        const bool bMicrosoftSyntax = eOp == ocLogNormDist_MS;
                        warnIfLegacyStatisticalDistributionReached(
                            bMicrosoftSyntax ? u"LOGNORM.DIST" : u"LOGNORMDIST");
                        const sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, bMicrosoftSyntax ? 4 : 1, 4))
                            break;
                        const bool bCumulative = nParamCount != 4 || GetBool();
                        const double fSigma = nParamCount >= 3 ? GetDouble() : 1.0;
                        const double fMean = nParamCount >= 2 ? GetDouble() : 0.0;
                        const double fX = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyLogNormDist(fX, fMean, fSigma, bCumulative));
                    }
                    break;
                    case ocTDist:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"TDIST");
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        const double fFlag = GetDouble();
                        const double fDegreesFreedom = GetDouble();
                        const double fT = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyTDistLegacy(fT, fDegreesFreedom, fFlag));
                    }
                    break;
                    case ocTDist_MS:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"T.DIST");
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        const bool bCumulative = GetBool();
                        const double fDegreesFreedom = GetDouble();
                        const double fT = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyTDistMs(fT, fDegreesFreedom, bCumulative));
                    }
                    break;
                    case ocTDist_RT:
                    case ocTDist_2T:
                    {
                        const int nTails = eOp == ocTDist_RT ? 1 : 2;
                        warnIfLegacyStatisticalDistributionReached(
                            nTails == 1 ? u"T.DIST.RT" : u"T.DIST.2T");
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        const double fDegreesFreedom = GetDouble();
                        const double fT = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyTDistTails(fT, fDegreesFreedom, nTails));
                    }
                    break;
                    case ocFDist:
                    case ocFDist_RT:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"FDIST");
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        const double fDegreesFreedom2 = GetDouble();
                        const double fDegreesFreedom1 = GetDouble();
                        const double fRatio = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyFDistRightTail(
                                    fRatio, fDegreesFreedom1, fDegreesFreedom2));
                    }
                    break;
                    case ocFDist_LT:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"F.DIST");
                        const int nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 3, 4))
                            break;
                        bool bCumulative = true;
                        if (nParamCount == 4)
                        {
                            if (IsMissing())
                                Pop();
                            else
                                bCumulative = GetBool();
                        }
                        const double fDegreesFreedom2 = GetDouble();
                        const double fDegreesFreedom1 = GetDouble();
                        const double fRatio = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyFDistLeftTail(
                                    fRatio, fDegreesFreedom1, fDegreesFreedom2, bCumulative));
                    }
                    break;
                    case ocChiDist:
                    case ocChiDist_MS:
                    {
                        const bool bOdfSyntax = eOp == ocChiDist;
                        warnIfLegacyStatisticalDistributionReached(
                            bOdfSyntax ? u"LEGACY.CHIDIST" : u"CHISQ.DIST.RT");
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        const double fDegreesFreedom = GetDouble();
                        const double fChi = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyChiDist(fChi, fDegreesFreedom, bOdfSyntax));
                    }
                    break;
                    case ocChiSqDist:
                    case ocChiSqDist_MS:
                    {
                        const bool bMicrosoftSyntax = eOp == ocChiSqDist_MS;
                        warnIfLegacyStatisticalDistributionReached(
                            bMicrosoftSyntax ? u"CHISQ.DIST" : u"CHISQDIST");
                        const sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, bMicrosoftSyntax ? 3 : 2, 3))
                            break;
                        bool bCumulative = true;
                        if (bMicrosoftSyntax || nParamCount == 3)
                            bCumulative = GetBool();
                        const double fDegreesFreedom = GetDouble();
                        const double fX = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyChiSqDist(
                                    fX, fDegreesFreedom, bCumulative, bMicrosoftSyntax));
                    }
                    break;
                    case ocStandard:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"STANDARDIZE");
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        const double fSigma = GetDouble();
                        const double fMean = GetDouble();
                        const double fX = GetDouble();
                        pushValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyStandardize(fX, fMean, fSigma));
                    }
                    break;
                    case ocAveDev:
                        warnIfLegacyStatisticalDistributionReached(u"AVEDEV");
                        seinterpcompatdispatch::Dispatcher::statisticalAveDev(*this);
                        break;
                    case ocDevSq:
                        warnIfLegacyStatisticalDistributionReached(u"DEVSQ");
                        seinterpcompatdispatch::Dispatcher::statisticalDevSq(*this);
                        break;
                    case ocKurt:
                        warnIfLegacyStatisticalDistributionReached(u"KURT");
                        seinterpcompatdispatch::Dispatcher::statisticalKurt(*this);
                        break;
                    case ocSkew:
                        warnIfLegacyStatisticalDistributionReached(u"SKEW");
                        seinterpcompatdispatch::Dispatcher::statisticalSkew(*this, false);
                        break;
                    case ocSkewp:
                        warnIfLegacyStatisticalDistributionReached(u"SKEWP");
                        seinterpcompatdispatch::Dispatcher::statisticalSkew(*this, true);
                        break;
                    case ocModalValue:
                    case ocModalValue_MS:
                        warnIfLegacyStatisticalDistributionReached(u"MODE.SNGL");
                        seinterpcompatdispatch::Dispatcher::statisticalMode(*this, true);
                        break;
                    case ocModalValue_Multi:
                        warnIfLegacyStatisticalDistributionReached(u"MODE.MULT");
                        seinterpcompatdispatch::Dispatcher::statisticalMode(*this, false);
                        break;
                    case ocMedian:
                        warnIfLegacyStatisticalDistributionReached(u"MEDIAN");
                        seinterpcompatdispatch::Dispatcher::statisticalMedian(*this);
                        break;
                    case ocGeoMean:
                        warnIfLegacyStatisticalDistributionReached(u"GEOMEAN");
                        seinterpcompatdispatch::Dispatcher::statisticalGeoMean(*this);
                        break;
                    case ocHarMean:
                        warnIfLegacyStatisticalDistributionReached(u"HARMEAN");
                        seinterpcompatdispatch::Dispatcher::statisticalHarMean(*this);
                        break;
                    case ocWeibull:
                    case ocWeibull_MS:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"WEIBULL");
                        if (!MustHaveParamCount(GetByte(), 4))
                            break;
                        const bool bCumulative = GetDouble() != 0.0;
                        const double fBeta = GetDouble();
                        const double fAlpha = GetDouble();
                        const double fX = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyWeibull(fX, fAlpha, fBeta, bCumulative));
                    }
                    break;
                    case ocBinomInv:
                    case ocCritBinom:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"CRITBINOM");
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        const double fAlpha = GetDouble();
                        const double fProbability = GetDouble();
                        const double fTrials = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyCritBinom(
                                    fTrials, fProbability, fAlpha));
                    }
                    break;
                    case ocNegBinomVert:
                    case ocNegBinomDist_MS:
                    {
                        const bool bMicrosoftSyntax = eOp == ocNegBinomDist_MS;
                        warnIfLegacyStatisticalDistributionReached(
                            bMicrosoftSyntax ? u"NEGBINOM.DIST" : u"NEGBINOMDIST");
                        if (!MustHaveParamCount(GetByte(), bMicrosoftSyntax ? 4 : 3))
                            break;
                        const bool bCumulative = bMicrosoftSyntax ? GetBool() : false;
                        const double fProbability = GetDouble();
                        const double fSuccesses = GetDouble();
                        const double fFailures = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyNegBinomDist(
                                    fFailures, fSuccesses, fProbability,
                                    bCumulative, bMicrosoftSyntax));
                    }
                    break;
                    case ocNoName           : PushError(FormulaError::NoName); break;
                    case ocBad              :
                        if (!tryPushEngineBadLiteralError())
                        {
                            OSL_FAIL("engine-backed root error literal declined ocBad");
                            PushError(FormulaError::NoName);
                        }
                        break;
                    case ocZTest            :
                    case ocZTest_MS:
                        warnIfLegacyStatisticalDistributionReached(u"ZTEST");
                        seinterpcompatdispatch::Dispatcher::zTest(*this);
                        break;
                    case ocTTest            :
                    case ocTTest_MS:
                        warnIfLegacyStatisticalDistributionReached(u"TTEST");
                        seinterpcompatdispatch::Dispatcher::tTest(*this);
                        break;
                    case ocFTest            :
                    case ocFTest_MS:
                        warnIfLegacyStatisticalDistributionReached(u"FTEST");
                        seinterpcompatdispatch::Dispatcher::fTest(*this);
                        break;
                    case ocRank             :
                    case ocRank_Eq:
                        warnIfLegacyStatisticalDistributionReached(u"RANK.EQ");
                        seinterpcompatdispatch::Dispatcher::statisticalRank(*this, false);
                        break;
                    case ocRank_Avg:
                        warnIfLegacyStatisticalDistributionReached(u"RANK.AVG");
                        seinterpcompatdispatch::Dispatcher::statisticalRank(*this, true);
                        break;
                    case ocPercentile       :
                    case ocPercentile_Inc:
                        warnIfLegacyStatisticalDistributionReached(u"PERCENTILE.INC");
                        seinterpcompatdispatch::Dispatcher::statisticalPercentile(*this, true);
                        break;
                    case ocPercentile_Exc:
                        warnIfLegacyStatisticalDistributionReached(u"PERCENTILE.EXC");
                        seinterpcompatdispatch::Dispatcher::statisticalPercentile(*this, false);
                        break;
                    case ocPercentrank      :
                    case ocPercentrank_Inc:
                        warnIfLegacyStatisticalDistributionReached(u"PERCENTRANK.INC");
                        seinterpcompatdispatch::Dispatcher::statisticalPercentrank(*this, true);
                        break;
                    case ocPercentrank_Exc:
                        warnIfLegacyStatisticalDistributionReached(u"PERCENTRANK.EXC");
                        seinterpcompatdispatch::Dispatcher::statisticalPercentrank(*this, false);
                        break;
                    case ocLarge            : CalculateSmallLarge(false); break;
                    case ocSmall            : CalculateSmallLarge(true);  break;
                    case ocFrequency        : ScFrequency();            break;
                    case ocQuartile         :
                    case ocQuartile_Inc:
                        warnIfLegacyStatisticalDistributionReached(u"QUARTILE.INC");
                        seinterpcompatdispatch::Dispatcher::statisticalQuartile(*this, true);
                        break;
                    case ocQuartile_Exc:
                        warnIfLegacyStatisticalDistributionReached(u"QUARTILE.EXC");
                        seinterpcompatdispatch::Dispatcher::statisticalQuartile(*this, false);
                        break;
                    case ocNormInv:
                    case ocNormInv_MS:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"NORMINV");
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        const double fSigma = GetDouble();
                        const double fMean = GetDouble();
                        const double fProbability = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyNormInv(fProbability, fMean, fSigma));
                    }
                    break;
                    case ocSNormInv:
                    case ocSNormInv_MS:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"NORMSINV");
                        if (!MustHaveParamCount(GetByte(), 1))
                            break;
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacySNormInv(GetDouble()));
                    }
                    break;
                    case ocConfidence:
                    case ocConfidence_N:
                    case ocConfidence_T:
                    {
                        const bool bStudent = eOp == ocConfidence_T;
                        warnIfLegacyStatisticalDistributionReached(
                            bStudent ? u"CONFIDENCE.T" : u"CONFIDENCE");
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        const double fSampleSize = GetDouble();
                        const double fSigma = GetDouble();
                        const double fAlpha = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyConfidence(
                                    fAlpha, fSigma, fSampleSize, bStudent));
                    }
                    break;
                    case ocTrimMean:
                        warnIfLegacyStatisticalDistributionReached(u"TRIMMEAN");
                        seinterpcompatdispatch::Dispatcher::statisticalTrimMean(*this);
                        break;
                    case ocProb:
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"PROB",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on PROB reached ScInterpreter");
                        seinterpcompatdispatch::Dispatcher::probability(*this);
                        break;
                    case ocCorrel           : CalculatePearsonCovar(true, false, false); break;
                    case ocCovar            :
                    case ocCovarianceP      : CalculatePearsonCovar(false, false, false); break;
                    case ocCovarianceS      : CalculatePearsonCovar(false, false, true); break;
                    case ocPearson          : CalculatePearsonCovar(true, false, false); break;
                    case ocRSQ:
                        warnIfLegacyStatisticalDistributionReached(u"RSQ");
                        seinterpcompatdispatch::Dispatcher::statisticalRSQ(*this);
                        break;
                    case ocSTEYX            : CalculatePearsonCovar(true, true, false); break;
                    case ocSlope            : CalculateSlopeIntercept(true); break;
                    case ocIntercept        : CalculateSlopeIntercept(false); break;
                    case ocTrend            : ScTrend();                break;
                    case ocGrowth:
                        warnIfLegacyGrowthProjectionReached(u"GROWTH");
                        seinterpcompatdispatch::Dispatcher::growth(*this);
                        break;
                    case ocLinest           : ScLinest();               break;
                    case ocLogest           : ScLogest();               break;
                    case ocForecast_LIN     :
                    case ocForecast:
                        warnIfLegacyStatisticalDistributionReached(u"FORECAST");
                        seinterpcompatdispatch::Dispatcher::forecast(*this);
                        break;
                    case ocForecast_ETS_ADD : ScForecast_Ets( etsAdd );       break;
                    case ocForecast_ETS_SEA : ScForecast_Ets( etsSeason );    break;
                    case ocForecast_ETS_MUL : ScForecast_Ets( etsMult );      break;
                    case ocForecast_ETS_PIA : ScForecast_Ets( etsPIAdd );     break;
                    case ocForecast_ETS_PIM : ScForecast_Ets( etsPIMult );    break;
                    case ocForecast_ETS_STA : ScForecast_Ets( etsStatAdd );   break;
                    case ocForecast_ETS_STM : ScForecast_Ets( etsStatMult );  break;
                    case ocGammaLn          :
                    case ocGammaLn_MS       :
                        warnIfLegacyStatisticalDistributionReached(u"GAMMALN");
                        pushCalcMathValueResult(semath::evaluateLogGammaValue(GetDouble()));
                        break;
                    case ocGamma            :
                        warnIfLegacyStatisticalDistributionReached(u"GAMMA");
                        pushCalcMathValueResult(semath::evaluateGammaValue(GetDouble()));
                        break;
                    case ocGammaDist:
                    case ocGammaDist_MS:
                    {
                        const bool bOdfSyntax = eOp == ocGammaDist;
                        warnIfLegacyStatisticalDistributionReached(
                            bOdfSyntax ? u"GAMMADIST" : u"GAMMA.DIST");
                        const sal_uInt8 nMinParamCount = bOdfSyntax ? 3 : 4;
                        const sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, nMinParamCount, 4))
                            break;
                        bool bCumulative = true;
                        if (nParamCount == 4)
                            bCumulative = GetBool();
                        const double fBeta = GetDouble();
                        const double fAlpha = GetDouble();
                        const double fX = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyGammaDist(
                                    fX, fAlpha, fBeta, bCumulative, bOdfSyntax));
                    }
                    break;
                    case ocGammaInv:
                    case ocGammaInv_MS:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"GAMMAINV");
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        const double fBeta = GetDouble();
                        const double fAlpha = GetDouble();
                        const double fProbability = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyGammaInverse(
                                    fProbability, fAlpha, fBeta));
                    }
                    break;
                    case ocChiTest          :
                    case ocChiTest_MS:
                        warnIfLegacyStatisticalDistributionReached(u"LEGACY.CHITEST");
                        seinterpcompatdispatch::Dispatcher::chiTest(*this);
                        break;
                    case ocChiInv:
                    case ocChiInv_MS:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"CHIINV");
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        const double fDegreesFreedom = GetDouble();
                        const double fProbability = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyChiInv(fProbability, fDegreesFreedom));
                    }
                    break;
                    case ocChiSqInv:
                    case ocChiSqInv_MS:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"CHISQ.INV");
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        const double fDegreesFreedom = GetDouble();
                        const double fProbability = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyChiSqInv(fProbability, fDegreesFreedom));
                    }
                    break;
                    case ocTInv:
                    case ocTInv_2T:
                    case ocTInv_MS:
                    {
                        const int nType = eOp == ocTInv_MS ? 4 : 2;
                        warnIfLegacyStatisticalDistributionReached(
                            nType == 4 ? u"T.INV" : u"TINV");
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        const double fDegreesFreedom = GetDouble();
                        const double fProbability = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyTInv(fProbability, fDegreesFreedom, nType));
                    }
                    break;
                    case ocFInv:
                    case ocFInv_RT:
                    case ocFInv_LT:
                    {
                        const bool bLeftTail = eOp == ocFInv_LT;
                        warnIfLegacyStatisticalDistributionReached(
                            bLeftTail ? u"F.INV" : u"LEGACY.FINV");
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        const double fDegreesFreedom2 = GetDouble();
                        const double fDegreesFreedom1 = GetDouble();
                        const double fProbability = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyFInv(
                                    fProbability, fDegreesFreedom1,
                                    fDegreesFreedom2, bLeftTail));
                    }
                    break;
                    case ocLogInv:
                    case ocLogInv_MS:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"LOGINV");
                        const sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 1, 3))
                            break;
                        const double fSigma = nParamCount == 3 ? GetDouble() : 1.0;
                        const double fMean = nParamCount >= 2 ? GetDouble() : 0.0;
                        const double fProbability = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyLogNormInv(
                                    fProbability, fMean, fSigma));
                    }
                    break;
                    case ocBetaDist:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"BETADIST");
                        const sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 3, 6))
                            break;
                        const bool bCumulative = nParamCount == 6 ? GetBool() : true;
                        const double fUpperBound = nParamCount >= 5 ? GetDouble() : 1.0;
                        const double fLowerBound = nParamCount >= 4 ? GetDouble() : 0.0;
                        const double fBeta = GetDouble();
                        const double fAlpha = GetDouble();
                        const double fX = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyBetaDist(
                                    fX, fAlpha, fBeta, fLowerBound, fUpperBound,
                                    bCumulative, false));
                    }
                    break;
                    case ocBetaDist_MS:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"BETA.DIST");
                        const sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 4, 6))
                            break;
                        const double fUpperBound = nParamCount == 6 ? GetDouble() : 1.0;
                        const double fLowerBound = nParamCount >= 5 ? GetDouble() : 0.0;
                        const bool bCumulative = GetBool();
                        const double fBeta = GetDouble();
                        const double fAlpha = GetDouble();
                        const double fX = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyBetaDist(
                                    fX, fAlpha, fBeta, fLowerBound, fUpperBound,
                                    bCumulative, true));
                    }
                    break;
                    case ocBetaInv:
                    case ocBetaInv_MS:
                    {
                        warnIfLegacyStatisticalDistributionReached(u"BETAINV");
                        const sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 3, 5))
                            break;
                        const double fUpperBound = nParamCount == 5 ? GetDouble() : 1.0;
                        const double fLowerBound = nParamCount >= 4 ? GetDouble() : 0.0;
                        const double fBeta = GetDouble();
                        const double fAlpha = GetDouble();
                        const double fProbability = GetDouble();
                        pushCalcMathValueResult(
                            spreadsheetengine::compat::libreoffice::interpreterdispatch::
                                evaluateLegacyBetaInv(
                                    fProbability, fAlpha, fBeta,
                                    fLowerBound, fUpperBound));
                    }
                    break;
                    case ocFourier          : ScFourier();              break;
                    case ocExternal         : ScExternal();                 break;
                    case ocTableOp          : ScTableOp();                  break;
                    case ocStop :                                           break;
                    case ocErrorType:
                    {
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"ERRORTYPE",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on ERRORTYPE reached ScInterpreter", true);
                        FormulaError nErr = GetErrorType();
                        if (nErr != FormulaError::NONE)
                        {
                            nGlobalError = FormulaError::NONE;
                            PushDouble(static_cast<double>(nErr));
                        }
                        else
                            PushNA();
                    }
                    break;
                    case ocErrorType_ODF:
                    {
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"ERROR.TYPE",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on ERROR.TYPE reached ScInterpreter", true);
                        FormulaError nErr = GetErrorType();
                        sal_uInt16 nErrType = 0;

                        switch (nErr)
                        {
                            case FormulaError::NoCode:
                                nErrType = 1;
                                break;
                            case FormulaError::DivisionByZero:
                                nErrType = 2;
                                break;
                            case FormulaError::NoValue:
                                nErrType = 3;
                                break;
                            case FormulaError::NoRef:
                                nErrType = 4;
                                break;
                            case FormulaError::NoName:
                                nErrType = 5;
                                break;
                            case FormulaError::IllegalFPOperation:
                                nErrType = 6;
                                break;
                            case FormulaError::NotAvailable:
                                nErrType = 7;
                                break;
                            default:
                                break;
                        }

                        if (nErrType)
                        {
                            nGlobalError = FormulaError::NONE;
                            PushDouble(nErrType);
                        }
                        else
                            PushNA();
                    }
                    break;
                    case ocCurrent          : ScCurrent();                  break;
                    case ocStyle            : ScStyle();                    break;
                    case ocDde              : ScDde();                      break;
                    case ocBase             :
                    {
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"BASE",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on numeral conversion reached ScInterpreter");
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 2, 3))
                            break;
                        std::optional<double> ofMinLength;
                        if (nParamCount == 3)
                            ofMinLength = GetDouble();
                        const double fBase = GetDouble();
                        const double fValue = GetDouble();
                        if (nGlobalError != FormulaError::NONE)
                        {
                            PushIllegalArgument();
                            break;
                        }
                        const auto aResult = seconvert::evaluateBaseValue(fValue, fBase, ofMinLength);
                        if (aResult)
                            PushString(selibreoffice::toLibreOfficeString(aResult.maValue));
                        else if (aResult.meError == spreadsheetengine::api::Error::StringOverflow)
                            PushError(FormulaError::StringOverflow);
                        else
                            PushIllegalArgument();
                    }
                    break;
                    case ocDecimal          :
                    {
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"DECIMAL",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on numeral conversion reached ScInterpreter");
                        if (!MustHaveParamCount(GetByte(), 2))
                            break;
                        const double fBase = GetDouble();
                        const OUString aText = GetString().getString();
                        if (nGlobalError != FormulaError::NONE)
                        {
                            PushIllegalArgument();
                            break;
                        }
                        const auto aResult
                            = seconvert::evaluateDecimalValue(selibreoffice::toApiString(aText), fBase);
                        if (aResult)
                            PushDouble(aResult.maValue);
                        else
                            PushIllegalArgument();
                    }
                    break;
                    case ocConvertOOo       :
                    {
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"CONVERT",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on CONVERT reached ScInterpreter");
                        if (!MustHaveParamCount(GetByte(), 3))
                            break;
                        OUString aToUnit = GetString().getString();
                        OUString aFromUnit = GetString().getString();
                        double fVal = GetDouble();
                        if (nGlobalError != FormulaError::NONE)
                        {
                            PushError(nGlobalError);
                            break;
                        }
                        const auto aResult = seconvert::evaluateConvertValue(
                            fVal, selibreoffice::toApiString(aFromUnit),
                            selibreoffice::toApiString(aToUnit));
                        if (aResult)
                            PushDouble(aResult.maValue);
                        else
                            PushNA();
                    }
                    break;
                    case ocEuroConvert      :
                    {
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"EUROCONVERT",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on numeral conversion reached ScInterpreter");
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 3, 5))
                            break;
                        double fPrecision = 0.0;
                        if (nParamCount == 5)
                        {
                            fPrecision = ::rtl::math::approxFloor(GetDouble());
                            if (fPrecision < 3)
                            {
                                PushIllegalArgument();
                                break;
                            }
                        }
                        bool bFullPrecision = nParamCount >= 4 && GetBool();
                        OUString aToUnit = GetString().getString();
                        OUString aFromUnit = GetString().getString();
                        double fVal = GetDouble();
                        if (nGlobalError != FormulaError::NONE)
                        {
                            PushError(nGlobalError);
                            break;
                        }
                        const auto aConverted = seconvert::evaluateEuroConvertValue(
                            fVal, selibreoffice::toApiString(aFromUnit),
                            selibreoffice::toApiString(aToUnit), true, !bFullPrecision);
                        if (!aConverted)
                        {
                            PushIllegalArgument();
                            break;
                        }
                        double fRes = aConverted.maValue;
                        if (fPrecision && !aFromUnit.equalsIgnoreAsciiCase("EUR")
                            && !aFromUnit.equalsIgnoreAsciiCase(aToUnit))
                        {
                            const auto aIntermediate = seconvert::evaluateEuroConvertValue(
                                fVal, selibreoffice::toApiString(aFromUnit), u"EUR", true, false);
                            if (!aIntermediate)
                            {
                                PushIllegalArgument();
                                break;
                            }
                            const double fRoundedIntermediate
                                = ::rtl::math::round(aIntermediate.maValue, static_cast<int>(fPrecision));
                            const auto aTriangulated = seconvert::evaluateEuroConvertValue(
                                fRoundedIntermediate, u"EUR", selibreoffice::toApiString(aToUnit),
                                true, !bFullPrecision);
                            if (!aTriangulated)
                            {
                                PushIllegalArgument();
                                break;
                            }
                            fRes = aTriangulated.maValue;
                        }
                        PushDouble(fRes);
                    }
                    break;
                    case ocRoman            :
                    {
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"ROMAN",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on numeral conversion reached ScInterpreter");
                        sal_uInt8 nParamCount = GetByte();
                        if (!MustHaveParamCount(nParamCount, 1, 2))
                            break;
                        std::optional<double> ofMode;
                        if (nParamCount == 2)
                            ofMode = GetDouble();
                        const double fValue = GetDouble();
                        if (nGlobalError != FormulaError::NONE)
                        {
                            PushError(nGlobalError);
                            break;
                        }
                        const auto aResult = seconvert::evaluateRomanValue(fValue, ofMode);
                        if (aResult)
                            PushString(selibreoffice::toLibreOfficeString(aResult.maValue));
                        else
                            PushIllegalArgument();
                    }
                    break;
                    case ocArabic           :
                    {
                        warnIfLegacyDispatchReached(
                            "family-local default-on", u"ARABIC",
                            [](std::u16string_view rFormula) {
                                return setaileval::isFamilyLocalDefaultOnFormula(rFormula);
                            },
                            "family-local default-on numeral conversion reached ScInterpreter");
                        if (!MustHaveParamCount(GetByte(), 1))
                            break;
                        const OUString aRoman = GetString().getString();
                        if (nGlobalError != FormulaError::NONE)
                        {
                            PushError(nGlobalError);
                            break;
                        }
                        if (const auto oArabic
                            = seconvert::convertFromRoman(selibreoffice::toApiString(aRoman)))
                            PushInt(*oArabic);
                        else
                            PushIllegalArgument();
                    }
                    break;
                    case ocInfo             : ScInfo();                 break;
                    case ocHyperLink        : ScHyperLink();            break;
                    case ocBahtText         : pushLegacyBahtText();     break;
                    case ocGetPivotData     : ScGetPivotData();             break;
                    case ocJis              :
                        pushLegacyJisAsc(u"JIS", [&](const OUString& rText) {
                            return selibreoffice::convertIntoFullWidth(rText);
                        });
                        break;
                    case ocAsc              :
                        pushLegacyJisAsc(u"ASC", [&](const OUString& rText) {
                            return selibreoffice::convertIntoHalfWidth(rText);
                        });
                        break;
                    case ocLenB             : pushLegacyLenB();         break;
                    case ocRightB           : pushLegacyRightB();       break;
                    case ocLeftB            : pushLegacyLeftB();        break;
                    case ocMidB             : pushLegacyMidB();         break;
                    case ocReplaceB         : pushLegacyReplaceB();     break;
                    case ocFindB            : pushLegacyFindB();        break;
                    case ocSearchB          : pushLegacySearchB();      break;
                    case ocUnicode          : pushLegacyUnicode();          break;
                    case ocUnichar          : pushLegacyUnichar();          break;
                    case ocBitAnd           :
                        pushLegacyBitwise(u"BITAND",
                                          [](double fLeft, double fRight) {
                                              return spreadsheetengine::core::math::computeBitAnd(
                                                  fLeft, fRight);
                                          });
                        break;
                    case ocBitOr            :
                        pushLegacyBitwise(u"BITOR",
                                          [](double fLeft, double fRight) {
                                              return spreadsheetengine::core::math::computeBitOr(
                                                  fLeft, fRight);
                                          });
                        break;
                    case ocBitXor           :
                        pushLegacyBitwise(u"BITXOR",
                                          [](double fLeft, double fRight) {
                                              return spreadsheetengine::core::math::computeBitXor(
                                                  fLeft, fRight);
                                          });
                        break;
                    case ocBitRshift        :
                        pushLegacyBitwise(u"BITRSHIFT",
                                          [](double fLeft, double fRight) {
                                              return spreadsheetengine::core::math::computeBitRightShift(
                                                  fLeft, fRight);
                                          });
                        break;
                    case ocBitLshift        :
                        pushLegacyBitwise(u"BITLSHIFT",
                                          [](double fLeft, double fRight) {
                                              return spreadsheetengine::core::math::computeBitLeftShift(
                                                  fLeft, fRight);
                                          });
                        break;
                    case ocTTT              : ScTTT();                      break;
                    case ocDebugVar         : ScDebugVar();                 break;
                    case ocNone : nFuncFmtType = SvNumFormatType::UNDEFINED;    break;
                    default : PushError( FormulaError::UnknownOpCode);                 break;
                }

                // If the function pushed a subroutine as result, continue with
                // execution of the subroutine.
                if (sp > nStackBase && pStack[sp-1]->GetOpCode() == ocCall)
                {
                    Pop(); continue;
                }

                if (FormulaCompiler::IsOpCodeVolatile(eOp))
                    meVolatileType = VOLATILE;

                // Remember result matrix in case it could be reused.
                if (sp && GetStackType() == svMatrix)
                    maTokenMatrixMap.emplace(pCur, pStack[sp-1]);

                // outer function determines format of an expression
                if ( nFuncFmtType != SvNumFormatType::UNDEFINED )
                {
                    nRetTypeExpr = nFuncFmtType;
                    // Inherit the format index for currency, date or time formats.
                    switch (nFuncFmtType)
                    {
                        case SvNumFormatType::CURRENCY:
                        case SvNumFormatType::DATE:
                        case SvNumFormatType::TIME:
                        case SvNumFormatType::DATETIME:
                        case SvNumFormatType::DURATION:
                            nRetIndexExpr = nFuncFmtIndex;
                        break;
                        default:
                            nRetIndexExpr = 0;
                    }
                }
            }
        }

        // Need a clean stack environment for the JumpMatrix to work.
        if (nGlobalError != FormulaError::NONE && eOp != ocPush && sp > nStackBase + 1)
        {
            // Not all functions pop all parameters in case an error is
            // generated. Clean up stack. Assumes that every function pushes a
            // result, may be arbitrary in case of error.
            FormulaConstTokenRef xLocalResult = pStack[ sp - 1 ];
            while (sp > nStackBase)
                Pop();
            PushTokenRef( xLocalResult );
        }

        bool bGotResult;
        do
        {
            bGotResult = false;
            sal_uInt8 nLevel = 0;
            if ( GetStackType( ++nLevel ) == svJumpMatrix )
                ;   // nothing
            else if ( GetStackType( ++nLevel ) == svJumpMatrix )
                ;   // nothing
            else
                nLevel = 0;
            if ( nLevel == 1 || (nLevel == 2 && aCode.IsEndOfPath()) )
            {
                if (nLevel == 1)
                    aErrorFunctionStack.push_back( nErrorFunction);
                bGotResult = JumpMatrix( nLevel );
                if (aErrorFunctionStack.empty())
                    assert(!"ScInterpreter::Interpret - aErrorFunctionStack empty in JumpMatrix context");
                else
                {
                    nErrorFunction = aErrorFunctionStack.back();
                    if (bGotResult)
                        aErrorFunctionStack.pop_back();
                }
            }
            else
                pJumpMatrix = nullptr;
        } while ( bGotResult );

        if( IsErrFunc(eOp) )
            ++nErrorFunction;

        if ( nGlobalError != FormulaError::NONE )
        {
            if ( !nErrorFunctionCount )
            {   // count of errorcode functions in formula
                FormulaTokenArrayPlainIterator aIter(*pArr);
                for ( FormulaToken* t = aIter.FirstRPN(); t; t = aIter.NextRPN() )
                {
                    if ( IsErrFunc(t->GetOpCode()) )
                        ++nErrorFunctionCount;
                }
            }
            if ( nErrorFunction >= nErrorFunctionCount )
                ++nErrorFunction;   // that's it, error => terminate
            else if (nErrorFunctionCount && sp && GetStackType() == svError)
            {
                // Clear global error if we have an individual error result, so
                // an error evaluating function can receive multiple arguments
                // and not all evaluated arguments inheriting the error.
                // This is important for at least IFS() and SWITCH() as long as
                // they are classified as error evaluating functions and not
                // implemented as short-cutting jump code paths, but also for
                // more than one evaluated argument to AGGREGATE() or COUNT()
                // that may ignore errors.
                nGlobalError = FormulaError::NONE;
            }
        }
    }

    // End: obtain result

    bool bForcedResultType;
    switch (eOp)
    {
        case ocGetDateValue:
        case ocGetTimeValue:
            // Force final result of DATEVALUE and TIMEVALUE to number type,
            // which so far was date or time for calculations.
            nRetTypeExpr = nFuncFmtType = SvNumFormatType::NUMBER;
            nRetIndexExpr = nFuncFmtIndex = 0;
            bForcedResultType = true;
        break;
        default:
            bForcedResultType = false;
    }

    if (sp == 1)
    {
        pCur = pStack[ sp-1 ];
        if( pCur->GetOpCode() == ocPush )
        {
            // An svRefList can be resolved if it a) contains just one
            // reference, or b) in array context contains an array of single
            // cell references.
            if (pCur->GetType() == svRefList)
            {
                PopRefListPushMatrixOrRef();
                pCur = pStack[ sp-1 ];
            }
            switch( pCur->GetType() )
            {
                case svEmptyCell:
                    ;   // nothing
                break;
                case svError:
                    nGlobalError = pCur->GetError();
                break;
                case svDouble :
                    {
                        // If typed, pop token to obtain type information and
                        // push a plain untyped double so the result token to
                        // be transferred to the formula cell result does not
                        // unnecessarily duplicate the information.
                        if (pCur->GetDoubleType() != 0)
                        {
                            double fVal = PopDouble();
                            if (!bForcedResultType)
                            {
                                if (nCurFmtType != nFuncFmtType)
                                    nRetIndexExpr = 0;  // carry format index only for matching type
                                nRetTypeExpr = nFuncFmtType = nCurFmtType;
                            }
                            if (nRetTypeExpr == SvNumFormatType::DURATION)
                            {
                                // Round the duration in case a wall clock time
                                // display format is used instead of a duration
                                // format. To micro seconds which then catches
                                // the converted hh:mm:ss.9999997 cases.
                                if (fVal != 0.0)
                                {
                                    fVal *= 86400.0;
                                    fVal = rtl::math::round( fVal, 6);
                                    fVal /= 86400.0;
                                }
                            }
                            PushTempToken( CreateFormulaDoubleToken( fVal));
                        }
                        if ( nFuncFmtType == SvNumFormatType::UNDEFINED )
                        {
                            nRetTypeExpr = SvNumFormatType::NUMBER;
                            nRetIndexExpr = 0;
                        }
                    }
                break;
                case svString :
                    nRetTypeExpr = SvNumFormatType::TEXT;
                    nRetIndexExpr = 0;
                break;
                case svSingleRef :
                {
                    ScAddress aAdr;
                    PopSingleRef( aAdr );
                    if( nGlobalError == FormulaError::NONE)
                        PushCellResultToken( false, aAdr, &nRetTypeExpr, &nRetIndexExpr, true);
                }
                break;
                case svRefList :
                    PopError();     // maybe #REF! takes precedence over #VALUE!
                    PushError( FormulaError::NoValue);
                break;
                case svDoubleRef :
                {
                    if ( bMatrixFormula )
                    {   // create matrix for {=A1:A5}
                        PopDoubleRefPushMatrix();
                        ScMatrixRef xMat = PopMatrix();
                        QueryMatrixType(xMat, nRetTypeExpr, nRetIndexExpr);
                    }
                    else
                    {
                        ScRange aRange;
                        PopDoubleRef( aRange );
                        ScAddress aAdr;
                        if ( nGlobalError == FormulaError::NONE && DoubleRefToPosSingleRef( aRange, aAdr))
                            PushCellResultToken( false, aAdr, &nRetTypeExpr, &nRetIndexExpr, true);
                    }
                }
                break;
                case svExternalDoubleRef:
                {
                    ScMatrixRef xMat;
                    PopExternalDoubleRef(xMat);
                    QueryMatrixType(xMat, nRetTypeExpr, nRetIndexExpr);
                }
                break;
                case svMatrix :
                {
                    sc::RangeMatrix aMat = PopRangeMatrix();
                    if (aMat.isRangeValid())
                    {
                        // This matrix represents a range reference. Apply implicit intersection.
                        double fVal = applyImplicitIntersection(aMat, aPos);
                        if (std::isnan(fVal))
                            PushNoValue();
                        else
                            PushInt(fVal);
                    }
                    else
                        // This is a normal matrix.
                        QueryMatrixType(aMat.mpMat, nRetTypeExpr, nRetIndexExpr);
                }
                break;
                case svExternalSingleRef:
                {
                    FormulaTokenRef xToken;
                    ScExternalRefCache::CellFormat aFmt;
                    PopExternalSingleRef(xToken, &aFmt);
                    if (nGlobalError != FormulaError::NONE)
                        break;

                    PushTokenRef(xToken);

                    if (aFmt.mbIsSet)
                    {
                        nFuncFmtType = aFmt.mnType;
                        nFuncFmtIndex = aFmt.mnIndex;
                    }
                }
                break;
                default :
                    SetError( FormulaError::UnknownStackVariable);
            }
        }
        else
            SetError( FormulaError::UnknownStackVariable);
    }
    else if (sp > 1)
        SetError( FormulaError::OperatorExpected);
    else
        SetError( FormulaError::NoCode);

    if (bForcedResultType || nRetTypeExpr != SvNumFormatType::UNDEFINED)
    {
        nRetFmtType = nRetTypeExpr;
        nRetFmtIndex = nRetIndexExpr;
    }
    else if( nFuncFmtType != SvNumFormatType::UNDEFINED )
    {
        nRetFmtType = nFuncFmtType;
        nRetFmtIndex = nFuncFmtIndex;
    }
    else
        nRetFmtType = SvNumFormatType::NUMBER;

    if (nGlobalError != FormulaError::NONE && GetStackType() != svError )
        PushError( nGlobalError);

    // THE final result.
    xResult = PopToken();
    if (!xResult)
        xResult = new FormulaErrorToken( FormulaError::UnknownStackVariable);

    // release tokens in expression stack
    const FormulaToken** p = pStack;
    while( maxsp-- )
        (*p++)->DecRef();

    StackVar eType = xResult->GetType();
    if (eType == svMatrix)
        // Results are immutable in case they would be reused as input for new
        // interpreters.
        xResult->GetMatrix()->SetImmutable();
    return eType;
}

void ScInterpreter::AssertFormulaMatrix()
{
    bMatrixFormula = true;
}

const svl::SharedString & ScInterpreter::GetStringResult() const
{
    return xResult->GetString();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
