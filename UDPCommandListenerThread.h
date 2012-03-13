/* __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2008, ART+COM AG Berlin, Germany <www.artcom.de>
//
// These coded instructions, statements, and computer programs contain
// proprietary information of ART+COM AG Berlin, and are copy protected
// by law. They may be used, modified and redistributed under the terms
// of GNU General Public License referenced below.
//
// Alternative licensing without the obligations of the GPL is
// available upon request.
//
// GPL v3 Licensing:
//
// This file is part of the ART+COM Y60 Platform.
//
// ART+COM Y60 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// ART+COM Y60 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with ART+COM Y60.  If not, see <http://www.gnu.org/licenses/>.
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Description: TODO
//
// Last Review: NEVER, NOONE
//
//  review status report: (perfect, ok, fair, poor, disaster, notapplicable, unknown)
//    usefullness            : unknown
//    formatting             : unknown
//    documentation          : unknown
//    test coverage          : unknown
//    names                  : unknown
//    style guide conformance: unknown
//    technical soundness    : unknown
//    dead code              : unknown
//    readability            : unknown
//    understandabilty       : unknown
//    interfaces             : unknown
//    confidence             : unknown
//    integration            : unknown
//    dependencies           : unknown
//    cheesyness             : unknown
//
//    overall review status  : unknown
//
//    recommendations:
//       - unknown
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
*/

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
