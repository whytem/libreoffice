/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>

#include <spreadsheetengine/detail/OdfFormulaParser.hxx>
#include <spreadsheetengine/detail/WorkbookCompileHost.hxx>

namespace spreadsheetengine::detail::compiler
{

enum class FormulaPreflightReason : sal_uInt8
{
    Ready = 0,
    ParseFailure,
    MissingNamedReference,
    UnsupportedArrayElement,
    UnsupportedRangeConstructorOperand,
    UnsupportedReferenceListElement
};

[[nodiscard]] constexpr api::StringView preflightReasonName(FormulaPreflightReason eReason)
{
    switch (eReason)
    {
        case FormulaPreflightReason::Ready:
            return u"ready";
        case FormulaPreflightReason::ParseFailure:
            return u"parse_failure";
        case FormulaPreflightReason::MissingNamedReference:
            return u"missing_named_reference";
        case FormulaPreflightReason::UnsupportedArrayElement:
            return u"unsupported_array_element";
        case FormulaPreflightReason::UnsupportedRangeConstructorOperand:
            return u"unsupported_range_constructor_operand";
        case FormulaPreflightReason::UnsupportedReferenceListElement:
            return u"unsupported_reference_list_element";
    }

    return u"unknown";
}

struct FormulaPreflightResult
{
    FormulaPreflightReason meReason = FormulaPreflightReason::Ready;
    api::String maDetail;
    std::size_t mnFailureOffset = 0;
    bool mbUsesNamedReference = false;
    bool mbUsesArrayConstant = false;
    bool mbUsesFunctionCall = false;
    bool mbUsesCellReference = false;
    bool mbUsesRangeReference = false;

    constexpr explicit operator bool() const
    {
        return meReason == FormulaPreflightReason::Ready;
    }
};

namespace detail
{

[[nodiscard]] constexpr api::StringView nodeKindName(core::formula::NodeKind eKind)
{
    using core::formula::NodeKind;

    switch (eKind)
    {
        case NodeKind::NumberLiteral:
            return u"NumberLiteral";
        case NodeKind::StringLiteral:
            return u"StringLiteral";
        case NodeKind::BooleanLiteral:
            return u"BooleanLiteral";
        case NodeKind::ErrorLiteral:
            return u"ErrorLiteral";
        case NodeKind::EmptyArgument:
            return u"EmptyArgument";
        case NodeKind::CellReference:
            return u"CellReference";
        case NodeKind::RangeReference:
            return u"RangeReference";
        case NodeKind::NamedReference:
            return u"NamedReference";
        case NodeKind::RangeConstructor:
            return u"RangeConstructor";
        case NodeKind::ReferenceList:
            return u"ReferenceList";
        case NodeKind::ArrayConstant:
            return u"ArrayConstant";
        case NodeKind::UnaryOperation:
            return u"UnaryOperation";
        case NodeKind::BinaryOperation:
            return u"BinaryOperation";
        case NodeKind::FunctionCall:
            return u"FunctionCall";
    }

    return u"Unknown";
}

inline void setFailure(
    FormulaPreflightResult& rResult, FormulaPreflightReason eReason, api::String aDetail = {},
    std::size_t nFailureOffset = 0)
{
    rResult.meReason = eReason;
    rResult.maDetail = std::move(aDetail);
    rResult.mnFailureOffset = nFailureOffset;
}

[[nodiscard]] inline bool preflightNode(
    const core::formula::Node& rNode, const WorkbookCompileHost& rHost,
    const CompileContext& rContext, FormulaPreflightResult& rResult);

[[nodiscard]] inline api::StringView normalizeReferenceFunctionName(api::StringView rName)
{
    if (rName == u"COM.MICROSOFT.XLOOKUP")
        return u"XLOOKUP";
    return rName;
}

[[nodiscard]] inline bool isSupportedArrayElement(
    const core::formula::Node& rNode, const WorkbookCompileHost& rHost,
    const CompileContext& rContext, FormulaPreflightResult& rResult)
{
    using core::formula::NodeKind;
    (void)rHost;
    (void)rContext;
    (void)rResult;

    switch (rNode.meKind)
    {
        case NodeKind::NumberLiteral:
        case NodeKind::StringLiteral:
        case NodeKind::BooleanLiteral:
        case NodeKind::ErrorLiteral:
        case NodeKind::EmptyArgument:
            return true;
        case NodeKind::UnaryOperation:
            if (rNode.maChildren.size() != 1 || !rNode.maChildren.front())
                return false;
            return isSupportedArrayElement(*rNode.maChildren.front(), rHost, rContext, rResult);
        default:
            return false;
    }
}

[[nodiscard]] inline bool isReferenceLikeNode(
    const core::formula::Node& rNode, const WorkbookCompileHost& rHost,
    const CompileContext& rContext, FormulaPreflightResult& rResult);

[[nodiscard]] inline bool isReferenceReturningFunction(
    const core::formula::Node& rNode, const WorkbookCompileHost& rHost,
    const CompileContext& rContext, FormulaPreflightResult& rResult)
{
    using core::formula::NodeKind;

    if (rNode.meKind != NodeKind::FunctionCall)
        return false;

    rResult.mbUsesFunctionCall = true;
    const api::StringView aFunctionName = normalizeReferenceFunctionName(rNode.maPrimaryText);

    if (aFunctionName == u"CHOOSE")
    {
        if (rNode.maChildren.size() < 2 || !rNode.maChildren[0]
            || !preflightNode(*rNode.maChildren[0], rHost, rContext, rResult))
        {
            return false;
        }

        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            if (!rNode.maChildren[nIndex]
                || !isReferenceLikeNode(*rNode.maChildren[nIndex], rHost, rContext, rResult))
            {
                return false;
            }
        }
        return true;
    }

