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


#include "Application.h"

#include <ctime>
#include <sstream>
#include <typeinfo>
#include <cctype>

#include <asl/base/MappedBlock.h>
#include <asl/base/Exception.h>
#include <asl/base/file_functions.h>
#include <asl/base/os_functions.h>
#include <asl/base/proc_functions.h>
#include <asl/base/Auto.h>

#include "Logger.h"
#include "system_functions.h"
#include "parse_helpers.h"


#ifdef WIN32
#   include <Tlhelp32.h>
#endif

using namespace std;  // Manually included.
using namespace dom;  // Manually included.

const char * ourWeekdayMap[] = {
    "monday",
    "tuesday",
    "wednesday",
    "thursday",
    "friday",
    "saturday",
    "sunday",
    0
};

struct to_upper {
      char operator()(char c1) {return static_cast<char>(std::toupper(c1));}
};

template< typename T >
inline T convertFromString(const std::string& str)
{
    std::istringstream iss(str);
    T obj;
    iss >> obj;
    if( !iss ) {
        AC_ERROR << "Conversion from \"" << str << "\" to '" << typeid(T).name() << "' failed!";
    }
    return obj;
}

const std::string STARTUP_COUNT_ENV = "AC_STARTUP_COUNT";

Application::Application(Logger & theLogger)
:
_myApplicationWatchdogDirectory(""),
_myFileName(""),
_myAppLogFile(""),
_myAppLogFileFormatString(""),
_myAppLogFileFormatter(false),
_myWorkingDirectory(""),
_myWindowTitle(""),
#ifdef WIN32
_myShowWindowMode(""),
#endif
_myAppStartTimeInSeconds(0),
_myRecvRestart(false),
_myAllowMissingHeartbeats(3),
_myHeartbeatFrequency(0),
_myPerformECG(false),
_myRestartMemoryThreshold(0),
_myRestartDay(""),
_myRestartTimeInSecondsToday(0),
_myCheckMemoryTimeInSecondsToday(0),
_myMemoryThresholdTimed(0),
_myRestartMode(UNSET),
_myRestartCheck(false),
_myApplicationPaused(false),
_myCheckedMemoryToday(false),
_myRestartedToday(false),
_myMemoryIsFull(false),
_myItIsTimeToRestart(false),
_myHeartIsBroken(false),
_myStartTimeInSeconds(0),
_myDayChanged(false),
_myLogger(theLogger),
_myStartDelay(0),
_myRestartDelay(10),
_myStartupCount(0)
{
}

Application::~Application() {
}

void
Application::setupEnvironment(const NodePtr & theEnvironmentSettings) {
    for (NodeList::size_type myEnvNr = 0; myEnvNr < theEnvironmentSettings->childNodesLength(); myEnvNr++) {
        const dom::NodePtr & myEnvNode = theEnvironmentSettings->childNode(myEnvNr);
        if (myEnvNode->nodeType() == dom::Node::ELEMENT_NODE) {
            string myEnviromentVariable = myEnvNode->getAttributeString("name");
            if (myEnviromentVariable.size()>0 && myEnvNode->childNodesLength() == 1) {
                string myEnvironmentValue = asl::expandEnvironment(
                        myEnvNode->firstChild()->nodeValue() );
                AC_DEBUG <<"Environment variable : "<<myEnvNr << ": " << myEnviromentVariable
                        << " -> " << myEnvironmentValue ;
                _myEnvironmentVariables[myEnviromentVariable] = myEnvironmentValue;
            }
        }
    }
}

