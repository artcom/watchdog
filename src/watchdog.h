// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2012, ART+COM AG Berlin, Germany <www.artcom.de>
//
// This file is part of the ART+COM watchdog.
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//=============================================================================
//
//
//  Description: The watchdog restarts the application, if it is closed
//               manually or by accident
//
//
//=============================================================================

#include "Logger.h"
#include "Application.h"
#include "UDPCommandListenerThread.h"

#include <vector>
#include <string>
#include <fstream>

namespace dom {
    class Document;
};

class Projector;

class WatchDog {
public:
    WatchDog();
    bool init(dom::Document & theConfigDoc, bool theRestartAppFlag);
    void arm();
    bool watch();

private:
    Logger              _myLogger;
    std::string         _myLogFilename;
    int                 _myWatchFrequency;

    std::string         _myStartupCommand;
    std::string         _myShutdownCommand;
    std::string         _myApplicationTerminatedCommand;
    std::string         _myApplicationPreTerminateCommand;
    std::string         _myContinuousStateChangeIP;
    int                 _myContinuousStateChangePort;
    bool                _myIgnoreTerminateCmdOnUdpCmd;
    Application         _myAppToWatch;

    UDPCommandListenerThread * _myUDPCommandListenerThread;

    std::vector<Projector *> _myProjectors;
    bool                _myPowerUpProjectorsOnStartup;

    void                checkForReboot();
    void                checkForHalt();
    void                continuousStatusReport(std::string theStateMsg);
    long                _myRebootTimeInSecondsToday;
    long                _myHaltTimeInSecondsToday;
    bool                _myRestartAppFlag;
};
