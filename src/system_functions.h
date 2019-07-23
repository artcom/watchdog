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


#ifndef INCL_SYSTEM_FUNCTIONS
#define INCL_SYSTEM_FUNCTIONS

#include <string>
#include <vector>

#include <asl/base/proc_functions.h>

#include "Logger.h"

#ifdef WIN32
#   include <windows.h>
#elif defined(LINUX) || defined(OSX)
#   include <sys/types.h>
#   include <unistd.h>
#else
#error your platform is missing!
#endif

typedef asl::ProcessID ProcessResult;

#ifdef WIN32
    enum {
        PR_RUNNING = WAIT_TIMEOUT,
        PR_TERMINATED = WAIT_OBJECT_0,
        PR_FAILED = WAIT_FAILED
    };
    typedef PROCESS_INFORMATION ProcessInfo;
    typedef DWORD ErrorNumber;

    //showWindow options
    const std::string MINIMIZED = "minimized";

#elif defined(LINUX) || defined(OSX)
    enum {
        PR_RUNNING,
        PR_TERMINATED,
        PR_FAILED
    };
    typedef int ProcessInfo;
    typedef int ErrorNumber;
#else
#error your platform is missing!
#endif


void initiateSystemReboot();
void initiateSystemShutdown();
ProcessResult waitForApp( const ProcessInfo & theProcessInfo, int theTimeout, Logger & theLogger );
bool launchApp( const std::string & theFileName,
                const std::vector<std::string> & theArguments,
                const std::string & theWorkingDirectory,
                const std::string & theAppLogFile,
#ifdef WIN32
                const std::string & theShowWindowMode,
#endif
                ProcessInfo & theProcessInfo);
ProcessResult closeApp( const std::string & theWindowTitle, const ProcessInfo & theProcessInfo,
               Logger & theLogger);
ErrorNumber getLastErrorNumber();
std::string getLastError();
std::string getLastError( ErrorNumber theErrorNumber);
void dumpLastError(const std::string& theErrorLocation);

#endif

