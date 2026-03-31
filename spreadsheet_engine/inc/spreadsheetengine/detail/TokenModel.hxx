/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <spreadsheetengine/api/ReferenceData.hxx>
#include <spreadsheetengine/api/String.hxx>

namespace spreadsheetengine::detail::token
{

using OpCodeValue = std::uint16_t;
using ParamClassValue = std::uint8_t;
using ErrorCode = std::uint16_t;

constexpr OpCodeValue kOpCodeNone = 0;
constexpr OpCodeValue kOpCodePush = 0;
constexpr OpCodeValue kOpCodeName = 4;
constexpr OpCodeValue kOpCodeIf = 6;
constexpr OpCodeValue kOpCodeIfError = 7;
constexpr OpCodeValue kOpCodeIfNa = 8;
constexpr OpCodeValue kOpCodeOpen = 10;
constexpr OpCodeValue kOpCodeClose = 11;
constexpr OpCodeValue kOpCodeSep = 12;
constexpr OpCodeValue kOpCodeMissing = 13;
constexpr OpCodeValue kOpCodeBad = 14;
constexpr OpCodeValue kOpCodeAdd = 50;
constexpr OpCodeValue kOpCodeSub = 51;
constexpr OpCodeValue kOpCodeMul = 52;
constexpr OpCodeValue kOpCodeDiv = 53;
constexpr OpCodeValue kOpCodeAmpersand = 54;
constexpr OpCodeValue kOpCodePow = 55;
constexpr OpCodeValue kOpCodeEqual = 56;
constexpr OpCodeValue kOpCodeNotEqual = 57;
constexpr OpCodeValue kOpCodeLess = 58;
constexpr OpCodeValue kOpCodeGreater = 59;
constexpr OpCodeValue kOpCodeLessEqual = 60;
constexpr OpCodeValue kOpCodeGreaterEqual = 61;
constexpr OpCodeValue kOpCodeAnd = 62;
constexpr OpCodeValue kOpCodeUnion = 65;
constexpr OpCodeValue kOpCodeRange = 66;
constexpr OpCodeValue kOpCodeNegSub = 70;
constexpr OpCodeValue kOpCodePi = 75;
constexpr OpCodeValue kOpCodeTrue = 77;
constexpr OpCodeValue kOpCodeFalse = 78;
constexpr OpCodeValue kOpCodeGetActDate = 79;
constexpr OpCodeValue kOpCodeNoValue = 81;
constexpr OpCodeValue kOpCodeDegrees = 90;
constexpr OpCodeValue kOpCodeXor = 401;
constexpr OpCodeValue kOpCodeArcCot = 99;
constexpr OpCodeValue kOpCodeAtanh = 106;
constexpr OpCodeValue kOpCodeGetYear = 116;
constexpr OpCodeValue kOpCodeGetMonth = 117;
constexpr OpCodeValue kOpCodeGetDay = 118;
constexpr OpCodeValue kOpCodeGetHour = 119;
constexpr OpCodeValue kOpCodeGetMin = 120;
constexpr OpCodeValue kOpCodeGetSec = 121;
constexpr OpCodeValue kOpCodeIsEmpty = 127;
constexpr OpCodeValue kOpCodeIsNv = 135;
constexpr OpCodeValue kOpCodeIsError = 137;
constexpr OpCodeValue kOpCodeIsEven = 138;
constexpr OpCodeValue kOpCodeIsOdd = 139;
constexpr OpCodeValue kOpCodeN = 140;
constexpr OpCodeValue kOpCodeGetDateValue = 141;
constexpr OpCodeValue kOpCodeGetTimeValue = 142;
constexpr OpCodeValue kOpCodeCode = 143;
constexpr OpCodeValue kOpCodeUpper = 145;
constexpr OpCodeValue kOpCodeLower = 147;
constexpr OpCodeValue kOpCodeLen = 148;
constexpr OpCodeValue kOpCodeT = 149;
constexpr OpCodeValue kOpCodeValue = 150;
constexpr OpCodeValue kOpCodeClean = 151;
constexpr OpCodeValue kOpCodeChar = 152;
constexpr OpCodeValue kOpCodeFormula = 163;
constexpr OpCodeValue kOpCodeJis = 167;
constexpr OpCodeValue kOpCodeAsc = 168;
constexpr OpCodeValue kOpCodeUnichar = 170;
constexpr OpCodeValue kOpCodeIsoWeeknum = 177;
constexpr OpCodeValue kOpCodeCeil = 202;
constexpr OpCodeValue kOpCodeFloor = 203;
constexpr OpCodeValue kOpCodeRound = 204;
constexpr OpCodeValue kOpCodeRoundUp = 205;
constexpr OpCodeValue kOpCodeLog = 208;
constexpr OpCodeValue kOpCodeRoundDown = 206;
constexpr OpCodeValue kOpCodeGcd = 210;
constexpr OpCodeValue kOpCodeLcm = 211;
constexpr OpCodeValue kOpCodeMod = 212;
constexpr OpCodeValue kOpCodeGetDate = 218;
constexpr OpCodeValue kOpCodeGetTime = 219;
constexpr OpCodeValue kOpCodeGetDiffDate360 = 221;
constexpr OpCodeValue kOpCodeMin = 222;
constexpr OpCodeValue kOpCodeMax = 223;
constexpr OpCodeValue kOpCodeSum = 224;
constexpr OpCodeValue kOpCodeColumns = 252;
constexpr OpCodeValue kOpCodeSubTotal = 266;
constexpr OpCodeValue kOpCodeIndirect = 279;
constexpr OpCodeValue kOpCodeAddress = 280;
constexpr OpCodeValue kOpCodeMatch = 281;
constexpr OpCodeValue kOpCodeSumIf = 284;
constexpr OpCodeValue kOpCodeLookup = 285;
constexpr OpCodeValue kOpCodeVLookup = 286;
constexpr OpCodeValue kOpCodeHLookup = 287;
constexpr OpCodeValue kOpCodeOffset = 289;
constexpr OpCodeValue kOpCodeAreas = 291;
constexpr OpCodeValue kOpCodeReplace = 293;
constexpr OpCodeValue kOpCodeExact = 296;
constexpr OpCodeValue kOpCodeLeft = 297;
constexpr OpCodeValue kOpCodeRight = 298;
constexpr OpCodeValue kOpCodeSearch = 299;
constexpr OpCodeValue kOpCodeMid = 300;
constexpr OpCodeValue kOpCodeText = 301;
constexpr OpCodeValue kOpCodeConcat = 304;
constexpr OpCodeValue kOpCodeMatMult = 308;
constexpr OpCodeValue kOpCodeEasterSunday = 380;
constexpr OpCodeValue kOpCodeDecimal = 381;
constexpr OpCodeValue kOpCodeWeek = 365;
constexpr OpCodeValue kOpCodeGetDayOfWeek = 366;
constexpr OpCodeValue kOpCodeBase = 370;
constexpr OpCodeValue kOpCodeHyperLink = 387;
constexpr OpCodeValue kOpCodeGetPivotData = 390;
constexpr OpCodeValue kOpCodeEuroConvert = 391;
constexpr OpCodeValue kOpCodeDateDif = 400;
constexpr OpCodeValue kOpCodeLenB = 407;
constexpr OpCodeValue kOpCodeNetWorkdaysMs = 468;
constexpr OpCodeValue kOpCodeAggregate = 470;
constexpr OpCodeValue kOpCodeNetWorkdays = 474;
constexpr OpCodeValue kOpCodeRawSubtract = 477;
constexpr OpCodeValue kOpCodeConcatMs = 487;
constexpr OpCodeValue kOpCodeTextJoinMs = 488;
constexpr OpCodeValue kOpCodeReplaceB = 494;
constexpr OpCodeValue kOpCodeFindB = 495;
constexpr OpCodeValue kOpCodeSearchB = 496;
constexpr ParamClassValue kParamClassUnknown = 0;
constexpr ErrorCode kErrorCodeNone = 0;

enum class Kind : std::uint8_t
{
    PlainOpcode,
    Missing,
    Byte,
    Value,
    String,
    StringName,
    SingleRef,
    DoubleRef,
    RangeName,
    DatabaseRange,
    ExternalSingleRef,
    ExternalDoubleRef,
    ExternalName,
    Matrix,
    ColRowName,
    TableRef,
    Error,
    Jump,
    Whitespace
};

enum class VectorState : std::uint8_t
{
    Disabled,
    DisabledNotInSubSet,
    DisabledByOpCode,
    DisabledByStackVariable,
    Enabled,
    CheckReference,
    Unknown
};

[[nodiscard]] constexpr std::u16string_view kindName(const Kind eKind)
{
    switch (eKind)
    {
        case Kind::PlainOpcode:
            return u"PlainOpcode";
        case Kind::Missing:
            return u"Missing";
        case Kind::Byte:
            return u"Byte";
        case Kind::Value:
            return u"Value";
        case Kind::String:
            return u"String";
        case Kind::StringName:
            return u"StringName";
        case Kind::SingleRef:
            return u"SingleRef";
        case Kind::DoubleRef:
            return u"DoubleRef";
        case Kind::RangeName:
            return u"RangeName";
        case Kind::DatabaseRange:
            return u"DatabaseRange";
        case Kind::ExternalSingleRef:
            return u"ExternalSingleRef";
        case Kind::ExternalDoubleRef:
            return u"ExternalDoubleRef";
        case Kind::ExternalName:
            return u"ExternalName";
        case Kind::Matrix:
            return u"Matrix";
        case Kind::ColRowName:
            return u"ColRowName";
        case Kind::TableRef:
            return u"TableRef";
        case Kind::Error:
            return u"Error";
        case Kind::Jump:
            return u"Jump";
        case Kind::Whitespace:
            return u"Whitespace";
    }
    return u"Unknown";
}

struct ByteData
{
    std::uint8_t mnByte = 0;
    ParamClassValue mnInForceArray = kParamClassUnknown;

