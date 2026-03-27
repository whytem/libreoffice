/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/detail/OdfFormulaParser.hxx>

#include <cctype>
#include <cstdlib>
#include <string>
#include <utility>

namespace spreadsheetengine::core::formula
{
namespace
{

[[nodiscard]] constexpr bool isAsciiSpace(char16_t cChar)
{
    return cChar == u' ' || cChar == u'\t' || cChar == u'\n' || cChar == u'\r';
}

[[nodiscard]] constexpr bool isIdentifierStart(char16_t cChar)
{
    return (cChar >= u'A' && cChar <= u'Z') || (cChar >= u'a' && cChar <= u'z') || cChar == u'_';
}

[[nodiscard]] constexpr bool isIdentifierContinue(char16_t cChar)
{
    return isIdentifierStart(cChar) || (cChar >= u'0' && cChar <= u'9') || cChar == u'.';
}

[[nodiscard]] constexpr bool isDigit(char16_t cChar)
{
    return cChar >= u'0' && cChar <= u'9';
}

[[nodiscard]] constexpr bool isErrorLiteralContinue(char16_t cChar)
{
    return (cChar >= u'A' && cChar <= u'Z') || (cChar >= u'a' && cChar <= u'z')
           || (cChar >= u'0' && cChar <= u'9') || cChar == u'/' || cChar == u'!'
           || cChar == u'?' || cChar == u'_' || cChar == u'.';
}

[[nodiscard]] constexpr bool isReferenceTerminator(char16_t cChar)
{
    return cChar == u'\0' || isAsciiSpace(cChar) || cChar == u')' || cChar == u'}'
           || cChar == u';' || cChar == u',' || cChar == u':' || cChar == u'+'
           || cChar == u'-' || cChar == u'*' || cChar == u'/' || cChar == u'^'
           || cChar == u'&' || cChar == u'=' || cChar == u'<' || cChar == u'>';
}

[[nodiscard]] std::unique_ptr<Node> makeNode(NodeKind eKind)
{
    auto pNode = std::make_unique<Node>();
    pNode->meKind = eKind;
    return pNode;
}

class Parser
{
    api::StringView mrInput;
    std::size_t mnPos = 0;
    ParseError maError;
    bool mbError = false;

    void fail(api::StringView rMessage)
    {
        if (mbError)
            return;
        maError = { api::String(rMessage), mnPos };
        mbError = true;
    }

    void skipSpaces()
    {
        while (mnPos < mrInput.size() && isAsciiSpace(mrInput[mnPos]))
            ++mnPos;
    }

    [[nodiscard]] bool atEnd() const { return mnPos >= mrInput.size(); }

    [[nodiscard]] char16_t peek() const
    {
        return atEnd() ? u'\0' : mrInput[mnPos];
    }

    [[nodiscard]] char16_t peekAhead(std::size_t nOffset) const
    {
        const std::size_t nIndex = mnPos + nOffset;
        return nIndex < mrInput.size() ? mrInput[nIndex] : u'\0';
    }

    [[nodiscard]] bool consume(char16_t cExpected)
    {
        if (peek() != cExpected)
            return false;
        ++mnPos;
        return true;
    }

    [[nodiscard]] bool consumeIfPresent(api::StringView rPrefix)
    {
        if (mrInput.substr(mnPos, rPrefix.size()) != rPrefix)
            return false;
        mnPos += rPrefix.size();
        return true;
    }

    [[nodiscard]] api::StringView parseIdentifierToken()
    {
        const std::size_t nStart = mnPos;
        if (!isIdentifierStart(peek()))
            return {};

        ++mnPos;
        while (isIdentifierContinue(peek()))
            ++mnPos;

        return mrInput.substr(nStart, mnPos - nStart);
    }

    [[nodiscard]] bool consumeBareCellAddressRemainder()
    {
        if (peek() == u'$')
            ++mnPos;

        std::size_t nColumnCount = 0;
        while (true)
        {
            const char16_t cChar = peek();
            const bool bAlpha
                = (cChar >= u'A' && cChar <= u'Z') || (cChar >= u'a' && cChar <= u'z');
            if (!bAlpha)
                break;
            ++mnPos;
            ++nColumnCount;
        }

        if (nColumnCount == 0 || nColumnCount > 3)
            return false;

        if (peek() == u'$')
            ++mnPos;

        std::size_t nRowCount = 0;
        while (isDigit(peek()))
        {
            ++mnPos;
            ++nRowCount;
        }

        return nRowCount > 0;
    }

