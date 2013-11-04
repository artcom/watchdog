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

#include "watchdog.h"

#include <typeinfo>

#ifdef WIN32
#include <windows.h>
#endif

#include <iostream>
#include <fstream>
#include <algorithm>


#include <asl/dom/Nodes.h>
#include <asl/base/buildinfo.h>
#include <asl/base/string_functions.h>
#include <asl/base/file_functions.h>
#include <asl/base/os_functions.h>
#include <asl/base/Exception.h>
#include <asl/base/Arguments.h>
#include <asl/base/Time.h>
#include <asl/base/settings.h>
#include <asl/net/UDPSocket.h>

#ifndef WIN32
#   include <asl/base/signal_functions.h>
#endif

#include "system_functions.h"
#include "Projector.h"
#include "parse_helpers.h"



using namespace std;
using namespace inet;
using namespace asl;

const asl::Unsigned16 MAX_PORT = 65535;


const string ourDefaultConfigFile = "watchdog.xml";

asl::Arguments ourArguments;
const asl::Arguments::AllowedOptionWithDocumentation ourAllowedOptions[] = {
    {"", "watchdog.xml", "XML configuration file"},
    {"--configfile", "watchdog.xml", "XML configuration file. DEPRECATED. Use argument to specify xml-file instead."},
    {"--no_restart", "", "start only once, do not restart"},
    {"--revisions", "", "show component revisions"},
    {"", ""}
};

WatchDog::WatchDog()
    : _myWatchFrequency(30),
      _myStartupCommand(""),
      _myShutdownCommand(""),
      _myApplicationTerminatedCommand(""),
      _myPostApplicationLaunchCommand(""),
      _myApplicationPreTerminateCommand(""),
      _myContinuousStateChangeIP(""),
      _myContinuousStateChangePort(-1),
      _myIgnoreTerminateCmdOnUdpCmd(false),
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

void
WatchDog::continuousStatusReport( std::string theStateMsg) {
    if (_myContinuousStateChangeIP != "" && _myContinuousStateChangePort != -1) {
        try {
            AC_DEBUG << "send State: " << theStateMsg;
            UDPSocket * myUDPClient = 0;
            Unsigned32 inHostAddress = getHostAddress(_myContinuousStateChangeIP.c_str());
            // try to find a free client port between _myContinuousStateChangePort+1 and MAX_PORT
            for (unsigned int clientPort = _myContinuousStateChangePort+1; clientPort <= MAX_PORT; clientPort++)
            {
                try {
                    myUDPClient = new UDPSocket(INADDR_ANY, clientPort);
                    break;
                }
                catch (SocketException & ) {
                    myUDPClient = 0;
                }
            }
            if (myUDPClient) {
                myUDPClient->sendTo(inHostAddress, _myContinuousStateChangePort, theStateMsg.c_str(), theStateMsg.size());
                delete myUDPClient;
            }
        }
        catch (Exception & ) {
            _myLogger.logToFile(std::string("Sorry, cannot establish socket connection to ip: '") + _myContinuousStateChangeIP + "'");            
        }
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

        if (_myStartupCommand != "") {
            int myError = system(_myStartupCommand.c_str());
            cerr << "startup command: \"" << _myStartupCommand << "\" return with " << myError << endl;
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
                continuousStatusReport("loading");
                _myAppToWatch.launch();

            } else {
                myReturnString = "Application paused.";
            }

            if (_myPostApplicationLaunchCommand != "") {
                _myLogger.logToFile(string("application is launched, execute additional command: '") + _myPostApplicationLaunchCommand + "'");
                int myError = system(_myPostApplicationLaunchCommand.c_str());
                _myLogger.logToFile(string("application is launched, execute additional command, returned with error: ") + asl::as_string(myError));
            }

            continuousStatusReport("runnning");
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

            if (_myApplicationPreTerminateCommand != "") {
                _myLogger.logToFile(string("application will be terminated, send additional pre_restart command: '") + _myApplicationPreTerminateCommand + "'");
                int myError = system(_myApplicationPreTerminateCommand.c_str());
                _myLogger.logToFile(string("application will be terminated, send additional pre_restart command, returned with error: ") + asl::as_string(myError));
            }
            _myAppToWatch.terminate(myRestartMessage, false);

            _myLogger.logToFile(_myAppToWatch.getFilename() + string(" exited: ") + myReturnString);

            if (!_myRestartAppFlag) {
                _myLogger.logToFile(string("watchdog will stop working now "));
                exit(0);
            }

            unsigned myRestartDelay = _myAppToWatch.getRestartDelay();

            if (!_myAppToWatch.paused()) {
                if (_myApplicationTerminatedCommand != "") {
                    if (!_myIgnoreTerminateCmdOnUdpCmd || myRestartMessage != RECEIVED_RESTART_APP_STRING) {
                        _myLogger.logToFile(string("application terminated, send additional restart command: '") + _myApplicationTerminatedCommand + "'");
                        int myError = system(_myApplicationTerminatedCommand.c_str());
                        _myLogger.logToFile(string("application terminated, send additional restart command, returned with error: ") + asl::as_string(myError));
                    }
                }

                cerr << "Watchdog - Restarting application in " << myRestartDelay << " seconds" << endl;
                continuousStatusReport("restarting");
                for (unsigned i = 0; i < myRestartDelay * 2; ++i) {
                    cerr << ".";
                    asl::msleep(500);
                }

                cerr << endl;
            } else {
                cerr << "Watchdog - Application is currently paused" << endl;
                continuousStatusReport("pausing");
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
            if (_myShutdownCommand != "") {
                int myError = system(_myShutdownCommand.c_str());
                cerr << "shutdown command: \"" << _myShutdownCommand << "\" return with " << myError << endl;
            }
            
            initiateSystemReboot();
        }
    }
}