    [[nodiscard]] constexpr bool operator==(const ByteData& rOther) const = default;
};

struct StringData
{
    api::String maText;
    api::String maCaseFolded;

    [[nodiscard]] constexpr bool operator==(const StringData& rOther) const = default;
};

struct NameData
{
    std::int16_t mnSheet = -1;
    std::uint16_t mnIndex = 0;

    [[nodiscard]] constexpr bool operator==(const NameData& rOther) const = default;
};

struct DatabaseRangeData
{
    std::uint16_t mnIndex = 0;

    [[nodiscard]] constexpr bool operator==(const DatabaseRangeData& rOther) const = default;
};

struct ExternalSingleRefData
{
    std::uint16_t mnFileId = 0;
    api::String maTabName;
    api::refdata::SingleRefData maReference;

    [[nodiscard]] constexpr bool operator==(const ExternalSingleRefData& rOther) const = default;
};

struct ExternalDoubleRefData
{
    std::uint16_t mnFileId = 0;
    api::String maTabName;
    api::refdata::ComplexRefData maReference;

    [[nodiscard]] constexpr bool operator==(const ExternalDoubleRefData& rOther) const = default;
};

struct ExternalNameData
{
    std::uint16_t mnFileId = 0;
    api::String maName;

    [[nodiscard]] constexpr bool operator==(const ExternalNameData& rOther) const = default;
};

using MatrixScalar = std::variant<double, api::String, ErrorCode>;

struct MatrixData
{
    sal_Int32 mnColumns = 0;
    sal_Int32 mnRows = 0;
    std::vector<MatrixScalar> maValues;