    [[nodiscard]] std::unique_ptr<Node> parseBareReference()
    {
        const std::size_t nStart = mnPos;

        bool bParsedAddress = consumeBareCellAddressRemainder();
        if (!bParsedAddress)
        {
            mnPos = nStart;

            const auto aSheetToken = parseIdentifierToken();
            if (aSheetToken.empty() || !consume(u'.') || !consumeBareCellAddressRemainder())
            {
                mnPos = nStart;
                return nullptr;
            }
            bParsedAddress = true;
        }

        if (!bParsedAddress)
            return nullptr;

        const auto aPrimaryToken = mrInput.substr(nStart, mnPos - nStart);
        if (peek() == u'(')
        {
            mnPos = nStart;
            return nullptr;
        }
        if (peek() != u':' && !isReferenceTerminator(peek()))
        {
            mnPos = nStart;
            return nullptr;
        }
        if (peek() != u':')
        {
            auto pNode = makeNode(NodeKind::CellReference);
            pNode->maPrimaryText = api::String(aPrimaryToken);
            return pNode;
        }

        ++mnPos;
        const std::size_t nSecondStart = mnPos;
        if (!consumeBareCellAddressRemainder())
        {
            mnPos = nStart;
            return nullptr;
        }
        if (!isReferenceTerminator(peek()))
        {
            mnPos = nStart;
            return nullptr;
        }

        auto pNode = makeNode(NodeKind::RangeReference);
        pNode->maPrimaryText = api::String(aPrimaryToken);
        pNode->maSecondaryText = api::String(mrInput.substr(nSecondStart, mnPos - nSecondStart));
        return pNode;
    }

    [[nodiscard]] std::unique_ptr<Node> parseNumberLiteral()
    {
        const std::size_t nStart = mnPos;

        if (peek() == u'.')
            ++mnPos;
        while (isDigit(peek()))
            ++mnPos;

        if (peek() == u'.')
        {
            ++mnPos;
            while (isDigit(peek()))
                ++mnPos;
        }

        if (peek() == u'e' || peek() == u'E')
        {
            ++mnPos;
            if (peek() == u'+' || peek() == u'-')
                ++mnPos;
            while (isDigit(peek()))
                ++mnPos;
        }

        const auto aToken = mrInput.substr(nStart, mnPos - nStart);
        std::string aAscii;
        aAscii.reserve(aToken.size());
        for (const char16_t cChar : aToken)
            aAscii.push_back(static_cast<char>(cChar));

        auto pNode = makeNode(NodeKind::NumberLiteral);
        pNode->mfNumber = std::strtod(aAscii.c_str(), nullptr);
        pNode->maPrimaryText = api::String(aToken);
        return pNode;
    }

    [[nodiscard]] std::unique_ptr<Node> parseStringLiteral()
    {
        if (!consume(u'"'))
            return nullptr;

        api::String aText;
        while (!atEnd())
        {
            const char16_t cChar = peek();
            ++mnPos;

            if (cChar == u'"')
            {
                if (peek() == u'"')
                {
                    aText.push_back(u'"');
                    ++mnPos;
                    continue;
                }
                auto pNode = makeNode(NodeKind::StringLiteral);
                pNode->maPrimaryText = std::move(aText);
                return pNode;
            }

            aText.push_back(cChar);
        }

        fail(u"unterminated string literal");
        return nullptr;
    }

    [[nodiscard]] std::unique_ptr<Node> parseErrorLiteral()
    {
        const std::size_t nStart = mnPos;
        if (!consume(u'#'))
            return nullptr;

        while (!atEnd() && isErrorLiteralContinue(peek()))
            ++mnPos;

        auto pNode = makeNode(NodeKind::ErrorLiteral);
        pNode->maPrimaryText = api::String(mrInput.substr(nStart, mnPos - nStart));
        return pNode;
    }

    [[nodiscard]] std::unique_ptr<Node> parseBracketReference()
    {
        if (!consume(u'['))
            return nullptr;

        const std::size_t nStart = mnPos;
        while (!atEnd() && peek() != u']')
            ++mnPos;

        if (!consume(u']'))
        {
            fail(u"unterminated bracket reference");
            return nullptr;
        }

        const auto aToken = mrInput.substr(nStart, (mnPos - 1) - nStart);
        const std::size_t nColonPos = aToken.find(u':');

        if (nColonPos == api::StringView::npos)
        {
            auto pNode = makeNode(NodeKind::CellReference);
            pNode->maPrimaryText = api::String(aToken);
            return pNode;
        }

        auto pNode = makeNode(NodeKind::RangeReference);
        pNode->maPrimaryText = api::String(aToken.substr(0, nColonPos));
        pNode->maSecondaryText = api::String(aToken.substr(nColonPos + 1));
        return pNode;
    }

