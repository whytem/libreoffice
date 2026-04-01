/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/detail/compiler/CompiledFormulaInflation.hxx>
#include <cstdint>

#include <spreadsheetengine/detail/BuiltinExternalNames.hxx>
#include <spreadsheetengine/detail/WorkbookCompilerLowering.hxx>
#include <spreadsheetengine/runtime/ReferenceText.hxx>

#include "FormulaEvaluatorUtils.hxx"

#include <memory>
#include <utility>
#include <vector>

namespace spreadsheetengine::detail::compiler
{
namespace
{

namespace seformula = spreadsheetengine::core::formula;
namespace setoken = spreadsheetengine::detail::token;
using spreadsheetengine::core::eval::detail::formatNumber;

[[nodiscard]] constexpr spreadsheetengine::api::refdata::SheetLimits runtimeSheetLimits(
    const spreadsheetengine::core::workbook::Workbook& rWorkbook)
{
    return { spreadsheetengine::detail::compiler::detail::kSmokeMaxColumn,
        spreadsheetengine::detail::compiler::detail::kSmokeMaxRow,
        static_cast<spreadsheetengine::api::SheetId>(
            rWorkbook.maSheets.empty() ? 0 : rWorkbook.maSheets.size() - 1) };
}

[[nodiscard]] spreadsheetengine::api::String formatAbsoluteCellReferenceToken(
    const spreadsheetengine::api::CellAddress& rAddress,
    const spreadsheetengine::core::workbook::Workbook& rWorkbook,
    spreadsheetengine::api::SheetId nCurrentSheet)
{
    spreadsheetengine::api::String aToken;
    if (rAddress.mnSheet == nCurrentSheet)
    {
        aToken = u".";
    }
    else
    {
        if (rAddress.mnSheet < 0
            || static_cast<std::size_t>(rAddress.mnSheet) >= rWorkbook.maSheets.size())
        {
            return {};
        }
        aToken = spreadsheetengine::runtime::referencetext::quoteSheetNameForFormula(
            rWorkbook.maSheets[static_cast<std::size_t>(rAddress.mnSheet)].maName);
        aToken.push_back(u'.');
    }

    aToken += spreadsheetengine::runtime::referencetext::columnNameFromIndex(rAddress.mnColumn);
    aToken += formatNumber(static_cast<double>(rAddress.mnRow + 1));
    return aToken;
}

[[nodiscard]] spreadsheetengine::api::String formatSingleReferenceToken(
    const spreadsheetengine::api::refdata::SingleRefData& rReference,
    const spreadsheetengine::core::workbook::Workbook& rWorkbook,
    const spreadsheetengine::api::CellAddress& rCurrentAddress)
{
    const auto aAbsolute = spreadsheetengine::api::refdata::toAbsoluteAddress(
        rReference, runtimeSheetLimits(rWorkbook), rCurrentAddress);
    return formatAbsoluteCellReferenceToken(aAbsolute, rWorkbook, rCurrentAddress.mnSheet);
}

[[nodiscard]] spreadsheetengine::api::String errorCodeToLiteral(setoken::ErrorCode nErrorCode)
{
    switch (nErrorCode)
    {
        case 2042:
            return u"#N/A";
        case 2007:
            return u"#DIV/0!";
        case 2015:
            return u"#VALUE!";
        case 2023:
            return u"#REF!";
        case 2029:
            return u"#NAME?";
        case 2036:
            return u"#NUM!";
        case 2000:
            return u"#NULL!";
        default:
        {
            spreadsheetengine::api::String aLiteral = u"#ERR";
            aLiteral += formatNumber(static_cast<double>(nErrorCode));
            aLiteral.push_back(u'!');
            return aLiteral;
        }
    }
}

struct InflatedStackItem
{
    enum class Kind : std::uint8_t
    {
        Node = 0,
        FunctionName,
        Byte
    };

