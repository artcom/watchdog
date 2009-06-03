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

#include "UDPCommandListenerThread.h"
#include "system_functions.h"

#include "Projector.h"

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x500
#endif

#include <asl/net/UDPSocket.h>
#include <asl/net/net.h>

#include <iostream>

#ifdef WIN32
#include <windows.h>
#endif

#include "Application.h"

using namespace inet;
using namespace std;

inline bool isCommand(const std::string & theReceivedCommand, const std::string & theExpectedCommand) {
	return (!theExpectedCommand.empty() && theReceivedCommand == theExpectedCommand);    
}

UDPCommandListenerThread::UDPCommandListenerThread(std::vector<Projector *> theProjectors,
                                                   Application & theApplication,
                                                   const dom::NodePtr & theConfigNode,
                                                   Logger & theLogger)
:   _myProjectors(theProjectors),
    _myUDPPort(2342),
    _myApplication(theApplication),
    _myLogger(theLogger),
    _myPowerDownProjectorsOnHalt(false),
    _myShutterCloseProjectorsOnStop(false),
    _myShutterCloseProjectorsOnReboot(false),
    _mySystemHaltCommand(""), 
    _myRestartAppCommand(""), 
    _mySystemRebootCommand(""),
    _myStopAppCommand(""), 
    _myStartAppCommand(""),    
    
    _myStatusReportCommand(""),
    _myStatusReceiverHost(),
    _myStatusReceiverPort(0),
    _myStatusLoadingDelay(0)
{
    // check for UDP port
    if (theConfigNode->getAttribute("port")) {
        _myUDPPort = asl::as<int>(theConfigNode->getAttribute("port")->nodeValue());
        AC_DEBUG << "_myPort: " << _myUDPPort;
    }
    
    // check for system halt command configuration
    if (theConfigNode->childNode("SystemHalt")) {
        const dom::NodePtr & mySystemHaltNode = theConfigNode->childNode("SystemHalt");
        const dom::NodePtr & myAttribute = mySystemHaltNode->getAttribute("powerDownProjectors");
        if (myAttribute) {
            _myPowerDownProjectorsOnHalt = asl::as<bool>(myAttribute->nodeValue());
        }
        _mySystemHaltCommand = mySystemHaltNode->getAttributeString("command");
        AC_DEBUG <<"_mySystemHaltCommand: " << _mySystemHaltCommand;
    }
    
    // check for system reboot command configuration
    if (theConfigNode->childNode("SystemReboot")) {
        const dom::NodePtr & mySystemRebootNode = theConfigNode->childNode("SystemReboot");
        const dom::NodePtr & myAttribute = mySystemRebootNode->getAttribute("shutterCloseProjectors");
        if (myAttribute) {
            _myShutterCloseProjectorsOnReboot = asl::as<bool>(myAttribute->nodeValue());
        }
        _mySystemRebootCommand = mySystemRebootNode->getAttributeString("command");
        AC_DEBUG <<"_mySystemRebootCommand: " << _mySystemRebootCommand;
        AC_DEBUG <<"_myShutterCloseProjectorsOnReboot: " << _myShutterCloseProjectorsOnReboot;
    }
    
    // check for application restart command configuration
    if (theConfigNode->childNode("RestartApplication")) {
        const dom::NodePtr & myRestartAppNode = theConfigNode->childNode("RestartApplication");
        _myRestartAppCommand = myRestartAppNode->getAttributeString("command");
        AC_DEBUG <<"_myRestartAppCommand: " << _myRestartAppCommand;
    }
    
    // check for application stop command configuration
    if (theConfigNode->childNode("StopApplication")) {
        const dom::NodePtr & myStopAppNode = theConfigNode->childNode("StopApplication");
        const dom::NodePtr & myAttribute = myStopAppNode->getAttribute("shutterCloseProjectors");
        if (myAttribute) {
            _myShutterCloseProjectorsOnStop = asl::as<bool>(myAttribute->nodeValue());
        }
        _myStopAppCommand = myStopAppNode->getAttributeString("command");
        AC_DEBUG <<"_myStopAppCommand: " << _myStopAppCommand;
        AC_DEBUG <<"_myShutterCloseProjectorsOnStop: " << _myShutterCloseProjectorsOnStop;
    }
    
    // check for application start command configuration
    if (theConfigNode->childNode("StartApplication")) {
        const dom::NodePtr & myStartAppNode = theConfigNode->childNode("StartApplication");
        _myStartAppCommand = myStartAppNode->getAttributeString("command");
        AC_DEBUG <<"_myStartAppCommand: " << _myStartAppCommand;
    }
    
    // check for status report configuration 
    if (theConfigNode->childNode("StatusReport")) {
        const dom::NodePtr & myStatusReportNode = theConfigNode->childNode("StatusReport");
        
        _myStatusReportCommand = myStatusReportNode->getAttributeString("command");
        _myStatusReceiverHost  = myStatusReportNode->getAttributeString("host");
        _myStatusReceiverPort  = asl::as<unsigned int>( myStatusReportNode->getAttributeString("port") );
        _myStatusLoadingDelay  = asl::as<unsigned int>( myStatusReportNode->getAttributeString("loadingtime") );
	
        AC_DEBUG << "_myStatusReportNode: " << _myStatusReportCommand << ": status will be send to: '" << _myStatusReceiverHost << ":" << _myStatusReceiverPort << "'";
    }

    if (_myProjectors.size() > 0 ) {
        cout << "Projectors: " << endl;    
        for (std::vector<Projector*>::size_type i = 0; i < _myProjectors.size(); i++) {
            cout << (i+1) << ": " << _myProjectors[i]->getDescription()<< endl;;
        }
    }
}

