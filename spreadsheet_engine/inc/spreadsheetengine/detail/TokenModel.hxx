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

using OpCodeValue = sal_uInt16;
using ParamClassValue = sal_uInt8;
using ErrorCode = sal_uInt16;

constexpr OpCodeValue kOpCodeNone = 0;
constexpr ParamClassValue kParamClassUnknown = 0;
constexpr ErrorCode kErrorCodeNone = 0;

enum class Kind : sal_uInt8
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

enum class VectorState : sal_uInt8
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
    sal_uInt8 mnByte = 0;
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
    sal_Int16 mnSheet = -1;
    sal_uInt16 mnIndex = 0;

    [[nodiscard]] constexpr bool operator==(const NameData& rOther) const = default;
};

struct DatabaseRangeData
{
    sal_uInt16 mnIndex = 0;

    [[nodiscard]] constexpr bool operator==(const DatabaseRangeData& rOther) const = default;
};

struct ExternalSingleRefData
{
    sal_uInt16 mnFileId = 0;
    api::String maTabName;
    api::refdata::SingleRefData maReference;

    [[nodiscard]] constexpr bool operator==(const ExternalSingleRefData& rOther) const = default;
};

struct ExternalDoubleRefData
{
    sal_uInt16 mnFileId = 0;
    api::String maTabName;
    api::refdata::ComplexRefData maReference;

    [[nodiscard]] constexpr bool operator==(const ExternalDoubleRefData& rOther) const = default;
};

struct ExternalNameData
{
    sal_uInt16 mnFileId = 0;
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

enum class TableRefItem : sal_uInt16
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
    return static_cast<TableRefItem>(static_cast<sal_uInt16>(eLeft)
                                     | static_cast<sal_uInt16>(eRight));
}

constexpr TableRefItem& operator|=(TableRefItem& reLeft, TableRefItem eRight)
{
    reLeft = reLeft | eRight;
    return reLeft;
}

struct TableRefData
{
    sal_uInt16 mnIndex = 0;
    TableRefItem meItem = TableRefItem::None;

    [[nodiscard]] constexpr bool operator==(const TableRefData& rOther) const = default;
};

struct JumpData
{
    std::vector<short> maJumps;

    [[nodiscard]] constexpr bool operator==(const JumpData& rOther) const = default;
};

struct WhitespaceData
{
    sal_uInt8 mnCount = 0;
    sal_Unicode mcChar = 0;

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
    std::size_t nHash = detail::hashIntegralLike(rFormula.meVectorState);
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