    if (aFunctionName == u"INDEX")
    {
        if (rNode.maChildren.empty() || !rNode.maChildren[0]
            || !isReferenceLikeNode(*rNode.maChildren[0], rHost, rContext, rResult))
        {
            return false;
        }

        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            if (rNode.maChildren[nIndex]
                && !preflightNode(*rNode.maChildren[nIndex], rHost, rContext, rResult))
            {
                return false;
            }
        }
        return true;
    }

    if (aFunctionName == u"XLOOKUP")
    {
        if (rNode.maChildren.size() < 3 || !rNode.maChildren[0] || !rNode.maChildren[1]
            || !rNode.maChildren[2]
            || !preflightNode(*rNode.maChildren[0], rHost, rContext, rResult)
            || !isReferenceLikeNode(*rNode.maChildren[1], rHost, rContext, rResult)
            || !isReferenceLikeNode(*rNode.maChildren[2], rHost, rContext, rResult))
        {
            return false;
        }

        for (std::size_t nIndex = 3; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            if (rNode.maChildren[nIndex]
                && !preflightNode(*rNode.maChildren[nIndex], rHost, rContext, rResult))
            {
                return false;
            }
        }
        return true;
    }

    return false;
}

[[nodiscard]] inline bool isReferenceLikeNode(
    const core::formula::Node& rNode, const WorkbookCompileHost& rHost,
    const CompileContext& rContext, FormulaPreflightResult& rResult)
{
    using core::formula::NodeKind;

    switch (rNode.meKind)
    {
        case NodeKind::CellReference:
        case NodeKind::RangeReference:
            return true;

        case NodeKind::NamedReference:
        {
            // Standalone lowering preserves unresolved names too, so keep them eligible
            // inside reference-returning constructs such as LET-local unions.
            return true;
        }

        case NodeKind::RangeConstructor:
            if (rNode.maChildren.size() != 2 || !rNode.maChildren[0] || !rNode.maChildren[1])
                return false;
            return isReferenceLikeNode(*rNode.maChildren[0], rHost, rContext, rResult)
                   && isReferenceLikeNode(*rNode.maChildren[1], rHost, rContext, rResult);

        case NodeKind::ReferenceList:
            for (const auto& pChild : rNode.maChildren)
            {
                if (!pChild || !isReferenceLikeNode(*pChild, rHost, rContext, rResult))
                    return false;
            }
            return true;

        case NodeKind::FunctionCall:
            return isReferenceReturningFunction(rNode, rHost, rContext, rResult);

        default:
            return false;
    }
}

