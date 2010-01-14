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
//
//   $RCSfile: watchdog.h,v $
//   $Author: valentin $
//   $Revision: 1.12 $
//   $Date: 2005/03/29 14:16:43 $
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
    bool init(dom::Document & theConfigDoc);
    void arm();
    bool watch();

private:
    Logger              _myLogger;
    std::string         _myLogFilename;
    int                 _myWatchFrequency;

    Application         _myAppToWatch;

    UDPCommandListenerThread * _myUDPCommandListenerThread;

    std::vector<Projector *> _myProjectors;
    bool                _myPowerUpProjectorsOnStartup;

    void                checkForReboot();
    void                checkForHalt();

    long                _myRebootTimeInSecondsToday;
    long                _myHaltTimeInSecondsToday;
    bool                _myApplicationPaused;
};