    [[nodiscard]] std::unique_ptr<Node> parseFunctionCall(api::StringView rName)
    {
        auto pNode = makeNode(NodeKind::FunctionCall);
        pNode->maPrimaryText = api::String(rName);

        if (!consume(u'('))
        {
            fail(u"expected '(' after function name");
            return nullptr;
        }

        skipSpaces();
        bool bExpectArgument = true;
        while (!atEnd())
        {
            if (peek() == u')')
            {
                if (bExpectArgument && !pNode->maChildren.empty())
                    pNode->maChildren.push_back(makeNode(NodeKind::EmptyArgument));
                ++mnPos;
                return pNode;
            }

            if (bExpectArgument)
            {
                if (peek() == u';' || peek() == u',')
                {
                    pNode->maChildren.push_back(makeNode(NodeKind::EmptyArgument));
                    ++mnPos;
                    skipSpaces();
                    continue;
                }

                auto pArgument = parseLogicalChain();
                if (!pArgument)
                    return nullptr;
                pNode->maChildren.push_back(std::move(pArgument));
                bExpectArgument = false;
                skipSpaces();
                continue;
            }

            if (peek() == u';' || peek() == u',')
            {
                ++mnPos;
                skipSpaces();
                bExpectArgument = true;
                continue;
            }

            fail(u"expected function argument separator");
            return nullptr;
        }

        fail(u"unterminated function call");
        return nullptr;
    }

    [[nodiscard]] std::unique_ptr<Node> parseArrayConstant()
    {
        if (!consume(u'{'))
            return nullptr;

        auto pNode = makeNode(NodeKind::ArrayConstant);
        sal_Int32 nRows = 1;
        sal_Int32 nCurrentColumns = 0;
        sal_Int32 nExpectedColumns = -1;

        skipSpaces();
        if (consume(u'}'))
        {
            fail(u"empty array constant");
            return nullptr;
        }

        while (!atEnd())
        {
            auto pElement = parseLogicalChain();
            if (!pElement)
                return nullptr;
            pNode->maChildren.push_back(std::move(pElement));
            ++nCurrentColumns;

            skipSpaces();
            if (consume(u'}'))
            {
                if (nExpectedColumns < 0)
                    nExpectedColumns = nCurrentColumns;
                else if (nCurrentColumns != nExpectedColumns)
                {
                    fail(u"ragged array constant");
                    return nullptr;
                }

                pNode->mnArrayRows = nRows;
                pNode->mnArrayColumns = nExpectedColumns;
                return pNode;
            }

            if (consume(u'|'))
            {
                skipSpaces();
                continue;
            }

            if (consume(u';'))
            {
                if (nExpectedColumns < 0)
                    nExpectedColumns = nCurrentColumns;
                else if (nCurrentColumns != nExpectedColumns)
                {
                    fail(u"ragged array constant");
                    return nullptr;
                }

                ++nRows;
                nCurrentColumns = 0;
                skipSpaces();
                continue;
            }

            fail(u"expected array separator");
            return nullptr;
        }

        fail(u"unterminated array constant");
        return nullptr;
    }

    [[nodiscard]] std::unique_ptr<Node> parseIdentifierLike()
    {
        const auto aToken = parseIdentifierToken();
        if (aToken.empty())
            return nullptr;

        skipSpaces();
        if (peek() == u'(')
            return parseFunctionCall(aToken);

        if (aToken == u"TRUE")
        {
            auto pNode = makeNode(NodeKind::BooleanLiteral);
            pNode->mbBoolean = true;
            pNode->maPrimaryText = u"TRUE";
            return pNode;
        }

        if (aToken == u"FALSE")
        {
            auto pNode = makeNode(NodeKind::BooleanLiteral);
            pNode->mbBoolean = false;
            pNode->maPrimaryText = u"FALSE";
            return pNode;
        }

        auto pNode = makeNode(NodeKind::NamedReference);
        pNode->maPrimaryText = api::String(aToken);
        return pNode;
    }

