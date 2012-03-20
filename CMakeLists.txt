cmake_minimum_required(VERSION 2.6.2)

project(Watchdog)

# tell acmake that it is being used in integrated mode
set(ACMAKE_INTEGRATED_BUILD YES)

# add acmake to build
add_subdirectory(acmake)
find_package(AcMake)

# declare our packaging solution
ac_begin_solution(
    WATCHDOG
    VERSION_MAJOR 1 VERSION_MINOR 0 VERSION_PATCH 6
    DESCRIPTION  "Watchdog"
    VENDOR       "ART+COM AG"
    CONTACT      "y60@artcom.de"
    DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/DESCRIPTION.txt"
    LICENSE_FILE     "${CMAKE_SOURCE_DIR}/LICENSE.txt"
    PACKAGE_ICON     "${CMAKE_SOURCE_DIR}/installer/windows/NSIS-Header.bmp"
    INSTALLER_ICON   "${CMAKE_SOURCE_DIR}/installer/windows/shopping_bag.ico"
    UNINSTALLER_ICON "${CMAKE_SOURCE_DIR}/installer/windows/shopping_bag_grey.ico"
)

add_subdirectory(asl)
add_subdirectory(src)
ac_end_solution(WATCHDOG)


