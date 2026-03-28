/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cstdlib>
#include <iostream>
#include <optional>

#include <spreadsheetengine/detail/CompileHost.hxx>
#include <spreadsheetengine/detail/CompilerPipeline.hxx>
#include <spreadsheetengine/detail/FodsCompilerPreflight.hxx>
#include <spreadsheetengine/detail/SharedFormulaToken.hxx>
#include <spreadsheetengine/detail/TokenStringifier.hxx>
#include <spreadsheetengine/detail/TokenModel.hxx>
#include <spreadsheetengine/detail/WorkbookCompilerLowering.hxx>
#include <spreadsheetengine/detail/WorkbookCompileHost.hxx>

#include "TestSupport.hxx"

namespace
{

using spreadsheetengine::standalone::test::fail;

std::string toUtf8(spreadsheetengine::api::StringView rText)
{
    std::string aUtf8;
    aUtf8.reserve(rText.size());
    for (std::size_t nIndex = 0; nIndex < rText.size(); ++nIndex)
    {
        char32_t nCodePoint = rText[nIndex];
        if (0xD800 <= nCodePoint && nCodePoint <= 0xDBFF && nIndex + 1 < rText.size())
        {
            const char32_t nTrail = rText[nIndex + 1];
            if (0xDC00 <= nTrail && nTrail <= 0xDFFF)
            {
                nCodePoint = 0x10000 + ((nCodePoint - 0xD800) << 10) + (nTrail - 0xDC00);
                ++nIndex;
            }
        }

        if (nCodePoint <= 0x7F)
            aUtf8.push_back(static_cast<char>(nCodePoint));
        else if (nCodePoint <= 0x7FF)
        {
            aUtf8.push_back(static_cast<char>(0xC0 | (nCodePoint >> 6)));
            aUtf8.push_back(static_cast<char>(0x80 | (nCodePoint & 0x3F)));
        }
        else if (nCodePoint <= 0xFFFF)
        {
            aUtf8.push_back(static_cast<char>(0xE0 | (nCodePoint >> 12)));
            aUtf8.push_back(static_cast<char>(0x80 | ((nCodePoint >> 6) & 0x3F)));
            aUtf8.push_back(static_cast<char>(0x80 | (nCodePoint & 0x3F)));
        }
        else
        {
            aUtf8.push_back(static_cast<char>(0xF0 | (nCodePoint >> 18)));
            aUtf8.push_back(static_cast<char>(0x80 | ((nCodePoint >> 12) & 0x3F)));
            aUtf8.push_back(static_cast<char>(0x80 | ((nCodePoint >> 6) & 0x3F)));
            aUtf8.push_back(static_cast<char>(0x80 | (nCodePoint & 0x3F)));
        }
    }
    return aUtf8;
}

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

int testWorkbookCompileHost()
{
    namespace secompiler = spreadsheetengine::detail::compiler;
    namespace seworkbook = spreadsheetengine::core::workbook;

    seworkbook::Workbook aWorkbook;
    aWorkbook.maSheets.push_back(seworkbook::Sheet { u"Sheet1" });
    aWorkbook.maSheets.push_back(seworkbook::Sheet { u"Lookup" });
    aWorkbook.maSheets.push_back(seworkbook::Sheet { u"AOO #117989" });
    aWorkbook.maNamedRanges.push_back(
        seworkbook::NamedRange { u"GlobalRange", {}, u"$Sheet1.$A$1", u"$Sheet1.$A$1:.$A$2" });
    aWorkbook.maNamedRanges.push_back(seworkbook::NamedRange { u"ShadowedName", u"Lookup",
        u"$Lookup.$B$2", u"$Lookup.$B$2:.$B$4" });
    aWorkbook.maNamedRanges.push_back(
        seworkbook::NamedRange { u"ShadowedName", {}, u"$Sheet1.$C$1", u"$Sheet1.$C$1:.$C$2" });
    aWorkbook.maNamedRanges.push_back(
        seworkbook::NamedRange { u"MixedCase", u"Lookup", u"$Lookup.$D$1", u"$Lookup.$D$1:.$D$1" });

    secompiler::WorkbookCompileHost aHost(aWorkbook);
    const auto aHosts = aHost.hosts();
    if (!secompiler::hasCompleteHostBundle(aHosts))
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compile host bundle should be complete");
    }

