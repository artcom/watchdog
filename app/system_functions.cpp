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
//   $RCSfile: system_functions.cpp,v $
//   $Author: ulrich $
//   $Revision: 1.3 $
//   $Date: 2005/04/06 11:14:53 $
//
//
//  Description: The watchdog restarts the application, if it is closed
//               manually or by accident
//
//
//=============================================================================

#include "system_functions.h"
#include <asl/base/Time.h>

#ifdef WIN32
#   include <windows.h>
#elif defined(LINUX) || defined(OSX)
#   include <sys/wait.h>
#   include <errno.h>
#   include <unistd.h>
#   include <string.h>
#else
#   error Your platform is missing!
#endif

#ifdef LINUX
#   include <linux/reboot.h>
#   include <sys/reboot.h>
#endif

#include <iostream>
#include <sstream>
#include <cstdlib>

/*#include <time.h>
#include <fstream>
#include <algorithm>
*/
using namespace std;


void initiateSystemReboot() {
#ifdef WIN32
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // Get a token for this process.
    BOOL ok = OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    if (!ok) {
        dumpLastError ("OpenProcessToken");
    }

    // Get the LUID for the shutdown privilege.
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
        &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get the shutdown privilege for this process.
    ok = AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
            (PTOKEN_PRIVILEGES)NULL, 0);
    if (!ok) {
        dumpLastError ("AdjustTokenPrivileges");
    }

    // Rumms.
    // Note that EWX_FORCEIFHUNG doesn't seem to work (Win 2000), so
    // we kill everything using EWX_FORCE.
    ok = ExitWindowsEx(EWX_REBOOT | EWX_FORCE,
            SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER |
            SHTDN_REASON_FLAG_PLANNED);
    if (!ok) {
        dumpLastError("ExitWindowsEx");
    }
#elif defined(LINUX) || defined(OSX)
    // Although this is not the recommended way to programmatically reboot OSX
    // it seems to work. Because we never deployed on OSX I'll leave it with that.
    // The official way to reboot OS X is documented here:
    // http://developer.apple.com/qa/qa2001/qa1134.html
    // google keywords: "programmatically reboot mac os x"
    // [DS]
    int myResult = system("/sbin/reboot");
    if (myResult == -1) {
        dumpLastError("reboot");
    }
#else
#error Your platform is missing!
#endif
}

void initiateSystemShutdown() {
#ifdef WIN32
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // Get a token for this process.
    BOOL ok = OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    if (!ok) {
        dumpLastError ("OpenProcessToken");
    }

    // Get the LUID for the shutdown privilege.
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
        &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get the shutdown privilege for this process.
    ok = AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
            (PTOKEN_PRIVILEGES)NULL, 0);
    if (!ok) {
        dumpLastError ("AdjustTokenPrivileges");
    }

    // Rumms.
    // Note that EWX_FORCEIFHUNG doesn't seem to work (Win 2000), so
    // we kill everything using EWX_FORCE.
    ok = ExitWindowsEx(EWX_POWEROFF | EWX_FORCE,
            SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER |
            SHTDN_REASON_FLAG_PLANNED);
    if (!ok) {
        dumpLastError("ExitWindowsEx");
    }
#elif defined(LINUX) || defined(OSX)
    // See comment in initiateSystemReboot() [DS]
    int myResult = system("/sbin/halt");
    if (myResult == -1) {
        dumpLastError("reboot");
    }

#else
#error Your platform is missing!
#endif
}

ProcessResult waitForApp( const ProcessInfo & theProcessInfo, int theTimeout, Logger & theLogger ) {
#ifdef WIN32
    return WaitForSingleObject( theProcessInfo.hProcess, theTimeout );
#elif defined(LINUX) || defined(OSX)
    asl::msleep(theTimeout);
    int myStatus;
    pid_t myResult = waitpid( theProcessInfo, &myStatus, WNOHANG );
    if (myResult == -1) {
        if (getLastErrorNumber() == ECHILD ) {
            return PR_TERMINATED;
        } else {
            return PR_FAILED;
        }
    }
    if (myResult == theProcessInfo) {
        std::ostringstream myOss;
        if (WIFEXITED(myStatus)) {
            myOss << "Process exited with status " << WEXITSTATUS(myStatus)
	    	  << ".";
            theLogger.logToFile(myOss.str());
            return PR_TERMINATED;
        } else if (WIFSIGNALED(myStatus)) {
	        int mySignal = WTERMSIG(myStatus);
            myOss << "Process terminated with signal " << mySignal
	    	  << " (" << strsignal(mySignal) << ").";
            theLogger.logToFile(myOss.str());
            return PR_TERMINATED;
        }
    }
    return PR_RUNNING;
#else
#error Your platform is missing!
#endif
}

bool launchApp( const std::string & theFileName,
                const std::vector<std::string> & theArguments,
                const std::string & theWorkingDirectory,
                ProcessInfo & theProcessInfo)
{
#ifdef WIN32
    STARTUPINFO StartupInfo = {
        sizeof(STARTUPINFO),
        NULL, NULL, NULL, 0, 0, 0, 0, 0, 0,
        0, STARTF_USESHOWWINDOW, SW_SHOWDEFAULT,
        0, NULL, NULL, NULL, NULL
    };

    std::string myCommandLine = theFileName;
    for (std::vector<std::string>::size_type i = 0; i < theArguments.size(); i++) {
        myCommandLine += " ";
        myCommandLine += theArguments[i];
    }

    return 0 != CreateProcess(NULL, &(myCommandLine[0]),
                              NULL, NULL, TRUE, 0,
                              NULL,
                              theWorkingDirectory.empty() ? NULL : theWorkingDirectory.c_str(),
                              &StartupInfo,
                              &theProcessInfo);


#elif defined(LINUX) || defined(OSX)
    theProcessInfo = fork();
    if (theProcessInfo == -1) {
        return false;
    }
    if (theProcessInfo != 0) {
        return true;
    }
    std::vector<char*> myArgv;
    std::cerr << "Starting application " << theFileName << " ";
    myArgv.push_back( const_cast<char*>(theFileName.c_str()) );
    for (std::vector<std::string>::size_type i = 0; i < theArguments.size(); i++) {
        myArgv.push_back( const_cast<char*>(theArguments[i].c_str()) );
        std::cerr << theArguments[i] << " ";
    }
    if ( ! theWorkingDirectory.empty() ) {
        std::cerr << "in directory: '" << theWorkingDirectory << "'";
    }
    myArgv.push_back(NULL);
    std::cerr << std::endl;
    if ( ! theWorkingDirectory.empty() ) {
        if (chdir( theWorkingDirectory.c_str() ) < 0) {
            dumpLastError( std::string("chdir('") + theWorkingDirectory + "')" );
            std::abort();
        }
    }
    execvp( theFileName.c_str(), &myArgv[0] );
    dumpLastError(theFileName);
    std::abort();
#else
#error Your platform is missing!
#endif
}

void closeApp( const std::string & theWindowTitle, const ProcessInfo & theProcessInfo,
               Logger & theLogger)
{
#ifdef WIN32
    // Send WM_CLOSE to window
    if (!theWindowTitle.empty()) {
        HWND myHWnd = FindWindow (0, theWindowTitle.c_str());
        if (!myHWnd) {
            theLogger.logToFile(string("Could not find Window ") + theWindowTitle);
        } else {
            // Find process handle for window
            DWORD myWindowProcessId;
            /*DWORD myWindowThreadId =*/ GetWindowThreadProcessId(myHWnd, &myWindowProcessId);

            SendMessage(myHWnd, WM_CLOSE, 0, 0);
            theLogger.logToFile("WM_CLOSE sent.");
            asl::msleep(500); // Give it some time to work...

            // Terminate process
            HANDLE myWindowProcessHandle = OpenProcess(0, false, myWindowProcessId);
            if (myWindowProcessHandle) {
                TerminateProcess(myWindowProcessHandle, 0);
                CloseHandle(myWindowProcessHandle);
            }
        }
    }
    else {
        // Terminate process
        theLogger.logToFile("Try to terminate application.");
        TerminateProcess(theProcessInfo.hProcess, 0);
        theLogger.logToFile("O.k., terminated.");

        // Close process and thread handles.
        CloseHandle( theProcessInfo.hProcess );
        CloseHandle( theProcessInfo.hThread );
    }
#elif defined(LINUX) || defined(OSX)
    theLogger.logToFile("Try to terminate application.");
    kill( theProcessInfo, SIGTERM );
    ProcessResult myResult = waitForApp( theProcessInfo, 500, theLogger );
    switch (myResult) {
        case PR_RUNNING:
            theLogger.logToFile("Sending SIGKILL...");
            kill( theProcessInfo, SIGKILL );
            break;
        case PR_TERMINATED:
            theLogger.logToFile("O.k., terminated.");
            break;
        default:
            theLogger.logToFile("Error waiting for process: " + getLastError());
            std::abort();
            break;
    }
#else
#error Your platform is missing!
#endif
}

ErrorNumber getLastErrorNumber() {
#ifdef WIN32
    return GetLastError();
#elif defined(LINUX) || defined(OSX)
    return errno;
#else
#error Your platform is missing!
#endif
}


std::string getLastError( ErrorNumber theErrorNumber) {
#ifdef WIN32
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            theErrorNumber,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0,
            NULL
        );
        std::string myResult((LPCSTR)lpMsgBuf);
        LocalFree(lpMsgBuf);
        return myResult;
#elif defined(LINUX) || defined(OSX)
        return strerror( theErrorNumber );
#else
#error Your platform is missing!
#endif
}


std::string getLastError() {
    return getLastError( getLastErrorNumber() );
}


void
dumpLastError(const string& theErrorLocation) {
    ErrorNumber myErrorNumber = getLastErrorNumber();
    cerr << "Warning: \"" << theErrorLocation << "\" failed.\n";
    cerr << "         Error was \"" << getLastError( myErrorNumber )
         << "\" with code : " << myErrorNumber << endl;
}