    [[nodiscard]] std::unique_ptr<Node> parsePrimary()
    {
        skipSpaces();

        if (atEnd())
        {
            fail(u"unexpected end of formula");
            return nullptr;
        }

        if (peek() == u'(')
        {
            ++mnPos;
            auto pNode = parseLogicalChain();
            skipSpaces();
            if (!consume(u')'))
            {
                fail(u"expected ')'");
                return nullptr;
            }
            return pNode;
        }

        if (peek() == u'[')
            return parseBracketReference();

        if (peek() == u'{')
            return parseArrayConstant();

        if (peek() == u'"')
            return parseStringLiteral();

        if (peek() == u'#')
            return parseErrorLiteral();

        if (isDigit(peek()) || (peek() == u'.' && isDigit(peekAhead(1))))
            return parseNumberLiteral();

        if (peek() == u'$' || isIdentifierStart(peek()))
        {
            if (auto pReference = parseBareReference())
                return pReference;
        }

        if (isIdentifierStart(peek()))
            return parseIdentifierLike();

        fail(u"unexpected token");
        return nullptr;
    }

    [[nodiscard]] std::unique_ptr<Node> parseReferenceList()
    {
        auto pLeft = parseRangeConstructor();
        if (!pLeft)
            return nullptr;

        while (true)
        {
            skipSpaces();
            if (!consume(u'~'))
                return pLeft;

            auto pRight = parseRangeConstructor();
            if (!pRight)
                return nullptr;

            if (pLeft->meKind == NodeKind::ReferenceList)
            {
                pLeft->maChildren.push_back(std::move(pRight));
                continue;
            }

            auto pNode = makeNode(NodeKind::ReferenceList);
            pNode->maChildren.push_back(std::move(pLeft));
            pNode->maChildren.push_back(std::move(pRight));
            pLeft = std::move(pNode);
        }
    }

    [[nodiscard]] std::unique_ptr<Node> parseRangeConstructor()
    {
        auto pLeft = parsePrimary();
        if (!pLeft)
            return nullptr;

        skipSpaces();
        if (!consume(u':'))
            return pLeft;

        auto pRight = parsePrimary();
        if (!pRight)
            return nullptr;

        auto pNode = makeNode(NodeKind::RangeConstructor);
        pNode->maChildren.push_back(std::move(pLeft));
        pNode->maChildren.push_back(std::move(pRight));
        return pNode;
    }

    [[nodiscard]] std::unique_ptr<Node> parseUnary()
    {
        skipSpaces();
        if (peek() == u'+')
        {
            ++mnPos;
            auto pChild = parseUnary();
            if (!pChild)
                return nullptr;
            auto pNode = makeNode(NodeKind::UnaryOperation);
            pNode->meUnaryOperator = UnaryOperator::Plus;
            pNode->maChildren.push_back(std::move(pChild));
            return pNode;
        }

        if (peek() == u'-')
        {
            ++mnPos;
            auto pChild = parseUnary();
            if (!pChild)
                return nullptr;
            auto pNode = makeNode(NodeKind::UnaryOperation);
            pNode->meUnaryOperator = UnaryOperator::Minus;
            pNode->maChildren.push_back(std::move(pChild));
            return pNode;
        }

        return parseReferenceList();
    }

    [[nodiscard]] std::unique_ptr<Node> parsePower()
    {
        auto pLeft = parseUnary();
        if (!pLeft)
            return nullptr;

        skipSpaces();
        if (!consume(u'^'))
            return pLeft;

        auto pRight = parsePower();
        if (!pRight)
            return nullptr;

        auto pNode = makeNode(NodeKind::BinaryOperation);
        pNode->meBinaryOperator = BinaryOperator::Power;
        pNode->maChildren.push_back(std::move(pLeft));
        pNode->maChildren.push_back(std::move(pRight));
        return pNode;
    }

    [[nodiscard]] std::unique_ptr<Node> parseMultiplyDivide()
    {
        auto pLeft = parsePower();
        if (!pLeft)
            return nullptr;

        while (true)
        {
            skipSpaces();
            BinaryOperator eOp = BinaryOperator::Multiply;
            if (consume(u'*'))
                eOp = BinaryOperator::Multiply;
            else if (consume(u'/'))
                eOp = BinaryOperator::Divide;
            else
                return pLeft;

            auto pRight = parsePower();
            if (!pRight)
                return nullptr;

            auto pNode = makeNode(NodeKind::BinaryOperation);
            pNode->meBinaryOperator = eOp;
            pNode->maChildren.push_back(std::move(pLeft));
            pNode->maChildren.push_back(std::move(pRight));
            pLeft = std::move(pNode);
        }
    }