    Kind meKind = Kind::Node;
    std::unique_ptr<seformula::Node> mpNode;
    spreadsheetengine::api::String maText;
    setoken::ByteData maByte;
};

[[nodiscard]] std::unique_ptr<seformula::Node> makeSimpleNode(seformula::NodeKind eKind)
{
    auto pNode = std::make_unique<seformula::Node>();
    pNode->meKind = eKind;
    return pNode;
}

[[nodiscard]] std::unique_ptr<seformula::Node> inflateMatrixScalarNode(
    const setoken::MatrixScalar& rScalar)
{
    auto pNode = std::make_unique<seformula::Node>();
    if (const auto* pNumber = std::get_if<double>(&rScalar))
    {
        pNode->meKind = seformula::NodeKind::NumberLiteral;
        pNode->mfNumber = *pNumber;
        return pNode;
    }
    if (const auto* pString = std::get_if<spreadsheetengine::api::String>(&rScalar))
    {
        pNode->meKind = seformula::NodeKind::StringLiteral;
        pNode->maPrimaryText = *pString;
        return pNode;
    }

    pNode->meKind = seformula::NodeKind::ErrorLiteral;
    pNode->maPrimaryText = errorCodeToLiteral(std::get<setoken::ErrorCode>(rScalar));
    return pNode;
}

[[nodiscard]] bool popNode(
    std::vector<InflatedStackItem>& rStack, std::unique_ptr<seformula::Node>& rpNode)
{
    if (rStack.empty() || rStack.back().meKind != InflatedStackItem::Kind::Node)
        return false;
    rpNode = std::move(rStack.back().mpNode);
    rStack.pop_back();
    return true;
}

[[nodiscard]] bool popByte(std::vector<InflatedStackItem>& rStack, setoken::ByteData& rByte)
{
    if (rStack.empty() || rStack.back().meKind != InflatedStackItem::Kind::Byte)
        return false;
    rByte = rStack.back().maByte;
    rStack.pop_back();
    return true;
}

[[nodiscard]] bool popFunctionName(
    std::vector<InflatedStackItem>& rStack, spreadsheetengine::api::String& rName)
{
    if (rStack.empty() || rStack.back().meKind != InflatedStackItem::Kind::FunctionName)
        return false;
    rName = std::move(rStack.back().maText);
    rStack.pop_back();
    return true;
}

} // namespace

std::optional<std::unique_ptr<spreadsheetengine::core::formula::Node>> inflateCompiledFormulaNode(
    const setoken::CompiledFormula& rFormula,
    const spreadsheetengine::core::workbook::Workbook& rWorkbook,
    const spreadsheetengine::api::CellAddress& rCurrentAddress)
{
    std::vector<InflatedStackItem> aStack;
    aStack.reserve(rFormula.maTokens.size());

    for (const auto& rToken : rFormula.maTokens)
    {
        switch (rToken.meKind)
        {
            case setoken::Kind::Missing:
                aStack.push_back({ InflatedStackItem::Kind::Node,
                    makeSimpleNode(seformula::NodeKind::EmptyArgument), {}, {} });
                break;
            case setoken::Kind::Value:
            {
                auto pNode = makeSimpleNode(seformula::NodeKind::NumberLiteral);
                pNode->mfNumber = std::get<double>(rToken.maPayload);
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::String:
            {
                auto pNode = makeSimpleNode(seformula::NodeKind::StringLiteral);
                pNode->maPrimaryText = std::get<setoken::StringData>(rToken.maPayload).maText;
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::StringName:
            {
                if (rToken.mnOpCode == setoken::kOpCodeName)
                {
                    auto pNode = makeSimpleNode(seformula::NodeKind::NamedReference);
                    pNode->maPrimaryText = std::get<setoken::StringData>(rToken.maPayload).maText;
                    aStack.push_back(
                        { InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                }
                else
                {
                    aStack.push_back({ InflatedStackItem::Kind::FunctionName, nullptr,
                        std::get<setoken::StringData>(rToken.maPayload).maText, {} });
                }
                break;
            }
            case setoken::Kind::ExternalName:
            {
                const auto& rExternalName = std::get<setoken::ExternalNameData>(rToken.maPayload);
                const auto oBuiltinSymbol = spreadsheetengine::detail::compiler::
                    lookupBuiltinExternalSymbol(rExternalName.maName);
                aStack.push_back({ InflatedStackItem::Kind::FunctionName, nullptr,
                    oBuiltinSymbol ? *oBuiltinSymbol : rExternalName.maName, {} });
                break;
            }
            case setoken::Kind::Byte:
                aStack.push_back({ InflatedStackItem::Kind::Byte, nullptr, {},
                    std::get<setoken::ByteData>(rToken.maPayload) });
                break;
            case setoken::Kind::Error:
            {
                auto pNode = makeSimpleNode(seformula::NodeKind::ErrorLiteral);
                pNode->maPrimaryText
                    = errorCodeToLiteral(std::get<setoken::ErrorCode>(rToken.maPayload));
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::SingleRef:
            {
                auto pNode = makeSimpleNode(seformula::NodeKind::CellReference);
                pNode->maPrimaryText = formatSingleReferenceToken(
                    std::get<spreadsheetengine::api::refdata::SingleRefData>(rToken.maPayload),
                    rWorkbook, rCurrentAddress);
                if (pNode->maPrimaryText.empty())
                    return std::nullopt;
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::DoubleRef:
            {
                const auto& rReference
                    = std::get<spreadsheetengine::api::refdata::ComplexRefData>(rToken.maPayload);
                auto pNode = makeSimpleNode(seformula::NodeKind::RangeReference);
                pNode->maPrimaryText
                    = formatSingleReferenceToken(rReference.maRef1, rWorkbook, rCurrentAddress);
                pNode->maSecondaryText
                    = formatSingleReferenceToken(rReference.maRef2, rWorkbook, rCurrentAddress);
                if (pNode->maPrimaryText.empty() || pNode->maSecondaryText.empty())
                    return std::nullopt;
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::RangeName:
            {
                const auto& rName = std::get<setoken::NameData>(rToken.maPayload);
                if (rName.mnIndex == 0
                    || static_cast<std::size_t>(rName.mnIndex - 1) >= rWorkbook.maNamedRanges.size())
                {
                    return std::nullopt;
                }
                auto pNode = makeSimpleNode(seformula::NodeKind::NamedReference);
                pNode->maPrimaryText
                    = rWorkbook.maNamedRanges[static_cast<std::size_t>(rName.mnIndex - 1)].maName;
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::Matrix:
            {
                const auto& rMatrix = std::get<setoken::MatrixData>(rToken.maPayload);
                auto pNode = makeSimpleNode(seformula::NodeKind::ArrayConstant);
                pNode->mnArrayRows = rMatrix.mnRows;
                pNode->mnArrayColumns = rMatrix.mnColumns;
                for (const auto& rScalar : rMatrix.maValues)
                    pNode->maChildren.push_back(inflateMatrixScalarNode(rScalar));
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::PlainOpcode:
            {
                auto makeUnary = [&](seformula::UnaryOperator eOperator) -> bool {
                    std::unique_ptr<seformula::Node> pChild;
                    if (!popNode(aStack, pChild))
                        return false;
                    auto pNode = makeSimpleNode(seformula::NodeKind::UnaryOperation);
                    pNode->meUnaryOperator = eOperator;
                    pNode->maChildren.push_back(std::move(pChild));
                    aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                    return true;
                };

                auto makeBinary = [&](seformula::BinaryOperator eOperator) -> bool {
                    std::unique_ptr<seformula::Node> pRight;
                    std::unique_ptr<seformula::Node> pLeft;
                    if (!popNode(aStack, pRight) || !popNode(aStack, pLeft))
                        return false;
                    auto pNode = makeSimpleNode(seformula::NodeKind::BinaryOperation);
                    pNode->meBinaryOperator = eOperator;
                    pNode->maChildren.push_back(std::move(pLeft));
                    pNode->maChildren.push_back(std::move(pRight));
                    aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                    return true;
                };

                switch (rToken.mnOpCode)
                {
                    case spreadsheetengine::detail::compiler::detail::kLoweredOpUnaryPlus:
                        if (!makeUnary(seformula::UnaryOperator::Plus))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeNegSub:
                        if (!makeUnary(seformula::UnaryOperator::Minus))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeAdd:
                        if (!makeBinary(seformula::BinaryOperator::Add))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeSub:
                        if (!makeBinary(seformula::BinaryOperator::Subtract))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeMul:
                        if (!makeBinary(seformula::BinaryOperator::Multiply))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeDiv:
                        if (!makeBinary(seformula::BinaryOperator::Divide))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodePow:
                        if (!makeBinary(seformula::BinaryOperator::Power))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeAmpersand:
                        if (!makeBinary(seformula::BinaryOperator::Concat))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeEqual:
                        if (!makeBinary(seformula::BinaryOperator::Equal))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeNotEqual:
                        if (!makeBinary(seformula::BinaryOperator::NotEqual))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeLess:
                        if (!makeBinary(seformula::BinaryOperator::Less))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeLessEqual:
                        if (!makeBinary(seformula::BinaryOperator::LessEqual))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeGreater:
                        if (!makeBinary(seformula::BinaryOperator::Greater))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeGreaterEqual:
                        if (!makeBinary(seformula::BinaryOperator::GreaterEqual))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeRange:
                    {
                        std::unique_ptr<seformula::Node> pRight;
                        std::unique_ptr<seformula::Node> pLeft;
                        if (!popNode(aStack, pRight) || !popNode(aStack, pLeft))
                            return std::nullopt;
                        auto pNode = makeSimpleNode(seformula::NodeKind::RangeConstructor);
                        pNode->maChildren.push_back(std::move(pLeft));
                        pNode->maChildren.push_back(std::move(pRight));
                        aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                        break;
                    }
                    case setoken::kOpCodeUnion:
                    {
                        std::unique_ptr<seformula::Node> pRight;
                        std::unique_ptr<seformula::Node> pLeft;
                        if (!popNode(aStack, pRight) || !popNode(aStack, pLeft))
                            return std::nullopt;
                        auto pNode = makeSimpleNode(seformula::NodeKind::ReferenceList);
                        auto appendChild = [&](std::unique_ptr<seformula::Node> pChild) {
                            if (pChild->meKind == seformula::NodeKind::ReferenceList)
                            {
                                for (auto& pGrandChild : pChild->maChildren)
                                    pNode->maChildren.push_back(std::move(pGrandChild));
                                return;
                            }
                            pNode->maChildren.push_back(std::move(pChild));
                        };
                        appendChild(std::move(pLeft));
                        appendChild(std::move(pRight));
                        aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                        break;
                    }
                    case spreadsheetengine::detail::compiler::detail::kLoweredOpReferenceList:
                    {
                        setoken::ByteData aCount;
                        if (!popByte(aStack, aCount))
                            return std::nullopt;
                        auto pNode = makeSimpleNode(seformula::NodeKind::ReferenceList);
                        std::vector<std::unique_ptr<seformula::Node>> aChildren(
                            static_cast<std::size_t>(aCount.mnByte));
                        for (std::size_t nIndex = aChildren.size(); nIndex-- > 0;)
                        {
                            if (!popNode(aStack, aChildren[nIndex]))
                                return std::nullopt;
                        }
                        pNode->maChildren = std::move(aChildren);
                        aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                        break;
                    }
                    case spreadsheetengine::detail::compiler::detail::kLoweredOpFunctionCall:
                    {
                        setoken::ByteData aCount;
                        spreadsheetengine::api::String aName;
                        if (!popByte(aStack, aCount) || !popFunctionName(aStack, aName))
                            return std::nullopt;
                        auto pNode = makeSimpleNode(seformula::NodeKind::FunctionCall);
                        pNode->maPrimaryText = aName;
                        std::vector<std::unique_ptr<seformula::Node>> aChildren(
                            static_cast<std::size_t>(aCount.mnByte));
                        for (std::size_t nIndex = aChildren.size(); nIndex-- > 0;)
                        {
                            if (!popNode(aStack, aChildren[nIndex]))
                                return std::nullopt;
                        }
                        pNode->maChildren = std::move(aChildren);
                        aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                        break;
                    }
                    default:
                        return std::nullopt;
                }
                break;
            }
            default:
                return std::nullopt;
        }
    }

    if (aStack.size() != 1 || aStack.back().meKind != InflatedStackItem::Kind::Node)
        return std::nullopt;

    return std::move(aStack.back().mpNode);
}

} // namespace spreadsheetengine::detail::compiler

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
