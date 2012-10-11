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


#include "UDPCommandListenerThread.h"

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x500
#endif

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <asl/net/UDPSocket.h>
#include <asl/net/net_functions.h>
#include <asl/net/net.h>

#include <iostream>

#ifdef WIN32
#include <windows.h>
#endif

#include "Application.h"
#include "Projector.h"
#include "system_functions.h"

const asl::Unsigned16 MAX_PORT = 65535;

using namespace inet;
using namespace std;

inline bool isCommand(const std::string & theReceivedCommand, const std::string & theExpectedCommand) {
    return (!theExpectedCommand.empty() && theReceivedCommand == theExpectedCommand);
}

UDPCommandListenerThread::UDPCommandListenerThread(std::vector<Projector *> theProjectors,
                                                   Application & theApplication,
                                                   const dom::NodePtr & theConfigNode,
                                                   Logger & theLogger,
                                                   std::string & theShutdownCommand)
:   _myProjectors(theProjectors),
    _myUDPPort(2342),
    _myReturnMessagePort(-1),
    _myApplication(theApplication),
    _myLogger(theLogger),
    _myPowerDownProjectorsOnHalt(false),
    _myShutterCloseProjectorsOnStop(false),
    _myShutterCloseProjectorsOnReboot(false),
    _mySystemHaltCommand(""),
    _myRestartAppCommand(""),
    _mySwitchAppCommand(""),
    _mySystemRebootCommand(""),
    _myStopAppCommand(""),
    _myStartAppCommand(""),
    _myStatusReportCommand(""),
    _myStatusLoadingDelay(0), 
    _myShutdownCommand(theShutdownCommand)
{
    // check for UDP port
    if (theConfigNode->getAttribute("port")) {
        _myUDPPort = asl::as<int>(theConfigNode->getAttribute("port")->nodeValue());
        AC_DEBUG << "_myPort: " << _myUDPPort;
    }
    if (theConfigNode->getAttribute("returnport")) {
        _myReturnMessagePort = asl::as<int>(theConfigNode->getAttribute("returnport")->nodeValue());
        AC_DEBUG << "return message to port: " << _myReturnMessagePort;
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

    // check for application switch command configuration
    if (theConfigNode->childNode("SwitchApplication")) {
        const dom::NodePtr & mySwitchAppNode = theConfigNode->childNode("SwitchApplication");
        _mySwitchAppCommand = mySwitchAppNode->getAttributeString("command");
        AC_DEBUG <<"_mySwitchAppCommand: " << _mySwitchAppCommand;
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
        AC_DEBUG <<"_myStatusReportCommand: " << _myStatusReportCommand;
        const dom::NodePtr & myAttribute = myStatusReportNode->getAttribute("loadingtime");
        if (myAttribute) {
            _myStatusLoadingDelay  = asl::as<unsigned int>( myAttribute->nodeValue());
            AC_DEBUG <<"_myStatusLoadingDelay: " << _myStatusLoadingDelay;
        }

    }
    if (theConfigNode->childNode("IpWhitelist")) {
        const dom::NodePtr & myIpWhitelistNode = theConfigNode->childNode("IpWhitelist");
        for (unsigned i = 0; i < myIpWhitelistNode->childNodesLength(); ++i) {
            const dom::NodePtr & myIpWhitelistEntryNode = myIpWhitelistNode->childNode(i);
            if (myIpWhitelistEntryNode->nodeType() == dom::Node::ELEMENT_NODE) {
                std::string myAllowedIp = myIpWhitelistEntryNode->firstChild()->nodeValue();
                AC_DEBUG << "add ip '" << myAllowedIp << "' to udpcontrol whitelist";
                _myAllowedIps.push_back(getHostAddress(myAllowedIp.c_str()));
            }
        }
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

/*void
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
}*/

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

    if (_myShutdownCommand != "") {
        int myError = system(_myShutdownCommand.c_str());
        cerr << "shutdown command: \"" << _myShutdownCommand << "\" return with " << myError << endl;
    }

    initiateSystemShutdown();
}

void
UDPCommandListenerThread::initiateReboot() {

    if (_myShutterCloseProjectorsOnReboot) {
        // close shutter on all connected projectors
        controlProjector("projector_shutter_close");
    }

    if (_myShutdownCommand != "") {
        int myError = system(_myShutdownCommand.c_str());
        cerr << "shutdown command: \"" << _myShutdownCommand << "\" return with " << myError << endl;
    }
    initiateSystemReboot();
}

bool
UDPCommandListenerThread::allowedIp(asl::Unsigned32 theHostAddress) {
    if (_myAllowedIps.size() == 0 ||
        std::find(_myAllowedIps.begin(), _myAllowedIps.end(), theHostAddress) != _myAllowedIps.end())
    {
        return true;
    } else {
        return false;
    }
}

void
UDPCommandListenerThread::sendReturnMessage(asl::Unsigned32 theClientHost, const std::string & theMessage) {
    if (theMessage.empty() || _myReturnMessagePort == -1) {
        return;
    }
    try {
        UDPSocket * myUDPClient = 0;
        for (unsigned int clientPort = _myUDPPort+1; clientPort <= MAX_PORT; clientPort++)
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
            myUDPClient->sendTo(theClientHost, _myReturnMessagePort, theMessage.c_str(), theMessage.size());            
            cerr << "send message to client: " << theMessage << endl;                
        }
    } catch (SocketException & se) {
        ostringstream myOss;
        myOss << "### UDPCommandListenerThread::sendReturnMessage " << se.what();
        cerr << myOss.str() << endl;
        _myLogger.logToFile( myOss.str() );
    }
}

void
UDPCommandListenerThread::run() {
    cout << "Halt listener activated." << endl;
    cout << "* UDP Listener on port: " << _myUDPPort << endl;
    if (_myReturnMessagePort != -1) {
        cout << "* UDP Listener return messages on port: " << _myReturnMessagePort << endl;
    }
    cout << "* Commands:" << endl;
    cout << "      System Halt  : " << _mySystemHaltCommand << endl;
    cout << "      System Reboot: " << _mySystemRebootCommand << endl;
    cout << "Application restart: " << _myRestartAppCommand << endl;
    cout << "Application switch : " << _mySwitchAppCommand << endl;
    cout << "Application stop   : " << _myStopAppCommand << endl;
    cout << "Application start  : " << _myStartAppCommand << endl;

    try {
        UDPSocket myUDPServer( INADDR_ANY, static_cast<asl::Unsigned16>(_myUDPPort) );

        for (;;) {
            try {
                asl::Unsigned32 clientHost;
                asl::Unsigned16 clientPort;
                char myInputBuffer[2048];
                unsigned myBytesRead = myUDPServer.receiveFrom(
                        &clientHost, &clientPort,
                        myInputBuffer,sizeof(myInputBuffer)-1);
                if(allowedIp(clientHost)) {
                    myInputBuffer[myBytesRead] = 0;
                    std::istringstream myIss(myInputBuffer);
                    std::string myCommand;
                    std::getline(myIss, myCommand);
                    std::vector<std::string> myArguments;
                    boost::split(myArguments, myCommand, boost::is_any_of("/"));
                    myCommand = myArguments[0];
                    myArguments.erase(myArguments.begin());

                    std::string myMessage;
                    if (isCommand(myCommand, std::string("ping"))) {
                        _myLogger.logToFile( string("Ping from Network") );
                        myMessage = "pong";
                    } else if (isCommand(myCommand, _mySystemHaltCommand)) {
                        cerr << "Client received halt packet" << endl;
                        _myLogger.logToFile( string("Shutdown from Network") );
                        _myLogger.closeLogFile();
                        sendReturnMessage(clientHost, _mySystemHaltCommand);
                        initiateShutdown();
                    } else if (isCommand(myCommand, _mySystemRebootCommand)) {
                        cerr << "Client received reboot packet" << endl;
                        _myLogger.logToFile( string("Reboot from Network" ));
                        _myLogger.closeLogFile();
                        sendReturnMessage(clientHost, _mySystemRebootCommand);
                        initiateReboot();
                    } else if (isCommand(myCommand, _myRestartAppCommand)) {
                        cerr << "Client received restart application packet" << endl;
                        _myLogger.logToFile( string("Restart from Network" ));
                        _myApplication.restart();
                        myMessage = _myRestartAppCommand;
                    } else if (isCommand(myCommand, _mySwitchAppCommand) && myArguments.size() == 1) {
                        cerr << "Client received switch application (id '" << myArguments[0] 
                                << "') packet " << endl;
                        _myLogger.logToFile( string("Switch application from Network" ));
                        std::string myId = myArguments[0];
                        _myApplication.switchApplication(myId);
                        myMessage = _mySwitchAppCommand;
                    } else if (isCommand(myCommand, _myStopAppCommand)) {
                        cerr << "Client received stop application packet" << endl;
                        _myLogger.logToFile( string( "Stop application from Network" ));
                        if (_myShutterCloseProjectorsOnStop) {
                            // close shutter on all connected projectors
                            controlProjector("projector_shutter_close");
                        }
                        _myApplication.setPaused(true);
                        myMessage = _myStopAppCommand;
                    } else if (isCommand(myCommand, _myStartAppCommand)) {
                        cerr << "Client received start application packet" << endl;
                        _myLogger.logToFile( string( "Start application from Network" ));
                        _myApplication.setPaused(false);
                        if (_myShutterCloseProjectorsOnStop) {
                            // open shutter on all connected projectors
                            controlProjector("projector_shutter_open");
                        }
                        myMessage = _myStartAppCommand;
                    } else if (isCommand(myCommand, _myStatusReportCommand)) {
                        cerr << "Client received status report request" << endl;
                        _myLogger.logToFile( string("StatusReport request from Network" ));
    
                        ProcessResult myProcessResult = _myApplication.getProcessResult();
                        if ( myProcessResult == PR_FAILED ) {
                            myMessage = "error application launch failed";
                        } else if ( myProcessResult == PR_RUNNING ) {
                            myMessage = (static_cast<unsigned>(_myApplication.getRuntime()) > _myStatusLoadingDelay)
                                        ? "ok" : "loading";
                        } else if ( myProcessResult == PR_TERMINATED ) {
                            myMessage = "error application terminated";
                        }
                    } else if (controlProjector(myCommand) == true) {
                        // pass
                    } else {
                        ostringstream myOss;
                        myOss << "### UDPCommandListener: Unexpected packet '" << myCommand
                            << "'. Ignoring";
                        cerr << myOss.str() << endl;
                        _myLogger.logToFile( myOss.str() );
                    }
                    sendReturnMessage(clientHost, myMessage);
                } else {
                    ostringstream myOss;
                    myOss << "client host not allowed for udpcontrol: " << asl::as_dotted_address(clientHost);
                    cerr << myOss.str() << std::endl;
                    _myLogger.logToFile( myOss.str() );
                }
            } catch (SocketException & se) {
                ostringstream myOss;
                myOss <<  "### UDPCommandListener: " << se.what();
                cerr << myOss.str() << endl;
                _myLogger.logToFile( myOss.str() );
            }
        }
    } catch (SocketException & se) {
        ostringstream myOss;
        myOss <<  "### UDPCommandListener:loop " << se.what();
        cerr << myOss.str() << endl;
        _myLogger.logToFile( myOss.str() );
    }
    cerr << "stopped udp command listener thread.\n";
    _myLogger.logToFile( string( "Stopped command listener thread!" ) );
}
