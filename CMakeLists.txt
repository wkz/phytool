project(phytool C)

cmake_minimum_required(VERSION 2.6)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Linux MDIO register access")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "2")
set(CPACK_SOURCE_STRIP_FILES "")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/")
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_CONTACT "tobias@waldekranz.com")
include(CPack)

set(SRCS phytool.c print_mv6.c print_phy.c)

add_executable(phytool ${SRCS})

add_custom_command(
  TARGET phytool
  POST_BUILD
  COMMAND ln;-sf;phytool;mv6tool
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/phytool
    ${CMAKE_CURRENT_BINARY_DIR}/mv6tool
  DESTINATION bin
)