bool Application::setup(const dom::NodePtr & theAppNode, const std::string & theDirectory) {
    if (theDirectory.length() > 0) {
        _myApplicationWatchdogDirectory = theDirectory;
    }
    _myFileName = asl::expandEnvironment(theAppNode->getAttribute("binary")->nodeValue());
    AC_DEBUG <<"_myFileName: " << _myFileName;
    if (_myFileName.empty()){
        _myLogger.logToFile("### ERROR, no application binary to watch.");
        cerr <<"### ERROR, no application binary to watch." << endl;
        setPaused(true);
        return false;
    }

    if (theAppNode->getAttribute("windowtitle")) {
        _myWindowTitle = theAppNode->getAttribute("windowtitle")->nodeValue();
        AC_DEBUG <<"_myWindowTitle: " << _myWindowTitle;
    }
    if (theAppNode->getAttribute("showWindow")) {
#ifdef WIN32
        _myShowWindowMode = theAppNode->getAttribute("showWindow")->nodeValue();
        AC_DEBUG << "_myShowWindowMode: " << _myShowWindowMode;
#else
        AC_WARNING << "showWindow attribute is ignored for all OS except windows";
#endif
    }
    if (theAppNode->getAttribute("directory")) {
        _myWorkingDirectory = asl::expandEnvironment(
            theAppNode->getAttribute("directory")->nodeValue() );
        AC_DEBUG <<"_myWorkingDirectory: " << _myWorkingDirectory;
    }
    if (theAppNode->getAttribute("logFile")) {
#ifdef WIN32
        AC_WARNING << "logFile attribute is ignored for windows.";
#else
        _myAppLogFile = asl::expandEnvironment(
            theAppNode->getAttribute("logFile")->nodeValue() );
        AC_DEBUG <<"_myAppLogFile: " << _myAppLogFile;

        if (theAppNode->getAttribute("logFormatter")) {
            _myAppLogFileFormatString = theAppNode->getAttribute("logFormatter")->nodeValue();
            _myAppLogFileFormatter = true;
        }
#endif
    }
    if (theAppNode->childNode("EnvironmentVariables")) {
        setupEnvironment(theAppNode->childNode("EnvironmentVariables"));
        AC_DEBUG << "finished setting up environment variables";
    }
    _myArguments.clear();
    if (theAppNode->childNode("Arguments")) {
        const dom::NodePtr & myArguments = theAppNode->childNode("Arguments");
        AC_DEBUG << "arguments: " << myArguments;
        for (NodeList::size_type myArgumentNr = 0; myArgumentNr < myArguments->childNodesLength(); myArgumentNr++) {
            const dom::NodePtr & myArgumentNode = myArguments->childNode(myArgumentNr);
            if (myArgumentNode->nodeType() == dom::Node::ELEMENT_NODE &&
                myArgumentNode->hasChildNodes() ) {
                std::string myArgument = (*myArgumentNode)("#text").nodeValue();
                _myArguments.push_back(asl::expandEnvironment(myArgument));
                AC_DEBUG << "Argument : "<< myArgumentNr << ": " << _myArguments.back() ;
            }
        }
    }
    if (theAppNode->childNode("RestartDay")) {
        _myRestartCheck = true;
        _myRestartDay = (*theAppNode->childNode("RestartDay"))("#text").nodeValue();
        _myRestartMode |= RESTARTDAY;
        std::transform(_myRestartDay.begin(), _myRestartDay.end(), _myRestartDay.begin(), to_upper());
        AC_DEBUG <<"_myRestartDay : " << _myRestartDay;
    }
    if (theAppNode->childNode("RestartTime")) {
        _myRestartCheck = true;
        std::string myRestartTime = (*theAppNode->childNode("RestartTime"))("#text").nodeValue();
        std::string myHours = myRestartTime.substr(0, myRestartTime.find_first_of(':'));
        std::string myMinutes = myRestartTime.substr(myRestartTime.find_first_of(':')+1, myRestartTime.length());
        _myRestartTimeInSecondsToday = convertFromString<long>(myHours) * 3600;
        _myRestartTimeInSecondsToday += convertFromString<long>(myMinutes) * 60;
        _myRestartMode |= RESTARTTIME;
        AC_DEBUG <<"_myRestartTimeInSecondsToday : " << _myRestartTimeInSecondsToday;
    }
    if (theAppNode->childNode("CheckMemoryTime")) {
        _myRestartCheck = true;
        std::string myCheckMemoryTime = (*theAppNode->childNode("CheckMemoryTime"))("#text").nodeValue();
        std::string myHours = myCheckMemoryTime.substr(0, myCheckMemoryTime.find_first_of(':'));
        std::string myMinutes = myCheckMemoryTime.substr(myCheckMemoryTime.find_first_of(':')+1, myCheckMemoryTime.length());
        _myCheckMemoryTimeInSecondsToday = convertFromString<long>(myHours) * 3600;
        _myCheckMemoryTimeInSecondsToday += convertFromString<long>(myMinutes) * 60;
        _myRestartMode |= CHECKMEMORYTIME;
        AC_DEBUG <<"_myCheckMemoryTimeInSecondsToday : " << _myCheckMemoryTimeInSecondsToday;
    }
    if (theAppNode->childNode("CheckTimedMemoryThreshold")) {
        _myRestartCheck = true;
        _myMemoryThresholdTimed = asl::as<unsigned int>((*theAppNode->childNode("CheckTimedMemoryThreshold"))("#text").nodeValue());
        _myRestartMode |= CHECKTIMEDMEMORYTHRESHOLD;
        AC_DEBUG <<"_myMemoryThresholdTimed : " << _myMemoryThresholdTimed;
    }
    if (theAppNode->childNode("Memory_Threshold")) {
        _myRestartCheck = true;
        _myRestartMemoryThreshold = asl::as<unsigned int>((*theAppNode->childNode("Memory_Threshold"))("#text").nodeValue());
        _myRestartMode |= MEMTHRESHOLD;
        AC_DEBUG <<"_myRestartMemoryThreshold : " << _myRestartMemoryThreshold;
    }
    if (theAppNode->childNode("WaitDuringStartup")) {
        _myStartDelay = asl::as<unsigned>((*theAppNode->childNode("WaitDuringStartup"))("#text").nodeValue());
        AC_DEBUG <<"_myStartDelay: " << _myStartDelay ;
    }
    if (theAppNode->childNode("WaitDuringRestart")) {
        _myRestartDelay = asl::as<unsigned>((*theAppNode->childNode("WaitDuringRestart"))("#text").nodeValue());
        AC_DEBUG <<"_myRestartDelay: " << _myRestartDelay ;
    }

    if (theAppNode->childNode("Heartbeat")) {
        const dom::NodePtr & myHeartbeatNode = theAppNode->childNode("Heartbeat");
        if (myHeartbeatNode->childNode("Heartbeat_File")) {
            _myHeartbeatFile = asl::expandEnvironment((*myHeartbeatNode->childNode("Heartbeat_File"))("#text").nodeValue());
            AC_DEBUG <<"_myHeartbeatFile : " << _myHeartbeatFile ;
        }
        if (myHeartbeatNode->childNode("Allow_Missing_Heartbeats")) {
            _myAllowMissingHeartbeats = asl::as<int>((*myHeartbeatNode->childNode("Allow_Missing_Heartbeats"))("#text").nodeValue());
            AC_DEBUG <<"_myAllowMissingHeartbeats : " << _myAllowMissingHeartbeats ;
        }
        if (myHeartbeatNode->childNode("Heartbeat_Frequency")) {
            _myHeartbeatFrequency = asl::as<int>((*myHeartbeatNode->childNode("Heartbeat_Frequency"))("#text").nodeValue());
            AC_DEBUG <<"_myHeartbeatFrequency : " << _myHeartbeatFrequency ;
        }
        _myPerformECG = true;
        if ((_myHeartbeatFrequency == 0) || (_myHeartbeatFile.empty())) {
            _myPerformECG = false;
        }
        if (myHeartbeatNode->childNode("FirstHeartBeatDelay")) {
            _myAppStartTimeInSeconds = asl::as<int>((*myHeartbeatNode->childNode("FirstHeartBeatDelay"))("#text").nodeValue());
            AC_DEBUG <<"_myAppStartTimeInSeconds : " << _myAppStartTimeInSeconds;
        }
    }
    return true;
}

