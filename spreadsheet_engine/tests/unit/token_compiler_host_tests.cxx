/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cstdlib>
#include <iostream>
#include <optional>

#include <spreadsheetengine/detail/CompileHost.hxx>
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

} // namespace

int main()
{
    if (int nResult = testTokenHashAndEquality(); nResult != EXIT_SUCCESS)
        return nResult;

    if (int nResult = testCompileHostShape(); nResult != EXIT_SUCCESS)
        return nResult;

    std::cout << "spreadsheetengine token/compiler host tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