    const auto& rSupport = aHost.support();
    if (rSupport.meRangeNames != secompiler::LookupSupport::Supported
        || rSupport.meDatabaseRanges != secompiler::LookupSupport::Unsupported
        || rSupport.meTableRefs != secompiler::LookupSupport::Unsupported
        || rSupport.meColRowNames != secompiler::LookupSupport::Unsupported
        || rSupport.meExternalNames != secompiler::LookupSupport::Unsupported)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compile host support contract mismatch");
    }

    const auto oContext = secompiler::makeWorkbookCompileContext(aWorkbook, u"Lookup", 3, 4);
    if (!oContext)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compile context creation failed");
    }

    if (oContext->maGrammar.meLanguage
               != secompiler::kDefaultWorkbookCompileGrammar.meLanguage
        || oContext->maGrammar.meAddressConvention
               != secompiler::kDefaultWorkbookCompileGrammar.meAddressConvention
        || oContext->maGrammar.mbEnglish
               != secompiler::kDefaultWorkbookCompileGrammar.mbEnglish
        || oContext->maBaseAddress != spreadsheetengine::api::CellAddress { 1, 3, 4 }
        || oContext->mbAllowExternalReferences || oContext->mbForPersistence
        || oContext->mbComputeImplicitIntersection || oContext->mbMatrixFormula)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compile context defaults mismatch");
    }

    const auto oMissingContext
        = secompiler::makeWorkbookCompileContext(aWorkbook, u"Missing", 0, 0);
    if (oMissingContext)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compile context unexpectedly resolved missing sheet");
    }

    const auto aLocal = aHosts.mpNameResolver->lookupRangeName(u"ShadowedName", 1, *oContext);
    const auto aGlobalOnOtherSheet
        = aHosts.mpNameResolver->lookupRangeName(u"ShadowedName", 0, *oContext);
    const auto aGlobalWithoutScope
        = aHosts.mpNameResolver->lookupRangeName(u"ShadowedName", std::nullopt, *oContext);
    const auto aCaseFolded = aHosts.mpNameResolver->lookupRangeName(u"mixedcase", 1, *oContext);
    const auto aMissingName
        = aHosts.mpNameResolver->lookupRangeName(u"UnknownName", 1, *oContext);

    if (!aLocal || aLocal->mnSheet != 1 || aLocal->mnIndex != 2)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook local range name lookup mismatch");
    }
    if (!aGlobalOnOtherSheet || aGlobalOnOtherSheet->mnSheet != -1
        || aGlobalOnOtherSheet->mnIndex != 3)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook global fallback lookup mismatch");
    }
    if (!aGlobalWithoutScope || aGlobalWithoutScope->mnSheet != -1
        || aGlobalWithoutScope->mnIndex != 3)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook unscope range name lookup mismatch");
    }
    if (!aCaseFolded || aCaseFolded->mnSheet != 1 || aCaseFolded->mnIndex != 4)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook case-folded range name lookup mismatch");
    }
    if (aMissingName)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook missing range name unexpectedly resolved");
    }

    if (aHosts.mpDatabaseRangeResolver->lookupDatabaseRange(u"DB", *oContext)
        || aHosts.mpTableRefResolver->lookupTableReference(u"Table1", u"#Data", *oContext)
        || aHosts.mpColRowNameResolver->lookupColRowName(u"Heading", *oContext)
        || aHosts.mpExternalNameResolver->lookupExternalName(u"'file.ods'#$Name", *oContext))
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compile host unexpectedly resolved unsupported lookup");
    }

    return EXIT_SUCCESS;
}

