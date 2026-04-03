/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <optional>
#include <variant>
#include <vector>

#include <spreadsheetengine/api/ReferenceData.hxx>
#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/detail/TokenModel.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadow.hxx>

namespace spreadsheetengine::detail::substrate
{

enum class ExecutionIrInstructionKind : std::uint8_t
{
    PlainOpcode,
    FunctionCallMarker,
    UnaryPlusMarker,
    RangeConstructorMarker,
    ReferenceListMarker,
    MissingArgument,
    ByteLiteral,
    NumberLiteral,
    StringLiteral,
    StringNameLiteral,
    SingleReference,
    RangeReference,
    RangeNameReference,
    DatabaseRangeReference,
    ExternalSingleReference,
    ExternalRangeReference,
    ExternalNameReference,
    MatrixLiteral,
    ColumnRowNameReference,
    TableReference,
    ErrorLiteral,
    JumpTable,
    Whitespace
};

struct ExecutionIrByteData
{
    std::uint8_t mnByte = 0;
    token::ParamClassValue mnInForceArray = token::kParamClassUnknown;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrByteData& rOther) const = default;
};

struct ExecutionIrStringData
{
    api::String maText;
    api::String maCaseFolded;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrStringData& rOther) const = default;
};

struct ExecutionIrNameData
{
    std::int16_t mnSheet = -1;
    std::uint16_t mnIndex = 0;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrNameData& rOther) const = default;
};

struct ExecutionIrDatabaseRangeData
{
    std::uint16_t mnIndex = 0;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrDatabaseRangeData& rOther) const
        = default;
};

struct ExecutionIrExternalSingleRefData
{
    std::uint16_t mnFileId = 0;
    api::String maTabName;
    api::refdata::SingleRefData maReference;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrExternalSingleRefData& rOther) const
        = default;
};

struct ExecutionIrExternalDoubleRefData
{
    std::uint16_t mnFileId = 0;
    api::String maTabName;
    api::refdata::ComplexRefData maReference;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrExternalDoubleRefData& rOther) const
        = default;
};

struct ExecutionIrExternalNameData
{
    std::uint16_t mnFileId = 0;
    api::String maName;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrExternalNameData& rOther) const
        = default;
};

using ExecutionIrMatrixScalar = std::variant<double, api::String, token::ErrorCode>;

struct ExecutionIrMatrixData
{
    sal_Int32 mnColumns = 0;
    sal_Int32 mnRows = 0;
    std::vector<ExecutionIrMatrixScalar> maValues;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrMatrixData& rOther) const = default;
};

struct ExecutionIrTableRefData
{
    std::uint16_t mnIndex = 0;
    token::TableRefItem meItem = token::TableRefItem::None;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrTableRefData& rOther) const = default;
};

struct ExecutionIrJumpData
{
    std::vector<short> maJumps;
    token::ParamClassValue mnInForceArray = token::kParamClassUnknown;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrJumpData& rOther) const = default;
};

struct ExecutionIrWhitespaceData
{
    std::uint8_t mnCount = 0;
    char16_t mcChar = 0;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrWhitespaceData& rOther) const
        = default;
};

using ExecutionIrPayload = std::variant<std::monostate, ExecutionIrByteData, double,
    ExecutionIrStringData, api::refdata::SingleRefData, api::refdata::ComplexRefData,
    ExecutionIrNameData, ExecutionIrDatabaseRangeData, ExecutionIrExternalSingleRefData,
    ExecutionIrExternalDoubleRefData, ExecutionIrExternalNameData, ExecutionIrMatrixData,
    ExecutionIrTableRefData, token::ErrorCode, ExecutionIrJumpData, ExecutionIrWhitespaceData>;

struct ExecutionIrInstruction
{
    ExecutionIrInstructionKind meKind = ExecutionIrInstructionKind::PlainOpcode;
    token::OpCodeValue mnOpCode = token::kOpCodeNone;
    ExecutionIrPayload maPayload;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrInstruction& rOther) const = default;

    [[nodiscard]] constexpr bool carriesReference() const
    {
        switch (meKind)
        {
            case ExecutionIrInstructionKind::SingleReference:
            case ExecutionIrInstructionKind::RangeReference:
            case ExecutionIrInstructionKind::ExternalSingleReference:
            case ExecutionIrInstructionKind::ExternalRangeReference:
            case ExecutionIrInstructionKind::ColumnRowNameReference:
                return true;
            default:
                return false;
        }
    }
};

struct ExecutionIrFormulaSource
{
    api::String maFormula;
    api::String maNamespace;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrFormulaSource& rOther) const = default;
    [[nodiscard]] constexpr bool hasNamespace() const { return !maNamespace.empty(); }
};

struct ExecutionIrBuildFailure
{
    ShadowCellId maId;
    api::String maFormulaSource;
    api::String maFailureMessage;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrBuildFailure& rOther) const = default;
};

struct ExecutionIrFormulaRecord
{
    ShadowCellId maId;
    std::optional<ShadowFormulaGroupId> moFormulaGroup;
    ExecutionIrFormulaSource maSource;
    std::vector<ExecutionIrInstruction> maInstructions;
    token::ErrorCode mnCodeError = token::kErrorCodeNone;
    bool mbHyperLink = false;
    bool mbFromRangeName = false;
    bool mbShareable = true;
    token::VectorState meVectorState = token::VectorState::Unknown;
    bool mbOpenCLEnabled = false;
    bool mbThreadingEnabled = false;
    bool mbInFormulaTree = false;
    bool mbInFormulaTrack = false;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrFormulaRecord& rOther) const = default;

    [[nodiscard]] sal_Int32 getReferenceInstructionCount() const
    {
        return static_cast<sal_Int32>(std::count_if(maInstructions.begin(), maInstructions.end(),
            [](const ExecutionIrInstruction& rInstruction) {
                return rInstruction.carriesReference();
            }));
    }
};

struct ExecutionIrWorkbookShadow
{
    facade::WorkbookSnapshotInfo maSnapshot;
    api::Grammar maGrammar;
    std::vector<ExecutionIrFormulaRecord> maFormulaRecords;
    std::vector<ShadowFormulaGroupRecord> maFormulaGroups;
    std::vector<ExecutionIrBuildFailure> maBuildFailures;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrWorkbookShadow& rOther) const = default;

    [[nodiscard]] sal_Int32 getFormulaCount() const
    {
        return static_cast<sal_Int32>(maFormulaRecords.size());
    }

    [[nodiscard]] sal_Int32 getInstructionCount() const
    {
        sal_Int32 nCount = 0;
        for (const auto& rFormula : maFormulaRecords)
            nCount += static_cast<sal_Int32>(rFormula.maInstructions.size());
        return nCount;
    }

    [[nodiscard]] bool isFullyLowered() const { return maBuildFailures.empty(); }

    [[nodiscard]] const ExecutionIrFormulaRecord* findFormula(const api::CellAddress& rAddress) const
    {
        auto it = std::find_if(maFormulaRecords.begin(), maFormulaRecords.end(),
            [&rAddress](const ExecutionIrFormulaRecord& rRecord) {
                return rRecord.maId.maAddress == rAddress;
            });
        return it == maFormulaRecords.end() ? nullptr : &*it;
    }
};

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
