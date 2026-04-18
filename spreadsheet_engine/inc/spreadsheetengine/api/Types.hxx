/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

// When built inside LibreOffice, include sal/types.h so that sal_Int32 etc.
// are the real LibreOffice typedefs (critical for ABI compatibility with Calc
// on platforms where sal_Int32 is 'long' rather than 'int').
// When built standalone, define only the two aliases that engine headers
// still use in public signatures. All other integer types use standard
// C++ names directly.

#if __has_include(<sal/config.h>)
#include <sal/types.h>
#else
#include <cstdint>
using sal_Int32  = std::int32_t;
using sal_uInt32 = std::uint32_t;
using sal_Int64  = std::int64_t;
using sal_uInt64 = std::uint64_t;
#define SAL_DLLPUBLIC_EXPORT
#define SAL_DLLPUBLIC_IMPORT
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
