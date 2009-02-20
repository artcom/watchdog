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
//   $RCSfile: system_functions.h,v $
//   $Author: martin $
//   $Revision: 1.3 $
//   $Date: 2005/04/27 12:42:06 $
//
//
//  Description: The watchdog restarts the application, if it is closed
//               manually or by accident
//
//
//=============================================================================

#ifndef INCL_SYSTEM_FUNCTIONS
#define INCL_SYSTEM_FUNCTIONS

#include <string>
#include <vector>

#include "asl/base/proc_functions.h"

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
                ProcessInfo & theProcessInfo);
void closeApp( const std::string & theWindowTitle, const ProcessInfo & theProcessInfo,
               Logger & theLogger); 
ErrorNumber getLastErrorNumber();
std::string getLastError();
std::string getLastError( ErrorNumber theErrorNumber);
void dumpLastError(const std::string& theErrorLocation);

#endif

