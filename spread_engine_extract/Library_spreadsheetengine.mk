# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,spreadsheetengine))

$(eval $(call gb_Library_set_include,spreadsheetengine,\
    $$(INCLUDE) \
    -I$(SRCDIR)/spread_engine_extract/inc \
))

$(eval $(call gb_Library_add_defs,spreadsheetengine,\
    -DSPREADSHEETENGINE_DLLIMPLEMENTATION \
))

$(eval $(call gb_Library_use_libraries,spreadsheetengine,\
    sal \
))

$(eval $(call gb_Library_add_exception_objects,spreadsheetengine,\
    spread_engine_extract/source/core/Phase0 \
))

# vim: set noet sw=4 ts=4:
