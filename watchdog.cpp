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
//   $RCSfile: watchdog.cpp,v $
//   $Author: ulrich $
//   $Revision$
//   $Date: 2005/04/19 10:02:40 $
//
//
//  Description: The watchdog restarts the application, if it is closed
//               manually or by accident
//
//
//=============================================================================

#include "watchdog.h"
#include "system_functions.h"
#include "Projector.h"

#include <asl/dom/Nodes.h>
#include <asl/base/string_functions.h>
#include <asl/base/file_functions.h>
#include <asl/base/os_functions.h>
#include <asl/base/Exception.h>
#include <asl/base/Arguments.h>
#include <asl/base/Time.h>
#ifndef WIN32
#   include <asl/base/signal_functions.h>
#endif


#ifdef WIN32
#include <windows.h>
#endif

#include <time.h>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

const string ourDefaultConfigFile = "watchdog.xml";

asl::Arguments ourArguments;
const asl::Arguments::AllowedOptionWithDocumentation ourAllowedOptions[] = {
    {"--configfile", "weatchdog.xml", "XML configuration file"},
    {"--no_restart", "", "start only once, do not restart"},
    {"--help", "", "print help"},    
    {"", ""}
};

WatchDog::WatchDog()
    : _myWatchFrequency(30),
      _myAppToWatch(_myLogger),
      _myUDPCommandListenerThread(0),
      _myPowerUpProjectorsOnStartup(true),
      _myRebootTimeInSecondsToday(-1),
      _myHaltTimeInSecondsToday(-1),
      _myRestartAppFlag(true)
{
}

void
WatchDog::arm() {
#ifdef WIN32
    // allow the foreground window to be set instead of blinking in the taskbar
    // see q97925
    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (LPVOID)0, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
#endif

    if (_myProjectors.size() && _myPowerUpProjectorsOnStartup) {
        cerr << "Watchdog - Powering up projectors..." << endl;
        for (unsigned i = 0; i < _myProjectors.size(); ++i) {
            _myProjectors[i]->powerUp();
        }

        cerr << "Watchdog - Setting all projectors' inputs" << endl;
        for (unsigned i = 0; i < _myProjectors.size(); ++i) {
            // set projector's input to the configured one
            _myProjectors[i]->selectInput();
        }
    }

    if (_myAppToWatch.performECG()) {
        _myLogger.logToFile(std::string("Monitoring heartbeat file: ") + _myAppToWatch.getHeartbeatFile());
    }

    if (getElapsedSecondsToday() > _myAppToWatch.getRestartTimeInSecondsToday()) {
        _myAppToWatch.setRestartedToday(true);
    }
}

bool
WatchDog::watch() {
    try {
        // Run UDP command listener thread
        if (_myUDPCommandListenerThread) {
            cerr << "Watchdog - Starting udp command listener thread" << endl;
            _myUDPCommandListenerThread->fork();
            asl::msleep(100);
        }

        unsigned myStartDelay = _myAppToWatch.getStartDelay();
        if (myStartDelay > 0) {
            cerr << "Watchdog - Application will start in " << myStartDelay << " seconds." << endl;
            for (unsigned i = 0; i < myStartDelay * 2; ++i) {
                cerr << ".";
                asl::msleep(500);
            }
        }

        // Main loop
        for (;;) {
            std::string myReturnString;
            if (!_myAppToWatch.paused()) {
                myReturnString = "Internal quit.";
                _myLogger.logToFile( "Restarting application." );

                _myAppToWatch.launch();
            } else {
                myReturnString = "Application paused.";
            }

            std::string myRestartMessage = "Application shutdown";

            while (!_myAppToWatch.paused() && _myAppToWatch.getProcessResult() == PR_RUNNING) {
                // update projector state
                for (unsigned i = 0; i < _myProjectors.size(); ++i) {
                    _myProjectors[i]->update();
                }

                // watch application
                myReturnString = _myAppToWatch.runUntilNextCheck(_myWatchFrequency);
                _myAppToWatch.checkHeartbeat();
                _myAppToWatch.checkState();
                // system halt & reboot
                checkForHalt();
                checkForReboot();

                if (_myAppToWatch.checkForRestart(myRestartMessage)) {
                    break;
                }

            }

            _myAppToWatch.terminate(myRestartMessage, false);

            _myLogger.logToFile(_myAppToWatch.getFilename() + string(" exited: ") + myReturnString);

            if (!_myRestartAppFlag) {
                _myLogger.logToFile(string("watchdog will stop working now "));
                exit(0);
            }
            unsigned myRestartDelay = _myAppToWatch.getRestartDelay();

            if (!_myAppToWatch.paused()) {

                cerr << "Watchdog - Restarting application in " << myRestartDelay << " seconds" << endl;
                for (unsigned i = 0; i < myRestartDelay * 2; ++i) {
                    cerr << ".";
                    asl::msleep(500);
                }

                cerr << endl;
            } else {
                cerr << "Watchdog - Application is currently paused" << endl;
                while (_myAppToWatch.paused()) {
                    asl::msleep(1000);
                }
            }
        }
    } catch (const asl::Exception & ex) {
        cerr << "### Exception: " << ex << endl;
        _myLogger.logToFile(string("### Exception: " + ex.what()));
    } catch (...) {
        cerr << "### Error while starting:\n\n" + _myAppToWatch.getFilename() + " " + _myAppToWatch.getArguments() << endl;
        _myLogger.logToFile(string("### Error while starting:\n\n" + _myAppToWatch.getFilename() + " " + _myAppToWatch.getArguments()));
        exit(-1);
    }

    return false;
}