    [[nodiscard]] std::unique_ptr<Node> parseAddSubtract()
    {
        auto pLeft = parseMultiplyDivide();
        if (!pLeft)
            return nullptr;

        while (true)
        {
            skipSpaces();
            BinaryOperator eOp = BinaryOperator::Add;
            if (consume(u'+'))
                eOp = BinaryOperator::Add;
            else if (consume(u'-'))
                eOp = BinaryOperator::Subtract;
            else
                return pLeft;

            auto pRight = parseMultiplyDivide();
            if (!pRight)
                return nullptr;

            auto pNode = makeNode(NodeKind::BinaryOperation);
            pNode->meBinaryOperator = eOp;
            pNode->maChildren.push_back(std::move(pLeft));
            pNode->maChildren.push_back(std::move(pRight));
            pLeft = std::move(pNode);
        }
    }

    [[nodiscard]] std::unique_ptr<Node> parseConcat()
    {
        auto pLeft = parseAddSubtract();
        if (!pLeft)
            return nullptr;

        while (true)
        {
            skipSpaces();
            if (!consume(u'&'))
                return pLeft;

            auto pRight = parseAddSubtract();
            if (!pRight)
                return nullptr;

            auto pNode = makeNode(NodeKind::BinaryOperation);
            pNode->meBinaryOperator = BinaryOperator::Concat;
            pNode->maChildren.push_back(std::move(pLeft));
            pNode->maChildren.push_back(std::move(pRight));
            pLeft = std::move(pNode);
        }
    }

public:
    explicit Parser(api::StringView rInput)
        : mrInput(rInput)
    {
    }

    [[nodiscard]] std::unique_ptr<Node> parseComparison()
    {
        auto pLeft = parseConcat();
        if (!pLeft)
            return nullptr;

        while (true)
        {
            skipSpaces();
            BinaryOperator eOp = BinaryOperator::Equal;
            if (consumeIfPresent(u"<>"))
                eOp = BinaryOperator::NotEqual;
            else if (consumeIfPresent(u"<="))
                eOp = BinaryOperator::LessEqual;
            else if (consumeIfPresent(u">="))
                eOp = BinaryOperator::GreaterEqual;
            else if (consume(u'='))
                eOp = BinaryOperator::Equal;
            else if (consume(u'<'))
                eOp = BinaryOperator::Less;
            else if (consume(u'>'))
                eOp = BinaryOperator::Greater;
            else
                return pLeft;

            auto pRight = parseConcat();
            if (!pRight)
                return nullptr;

            auto pNode = makeNode(NodeKind::BinaryOperation);
            pNode->meBinaryOperator = eOp;
            pNode->maChildren.push_back(std::move(pLeft));
            pNode->maChildren.push_back(std::move(pRight));
            pLeft = std::move(pNode);
        }
    }

    [[nodiscard]] std::unique_ptr<Node> parseLogicalChain()
    {
        auto pLeft = parseComparison();
        if (!pLeft)
            return nullptr;

        while (true)
        {
            skipSpaces();
            const std::size_t nStart = mnPos;
            const auto aToken = parseIdentifierToken();
            if ((aToken != u"AND" && aToken != u"OR") || peek() != u'(')
            {
                mnPos = nStart;
                return pLeft;
            }

            auto pRightCall = parseFunctionCall(aToken);
            if (!pRightCall)
                return nullptr;

            if (pLeft->meKind == NodeKind::FunctionCall && pLeft->maPrimaryText == aToken)
            {
                for (auto& pChild : pRightCall->maChildren)
                    pLeft->maChildren.push_back(std::move(pChild));
                continue;
            }

            auto pNode = makeNode(NodeKind::FunctionCall);
            pNode->maPrimaryText = api::String(aToken);
            pNode->maChildren.push_back(std::move(pLeft));
            for (auto& pChild : pRightCall->maChildren)
                pNode->maChildren.push_back(std::move(pChild));
            pLeft = std::move(pNode);
        }
    }

    [[nodiscard]] ParseResult parse()
    {
        skipSpaces();
        if (!consumeIfPresent(u"of:="))
            (void)consumeIfPresent(u"of:");
        if (!mrInput.substr(mnPos).empty() && mrInput[mnPos] == u'=')
            ++mnPos;
        skipSpaces();

        auto pRoot = parseLogicalChain();
        if (!pRoot)
            return { nullptr, maError, false };

        skipSpaces();
        if (!atEnd())
        {
            fail(u"unexpected trailing input");
            return { nullptr, maError, false };
        }

        return { std::move(pRoot), maError, !mbError };
    }
};

} // namespace

ParseResult parseFormula(api::StringView rFormula)
{
    Parser aParser(rFormula);
    return aParser.parse();
}

} // namespace spreadsheetengine::core::formula

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
