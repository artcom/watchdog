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


#ifndef INCL_UDPCOMMANDLISTENERTHREAD_H
#define INCL_UDPCOMMANDLISTENERTHREAD_H


class Projector;

#include <asl/base/PosixThread.h>
#include <asl/dom/Nodes.h>

#include "Logger.h"

#include <string>
#include <vector>

class Application;
class UDPCommandListenerThread : public asl::PosixThread {
    public:
        UDPCommandListenerThread(std::vector<Projector *> theProjectors,
                                 Application & theApplication,
                                 const dom::NodePtr & theConfigNode,
                                 Logger & theLogger, 
                                 std::string & theShutdownCommand);
        virtual ~UDPCommandListenerThread();

        /*void setSystemHaltCommand(const std::string & theSystemhaltCommand);
        void setRestartAppCommand(const std::string & theRestartAppCommand);
        void setSystemRebootCommand(const std::string & theSystemRebootCommand);
        void setStopAppCommand(const std::string & theStopAppCommand);
        void setStartAppCommand(const std::string & theStartAppCommand);*/
    private:
        void run();
        bool allowedIp(asl::Unsigned32 theHostAddress);
        bool controlProjector(const std::string & theCommand);

        void initiateShutdown();
        void initiateReboot();

        std::vector<Projector*> _myProjectors;
        int                     _myUDPPort;
        Application &           _myApplication;
        Logger &                _myLogger;
        bool                    _myPowerDownProjectorsOnHalt;
        bool                    _myShutterCloseProjectorsOnStop;
        bool                    _myShutterCloseProjectorsOnReboot;
        std::string             _mySystemHaltCommand;
        std::string             _myRestartAppCommand;
        std::string             _mySystemRebootCommand;
        std::string             _myStopAppCommand;
        std::string             _myStartAppCommand;

        // status report
        std::string             _myStatusReportCommand;
        unsigned int            _myStatusLoadingDelay;
        
        std::string             _myShutdownCommand;
        std::vector<asl::Unsigned32> _myAllowedIps;
};

#endif
