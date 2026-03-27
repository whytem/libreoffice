/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "helper/qahelper.hxx"

#include <formula/errorcodes.hxx>
#include <formula/paramclass.hxx>
#include <formula/token.hxx>
#include <scmatrix.hxx>
#include <spreadsheetengine/compat/libreoffice/TokenBridge.hxx>
#include <token.hxx>
#include <tokenarray.hxx>

namespace
{

class TestTokenBridge : public ScUcalcTestBase
{
};

ScSingleRefData makeSingleRef()
{
    ScSingleRefData aReference;
    aReference.InitFlags();
    aReference.SetRelCol(2);
    aReference.SetRelRow(7);
    aReference.SetRelTab(0);
    aReference.SetFlag3D(true);
    return aReference;
}

ScComplexRefData makeDoubleRef()
{
    ScComplexRefData aReference;
    aReference.InitFlags();
    aReference.Ref1 = makeSingleRef();
    aReference.Ref2.SetRelCol(4);
    aReference.Ref2.SetRelRow(10);
    aReference.Ref2.SetRelTab(0);
    aReference.Ref2.SetFlag3D(true);
    aReference.bTrimToData = true;
    return aReference;
}

ScMatrixRef makeMatrix()
{
    ScMatrixRef xMatrix(new ScMatrix(2, 2));
    xMatrix->PutDouble(42.5, 0, 0);
    xMatrix->PutString(svl::SharedString(u"matrix"_ustr), 1, 0);
    xMatrix->PutError(FormulaError::DivisionByZero, 0, 1);
    xMatrix->PutDouble(-3.0, 1, 1);
    return xMatrix;
}

} // namespace