int testWorkbookCompilerPreflight()
{
    namespace secompiler = spreadsheetengine::detail::compiler;
    namespace seworkbook = spreadsheetengine::core::workbook;

    seworkbook::Workbook aWorkbook;
    aWorkbook.maSheets.push_back(seworkbook::Sheet { u"Sheet1" });
    aWorkbook.maSheets.push_back(seworkbook::Sheet { u"Lookup" });
    aWorkbook.maNamedRanges.push_back(
        seworkbook::NamedRange { u"GlobalRange", {}, u"$Sheet1.$A$1", u"$Sheet1.$A$1:.$A$2" });
    aWorkbook.maNamedRanges.push_back(
        seworkbook::NamedRange { u"LocalOnly", u"Lookup", u"$Lookup.$B$2", u"$Lookup.$B$2:.$B$4" });

    secompiler::WorkbookCompileHost aHost(aWorkbook);
    const auto oContext = secompiler::makeWorkbookCompileContext(aWorkbook, u"Lookup", 1, 1);
    if (!oContext)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler preflight context mismatch");
    }

    const auto aReady = secompiler::preflightFormulaSource(
        u"of:=SUM([.A1:.A3];LocalOnly;GlobalRange)", aHost, *oContext);
    if (!aReady || !aReady.mbUsesFunctionCall || !aReady.mbUsesRangeReference
        || !aReady.mbUsesNamedReference)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler preflight ready-path mismatch");
    }

    const auto aMissingName
        = secompiler::preflightFormulaSource(u"of:=SUM(MissingName)", aHost, *oContext);
    if (aMissingName
        || aMissingName.meReason != secompiler::FormulaPreflightReason::MissingNamedReference
        || aMissingName.maDetail != u"MissingName")
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler preflight missing-name mismatch");
    }

    const auto aBadArray
        = secompiler::preflightFormulaSource(u"of:=SUM({[.A1]})", aHost, *oContext);
    if (aBadArray
        || aBadArray.meReason != secompiler::FormulaPreflightReason::UnsupportedArrayElement
        || aBadArray.maDetail != u"CellReference")
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler preflight array mismatch");
    }

    const auto aSignedArray
        = secompiler::preflightFormulaSource(u"of:=SUM({0.7;1;-0.4;0.04;-0.002388})", aHost, *oContext);
    if (!aSignedArray || !aSignedArray.mbUsesArrayConstant)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler preflight signed-array mismatch");
    }

    const auto aBareRange
        = secompiler::preflightFormulaSource(u"of:=COM.MICROSOFT.TEXTJOIN(\"-\";1;I13:K13)",
            aHost, *oContext);
    if (!aBareRange || !aBareRange.mbUsesFunctionCall || !aBareRange.mbUsesRangeReference)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler preflight bare-range mismatch");
    }

    const auto aReferenceList = secompiler::preflightFormulaSource(
        u"of:=AREAS(([.A1:.B3]~[.F2]~[.G1]))", aHost, *oContext);
    if (!aReferenceList || !aReferenceList.mbUsesFunctionCall
        || !aReferenceList.mbUsesRangeReference || !aReferenceList.mbUsesCellReference)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler preflight reference-list mismatch");
    }

    const auto aBadReferenceList
        = secompiler::preflightFormulaSource(u"of:=AREAS(([.A1]~1))", aHost, *oContext);
    if (aBadReferenceList
        || aBadReferenceList.meReason
               != secompiler::FormulaPreflightReason::UnsupportedReferenceListElement
        || aBadReferenceList.maDetail != u"NumberLiteral")
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler preflight bad reference-list mismatch");
    }

    const auto aRangeConstructor = secompiler::preflightFormulaSource(
        u"of:=SUM([.$O6]:CHOOSE(([.$H$2]-1);[.$O6];[.$P6];[.$Q6];[.$R6]))", aHost, *oContext);
    if (!aRangeConstructor || !aRangeConstructor.mbUsesFunctionCall
        || !aRangeConstructor.mbUsesCellReference)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler preflight range-constructor mismatch");
    }

    const auto aBadRangeConstructor
        = secompiler::preflightFormulaSource(u"of:=SUM([.$O6]:ABS(1))", aHost, *oContext);
    if (aBadRangeConstructor
        || aBadRangeConstructor.meReason != secompiler::FormulaPreflightReason::ParseFailure)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler preflight bad range-constructor mismatch");
    }

    const auto aErrColon
        = secompiler::preflightFormulaSource(u"of:=ERROR.TYPE(err:7)", aHost, *oContext);
    if (aErrColon || aErrColon.meReason != secompiler::FormulaPreflightReason::ParseFailure)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler preflight err-colon mismatch");
    }

    const auto aAdjacentAnd = secompiler::preflightFormulaSource(
        u"of:=([.A3]=[.D3])AND([.B3]=[.E3])AND([.C3]=[.F3])", aHost, *oContext);
    if (!aAdjacentAnd || !aAdjacentAnd.mbUsesFunctionCall || !aAdjacentAnd.mbUsesCellReference)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler preflight adjacent-AND mismatch");
    }

    const auto aParseFailure = secompiler::preflightFormulaSource(u"of:=ABS(", aHost, *oContext);
    if (aParseFailure || aParseFailure.meReason != secompiler::FormulaPreflightReason::ParseFailure)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler preflight parse-failure mismatch");
    }

    return EXIT_SUCCESS;
}

