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
//   $RCSfile: Application.h,v $
//   $Author: ulrich $
//   $Revision: 1.8 $
//   $Date: 2005/04/07 12:27:16 $
//
//
//  Description: A simple application class representing a watched process
//
//
//=============================================================================

//
// watchdog.h :
//
#ifndef INCL_APPLICATION
#define INCL_APPLICATION

#include "system_functions.h"

#include <asl/dom/Nodes.h>

#include <map>
#include <string>
#include <vector>

#include <asl/dom/Nodes.h>
#include <asl/base/ThreadLock.h>

long getElapsedSecondsToday();

class Logger;

enum RestartMode {
    UNSET        = 0,
    MEMTHRESHOLD = 2,
    RESTARTDAY   = 4,
    RESTARTTIME  = 8,
    CHECKMEMORYTIME = 16,
    CHECKTIMEDMEMORYTHRESHOLD = 32
};

class Application {
    public:
        Application(Logger & theLogger);
        virtual ~Application();

        bool setup(const dom::NodePtr & theAppNode);
        bool checkForRestart( std::string & myRestartMessage );
        void launch();
        void checkHeartbeat();
        void checkState();
        void restart();
        void terminate(const std::string & theReason, bool theWMCloseAllowed);
        std::string runUntilNextCheck(int theWatchFrequency);
       
        unsigned getRestartDelay() const;
        unsigned getStartDelay() const;
        bool     paused() const;
        bool     performECG() const;
        bool     restartedToday() const;
        std::string getHeartbeatFile() const;
        time_t     getRestartTimeInSecondsToday() const;
        ProcessResult    getProcessResult() const;
        std::string getFilename() const;
        std::string getArguments() const; 
        
        void setPaused(bool thePausedFlag);
        void setRestartedToday(bool theRestartedTodayFlag);       

        void setupEnvironment(const dom::NodePtr & theEnvironmentSettings);


    private:
        void setEnvironmentVariables();

        std::map<std::string, std::string> _myEnvironmentVariables;

        std::string      _myFileName;
        std::string      _myWorkingDirectory;            
        std::vector<std::string> _myArguments;
        std::string      _myWindowTitle;
        long             _myAppStartTimeInSeconds;

        asl::ThreadLock  _myLock;
        bool             _myRecvRestart;

        int              _myAllowMissingHeartbeats;
        int              _myHeartbeatFrequency;
        std::string      _myHeartbeatFile;
        bool             _myPerformECG;

        unsigned int     _myRestartMemoryThreshold;

        std::string      _myRestartDay;
        time_t           _myRestartTimeInSecondsToday;

        time_t           _myCheckMemoryTimeInSecondsToday;
        unsigned int     _myMemoryThresholdTimed;

        // state
        int              _myRestartMode;
        bool             _myRestartCheck;
        bool             _myApplicationPaused;
        bool             _myCheckedMemoryToday;
        bool             _myRestartedToday;
        bool             _myMemoryIsFull;
        bool             _myItIsTimeToRestart;
        bool             _myHeartIsBroken;
        long             _myStartTimeInSeconds;
        ProcessInfo      _myProcessInfo;
        ProcessResult    _myProcessResult;
        std::string      _myLastWeekday;
        bool             _myDayChanged;
        Logger &         _myLogger;
        
        unsigned         _myStartDelay;
        unsigned         _myRestartDelay;
        unsigned         _myStartupCount;

};
#endif // INCL_APPLICATION