void
Application::setEnvironmentVariables() {
    std::map<std::string, std::string>::iterator myIter = _myEnvironmentVariables.begin();
    for( myIter = _myEnvironmentVariables.begin(); myIter != _myEnvironmentVariables.end(); ++myIter ) {
        asl::set_environment_var(myIter->first, myIter->second);
        _myLogger.logToFile( "Set environment variable: " + myIter->first + "=" + myIter->second);
        cerr << "Set environment variable: " << myIter->first <<
                "=" << myIter->second << endl;

    }
}

void
Application::terminate(const std::string & theReason, bool theWMCloseAllowed){
    _myLogger.logToFile(string("Terminate because: ") + theReason);
    if (_myProcessResult == PR_RUNNING) {
        _myProcessResult = closeApp( theWMCloseAllowed ? _myWindowTitle : std::string(""), _myProcessInfo,
                  _myLogger );
    }
}

bool
Application::checkForRestart( std::string & myRestartMessage ) {
    if (_myMemoryIsFull) {
        myRestartMessage = "Memory threshold reached.";
        return true;
    }
    if (_myItIsTimeToRestart) {
        myRestartMessage = "Time to restart reached.";
        return true;
    }
    if (_myHeartIsBroken) {
        myRestartMessage = "Failed to detect heartbeat.";
        return true;
    }
    if (_myRecvRestart) {
        myRestartMessage = RECEIVED_RESTART_APP_STRING;//"Received restart command.";
        asl::AutoLocker<asl::ThreadLock> myAutoLock(_myLock);
        _myRecvRestart = false;
        return true;
    }
    return false;
}

