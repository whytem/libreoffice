/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cstdlib>
#include <iostream>
#include <optional>

#include <spreadsheetengine/detail/CompileHost.hxx>
#include <spreadsheetengine/detail/CompilerPipeline.hxx>
#include <spreadsheetengine/detail/SharedFormulaToken.hxx>
#include <spreadsheetengine/detail/TokenStringifier.hxx>
#include <spreadsheetengine/detail/TokenModel.hxx>

#include "TestSupport.hxx"

namespace
{

using spreadsheetengine::standalone::test::fail;

class DummyCompileHost final : public spreadsheetengine::detail::compiler::NameResolver
    , public spreadsheetengine::detail::compiler::DatabaseRangeResolver
    , public spreadsheetengine::detail::compiler::TableRefResolver
    , public spreadsheetengine::detail::compiler::ColRowNameResolver
    , public spreadsheetengine::detail::compiler::ExternalNameResolver
{
public:
    [[nodiscard]] std::optional<spreadsheetengine::detail::token::NameData> lookupRangeName(
        spreadsheetengine::api::StringView rName,
        std::optional<spreadsheetengine::api::SheetId> onSheet,
        const spreadsheetengine::detail::compiler::CompileContext&) const override
    {
        if (rName == u"LocalName" && onSheet == 3)
            return spreadsheetengine::detail::token::NameData { 3, 17 };
        return std::nullopt;
    }

    [[nodiscard]]
    std::optional<spreadsheetengine::detail::token::DatabaseRangeData> lookupDatabaseRange(
        spreadsheetengine::api::StringView rName,
        const spreadsheetengine::detail::compiler::CompileContext&) const override
    {
        if (rName == u"DatabaseArea")
            return spreadsheetengine::detail::token::DatabaseRangeData { 9 };
        return std::nullopt;
    }

    [[nodiscard]] std::optional<spreadsheetengine::detail::token::TableRefData> lookupTableReference(
        spreadsheetengine::api::StringView rTableName, spreadsheetengine::api::StringView rItemName,
        const spreadsheetengine::detail::compiler::CompileContext&) const override
    {
        if (rTableName == u"Sales" && rItemName == u"#Data")
        {
            return spreadsheetengine::detail::token::TableRefData {
                4, spreadsheetengine::detail::token::TableRefItem::Data
            };
        }
        return std::nullopt;
    }

    [[nodiscard]]
    std::optional<spreadsheetengine::api::refdata::SingleRefData> lookupColRowName(
        spreadsheetengine::api::StringView rName,
        const spreadsheetengine::detail::compiler::CompileContext&) const override
    {
        if (rName == u"RowName")
        {
            spreadsheetengine::api::refdata::SingleRefData aData;
            aData.mnColumn = 5;
            aData.mnRow = 7;
            aData.mnSheet = 0;
            return aData;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<spreadsheetengine::detail::token::ExternalNameData>
    lookupExternalName(spreadsheetengine::api::StringView rSymbol,
        const spreadsheetengine::detail::compiler::CompileContext&) const override
    {
        if (rSymbol == u"'file.ods'#$GlobalName")
            return spreadsheetengine::detail::token::ExternalNameData { 2, u"GlobalName" };
        return std::nullopt;
    }
};

int testTokenHashAndEquality()
{
    namespace token = spreadsheetengine::detail::token;

    token::Token aTextToken {
        token::Kind::String,
        1,
        token::StringData { u"Revenue", u"revenue" },
    };
    token::Token aTextTokenCopy = aTextToken;
    token::Token aDifferentTextToken {
        token::Kind::String,
        1,
        token::StringData { u"Cost", u"cost" },
    };

    if (!(aTextToken == aTextTokenCopy))
        return fail("spreadsheetengine_token_compiler_host_tests", "token equality mismatch");
    if (token::hashToken(aTextToken) != token::hashToken(aTextTokenCopy))
        return fail("spreadsheetengine_token_compiler_host_tests", "token hash stability mismatch");
    if (token::hashToken(aTextToken) == token::hashToken(aDifferentTextToken))
        return fail("spreadsheetengine_token_compiler_host_tests", "token hash did not change");

    spreadsheetengine::api::refdata::SingleRefData aSingleRef;
    aSingleRef.mnColumn = 2;
    aSingleRef.mnRow = 10;
    aSingleRef.mnSheet = 1;
    aSingleRef.maFlags.mbColumnRelative = true;
    aSingleRef.maFlags.mbRowRelative = true;

    token::CompiledFormula aFormula {
        {
            { token::Kind::SingleRef, 1, aSingleRef },
            { token::Kind::PlainOpcode, 2, {} },
            { token::Kind::Value, 1, 12.5 },
        },
        token::XmlFormulaSource { u"of:=SUM([.A1:.A3])", u"of" },
        502,
        0x10,
        true,
        true,
        false,
        token::VectorState::Enabled,
        true,
        true,
    };

    token::CompiledFormula aFormulaCopy = aFormula;
    token::CompiledFormula aFormulaDifferent = aFormula;
    aFormulaDifferent.maTokens.back().maPayload = 13.5;

    if (!(aFormula == aFormulaCopy))
        return fail("spreadsheetengine_token_compiler_host_tests", "compiled formula equality mismatch");
    if (token::hashCompiledFormula(aFormula) != token::hashCompiledFormula(aFormulaCopy))
        return fail("spreadsheetengine_token_compiler_host_tests", "compiled formula hash mismatch");
    if (token::hashCompiledFormula(aFormula) == token::hashCompiledFormula(aFormulaDifferent))
        return fail("spreadsheetengine_token_compiler_host_tests",
            "compiled formula hash failed to distinguish payload change");
    aFormulaDifferent = aFormula;
    aFormulaDifferent.mnCodeError = 0;
    if (token::hashCompiledFormula(aFormula) == token::hashCompiledFormula(aFormulaDifferent))
        return fail("spreadsheetengine_token_compiler_host_tests",
            "compiled formula hash failed to distinguish metadata change");
    if (token::kindName(token::Kind::TableRef) != u"TableRef")
        return fail(
            "spreadsheetengine_token_compiler_host_tests", "token kind name mismatch");

    token::MatrixData aMatrix;
    aMatrix.mnColumns = 2;
    aMatrix.mnRows = 2;
    aMatrix.maValues = { 1.0, spreadsheetengine::api::String(u"two"), token::ErrorCode(0x7fff),
        4.0 };
    token::Token aMatrixToken { token::Kind::Matrix, 1, aMatrix };
    if (token::hashToken(aMatrixToken) == 0)
        return fail("spreadsheetengine_token_compiler_host_tests", "matrix token hash missing");

    return EXIT_SUCCESS;
}

int testCompileHostShape()
{
    using spreadsheetengine::detail::compiler::CompileContext;
    using spreadsheetengine::detail::compiler::CompileHosts;

    DummyCompileHost aHost;
    CompileHosts aHosts { &aHost, &aHost, &aHost, &aHost, &aHost };
    CompileContext aContext;
    aContext.maGrammar = { spreadsheetengine::api::FormulaLanguage::Odff,
        spreadsheetengine::api::AddressConvention::OdfA1, false };
    aContext.maBaseAddress = { 3, 8, 12 };

    if (!aHosts.mpNameResolver || !aHosts.mpDatabaseRangeResolver || !aHosts.mpTableRefResolver
        || !aHosts.mpColRowNameResolver || !aHosts.mpExternalNameResolver)
    {
        return fail("spreadsheetengine_token_compiler_host_tests", "compile host bundle mismatch");
    }

    const auto aLocalName = aHosts.mpNameResolver->lookupRangeName(u"LocalName", 3, aContext);
    const auto aDbRange = aHosts.mpDatabaseRangeResolver->lookupDatabaseRange(
        u"DatabaseArea", aContext);
    const auto aTableRef
        = aHosts.mpTableRefResolver->lookupTableReference(u"Sales", u"#Data", aContext);
    const auto aColRowName
        = aHosts.mpColRowNameResolver->lookupColRowName(u"RowName", aContext);
    const auto aExternalName = aHosts.mpExternalNameResolver->lookupExternalName(
        u"'file.ods'#$GlobalName", aContext);

    if (!aLocalName || aLocalName->mnSheet != 3 || aLocalName->mnIndex != 17)
        return fail("spreadsheetengine_token_compiler_host_tests", "range name lookup mismatch");
    if (!aDbRange || aDbRange->mnIndex != 9)
        return fail("spreadsheetengine_token_compiler_host_tests", "database range lookup mismatch");
    if (!aTableRef || aTableRef->mnIndex != 4
        || aTableRef->meItem != spreadsheetengine::detail::token::TableRefItem::Data)
    {
        return fail("spreadsheetengine_token_compiler_host_tests", "table ref lookup mismatch");
    }
    if (!aColRowName || aColRowName->mnColumn != 5 || aColRowName->mnRow != 7)
        return fail("spreadsheetengine_token_compiler_host_tests", "col/row name lookup mismatch");
    if (!aExternalName || aExternalName->mnFileId != 2 || aExternalName->maName != u"GlobalName")
        return fail("spreadsheetengine_token_compiler_host_tests", "external name lookup mismatch");

    return EXIT_SUCCESS;
}

int testCompilePipelineShape()
{
    using spreadsheetengine::detail::compiler::CompileContext;
    using spreadsheetengine::detail::compiler::CompileHosts;
    using spreadsheetengine::detail::compiler::CompileRequest;
    using spreadsheetengine::detail::compiler::CompileStatus;
    using spreadsheetengine::detail::compiler::FormulaSource;
    using spreadsheetengine::detail::compiler::hasCompleteHostBundle;

    DummyCompileHost aHost;
    CompileHosts aIncompleteHosts;
    CompileHosts aCompleteHosts { &aHost, &aHost, &aHost, &aHost, &aHost };

    if (hasCompleteHostBundle(aIncompleteHosts))
        return fail("spreadsheetengine_token_compiler_host_tests", "incomplete host bundle accepted");
    if (!hasCompleteHostBundle(aCompleteHosts))
        return fail("spreadsheetengine_token_compiler_host_tests", "complete host bundle rejected");

    CompileContext aContext;
    aContext.maGrammar = { spreadsheetengine::api::FormulaLanguage::English,
        spreadsheetengine::api::AddressConvention::OooA1, true };
    aContext.maBaseAddress = { 1, 2, 3 };

    CompileRequest aRequest {
        FormulaSource { u"=SUM(A1:A3)", u"" },
        aContext,
        aCompleteHosts,
    };

    if (aRequest.maSource.hasNamespace())
        return fail("spreadsheetengine_token_compiler_host_tests", "unexpected compile namespace state");

    aRequest.maSource.maNamespace = u"of";
    if (!aRequest.maSource.hasNamespace())
        return fail("spreadsheetengine_token_compiler_host_tests", "compile namespace state mismatch");

    CompileStatus aStatus;
    if (!static_cast<bool>(aStatus))
        return fail("spreadsheetengine_token_compiler_host_tests", "fresh compile status should be truthy");

    aStatus.maFailureMessage = u"failed";
    if (static_cast<bool>(aStatus))
        return fail("spreadsheetengine_token_compiler_host_tests", "failed compile status should be falsy");

    return EXIT_SUCCESS;
}

int testSharedFormulaTokenServices()
{
    namespace setoken = spreadsheetengine::detail::token;
    namespace seshared = spreadsheetengine::api::sharedformula;
    namespace sesharedtoken = spreadsheetengine::detail::sharedformulatoken;

    spreadsheetengine::api::refdata::SingleRefData aRelativeRef;
    aRelativeRef.mnColumn = 1;
    aRelativeRef.mnRow = 10;
    aRelativeRef.mnSheet = 0;
    aRelativeRef.maFlags.mbColumnRelative = true;
    aRelativeRef.maFlags.mbRowRelative = true;

    auto aShiftedRelativeRef = aRelativeRef;
    aShiftedRelativeRef.mnColumn = 4;
    aShiftedRelativeRef.mnRow = 99;

    auto aAbsoluteRef = aRelativeRef;
    aAbsoluteRef.maFlags.mbRowRelative = false;

    std::vector<setoken::Token> aHashLeft {
        { setoken::Kind::SingleRef, setoken::kOpCodePush, aRelativeRef },
        { setoken::Kind::PlainOpcode, setoken::kOpCodeAdd, {} },
    };
    std::vector<setoken::Token> aHashShifted {
        { setoken::Kind::SingleRef, setoken::kOpCodePush, aShiftedRelativeRef },
        { setoken::Kind::PlainOpcode, setoken::kOpCodeAdd, {} },
    };
    std::vector<setoken::Token> aHashAbsolute {
        { setoken::Kind::SingleRef, setoken::kOpCodePush, aAbsoluteRef },
        { setoken::Kind::PlainOpcode, setoken::kOpCodeAdd, {} },
    };
    if (sesharedtoken::hashSharedFormulaLexicalTokens(aHashLeft)
        != sesharedtoken::hashSharedFormulaLexicalTokens(aHashShifted))
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "shared-formula lexical hash should ignore absolute positions");
    }
    if (sesharedtoken::hashSharedFormulaLexicalTokens(aHashLeft)
        == sesharedtoken::hashSharedFormulaLexicalTokens(aHashAbsolute))
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "shared-formula lexical hash should distinguish row-relative flags");
    }

    std::vector<setoken::Token> aRelativeCompareLeft {
        { setoken::Kind::SingleRef, setoken::kOpCodePush, aRelativeRef },
        { setoken::Kind::PlainOpcode, setoken::kOpCodeAdd, {} },
    };
    std::vector<setoken::Token> aRelativeCompareRight = aRelativeCompareLeft;
    const auto eLexicalRelative = sesharedtoken::compareSharedFormulaTokenStreams(
        sesharedtoken::StreamKind::Lexical, aRelativeCompareLeft, aRelativeCompareRight);
    if (eLexicalRelative != seshared::TokenCompareState::EqualRelativeRef)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "shared-formula lexical comparison should keep relative refs variant");
    }

    std::vector<setoken::Token> aLexicalInvariant {
        { setoken::Kind::RangeName, setoken::kOpCodeName,
            setoken::NameData { 0, 7 } },
        { setoken::Kind::PlainOpcode, setoken::kOpCodeAdd, {} },
    };
    if (sesharedtoken::compareSharedFormulaTokenStreams(
            sesharedtoken::StreamKind::Lexical, aLexicalInvariant, aLexicalInvariant)
        != seshared::TokenCompareState::EqualInvariant)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "shared-formula lexical invariant comparison mismatch");
    }

    std::vector<setoken::Token> aRpnMatrix {
        { setoken::Kind::Matrix, setoken::kOpCodePush, setoken::MatrixData {} },
    };
    if (sesharedtoken::compareSharedFormulaTokenStreams(
            sesharedtoken::StreamKind::Rpn, aRpnMatrix, aRpnMatrix)
        != seshared::TokenCompareState::NotEqual)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "shared-formula RPN matrix comparison should refuse grouping");
    }

    return EXIT_SUCCESS;
}

