# - Find Net-SNMP
#
# -*- cmake -*-
#
# Find the Net-SNMP module
#
#  NETSNMP_INCLUDE_DIRS - where to find Net-SNMP.h, etc.
#  NETSNMP_LIBRARIES    - List of libraries when using Net-SNMP.
#  NETSNMP_FOUND        - True if Net-SNMP found.
#
#  NETSNMPAGENT_LIBRARY - agent library
#  NETSNMPTRAPD_LIBRARY - trapd library
#
######
# source: https://gist.github.com/michaelmelanson/111912

if(NETSNMP_INCLUDE_DIRS)
  # Already in cache, be silent
  set(NETSNMP_FIND_QUIETLY TRUE)
else()
  find_path(NETSNMP_INCLUDE_DIR net-snmp/library/snmp.h /usr/include)
  set(NETSNMP_INCLUDE_DIRS "${NETSNMP_INCLUDE_DIR}")
endif()

set(NETSNMP_LIBPATHS /usr/lib /usr/local/lib)

find_library(NETSNMP_LIBRARY        NAMES netsnmp        PATHS ${NETSNMP_LIBPATHS})
find_library(NETSNMPAGENT_LIBRARY   NAMES netsnmpagent   PATHS ${NETSNMP_LIBPATHS})
find_library(NETSNMPHELPERS_LIBRARY NAMES netsnmphelpers PATHS ${NETSNMP_LIBPATHS})
find_library(NETSNMPMIBS_LIBRARY    NAMES netsnmpmibs    PATHS ${NETSNMP_LIBPATHS})
find_library(NETSNMPTRAPD_LIBRARY   NAMES netsnmptrapd   PATHS ${NETSNMP_LIBPATHS})

set(NETSNMP_LIBRARIES
  ${NETSNMP_LIBRARY}
  ${NETSNMPHELPERS_LIBRARY}
  ${NETSNMPMIBS_LIBRARY}
#  ${NETSNMPAGENT_LIBRARY}
#  ${NETSNMPTRAPD_LIBRARY}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NETSNMP DEFAULT_MSG
  NETSNMP_INCLUDE_DIR NETSNMP_LIBRARIES)

mark_as_advanced(
  NETSNMP_LIBRARY
  NETSNMPAGENT_LIBRARY
  NETSNMPHELPERS_LIBRARY
  NETSNMPMIBS_LIBRARY
  NETSNMPTRAPD_LIBRARY
  NETSNMP_INCLUDE_DIR)