    [[nodiscard]] constexpr bool operator==(const MatrixData& rOther) const = default;
};

enum class TableRefItem : std::uint16_t
{
    None = 0,
    Table = 1 << 0,
    All = 1 << 1,
    Headers = 1 << 2,
    Data = 1 << 3,
    Totals = 1 << 4,
    ThisRow = 1 << 5
};

constexpr TableRefItem operator|(TableRefItem eLeft, TableRefItem eRight)
{
    return static_cast<TableRefItem>(static_cast<std::uint16_t>(eLeft)
                                     | static_cast<std::uint16_t>(eRight));
}

constexpr TableRefItem& operator|=(TableRefItem& reLeft, TableRefItem eRight)
{
    reLeft = reLeft | eRight;
    return reLeft;
}

struct TableRefData
{
    std::uint16_t mnIndex = 0;
    TableRefItem meItem = TableRefItem::None;

    [[nodiscard]] constexpr bool operator==(const TableRefData& rOther) const = default;
};

struct JumpData
{
    std::vector<short> maJumps;
    ParamClassValue mnInForceArray = kParamClassUnknown;

    [[nodiscard]] constexpr bool operator==(const JumpData& rOther) const = default;
};

struct WhitespaceData
{
    std::uint8_t mnCount = 0;
    char16_t mcChar = 0;

    [[nodiscard]] constexpr bool operator==(const WhitespaceData& rOther) const = default;
};

using Payload = std::variant<std::monostate, ByteData, double, StringData, api::refdata::SingleRefData,
    api::refdata::ComplexRefData, NameData, DatabaseRangeData, ExternalSingleRefData,
    ExternalDoubleRefData, ExternalNameData, MatrixData, TableRefData, ErrorCode, JumpData,
    WhitespaceData>;

struct Token
{
    Kind meKind = Kind::PlainOpcode;
    OpCodeValue mnOpCode = kOpCodeNone;
    Payload maPayload;

    [[nodiscard]] constexpr bool operator==(const Token& rOther) const = default;
};

struct XmlFormulaSource
{
    api::String maFormula;
    api::String maNamespace;