int testWorkbookCompilerLowering()
{
    namespace secompiler = spreadsheetengine::detail::compiler;
    namespace seworkbook = spreadsheetengine::core::workbook;
    namespace setoken = spreadsheetengine::detail::token;

    seworkbook::Workbook aWorkbook;
    aWorkbook.maSheets.push_back(seworkbook::Sheet { u"Sheet1" });
    aWorkbook.maSheets.push_back(seworkbook::Sheet { u"Lookup" });
    aWorkbook.maNamedRanges.push_back(
        seworkbook::NamedRange { u"GlobalRange", {}, u"$Sheet1.$A$1", u"$Sheet1.$A$1:.$A$2" });
    aWorkbook.maNamedRanges.push_back(
        seworkbook::NamedRange { u"LocalOnly", u"Lookup", u"$Lookup.$B$2", u"$Lookup.$B$2:.$B$4" });

    secompiler::WorkbookCompileHost aHost(aWorkbook);
    const auto oContext = secompiler::makeWorkbookCompileContext(aWorkbook, u"Lookup", 1, 1);
    if (!oContext)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lowering context mismatch");
    }

    const auto aBasic = secompiler::lowerFormulaSource(
        u"of:=SUM([.A1:.A3];LocalOnly;GlobalRange)", aHost, *oContext);
    if (!aBasic || aBasic.maFormula.moXmlFormulaSource || aBasic.maFormula.maTokens.size() != 6)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lowering basic formula mismatch");
    }
    if (aBasic.maFormula.maTokens[0].meKind != setoken::Kind::DoubleRef
        || aBasic.maFormula.maTokens[1].meKind != setoken::Kind::RangeName
        || aBasic.maFormula.maTokens[2].meKind != setoken::Kind::RangeName
        || aBasic.maFormula.maTokens[3].meKind != setoken::Kind::StringName
        || aBasic.maFormula.maTokens[4].meKind != setoken::Kind::Byte
        || aBasic.maFormula.maTokens[5].mnOpCode != secompiler::detail::kLoweredOpFunctionCall)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lowering token shape mismatch");
    }
    const auto& rLocalName = std::get<setoken::NameData>(aBasic.maFormula.maTokens[1].maPayload);
    const auto& rGlobalName = std::get<setoken::NameData>(aBasic.maFormula.maTokens[2].maPayload);
    if (rLocalName.mnSheet != 1 || rLocalName.mnIndex != 2 || rGlobalName.mnSheet != -1
        || rGlobalName.mnIndex != 1)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lowering named-range payload mismatch");
    }

    const auto aAdd = secompiler::lowerFormulaSource(u"of:=[.A1]+[.B1]", aHost, *oContext);
    if (!aAdd || aAdd.maFormula.maTokens.size() != 3
        || aAdd.maFormula.maTokens.back().mnOpCode != setoken::kOpCodeAdd)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lowering add-opcode mismatch");
    }

    const auto aLexicalSum = secompiler::lowerFormulaSourceLexical(
        u"of:=SUM([.A1:.A3];LocalOnly)", aHost, *oContext);
    if (!aLexicalSum || aLexicalSum.maFormula.maTokens.size() != 6
        || aLexicalSum.maFormula.maTokens[0].meKind != setoken::Kind::Byte
        || aLexicalSum.maFormula.maTokens[0].mnOpCode != setoken::kOpCodeSum
        || aLexicalSum.maFormula.maTokens[1].meKind != setoken::Kind::PlainOpcode
        || aLexicalSum.maFormula.maTokens[1].mnOpCode != setoken::kOpCodeOpen
        || aLexicalSum.maFormula.maTokens[3].mnOpCode != setoken::kOpCodeSep
        || aLexicalSum.maFormula.maTokens[5].mnOpCode != setoken::kOpCodeClose)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lexical SUM lowering mismatch");
    }

    const auto aLexicalIfError = secompiler::lowerFormulaSourceLexical(
        u"of:=IFERROR([.A1]/[.B1];0)", aHost, *oContext);
    if (!aLexicalIfError || aLexicalIfError.maFormula.maTokens.size() != 8
        || aLexicalIfError.maFormula.maTokens[0].meKind != setoken::Kind::Jump
        || aLexicalIfError.maFormula.maTokens[0].mnOpCode != setoken::kOpCodeIfError)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lexical IFERROR lowering mismatch");
    }

    const auto aLexicalCompatName = secompiler::lowerFormulaSourceLexical(
        u"of:=COM.MICROSOFT.CONCAT(\"a\";\"b\")", aHost, *oContext);
    if (!aLexicalCompatName || aLexicalCompatName.maFormula.maTokens.size() != 6
        || aLexicalCompatName.maFormula.maTokens[0].meKind != setoken::Kind::String
        || aLexicalCompatName.maFormula.maTokens[0].mnOpCode != setoken::kOpCodeBad)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lexical compat-name lowering mismatch");
    }

    const auto aNegSub = secompiler::lowerFormulaSource(u"of:=-[.A1]", aHost, *oContext);
    if (!aNegSub || aNegSub.maFormula.maTokens.size() != 2
        || aNegSub.maFormula.maTokens.back().mnOpCode != setoken::kOpCodeNegSub)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lowering negsub-opcode mismatch");
    }

    const auto aReferenceList = secompiler::lowerFormulaSource(
        u"of:=AREAS(([.A1:.B3]~[.F2]~[.G1]))", aHost, *oContext);
    if (!aReferenceList || aReferenceList.maFormula.maTokens.empty())
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lowering reference-list mismatch");
    }
    bool bSawReferenceList = false;
    int nUnionCount = 0;
    for (const auto& rToken : aReferenceList.maFormula.maTokens)
    {
        if (rToken.mnOpCode == setoken::kOpCodeUnion)
        {
            bSawReferenceList = true;
            ++nUnionCount;
        }
    }
    if (!bSawReferenceList || nUnionCount != 2)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lowering did not emit reference-list unions");
    }

    const auto aRangeConstructor = secompiler::lowerFormulaSource(
        u"of:=SUM([.$O6]:CHOOSE(([.$H$2]-1);[.$O6];[.$P6];[.$Q6]))", aHost, *oContext);
    if (!aRangeConstructor || aRangeConstructor.maFormula.maTokens.empty())
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lowering range-constructor mismatch");
    }
    bool bSawRangeConstructor = false;
    for (const auto& rToken : aRangeConstructor.maFormula.maTokens)
    {
        if (rToken.mnOpCode == setoken::kOpCodeRange)
            bSawRangeConstructor = true;
    }
    if (!bSawRangeConstructor)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lowering did not emit range-constructor opcode");
    }

    const auto aAdjacentAnd = secompiler::lowerFormulaSource(
        u"of:=([.A3]=[.D3])AND([.B3]=[.E3])AND([.C3]=[.F3])", aHost, *oContext);
    if (!aAdjacentAnd || aAdjacentAnd.maFormula.maTokens.empty()
        || aAdjacentAnd.maFormula.maTokens.back().mnOpCode
               != secompiler::detail::kLoweredOpFunctionCall)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lowering adjacent-AND mismatch");
    }

    const auto aExternalReference = secompiler::lowerFormulaSource(
        u"of:=COLUMN(['file:///fake_path/filename'#$Sheet3.B3:.D8])", aHost, *oContext);
    if (!aExternalReference || aExternalReference.maFormula.maTokens.empty()
        || aExternalReference.maFormula.maTokens.front().meKind
               != setoken::Kind::ExternalDoubleRef)
    {
        std::string aDetail = "workbook compiler lowering external-reference mismatch";
        if (!aExternalReference)
        {
            aDetail += " (reason=";
            aDetail += std::to_string(static_cast<int>(aExternalReference.meReason));
            if (!aExternalReference.maDetail.empty())
            {
                aDetail += ", detail=";
                aDetail += toUtf8(aExternalReference.maDetail);
            }
            aDetail += ")";
        }
        else if (!aExternalReference.maFormula.maTokens.empty())
        {
            aDetail += " (kind=";
            aDetail += std::to_string(
                static_cast<int>(aExternalReference.maFormula.maTokens.front().meKind));
            aDetail += ")";
        }
        return fail("spreadsheetengine_token_compiler_host_tests", aDetail.c_str());
    }

    const auto aQuotedSheetHash = secompiler::lowerFormulaSource(
        u"of:=AND(['AOO #117989'.D2:.D26])", aHost, *oContext);
    if (!aQuotedSheetHash || aQuotedSheetHash.maFormula.maTokens.empty()
        || aQuotedSheetHash.maFormula.maTokens.front().meKind != setoken::Kind::DoubleRef)
    {
        std::string aDetail = "workbook compiler lowering quoted-sheet-hash mismatch";
        if (!aQuotedSheetHash)
        {
            aDetail += " (reason=";
            aDetail += std::to_string(static_cast<int>(aQuotedSheetHash.meReason));
            if (!aQuotedSheetHash.maDetail.empty())
            {
                aDetail += ", detail=";
                aDetail += toUtf8(aQuotedSheetHash.maDetail);
            }
            aDetail += ")";
        }
        else if (!aQuotedSheetHash.maFormula.maTokens.empty())
        {
            aDetail += " (kind=";
            aDetail += std::to_string(
                static_cast<int>(aQuotedSheetHash.maFormula.maTokens.front().meKind));
            aDetail += ")";
        }
        return fail("spreadsheetengine_token_compiler_host_tests", aDetail.c_str());
    }

    const auto aWholeRowRange = secompiler::lowerFormulaSource(
        u"of:=MATCH([.$A$150];[.$150:.$150];0)", aHost, *oContext);
    if (!aWholeRowRange || aWholeRowRange.maFormula.maTokens.size() < 2
        || aWholeRowRange.maFormula.maTokens[1].meKind != setoken::Kind::DoubleRef)
    {
        std::string aDetail = "workbook compiler lowering whole-row range mismatch";
        if (!aWholeRowRange)
        {
            aDetail += " (reason=";
            aDetail += std::to_string(static_cast<int>(aWholeRowRange.meReason));
            if (!aWholeRowRange.maDetail.empty())
            {
                aDetail += ", detail=";
                aDetail += toUtf8(aWholeRowRange.maDetail);
            }
            aDetail += ")";
        }
        else
        {
            aDetail += " (size=";
            aDetail += std::to_string(aWholeRowRange.maFormula.maTokens.size());
            aDetail += ")";
        }
        return fail("spreadsheetengine_token_compiler_host_tests", aDetail.c_str());
    }

    const auto aErrColon = secompiler::lowerFormulaSource(u"of:=ERROR.TYPE(err:7)", aHost, *oContext);
    if (aErrColon || aErrColon.meReason != secompiler::FormulaLoweringReason::ParseFailure)
    {
        return fail("spreadsheetengine_token_compiler_host_tests",
            "workbook compiler lowering err-colon mismatch");
    }

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

    if (int nResult = testWorkbookCompileHost(); nResult != EXIT_SUCCESS)
        return nResult;

    if (int nResult = testWorkbookCompilerPreflight(); nResult != EXIT_SUCCESS)
        return nResult;

    if (int nResult = testWorkbookCompilerLowering(); nResult != EXIT_SUCCESS)
        return nResult;

    if (int nResult = testSharedFormulaTokenServices(); nResult != EXIT_SUCCESS)
        return nResult;

    if (int nResult = testTokenStringifier(); nResult != EXIT_SUCCESS)
        return nResult;

    std::cout << "spreadsheetengine token/compiler host tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