void
WatchDog::checkForHalt() {
    long myElapsedSecondsToday = getElapsedSecondsToday();
    if ((_myHaltTimeInSecondsToday!= -1) && (_myHaltTimeInSecondsToday< myElapsedSecondsToday) ) {
        if (myElapsedSecondsToday - _myHaltTimeInSecondsToday < (_myWatchFrequency * 3)) {
            if (_myShutdownCommand != "") {
                int myError = system(_myShutdownCommand.c_str());
                cerr << "shutdown command: \"" << _myShutdownCommand << "\" return with " << myError << endl;
            }
            
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
                asl::Time now;
                now.toLocalTime();
                static const char * myFormatString("%Y-%M-%D-%h-%m");
                std::ostringstream myTimeString;
                myTimeString << asl::formatTime(myFormatString) << now;
                _myLogFilename = _myLogFilename.substr(0, myDotPos) + "_" + myTimeString.str() + _myLogFilename.substr(myDotPos, _myLogFilename.size());
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
            
            // check for additional startup command
            if (myConfigNode->childNode("PreStartupCommand")) {
                _myStartupCommand = (*myConfigNode->childNode("PreStartupCommand")).firstChild()->nodeValue();
                AC_DEBUG << "_myStartupCommand: " << _myStartupCommand;
            }

            // check for application post launch command
            if (myConfigNode->childNode("PostStartupCommand")) {
                _myPostApplicationLaunchCommand = (*myConfigNode->childNode("PostStartupCommand")).firstChild()->nodeValue();
                AC_DEBUG << "_myPostApplicationLaunchCommand: " << _myPostApplicationLaunchCommand;
            }
            
            // check for additional shutdown command
            if (myConfigNode->childNode("PreShutdownCommand")) {
                _myShutdownCommand = (*myConfigNode->childNode("PreShutdownCommand")).firstChild()->nodeValue();
                AC_DEBUG << "_myShutdownCommand: " << _myShutdownCommand;
            }

            // check for application terminate command
            if (myConfigNode->childNode("AppTerminateCommand")) {
                if (myConfigNode->childNode("AppTerminateCommand")->getAttribute("ignoreOnUdpRestart")) {
                    _myIgnoreTerminateCmdOnUdpCmd = asl::as<bool>(myConfigNode->childNode("AppTerminateCommand")->getAttribute("ignoreOnUdpRestart")->nodeValue());
                }
                _myApplicationTerminatedCommand = (*myConfigNode->childNode("AppTerminateCommand")).firstChild()->nodeValue();
                AC_DEBUG << "_myApplicationTerminatedCommand: " << _myApplicationTerminatedCommand;
            }

            
            // check for application pre terminate command
            if (myConfigNode->childNode("AppPreTerminateCommand")) {
                _myApplicationPreTerminateCommand = (*myConfigNode->childNode("AppPreTerminateCommand")).firstChild()->nodeValue();
                AC_DEBUG << "_myApplicationPreTerminateCommand: " << _myApplicationPreTerminateCommand;
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

                _myUDPCommandListenerThread = new UDPCommandListenerThread(_myProjectors, _myAppToWatch, 
                                                        myUdpControlNode, _myLogger, _myShutdownCommand);

                if (myUdpControlNode->childNode("ContinuousStatusChangeReport")) {
                    const dom::NodePtr & myContinuousStatusChangeNode = myUdpControlNode->childNode("ContinuousStatusChangeReport");
                    const dom::NodePtr & myIPAttribute = myContinuousStatusChangeNode->getAttribute("ip");
                    if (myIPAttribute) {
                        _myContinuousStateChangeIP = myIPAttribute->nodeValue();
                    }
                    const dom::NodePtr & myPortAttribute = myContinuousStatusChangeNode->getAttribute("port");
                    if (myPortAttribute) {
                        _myContinuousStateChangePort = asl::as<int>(myPortAttribute->nodeValue());
                    }
                    if (_myContinuousStateChangeIP != "" && _myContinuousStateChangePort != -1) {
                        AC_INFO << "Continuous state change will will be send to IP: '" << _myContinuousStateChangeIP << "' port :" << _myContinuousStateChangePort;
                    }
                }

            }

            // check for system reboot time command configuration
            if (myConfigNode->childNode("RebootTime")) {
                std::string myRebootTime = (*myConfigNode->childNode("RebootTime"))("#text").nodeValue();
                std::string myHours = myRebootTime.substr(0, myRebootTime.find_first_of(':'));
                std::string myMinutes = myRebootTime.substr(myRebootTime.find_first_of(':')+1, myRebootTime.length());
                _myRebootTimeInSecondsToday = asl::as<int>(myHours) * 3600;
                _myRebootTimeInSecondsToday += asl::as<int>(myMinutes) * 60;
                AC_DEBUG <<"_myRebootTimeInSecondsToday : " << _myRebootTimeInSecondsToday;
            }

            // check for system halt time command configuration
            if (myConfigNode->childNode("HaltTime")) {
                std::string myHaltTime = (*myConfigNode->childNode("HaltTime"))("#text").nodeValue();
                std::string myHours = myHaltTime.substr(0, myHaltTime.find_first_of(':'));
                std::string myMinutes = myHaltTime.substr(myHaltTime.find_first_of(':')+1, myHaltTime.length());
                _myHaltTimeInSecondsToday = asl::as<int>(myHours) * 3600;
                _myHaltTimeInSecondsToday += asl::as<int>(myMinutes) * 60;
                AC_DEBUG <<"_myHaltTimeInSecondsToday : " << _myHaltTimeInSecondsToday;
            }
            
            // Setup application
            if (myConfigNode->childNode("Application")) {
                const dom::NodePtr & myApplicationNode = myConfigNode->childNode("Application");

                if (!_myAppToWatch.setup(myApplicationNode)) {
                    return false;
                }
            } else if (myConfigNode->childNode("SwitchableApplications")) {
                const dom::NodePtr & mySwitchableApplicationsNode = myConfigNode->childNode("SwitchableApplications");

                std::string myDirectory = mySwitchableApplicationsNode->getAttribute("directory")->nodeValue();
                std::string myInitialApplication = mySwitchableApplicationsNode->getAttribute("initial")->nodeValue();

                dom::Document myApplicationConfigDoc;
                bool ok = readConfigFile(myApplicationConfigDoc, myDirectory + "/" + myInitialApplication + ".xml");
                if (!ok) {
                    AC_WARNING << "configuration file for id '" << myInitialApplication << "' could not be opened.";
                    return false;
                }

                if (!myApplicationConfigDoc("Application")) {
                    AC_WARNING << "application xml has no Application-node";
                    return false;
                }
                const dom::NodePtr & myApplicationNode = myApplicationConfigDoc.childNode("Application");

                if (!_myAppToWatch.setup(myApplicationNode, myDirectory)) {
                    return false;
                }
            } else {
                AC_WARNING << "xml document should have either Application or SwitchableApplications node";
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

int
main(int argc, char* argv[] ) {
#ifdef WIN32
    // This will turn off uncaught exception handling
    SetErrorMode(SEM_NOGPFAULTERRORBOX);
#endif

    ourArguments.addAllowedOptionsWithDocumentation(ourAllowedOptions);
    ourArguments.setShortDescription("Default configfile: " + ourDefaultConfigFile);
    
    if (!ourArguments.parse(argc, argv)) {
        return 0;
    }
    bool myRestartAppFlag = true;
    dom::Document myConfigDoc;
    if (ourArguments.haveOption("--revisions")) {
        using asl::build_information;
        std::cout << build_information::get();
    }

    if (ourArguments.haveOption("--no_restart")) {
        myRestartAppFlag = false;
    }

    if (ourArguments.getCount() > 0) {
        bool ok = readConfigFile (myConfigDoc, ourArguments.getArgument(0));
        if (!ok) {
            AC_ERROR << "given config file '" << ourArguments.getArgument(0) << "' could not be read. exit.";
        }


    //deprecated
    } else if (ourArguments.haveOption("--configfile")) {
        AC_WARNING << "using --configfile-option is deprecated. Use argument instead.";
        bool ok = readConfigFile (myConfigDoc, ourArguments.getOptionArgument("--configfile"));
        if (!ok) {
            AC_ERROR << "given config file '" << ourArguments.getOptionArgument("--configfile") << "' could not be read. exit.";
        }


    } else {
        if (asl::fileExists(ourDefaultConfigFile)) {
            readConfigFile (myConfigDoc, ourDefaultConfigFile);
        } else {
            ourArguments.printUsage();
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