void
Application::restart() {
    if (!paused()) {
        asl::AutoLocker<asl::ThreadLock> myAutoLock(_myLock);
        _myRecvRestart = true;
    } else {
       setPaused( false );
    }
}

void
Application::switchApplication(std::string theId) {
    dom::Document myApplicationConfigDoc;
    bool ok = readConfigFile(myApplicationConfigDoc, _myApplicationWatchdogDirectory + "/" + theId + ".xml");
    if (!ok) {
        AC_WARNING << "config file for id '" << theId << "' could not be opened. IGNORE.";
        return;
    }

    if (!myApplicationConfigDoc("Application")) {
        AC_WARNING << "application xml has no Application-node";
        return;
    }
    const dom::NodePtr & myApplicationNode = myApplicationConfigDoc.childNode("Application");

    if (!setup(myApplicationNode)) {
        AC_WARNING << "Application instance might be corrupted due to problems while setup for new application";
        return;
    }

    asl::AutoLocker<asl::ThreadLock> myAutoLock(_myLock);
    _myRecvRestart = true;
}

void
Application::launch() {
    _myEnvironmentVariables[STARTUP_COUNT_ENV] = asl::as_string(++_myStartupCount);
    setEnvironmentVariables();
    if (_myFileName.empty()){
        _myLogger.logToFile("### ERROR, no application binary to watch.");
        cerr <<"### ERROR, no application binary to watch." << endl;
        setPaused(true);
        return;
    }
    std::string myAppLogFileWithTimestamp(_myAppLogFile);
    if (!_myAppLogFile.empty()) {
        if (_myAppLogFileFormatter) {
            if (_myAppLogFileFormatString.empty()) {
                _myAppLogFileFormatString = "%Y-%M-%D-%h-%m-%s";
            }
            std::string::size_type myDotPos = _myAppLogFile.rfind(".", _myAppLogFile.size());
            if (myDotPos == std::string::npos) {
                myDotPos = _myAppLogFile.size();
            }
            asl::Time now;
            now.toLocalTime();
            std::ostringstream myTimeString;
            myTimeString << asl::formatTime(_myAppLogFileFormatString.c_str()) << now;
            myAppLogFileWithTimestamp = _myAppLogFile.substr(0, myDotPos) + "_" + myTimeString.str()
                + _myAppLogFile.substr(myDotPos, _myAppLogFile.size());
        }
    }
    bool myResult = launchApp( _myFileName, _myArguments, asl::expandEnvironment(_myWorkingDirectory), myAppLogFileWithTimestamp,
#ifdef WIN32
                               _myShowWindowMode,
#endif
                               _myProcessInfo );

    std::string myCommandLine = _myFileName + " " + getArguments();

    std::string logstring = " Command : '" + myCommandLine  + "'" +
                             (_myAppLogFile != "" ? " redirecting stdout/stderr to : '" +
                              myAppLogFileWithTimestamp + "'" : "") +
                             (_myWorkingDirectory != "" ? " working directory: '" +
                              asl::expandEnvironment(_myWorkingDirectory) + "'" : "");
    if (!myResult) {
        _myLogger.logToFile( getLastError() + logstring);
        cerr << getLastError() << "\n\n" << logstring << endl;
        exit(-1);
    }
    _myStartTimeInSeconds = getElapsedSecondsToday();

    _myLogger.logToFile("Started successfully: " + logstring);
    _myProcessResult = PR_RUNNING;

    _myItIsTimeToRestart   = false;
    _myHeartIsBroken       = false;
    _myMemoryIsFull        = false;
}

