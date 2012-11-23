cmake_minimum_required(VERSION 2.6.2)

project(Watchdog)

find_package(AcMake)
find_package(ASL)

# declare our packaging solution
ac_begin_solution(
    Watchdog
    VERSION_MAJOR 1 VERSION_MINOR 0 VERSION_PATCH 8
    DESCRIPTION  "Watchdog"
    VENDOR       "ART+COM AG"
    CONTACT      "info@artcom.de"
    DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/DESCRIPTION.txt"
    LICENSE_FILE     "${CMAKE_SOURCE_DIR}/LICENSE.txt"
    PACKAGE_ICON     "${CMAKE_SOURCE_DIR}/installer/windows/NSIS-Header.bmp"
    INSTALLER_ICON   "${CMAKE_SOURCE_DIR}/installer/windows/shopping_bag.ico"
    UNINSTALLER_ICON "${CMAKE_SOURCE_DIR}/installer/windows/shopping_bag_grey.ico"
)


# declare cpack components
set(RUNTIME_COMPONENT Watchdog_runtime)
set(DEVELOPMENT_COMPONENT Watchdog_development)

add_subdirectory(src)

cpack_add_component( ${RUNTIME_COMPONENT}
            DISPLAY_NAME "Runtime"
            DESCRIPTION "runtime libraries" REQUIRED)
cpack_add_component( ${DEVELOPMENT_COMPONENT}
            DISPLAY_NAME "Development"
            DESCRIPTION "header files and import libraries")
            
ac_end_solution(Watchdog)