UDPCommandListenerThread::~UDPCommandListenerThread() {
}

void 
UDPCommandListenerThread::setSystemHaltCommand(const string & theSystemhaltCommand) {
    _mySystemHaltCommand = theSystemhaltCommand;
}

void 
UDPCommandListenerThread::setRestartAppCommand(const string & theRestartAppCommand) {
    _myRestartAppCommand = theRestartAppCommand;
}

void 
UDPCommandListenerThread::setStopAppCommand(const string & theStopAppCommand) {
    _myStopAppCommand = theStopAppCommand;
}

void 
UDPCommandListenerThread::setStartAppCommand(const string & theStartAppCommand) {
    _myStartAppCommand = theStartAppCommand;
}

void 
UDPCommandListenerThread::setSystemRebootCommand(const string & theSystemRebootCommand) {
    _mySystemRebootCommand = theSystemRebootCommand;
}

bool 
UDPCommandListenerThread::controlProjector(const std::string & theCommand)
{
    if (_myProjectors.size() == 0) {
        cerr << "No projectors configured, ignoring '" << theCommand << "'" << endl;
        return false;
    }

    bool myCommandHandled = true;
    for (unsigned i = 0; i < _myProjectors.size(); ++i) {
        myCommandHandled &= _myProjectors[i]->command(theCommand);
    }

    return myCommandHandled;
}

void
UDPCommandListenerThread::initiateShutdown() {
    
    if (_myPowerDownProjectorsOnHalt) {
        // shutdown all connected projectors
        controlProjector("projector_off");
    }
    initiateSystemShutdown();
}

void
UDPCommandListenerThread::initiateReboot() {
    
    if (_myShutterCloseProjectorsOnReboot) {
        // close shutter on all connected projectors
        controlProjector("projector_shutter_close");
    }
    initiateSystemReboot();
}


