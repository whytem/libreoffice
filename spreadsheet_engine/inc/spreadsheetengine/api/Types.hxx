/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

// When built inside LibreOffice, resolve to sal_* types so that Calc code
// passing sal_Int32 etc. across the engine boundary sees identical types.
// When built standalone, resolve to standard C++ fixed-width types.

#if __has_include(<sal/config.h>)
#include <sal/types.h>
#else
#include <cstdint>
using sal_Bool   = bool;
using sal_Unicode = char16_t;
using sal_Int8   = std::int8_t;
using sal_uInt8  = std::uint8_t;
using sal_Int16  = std::int16_t;
using sal_uInt16 = std::uint16_t;
using sal_Int32  = std::int32_t;
using sal_uInt32 = std::uint32_t;
using sal_Int64  = std::int64_t;
using sal_uInt64 = std::uint64_t;
#define SAL_DLLPUBLIC_EXPORT
#define SAL_DLLPUBLIC_IMPORT
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
