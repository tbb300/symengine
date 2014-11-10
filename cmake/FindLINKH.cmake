include(LibFindMacros)

libfind_path(link.h linkh)

set(LINKH_INCLUDE_DIRS ${LINKH_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LINKH DEFAULT_MSG
    LINKH_INCLUDE_DIR)

mark_as_advanced(LINKH_INCLUDE_DIR)