void
UDPCommandListenerThread::run() {
    cout << "Halt listener activated." << endl;
    cout << "* UDP Listener on port: " << _myUDPPort << endl;
    cout << "* Commands:" << endl;
    cout << "      System Halt  : " << _mySystemHaltCommand << endl;
    cout << "      System Reboot: " << _mySystemRebootCommand << endl;
    cout << "Application restart: " << _myRestartAppCommand << endl;
    cout << "Application stop   : " << _myStopAppCommand << endl;
    cout << "Application start  : " << _myStartAppCommand << endl;

    try {
        UDPSocket myUDPServer( INADDR_ANY, static_cast<asl::Unsigned16>(_myUDPPort) );

        for (;;) {
            char myInputBuffer[2048];
            unsigned myBytesRead = myUDPServer.receiveFrom(0,0,myInputBuffer,sizeof(myInputBuffer)-1);
            myInputBuffer[myBytesRead] = 0;
            std::istringstream myIss(myInputBuffer);
            std::string myCommand;
            std::getline(myIss, myCommand);
            cerr << "Received command: " << myCommand << "\nRestart App Command: " << _myRestartAppCommand << endl;
            if (isCommand(myCommand, _mySystemHaltCommand)) {
                cerr << "Client received halt packet" << endl;
                _myLogger.logToFile( string("Shutdown from Network") );
                _myLogger.closeLogFile();
                initiateShutdown();
            } else if (isCommand(myCommand, _mySystemRebootCommand)) {
                cerr << "Client received reboot packet" << endl;
                _myLogger.logToFile( string("Reboot from Network" ));
                _myLogger.closeLogFile();
                initiateReboot();
            } else if (isCommand(myCommand, _myRestartAppCommand)) {
                cerr << "Client received restart application packet" << endl;
                _myApplication.restart();
            } else if (isCommand(myCommand, _myStopAppCommand)) {
                cerr << "Client received stop application packet" << endl;
                _myLogger.logToFile( string( "Stop application from Network" ));
                if (_myShutterCloseProjectorsOnStop) {
                    // close shutter on all connected projectors
                    controlProjector("projector_shutter_close");
                }
                _myApplication.setPaused(true);
            } else if (isCommand(myCommand, _myStartAppCommand)) {
                cerr << "Client received start application packet" << endl;
                _myLogger.logToFile( string( "Start application from Network" ));
                _myApplication.setPaused(false);
                if (_myShutterCloseProjectorsOnStop) {
                    // open shutter on all connected projectors
                    controlProjector("projector_shutter_open");
                }
            } else if (isCommand(myCommand, _myStatusReportCommand)) {
                cerr << "Client received status report request" << endl;
                
                ProcessResult myProcessResult = _myApplication.getProcessResult(); 
    
                std::string myMessage = "undefined";
                if ( myProcessResult == PR_FAILED ) {
                    myMessage = "error application launch failed";
                } else if ( myProcessResult == PR_RUNNING ) {
                    myMessage = (_myApplication.getRuntime() > _myStatusLoadingDelay)?"ok":"loading";                     
                } else if ( myProcessResult == PR_TERMINATED ) {
                    myMessage = "error application terminated";
                }
                
                sendStatusMessage( myMessage );

            } else if (controlProjector(myCommand) == true) {
                // pass
            } else {
                cerr << "### UDPHaltListener: Unexpected packet '" << myCommand << "'. Ignoring" << endl;
                ostringstream myOss;
                myOss << "### UDPHaltListener: Unexpected packet '" << myCommand 
                    << "'. Ignoring";
                _myLogger.logToFile( myOss.str() );
            }
        }
    } catch (SocketException & se) {
        cerr << "### UDPHaltListener: " << se.what() << endl;
        ostringstream myOss;
        myOss <<  "### UDPHaltListener: " << se.what();
        _myLogger.logToFile( myOss.str() );
    }
    cerr << "stopped udp command listener thread.\n";
    _myLogger.logToFile( string( "Stopped command listener thread!" ) );
}

void
UDPCommandListenerThread::sendStatusMessage( const std::string & theMessage ) {
    if ( _myStatusReceiverHost == "" || _myStatusReceiverPort == 0 ) {
        return;
    }
    
    // try to find a free client port between serverPort+1 and MAX_PORT
    UDPSocket * myUDPClient = 0;
    for (asl::Unsigned16 clientPort = 5000; clientPort <= 6000; clientPort++)
    {
        try {
            myUDPClient = new UDPSocket( INADDR_ANY, clientPort );
            break;
        }
        catch (SocketException & se)
        {
            cerr << "TestSocket::UDPTest() " << "failed to listen on port " << clientPort << ":" << se.where() << endl;
            myUDPClient = 0;
        }
    }

    asl::Unsigned32 myHostAddress = getHostAddress( _myStatusReceiverHost.c_str() );
    myUDPClient->sendTo( myHostAddress, static_cast<asl::Unsigned16>( _myStatusReceiverPort ), theMessage.c_str(), theMessage.size() );
}
