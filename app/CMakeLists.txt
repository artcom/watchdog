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


ac_add_executable(
    asl-watchdog
    SOURCES ${SOURCES}
    HEADERS ${HEADERS}  
    DEPENDS aslbase aslnet asldom aslserial paintlib
)