long
Application::getRuntime() {
    return getElapsedSecondsToday() - _myStartTimeInSeconds;
}

void
Application::checkHeartbeat() {
    if (_myPerformECG && (getElapsedSecondsToday() - _myStartTimeInSeconds >= _myAppStartTimeInSeconds) ) {
        string myHeartbeatStr;
        asl::readFile(_myHeartbeatFile, myHeartbeatStr );
        if ( myHeartbeatStr.empty()) {
            _myLogger.logToFile("Could not read heartbeatfile: " + _myHeartbeatFile);
            _myHeartIsBroken =  false;
            return;
        }
        try {
            dom::Document myHeartbeatXml;
            myHeartbeatXml.parse(myHeartbeatStr);

            string nodename;
            if (myHeartbeatXml.childNode("heartbeat")) {
                nodename = "heartbeat";
            }
            if (myHeartbeatXml.childNode("Heartbeat")) {
                nodename = "Heartbeat";
            }
            if (nodename.empty()) {
                _myLogger.logToFile("Could not find xmlnode 'heartbeat' in heartbeatfile: " + _myHeartbeatFile);
                _myHeartIsBroken =  false;
                return;
            }
            const dom::NodePtr & heartbeatNode = myHeartbeatXml.childNode(nodename);
            string mySecondsSince1970Str = heartbeatNode->getAttributeString("secondsSince1970");
            time_t myCurrentSecondsSince_1_1_1970;
            time( &myCurrentSecondsSince_1_1_1970 );

            time_t myLastHeartbeatAge = myCurrentSecondsSince_1_1_1970
                                    - convertFromString<long>(mySecondsSince1970Str);
            AC_DEBUG <<" myCurrentSecondsSince_1_1_1970 : " << myCurrentSecondsSince_1_1_1970 ;
            AC_DEBUG <<" last heartbeat sec since 1.1.70: "
                     <<  convertFromString<long>(mySecondsSince1970Str) ;
            AC_DEBUG <<" last age : " << myLastHeartbeatAge ;
            if ( myLastHeartbeatAge > _myHeartbeatFrequency * _myAllowMissingHeartbeats) {
                _myHeartIsBroken = true;
            } else {
                _myHeartIsBroken =  false;
            }
        } catch (const asl::Exception& e) {
                std::cerr << "Failed to parse heartbeat file " <<
                    _myHeartbeatFile << " got exception: " << e << std::endl;
                _myLogger.logToFile(std::string("Failed to parse heartbeat file ") + _myHeartbeatFile +
                        std::string("got exception ") + asl::as_string(e));
                _myHeartIsBroken = false;
        }
    } else {
        _myHeartIsBroken =  false;
    }
}


std::string
Application::runUntilNextCheck(int theWatchFrequency) {
    // Wait until child process exits.
    _myProcessResult = waitForApp( _myProcessInfo, 1000 * theWatchFrequency, _myLogger );
    if (_myProcessResult == PR_FAILED) {
        return string("Return code of process: ") + getLastError();
    }
    return string("Internal quit.");
}

