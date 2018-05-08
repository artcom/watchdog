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


#ifndef INCL_APPLICATION
#define INCL_APPLICATION

#include "system_functions.h"

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
const char* const RECEIVED_RESTART_APP_STRING = "Received restart command.";

class Application {
    public:
        Application(Logger & theLogger);
        virtual ~Application();

        bool setup(const dom::NodePtr & theAppNode, const std::string & theDirectory = "");
        bool checkForRestart( std::string & myRestartMessage );
        void launch();
        void checkHeartbeat();
        void checkState();
        void restart();
        void switchApplication(std::string);
        void terminate(const std::string & theReason, bool theWMCloseAllowed);
        std::string runUntilNextCheck(int theWatchFrequency);

        unsigned getRestartDelay() const;
        unsigned getStartDelay() const;
        bool     paused() const;
        bool     performECG() const;
        bool     restartedToday() const;
        std::string getHeartbeatFile() const;
        long     getRestartTimeInSecondsToday() const;
        ProcessResult    getProcessResult() const;
        std::string getFilename() const;
        std::string getArguments() const;

        void setPaused(bool thePausedFlag);
        void setRestartedToday(bool theRestartedTodayFlag);

        void setupEnvironment(const dom::NodePtr & theEnvironmentSettings);
        long getRuntime();

    private:
        void setEnvironmentVariables();

        std::map<std::string, std::string> _myEnvironmentVariables;

        std::string      _myApplicationWatchdogDirectory;
        std::string      _myFileName;
        std::string      _myAppLogFile;
        std::string      _myAppLogFileFormatString;
        bool             _myAppLogFileFormatter;
        std::string      _myWorkingDirectory;
        std::vector<std::string> _myArguments;
        std::string      _myWindowTitle;
#ifdef WIN32
        std::string      _myShowWindowMode;
#endif
        long             _myAppStartTimeInSeconds;

        asl::ThreadLock  _myLock;
        bool             _myRecvRestart;

        int              _myAllowMissingHeartbeats;
        int              _myHeartbeatFrequency;
        std::string      _myHeartbeatFile;
        bool             _myPerformECG;

        unsigned int     _myRestartMemoryThreshold;

        std::string      _myRestartDay;
        long             _myRestartTimeInSecondsToday;

        long             _myCheckMemoryTimeInSecondsToday;
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
