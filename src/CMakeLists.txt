# __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
#
# Copyright (C) 1993-2012, ART+COM AG Berlin, Germany <www.artcom.de>
#
# It is distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
# __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

cmake_minimum_required(VERSION 2.6.2)

project(Watchdog)


ac_add_project(
    Watchdog
    DISPLAY_NAME "Watchdog"
    VENDOR "ART+COM AG"
    CONTACT "info@artcom.de"
    DESCRIPTION "ARTCOM AG Watchdog"
)



set(SOURCES
Application.cpp
CRC16.cpp
HitachiProjector.cpp
Logger.cpp
NecProjector.cpp
PanasonicProjector.cpp
PdF1Projector.cpp
Projector.cpp
UDPCommandListenerThread.cpp
system_functions.cpp
watchdog.cpp
)

set(HEADERS
Application.h
CRC16.h
HitachiProjector.h
Logger.h
NecProjector.h
PanasonicProjector.h
PdF1Projector.h
Projector.h
UDPCommandListenerThread.h
system_functions.h
watchdog.h
)

#TODO: tests are currently not processed in ac_add_executable
set(TESTS
Halt
Panasonic
PdF1
)
# Make sure we can access our includes without staging
include_directories(../ ${CMAKE_CURRENT_BINARY_DIR}/../)
ac_project_add_build_include_dir(${CMAKE_CURRENT_BINARY_DIR}/../)
ac_project_add_build_include_dir(${CMAKE_CURRENT_SOURCE_DIR}/../)

ac_add_executable(
    watchdog
    SOURCES ${SOURCES}
    HEADERS ${HEADERS}  
    DEPENDS aslbase aslnet asldom aslserial
)
ac_end_project(Watchdog)