[[nodiscard]] inline bool preflightNode(
    const core::formula::Node& rNode, const WorkbookCompileHost& rHost,
    const CompileContext& rContext, FormulaPreflightResult& rResult)
{
    using core::formula::NodeKind;

    switch (rNode.meKind)
    {
        case NodeKind::NumberLiteral:
        case NodeKind::StringLiteral:
        case NodeKind::BooleanLiteral:
        case NodeKind::ErrorLiteral:
        case NodeKind::EmptyArgument:
            return true;

        case NodeKind::CellReference:
            rResult.mbUsesCellReference = true;
            return true;

        case NodeKind::RangeReference:
            rResult.mbUsesRangeReference = true;
            return true;

        case NodeKind::NamedReference:
        {
            rResult.mbUsesNamedReference = true;
            std::optional<api::SheetId> oScopeSheet;
            if (rContext.maBaseAddress.mnSheet >= 0)
                oScopeSheet = rContext.maBaseAddress.mnSheet;

            if (rHost.lookupRangeName(rNode.maPrimaryText, oScopeSheet, rContext))
                return true;

            if (rContext.mbAllowExternalReferences
                && rHost.lookupExternalName(rNode.maPrimaryText, rContext))
            {
                return true;
            }

            // Calc still compiles unresolved bare names and lets evaluation produce the
            // eventual #NAME? / cached-error behavior. Keep those formulas on the shared
            // compiler path instead of classifying them as host-lookup blockers.
            return true;
        }

        case NodeKind::RangeConstructor:
            if (rNode.maChildren.size() != 2 || !rNode.maChildren[0] || !rNode.maChildren[1])
            {
                setFailure(rResult, FormulaPreflightReason::UnsupportedRangeConstructorOperand);
                return false;
            }

            for (const auto& pChild : rNode.maChildren)
            {
                if (pChild->meKind == NodeKind::CellReference)
                    rResult.mbUsesCellReference = true;
                else if (pChild->meKind == NodeKind::RangeReference)
                    rResult.mbUsesRangeReference = true;
                else if (pChild->meKind == NodeKind::NamedReference)
                    rResult.mbUsesNamedReference = true;
                else if (pChild->meKind == NodeKind::FunctionCall)
                    rResult.mbUsesFunctionCall = true;

                if (!isReferenceLikeNode(*pChild, rHost, rContext, rResult))
                {
                    setFailure(rResult, FormulaPreflightReason::UnsupportedRangeConstructorOperand,
                        api::String(nodeKindName(pChild->meKind)));
                    return false;
                }
            }
            return true;

        case NodeKind::ReferenceList:
            for (const auto& pChild : rNode.maChildren)
            {
                if (!pChild)
                    continue;

                if (pChild->meKind == NodeKind::CellReference)
                    rResult.mbUsesCellReference = true;
                else if (pChild->meKind == NodeKind::RangeReference)
                    rResult.mbUsesRangeReference = true;
                else if (pChild->meKind == NodeKind::NamedReference)
                    rResult.mbUsesNamedReference = true;

                if (isReferenceLikeNode(*pChild, rHost, rContext, rResult))
                    continue;

                setFailure(rResult, FormulaPreflightReason::UnsupportedReferenceListElement,
                    api::String(nodeKindName(pChild->meKind)));
                return false;
            }
            return true;

        case NodeKind::ArrayConstant:
            rResult.mbUsesArrayConstant = true;
            for (const auto& pChild : rNode.maChildren)
            {
                if (!pChild)
                    continue;

                if (isSupportedArrayElement(*pChild, rHost, rContext, rResult))
                    continue;

                setFailure(rResult, FormulaPreflightReason::UnsupportedArrayElement,
                    api::String(nodeKindName(pChild->meKind)));
                return false;
            }
            return true;

        case NodeKind::UnaryOperation:
        case NodeKind::BinaryOperation:
        case NodeKind::FunctionCall:
            if (rNode.meKind == NodeKind::FunctionCall)
                rResult.mbUsesFunctionCall = true;
            for (const auto& pChild : rNode.maChildren)
            {
                if (pChild && !preflightNode(*pChild, rHost, rContext, rResult))
                    return false;
            }
            return true;
    }

    return true;
}

} // namespace detail

[[nodiscard]] inline FormulaPreflightResult preflightFormulaSource(
    api::StringView rFormula, const WorkbookCompileHost& rHost, const CompileContext& rContext)
{
    FormulaPreflightResult aResult;
    const auto aParsed = core::formula::parseFormula(rFormula);
    if (!aParsed)
    {
        detail::setFailure(aResult, FormulaPreflightReason::ParseFailure,
            aParsed.maError.maMessage, aParsed.maError.mnOffset);
        return aResult;
    }

    if (aParsed.mpRoot)
    {
        const bool bReady = detail::preflightNode(*aParsed.mpRoot, rHost, rContext, aResult);
        (void)bReady;
    }

    return aResult;
}

} // namespace spreadsheetengine::detail::compiler

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
