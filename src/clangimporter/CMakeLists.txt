# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/12/22.

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLARPHP_CLANG_IMPORTER_SOURCES)

polar_merge_list(POLARPHP_CLANG_IMPORTER_SOURCES POLAR_HEADERS)

polar_add_host_library(PolarClangImporter STATIC
   ${POLARPHP_CLANG_IMPORTER_SOURCES})

target_link_libraries(PolarClangImporter
   PRIVATE
   PolarAST
   PolarLLParser)