void
WatchDog::checkForReboot() {
    long myElapsedSecondsToday = getElapsedSecondsToday();
    if ((_myRebootTimeInSecondsToday!= -1) && (_myRebootTimeInSecondsToday< myElapsedSecondsToday) ) {
        if (myElapsedSecondsToday - _myRebootTimeInSecondsToday < (_myWatchFrequency * 3)) {
            initiateSystemReboot();
        }
    }
}

void
WatchDog::checkForHalt() {
    long myElapsedSecondsToday = getElapsedSecondsToday();
    if ((_myHaltTimeInSecondsToday!= -1) && (_myHaltTimeInSecondsToday< myElapsedSecondsToday) ) {
        if (myElapsedSecondsToday - _myHaltTimeInSecondsToday < (_myWatchFrequency * 3)) {
            initiateSystemShutdown();
        }
    }
}


bool
WatchDog::init(dom::Document & theConfigDoc, bool theRestartAppFlag) {
    _myRestartAppFlag = theRestartAppFlag;
    try {
        if (theConfigDoc("WatchdogConfig")) {
            const dom::NodePtr & myConfigNode = theConfigDoc.childNode("WatchdogConfig");

            // Setup logfile
            {
                _myLogFilename = asl::expandEnvironment(myConfigNode->getAttribute("logfile")->nodeValue());
                std::string::size_type myDotPos = _myLogFilename.rfind(".", _myLogFilename.size());
                if (myDotPos == std::string::npos) {
                    myDotPos = _myLogFilename.size();
                }
#if defined(LINUX) || defined(OSX)
                time_t ltime = time(NULL);
                struct tm* today = localtime(&ltime);
#else
                __time64_t ltime;
                _time64( &ltime );
                struct tm *today = _localtime64( &ltime );
#endif
                char myTmpBuf[128];
                strftime(myTmpBuf, sizeof(myTmpBuf), "%Y_%m_%d_%H_%M", today);
                _myLogFilename = _myLogFilename.substr(0, myDotPos) + "_" + myTmpBuf + _myLogFilename.substr(myDotPos, _myLogFilename.size());
                AC_DEBUG <<"_myLogFilename: " << _myLogFilename;
                _myLogger.openLogFile(_myLogFilename);
            }

            // Setup watch frequency
            {
                _myWatchFrequency = asl::as<int>(myConfigNode->getAttribute("watchFrequency")->nodeValue());
                AC_DEBUG << "_myWatchFrequency: " << _myWatchFrequency ;
                if (_myWatchFrequency < 1){
                    cerr <<"### ERROR: WatchFrequency must have a value greater then 0 sec." << endl;
                    return false;
                }
            }

            // Setup UDP control
            if (myConfigNode->childNode("UdpControl")) {
                const dom::NodePtr & myUdpControlNode = myConfigNode->childNode("UdpControl");
                // Setup projector control
                if (myUdpControlNode->childNode("ProjectorControl")) {
                    const dom::NodePtr & myProjectorsNode = myUdpControlNode->childNode("ProjectorControl");
                    _myPowerUpProjectorsOnStartup = asl::as<bool>(myProjectorsNode->getAttribute("powerUpOnStartup")->nodeValue());
                    for (unsigned i = 0; i < myProjectorsNode->childNodesLength(); ++i) {
                        Projector* myProjector = Projector::getProjector(myProjectorsNode->childNode(i), &_myLogger);
                        if (myProjector) {
                            _myProjectors.push_back(myProjector);
                        }
                    }
                    AC_DEBUG <<"Found " << _myProjectors.size() << " projectors";
                }

                _myUDPCommandListenerThread = new UDPCommandListenerThread(_myProjectors, _myAppToWatch, myUdpControlNode, _myLogger);
            }

            // check for system reboot time command configuration
            if (myConfigNode->childNode("RebootTime")) {
                std::string myRebootTime = (*myConfigNode->childNode("RebootTime"))("#text").nodeValue();
                std::string myHours = myRebootTime.substr(0, myRebootTime.find_first_of(':'));
                std::string myMinutes = myRebootTime.substr(myRebootTime.find_first_of(':')+1, myRebootTime.length());
                _myRebootTimeInSecondsToday = atoi(myHours.c_str()) * 3600;
                _myRebootTimeInSecondsToday += atoi(myMinutes.c_str()) * 60;
                AC_DEBUG <<"_myRebootTimeInSecondsToday : " << _myRebootTimeInSecondsToday;
            }

            // check for system halt time command configuration
            if (myConfigNode->childNode("HaltTime")) {
                std::string myHaltTime = (*myConfigNode->childNode("HaltTime"))("#text").nodeValue();
                std::string myHours = myHaltTime.substr(0, myHaltTime.find_first_of(':'));
                std::string myMinutes = myHaltTime.substr(myHaltTime.find_first_of(':')+1, myHaltTime.length());
                _myHaltTimeInSecondsToday = atoi(myHours.c_str()) * 3600;
                _myHaltTimeInSecondsToday += atoi(myMinutes.c_str()) * 60;
                AC_DEBUG <<"_myHaltTimeInSecondsToday : " << _myHaltTimeInSecondsToday;
            }

            // Setup application
            if (myConfigNode->childNode("Application")) {
                const dom::NodePtr & myApplicationNode = myConfigNode->childNode("Application");

                if (!_myAppToWatch.setup(myConfigNode->childNode("Application"))) {
                    return false;
                }


                // WaitingScreen setup
                if (myApplicationNode->childNode("WaitingScreenImage")) {
                    std::string myWaitingScreenPath = asl::expandEnvironment((*myApplicationNode->childNode("WaitingScreenImage"))("#text").nodeValue());
                    int myWaitingScreenPosX = 0;
                    int myWaitingScreenPosY = 0;

                    if (myApplicationNode->childNode("WaitingScreenPosX")) {
                        int myValue = atoi(((*myApplicationNode->childNode("WaitingScreenPosX"))("#text").nodeValue()).c_str());
                        myWaitingScreenPosX = myValue;
                    }

                    if (myApplicationNode->childNode("WaitingScreenPosY")) {
                        int myValue = atoi(((*myApplicationNode->childNode("WaitingScreenPosY"))("#text").nodeValue()).c_str());
                        myWaitingScreenPosY = myValue;
                    }
                }
            }
        }

    } catch (const asl::Exception & ex) {
        cerr << "### Exception: " << ex;
    } catch (...) {
        cerr << "Error, while parsing xml config file" << endl;
        exit(-1);
    }
    return true;
}