    [[nodiscard]] constexpr bool operator==(const XmlFormulaSource& rOther) const = default;
};

struct CompiledFormula
{
    std::vector<Token> maTokens;
    std::optional<XmlFormulaSource> moXmlFormulaSource;
    ErrorCode mnCodeError = kErrorCodeNone;
    std::uint8_t mnRecalcModeBits = 0;
    bool mbHyperLink = false;
    bool mbFromRangeName = false;
    bool mbShareable = true;
    VectorState meVectorState = VectorState::Unknown;
    bool mbOpenCLEnabled = false;
    bool mbThreadingEnabled = false;

    [[nodiscard]] constexpr bool operator==(const CompiledFormula& rOther) const = default;
};

namespace detail
{

inline void hashCombine(std::size_t& rnSeed, std::size_t nValue)
{
    rnSeed ^= nValue + 0x9e3779b97f4a7c15ULL + (rnSeed << 6) + (rnSeed >> 2);
}

template <typename T>
    requires std::is_enum_v<T>
[[nodiscard]] inline std::size_t hashIntegralLike(T nValue)
{
    using Value = std::underlying_type_t<T>;
    return std::hash<Value> {}(static_cast<Value>(nValue));
}

template <typename T>
    requires (!std::is_enum_v<T>)
[[nodiscard]] inline std::size_t hashIntegralLike(T nValue)
{
    return std::hash<T> {}(nValue);
}

[[nodiscard]] inline std::size_t hashString(api::StringView rValue)
{
    return std::hash<std::u16string_view> {}(rValue);
}

[[nodiscard]] inline std::size_t hashSingleRef(const api::refdata::SingleRefData& rData)
{
    std::size_t nHash = 0;
    hashCombine(nHash, hashIntegralLike(rData.mnColumn));
    hashCombine(nHash, hashIntegralLike(rData.mnRow));
    hashCombine(nHash, hashIntegralLike(rData.mnSheet));
    hashCombine(nHash, hashIntegralLike(rData.maFlags.mbColumnRelative));
    hashCombine(nHash, hashIntegralLike(rData.maFlags.mbColumnDeleted));
    hashCombine(nHash, hashIntegralLike(rData.maFlags.mbRowRelative));
    hashCombine(nHash, hashIntegralLike(rData.maFlags.mbRowDeleted));
    hashCombine(nHash, hashIntegralLike(rData.maFlags.mbSheetRelative));
    hashCombine(nHash, hashIntegralLike(rData.maFlags.mbSheetDeleted));
    hashCombine(nHash, hashIntegralLike(rData.maFlags.mbFlag3D));
    hashCombine(nHash, hashIntegralLike(rData.maFlags.mbRelativeName));
    return nHash;
}

[[nodiscard]] inline std::size_t hashDoubleRef(const api::refdata::ComplexRefData& rData)
{
    std::size_t nHash = hashSingleRef(rData.maRef1);
    hashCombine(nHash, hashSingleRef(rData.maRef2));
    hashCombine(nHash, hashIntegralLike(rData.mbTrimToData));
    return nHash;
}

[[nodiscard]] inline std::size_t hashMatrixScalar(const MatrixScalar& rScalar)
{
    return std::visit(
        [](const auto& rValue) -> std::size_t {
            using Value = std::decay_t<decltype(rValue)>;
            if constexpr (std::is_same_v<Value, double>)
                return std::hash<double> {}(rValue);
            else if constexpr (std::is_same_v<Value, api::String>)
                return hashString(rValue);
            else
                return hashIntegralLike(rValue);
        },
        rScalar);
}

[[nodiscard]] inline std::size_t hashPayload(const Payload& rPayload)
{
    return std::visit(
        [](const auto& rValue) -> std::size_t {
            using Value = std::decay_t<decltype(rValue)>;
            if constexpr (std::is_same_v<Value, std::monostate>)
            {
                return 0;
            }
            else if constexpr (std::is_same_v<Value, ByteData>)
            {
                std::size_t nHash = hashIntegralLike(rValue.mnByte);
                hashCombine(nHash, hashIntegralLike(rValue.mnInForceArray));
                return nHash;
            }
            else if constexpr (std::is_same_v<Value, double>)
            {
                return std::hash<double> {}(rValue);
            }
            else if constexpr (std::is_same_v<Value, StringData>)
            {
                std::size_t nHash = hashString(rValue.maText);
                hashCombine(nHash, hashString(rValue.maCaseFolded));
                return nHash;
            }
            else if constexpr (std::is_same_v<Value, api::refdata::SingleRefData>)
            {
                return hashSingleRef(rValue);
            }
            else if constexpr (std::is_same_v<Value, api::refdata::ComplexRefData>)
            {
                return hashDoubleRef(rValue);
            }
            else if constexpr (std::is_same_v<Value, NameData>)
            {
                std::size_t nHash = hashIntegralLike(rValue.mnSheet);
                hashCombine(nHash, hashIntegralLike(rValue.mnIndex));
                return nHash;
            }
            else if constexpr (std::is_same_v<Value, DatabaseRangeData>)
            {
                return hashIntegralLike(rValue.mnIndex);
            }
            else if constexpr (std::is_same_v<Value, ExternalSingleRefData>)
            {
                std::size_t nHash = hashIntegralLike(rValue.mnFileId);
                hashCombine(nHash, hashString(rValue.maTabName));
                hashCombine(nHash, hashSingleRef(rValue.maReference));
                return nHash;
            }
            else if constexpr (std::is_same_v<Value, ExternalDoubleRefData>)
            {
                std::size_t nHash = hashIntegralLike(rValue.mnFileId);
                hashCombine(nHash, hashString(rValue.maTabName));
                hashCombine(nHash, hashDoubleRef(rValue.maReference));
                return nHash;
            }
            else if constexpr (std::is_same_v<Value, ExternalNameData>)
            {
                std::size_t nHash = hashIntegralLike(rValue.mnFileId);
                hashCombine(nHash, hashString(rValue.maName));
                return nHash;
            }
            else if constexpr (std::is_same_v<Value, MatrixData>)
            {
                std::size_t nHash = hashIntegralLike(rValue.mnColumns);
                hashCombine(nHash, hashIntegralLike(rValue.mnRows));
                for (const auto& rScalar : rValue.maValues)
                    hashCombine(nHash, hashMatrixScalar(rScalar));
                return nHash;
            }
            else if constexpr (std::is_same_v<Value, TableRefData>)
            {
                std::size_t nHash = hashIntegralLike(rValue.mnIndex);
                hashCombine(nHash, hashIntegralLike(rValue.meItem));
                return nHash;
            }
            else if constexpr (std::is_same_v<Value, ErrorCode>)
            {
                return hashIntegralLike(rValue);
            }
            else if constexpr (std::is_same_v<Value, JumpData>)
            {
                std::size_t nHash = 0;
                for (short nJump : rValue.maJumps)
                    hashCombine(nHash, hashIntegralLike(nJump));
                hashCombine(nHash, hashIntegralLike(rValue.mnInForceArray));
                return nHash;
            }
            else
            {
                std::size_t nHash = hashIntegralLike(rValue.mnCount);
                hashCombine(nHash, hashIntegralLike(rValue.mcChar));
                return nHash;
            }
        },
        rPayload);
}

} // namespace detail

[[nodiscard]] inline std::size_t hashToken(const Token& rToken)
{
    std::size_t nHash = detail::hashIntegralLike(rToken.meKind);
    detail::hashCombine(nHash, detail::hashIntegralLike(rToken.mnOpCode));
    detail::hashCombine(nHash, detail::hashPayload(rToken.maPayload));
    return nHash;
}

[[nodiscard]] inline std::size_t hashCompiledFormula(const CompiledFormula& rFormula)
{
    std::size_t nHash = detail::hashIntegralLike(rFormula.mnCodeError);
    detail::hashCombine(nHash, detail::hashIntegralLike(rFormula.mnRecalcModeBits));
    detail::hashCombine(nHash, detail::hashIntegralLike(rFormula.mbHyperLink));
    detail::hashCombine(nHash, detail::hashIntegralLike(rFormula.mbFromRangeName));
    detail::hashCombine(nHash, detail::hashIntegralLike(rFormula.mbShareable));
    detail::hashCombine(nHash, detail::hashIntegralLike(rFormula.meVectorState));
    detail::hashCombine(nHash, detail::hashIntegralLike(rFormula.mbOpenCLEnabled));
    detail::hashCombine(nHash, detail::hashIntegralLike(rFormula.mbThreadingEnabled));
    if (rFormula.moXmlFormulaSource)
    {
        detail::hashCombine(nHash, detail::hashString(rFormula.moXmlFormulaSource->maFormula));
        detail::hashCombine(
            nHash, detail::hashString(rFormula.moXmlFormulaSource->maNamespace));
    }
    for (const auto& rToken : rFormula.maTokens)
        detail::hashCombine(nHash, hashToken(rToken));
    return nHash;
}

} // namespace spreadsheetengine::detail::token

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