void
Application::checkState() {

    if (_myRestartCheck) {
        // get memory available/free
        unsigned myAvailMem = 0;
#if 1
        myAvailMem = asl::getFreeMemory();
#else
        const unsigned int pmu = asl::getProcessMemoryUsage(_myProcessInfo.dwProcessId);
        MEMORYSTATUS myMemoryStatus;
        GlobalMemoryStatus (&myMemoryStatus);
        AC_INFO << "virt free:  " << myMemoryStatus.dwAvailVirtual / 1024 << "kb";
        AC_INFO << "virt used:  " << (myMemoryStatus.dwTotalVirtual - myMemoryStatus.dwAvailVirtual) / 1024 << "kb";
        AC_INFO << "virt total: " << myMemoryStatus.dwTotalVirtual / 1024 << "kb";
        AC_INFO << "phy free:   " << myMemoryStatus.dwAvailPhys / 1024 << "kb";
        AC_INFO << "phy used:   " << (myMemoryStatus.dwTotalPhys - myMemoryStatus.dwAvailPhys) / 1024 << "kb";
        AC_INFO << "phy total:  " << myMemoryStatus.dwTotalPhys / 1024 << "kb";
        AC_INFO << "%:          " << myMemoryStatus.dwMemoryLoad;
        AC_INFO << "child proc: " << pmu << "kb";
        AC_INFO << "---------------------------------------";
        myAvailMem = myMemStatus.dwAvailPhys - pmu;
#endif
        _myMemoryIsFull = (myAvailMem <= _myRestartMemoryThreshold);
        if (_myMemoryIsFull) {
            _myLogger.logToFile(std::string("Avail. phy. mem.: ") + asl::as_string(myAvailMem));
        }

        long myElapsedSecondsToday = getElapsedSecondsToday();

        // get the day of the week
        time_t myTime;
        time(&myTime);
        struct tm * myPrintableTime = localtime(&myTime);
        std::string myWeekday = ourWeekdayMap[myPrintableTime->tm_wday];
        if (myWeekday != _myLastWeekday) {
            if ( ! _myLastWeekday.empty()) {
                _myDayChanged = true;
            }
            _myLastWeekday = myWeekday;
        } else {
            _myDayChanged = false;
        }

        std::transform(myWeekday.begin(), myWeekday.end(), myWeekday.begin(), to_upper());

        // time check
        if (_myRestartMode & RESTARTTIME) {
            if (!_myRestartedToday) {
                if (myElapsedSecondsToday > _myRestartTimeInSecondsToday) {
                    if ( _myRestartMode & RESTARTDAY) {
                        if (myWeekday == _myRestartDay) {
                            _myItIsTimeToRestart = true;
                            _myRestartedToday = true;
                        }
                    } else {
                        _myItIsTimeToRestart = true;
                        _myRestartedToday = true;
                    }
                }
            }
        }

        // day check
        if ( _myDayChanged ) {
            _myRestartedToday = false;
            _myCheckedMemoryToday = false;
            if (!(_myRestartMode & RESTARTTIME)) {
                if ( _myRestartMode & RESTARTDAY) {
                    if (myWeekday == _myRestartDay) {
                        _myItIsTimeToRestart = true;
                    }
                }
            }
        }

        // yellow section
        if ((_myRestartMode & CHECKMEMORYTIME) && (_myRestartMode & CHECKTIMEDMEMORYTHRESHOLD)) {
            if (!_myCheckedMemoryToday) {
                if (myElapsedSecondsToday > _myCheckMemoryTimeInSecondsToday) {
                    _myMemoryIsFull = myAvailMem <= _myMemoryThresholdTimed;
                    _myCheckedMemoryToday = true;
                }
            }
        }
    }
}

unsigned Application::getRestartDelay() const {
    return _myRestartDelay;
}

unsigned Application::getStartDelay() const {
    return _myStartDelay;
}

bool Application::paused() const {
    return _myApplicationPaused;
}

bool Application::performECG() const {
    return _myPerformECG;
}

bool Application::restartedToday() const {
    return _myRestartedToday;
}

std::string Application::getHeartbeatFile() const {
    return _myHeartbeatFile;
}

long Application::getRestartTimeInSecondsToday() const {
    return _myRestartTimeInSecondsToday;
}

ProcessResult Application::getProcessResult() const {
    return _myProcessResult;
}

std::string Application::getFilename() const {
    return _myFileName;
}

std::string Application::getArguments() const {
    std::string myResult;
    for (std::vector<std::string>::size_type i = 0; i < _myArguments.size(); i++) {
        myResult += _myArguments[i];
        myResult += " ";
    }
    if (!myResult.empty()) {
        myResult.resize( myResult.size() - 1 );
    }
    return myResult;
}

void Application::setPaused(bool thePausedFlag) {
    _myApplicationPaused = thePausedFlag;
}

void Application::setRestartedToday(bool theRestartedTodayFlag) {
    _myRestartedToday = theRestartedTodayFlag;
}

long getElapsedSecondsToday() {
    time_t ltime;
    struct tm *newtime;
    time( &ltime );
    newtime = localtime( &ltime );
    long myElapsedSecondsToday = newtime->tm_hour * 3600;
    myElapsedSecondsToday += newtime->tm_min * 60;
    myElapsedSecondsToday += newtime->tm_sec;
    return myElapsedSecondsToday;
}