CPPUNIT_TEST_FIXTURE(TestTokenBridge, testImportExportRoundTrip)
{
    using spreadsheetengine::compat::libreoffice::exportCompiledFormula;
    using spreadsheetengine::compat::libreoffice::importCompiledFormula;
    using spreadsheetengine::compat::libreoffice::toEngineVectorState;

    m_pDoc->InsertTab(0, u"Tokens"_ustr);

    ScTokenArray aOriginal(*m_pDoc);
    const ScSingleRefData aSingleRef = makeSingleRef();
    const ScComplexRefData aDoubleRef = makeDoubleRef();

    aOriginal.AddSingleReference(aSingleRef);
    aOriginal.AddToken(formula::FormulaMissingToken());
    aOriginal.AddToken(formula::FormulaByteToken(ocIf, 3, formula::ParamClass::Reference));
    aOriginal.AddDouble(12.5);
    aOriginal.AddString(svl::SharedString(u"literal"_ustr));
    aOriginal.AddStringName(u"lambda_name"_ustr);
    aOriginal.AddDoubleReference(aDoubleRef);
    aOriginal.AddRangeName(7, 0);
    aOriginal.AddDBRange(9);
    aOriginal.AddExternalSingleReference(4, svl::SharedString(u"ExtA"_ustr), aSingleRef);
    aOriginal.AddExternalDoubleReference(4, svl::SharedString(u"ExtB"_ustr), aDoubleRef);
    aOriginal.AddExternalName(4, svl::SharedString(u"BookName"_ustr));
    aOriginal.AddMatrix(makeMatrix());
    aOriginal.AddColRowName(aSingleRef);
    aOriginal.AddToken(ScTableRefToken(
        5, static_cast<ScTableRefToken::Item>(ScTableRefToken::DATA | ScTableRefToken::TOTALS)));
    aOriginal.AddToken(formula::FormulaErrorToken(FormulaError::NoValue));
    short pJump[] = { 2, 5, 9 };
    formula::FormulaJumpToken aJumpToken(ocIf, pJump);
    aJumpToken.SetInForceArray(formula::ParamClass::Value);
    aOriginal.AddToken(aJumpToken);
    aOriginal.AddToken(formula::FormulaSpaceToken(2, u' '));
    aOriginal.AddOpCode(ocAdd);

    aOriginal.SetCodeError(FormulaError::NoCode);
    aOriginal.SetHyperLink(true);
    aOriginal.SetFromRangeName(true);
    aOriginal.SetShareable(false);
    aOriginal.ClearRecalcMode();
    aOriginal.SetMaskedRecalcMode(ScRecalcMode::ALWAYS);
    aOriginal.SetCombinedBitsRecalcMode(ScRecalcMode::FORCED | ScRecalcMode::ONREFMOVE);

    const auto aImported = importCompiledFormula(aOriginal);
    const OUString aImportMessage = u"token import failed at index "_ustr
                                    + OUString::number(aImported.mnFailureIndex) + u": "_ustr
                                    + aImported.maFailureMessage;
    CPPUNIT_ASSERT_MESSAGE(aImportMessage.toUtf8().getStr(), static_cast<bool>(aImported));
    CPPUNIT_ASSERT_EQUAL(aOriginal.GetLen(), static_cast<sal_uInt16>(aImported.maFormula.maTokens.size()));
    CPPUNIT_ASSERT_EQUAL(
        static_cast<sal_uInt16>(aOriginal.GetCodeError()), aImported.maFormula.mnCodeError);
    CPPUNIT_ASSERT_EQUAL(
        static_cast<sal_uInt8>(aOriginal.GetRecalcMode()), aImported.maFormula.mnRecalcModeBits);
    CPPUNIT_ASSERT_EQUAL(aOriginal.IsHyperLink(), aImported.maFormula.mbHyperLink);
    CPPUNIT_ASSERT_EQUAL(aOriginal.IsFromRangeName(), aImported.maFormula.mbFromRangeName);
    CPPUNIT_ASSERT_EQUAL(aOriginal.IsShareable(), aImported.maFormula.mbShareable);
    CPPUNIT_ASSERT_EQUAL(
        toEngineVectorState(aOriginal.GetVectorState()), aImported.maFormula.meVectorState);
    CPPUNIT_ASSERT_EQUAL(aOriginal.IsEnabledForOpenCL(), aImported.maFormula.mbOpenCLEnabled);
    CPPUNIT_ASSERT_EQUAL(aOriginal.IsEnabledForThreading(), aImported.maFormula.mbThreadingEnabled);

    const auto aExported = exportCompiledFormula(aImported.maFormula, *m_pDoc);
    const OUString aExportMessage = u"token export failed at index "_ustr
                                    + OUString::number(aExported.mnFailureIndex) + u": "_ustr
                                    + aExported.maFailureMessage;
    CPPUNIT_ASSERT_MESSAGE(aExportMessage.toUtf8().getStr(), static_cast<bool>(aExported));
    CPPUNIT_ASSERT(aExported.mxTokenArray);
    CPPUNIT_ASSERT(spreadsheetengine::compat::libreoffice::tokenArraysEqualForBridge(
        aOriginal, *aExported.mxTokenArray));
    CPPUNIT_ASSERT_EQUAL(aOriginal.GetCodeError(), aExported.mxTokenArray->GetCodeError());
    CPPUNIT_ASSERT_EQUAL(aOriginal.GetRecalcMode(), aExported.mxTokenArray->GetRecalcMode());
    CPPUNIT_ASSERT_EQUAL(aOriginal.IsHyperLink(), aExported.mxTokenArray->IsHyperLink());
    CPPUNIT_ASSERT_EQUAL(aOriginal.IsFromRangeName(), aExported.mxTokenArray->IsFromRangeName());
    CPPUNIT_ASSERT_EQUAL(aOriginal.IsShareable(), aExported.mxTokenArray->IsShareable());
}