int testTokenStringifier()
{
    namespace setoken = spreadsheetengine::detail::token;
    namespace setokenstring = spreadsheetengine::detail::tokenstringifier;

    spreadsheetengine::api::refdata::SingleRefData aReference;
    aReference.mnColumn = 2;
    aReference.mnRow = 4;
    aReference.mnSheet = 0;
    aReference.maFlags.mbColumnRelative = true;
    aReference.maFlags.mbRowRelative = true;

    setoken::CompiledFormula aFormula {
        {
            { setoken::Kind::SingleRef, setoken::kOpCodePush, aReference },
            { setoken::Kind::PlainOpcode, setoken::kOpCodeAdd, {} },
            { setoken::Kind::Value, setoken::kOpCodePush, 12.5 },
        },
        std::nullopt,
        0,
        0,
        false,
        false,
        true,
        setoken::VectorState::Unknown,
        false,
        false,
    };

    const auto aTokenText = setokenstring::tokenToDiagnosticString(aFormula.maTokens.front());
    if (aTokenText.find(u"SingleRef") == std::u16string::npos
        || aTokenText.find(u"rel=[true,true,false]") == std::u16string::npos)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "token diagnostic stringifier mismatch");
    }

    const auto aFormulaText = setokenstring::compiledFormulaToDiagnosticString(aFormula);
    if (aFormulaText.find(u"tokens=[") == std::u16string::npos
        || aFormulaText.find(u"Value(op=0)") == std::u16string::npos)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "compiled formula diagnostic stringifier mismatch");
    }

    return EXIT_SUCCESS;
}

} // namespace

int main()
{
    if (int nResult = testTokenHashAndEquality(); nResult != EXIT_SUCCESS)
        return nResult;

    if (int nResult = testCompileHostShape(); nResult != EXIT_SUCCESS)
        return nResult;

    if (int nResult = testCompilePipelineShape(); nResult != EXIT_SUCCESS)
        return nResult;

    if (int nResult = testSharedFormulaTokenServices(); nResult != EXIT_SUCCESS)
        return nResult;

    if (int nResult = testTokenStringifier(); nResult != EXIT_SUCCESS)
        return nResult;

    std::cout << "spreadsheetengine token/compiler host tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