void
printUsage() {
    cerr << ourArguments.getProgramName() << " Copyright (C) 2003-2005 ART+COM" << endl;
    ourArguments.printUsage();
    cerr << "Default configfile: " << ourDefaultConfigFile << endl;
}

void
readConfigFile(dom::Document & theConfigDoc,  std::string theFileName) {
    AC_DEBUG << "Loading configuration data..." ;
    std::string myFileStr = asl::readFile(theFileName);
    if (myFileStr.empty()) {
        cerr << "Watchdog::readConfigFile: Can't open configuration file "
             << theFileName << "." << endl;
        exit(-1);
    }
    theConfigDoc.parseAll(myFileStr.c_str());
    if (!theConfigDoc) {
        cerr << "Watchdog:::readConfigFile: Error reading configuration file "
             << theFileName << "." << endl;
        exit(-1);
    }
}

int
main(int argc, char* argv[] ) {
#ifdef WIN32
    // This will turn off uncaught exception handling
    SetErrorMode(SEM_NOGPFAULTERRORBOX);
#endif

    ourArguments.addAllowedOptionsWithDocumentation(ourAllowedOptions);
    if (!ourArguments.parse(argc, argv)) {
        return 0;
    }
    bool myRestartAppFlag = true;
    dom::Document myConfigDoc;
    if (ourArguments.haveOption("--help")) {
            printUsage();
            return -1;
    }
    if (ourArguments.haveOption("--no_restart")) {
        myRestartAppFlag = false;
    }
    if (ourArguments.haveOption("--configfile")) {
        readConfigFile (myConfigDoc, ourArguments.getOptionArgument("--configfile"));
    } else {
        if (asl::fileExists(ourDefaultConfigFile)) {
            readConfigFile (myConfigDoc, ourDefaultConfigFile);
        } else {
            printUsage();
            return -1;
        }
    }

    asl::Exception::initExceptionBehaviour();
#ifndef WIN32
    asl::initSignalHandling();
#endif

    WatchDog myHasso;
    bool mySuccess = myHasso.init(myConfigDoc, myRestartAppFlag);

    if (mySuccess) {
        myHasso.arm();
        myHasso.watch();
    }
    exit(-1);
}