CPPUNIT_TEST_FIXTURE(TestTokenBridge, testXmlPlaceholderRoundTrip)
{
    using spreadsheetengine::compat::libreoffice::exportCompiledFormula;
    using spreadsheetengine::compat::libreoffice::importCompiledFormula;

    m_pDoc->InsertTab(0, u"XML"_ustr);

    ScTokenArray aOriginal(*m_pDoc);
    aOriginal.AssignXMLString(u"of:=SUM([.A1:.A3])"_ustr, u"of"_ustr);
    aOriginal.SetCodeError(FormulaError::NoCode);
    aOriginal.SetHyperLink(false);
    aOriginal.SetFromRangeName(false);
    aOriginal.SetShareable(true);
    aOriginal.ClearRecalcMode();
    aOriginal.SetMaskedRecalcMode(ScRecalcMode::ONLOAD_ONCE);

    const auto aImported = importCompiledFormula(aOriginal);
    CPPUNIT_ASSERT_MESSAGE(aImported.maFailureMessage.toUtf8().getStr(), static_cast<bool>(aImported));
    CPPUNIT_ASSERT(aImported.maFormula.moXmlFormulaSource.has_value());
    CPPUNIT_ASSERT(aImported.maFormula.maTokens.empty());
    CPPUNIT_ASSERT(
        aImported.maFormula.moXmlFormulaSource->maFormula
        == spreadsheetengine::api::String(u"of:=SUM([.A1:.A3])"));
    CPPUNIT_ASSERT(
        aImported.maFormula.moXmlFormulaSource->maNamespace
        == spreadsheetengine::api::String(u"of"));

    const auto aExported = exportCompiledFormula(aImported.maFormula, *m_pDoc);
    CPPUNIT_ASSERT_MESSAGE(aExported.maFailureMessage.toUtf8().getStr(), static_cast<bool>(aExported));
    CPPUNIT_ASSERT(aExported.mxTokenArray);
    CPPUNIT_ASSERT(aOriginal.EqualTokens(aExported.mxTokenArray.get()));
    CPPUNIT_ASSERT_EQUAL(aOriginal.GetCodeError(), aExported.mxTokenArray->GetCodeError());
    CPPUNIT_ASSERT_EQUAL(aOriginal.GetRecalcMode(), aExported.mxTokenArray->GetRecalcMode());
}

CPPUNIT_TEST_FIXTURE(TestTokenBridge, testCanonicalLexicalEquality)
{
    m_pDoc->InsertTab(0, u"Equality"_ustr);

    ScTokenArray aLeft(*m_pDoc);
    aLeft.AddMatrix(makeMatrix());

    ScTokenArray aRight(*m_pDoc);
    aRight.AddMatrix(makeMatrix());

    CPPUNIT_ASSERT(aLeft.EqualTokens(&aRight));

    ScMatrixRef xDifferent(new ScMatrix(2, 2));
    xDifferent->PutDouble(42.5, 0, 0);
    xDifferent->PutString(svl::SharedString(u"matrix"_ustr), 1, 0);
    xDifferent->PutError(FormulaError::DivisionByZero, 0, 1);
    xDifferent->PutDouble(-4.0, 1, 1);

    ScTokenArray aDifferent(*m_pDoc);
    aDifferent.AddMatrix(xDifferent);

    CPPUNIT_ASSERT(!aLeft.EqualTokens(&aDifferent));
}

CPPUNIT_TEST_FIXTURE(TestTokenBridge, testCanonicalLexicalHashing)
{
    m_pDoc->InsertTab(0, u"Hashing"_ustr);

    ScSingleRefData aRelativeRef;
    aRelativeRef.InitFlags();
    aRelativeRef.SetRelCol(1);
    aRelativeRef.SetRelRow(5);
    aRelativeRef.SetRelTab(0);

    ScTokenArray aLeft(*m_pDoc);
    aLeft.AddSingleReference(aRelativeRef);
    aLeft.AddOpCode(ocAdd);
    aLeft.GenHash();

    ScSingleRefData aShiftedRef = aRelativeRef;
    aShiftedRef.SetRelCol(7);
    aShiftedRef.SetRelRow(99);

    ScTokenArray aShifted(*m_pDoc);
    aShifted.AddSingleReference(aShiftedRef);
    aShifted.AddOpCode(ocAdd);
    aShifted.GenHash();

    CPPUNIT_ASSERT_EQUAL(aLeft.GetHash(), aShifted.GetHash());

    ScSingleRefData aAbsoluteRef = aRelativeRef;
    aAbsoluteRef.SetRowRel(false);
    aAbsoluteRef.SetAbsRow(5);

    ScTokenArray aAbsolute(*m_pDoc);
    aAbsolute.AddSingleReference(aAbsoluteRef);
    aAbsolute.AddOpCode(ocAdd);
    aAbsolute.GenHash();

    CPPUNIT_ASSERT(aLeft.GetHash() != aAbsolute.GetHash());
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
