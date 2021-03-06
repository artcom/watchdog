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
#include "system_functions.h"

const asl::Unsigned16 MAX_PORT = 65535;

using namespace inet;
using namespace std;

inline bool isCommand(const std::string & theReceivedCommand, const std::string & theExpectedCommand) {
    return (!theExpectedCommand.empty() && theReceivedCommand == theExpectedCommand);
}

UDPCommandListenerThread::UDPCommandListenerThread(Application & theApplication,
                                                   const dom::NodePtr & theConfigNode,
                                                   Logger & theLogger,
                                                   std::string & theShutdownCommand)
:   _myUDPPort(2342),
    _myReturnMessageFlag(false),
    _myReturnMessagePort(-1),
    _myApplication(theApplication),
    _myLogger(theLogger),
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
    if (theConfigNode->getAttribute("returnMessagePort")) {
        _myReturnMessagePort = asl::as<int>(theConfigNode->getAttribute("returnMessagePort")->nodeValue());
        AC_DEBUG << "_myReturnMessagePort: " << _myReturnMessagePort;
    }
    if (theConfigNode->getAttribute("returnmessage")) {
        _myReturnMessageFlag = asl::as<bool>(theConfigNode->getAttribute("returnmessage")->nodeValue());
        AC_DEBUG << "return message: " << _myReturnMessageFlag;
    }

    // check for system halt command configuration
    if (theConfigNode->childNode("SystemHalt")) {
        const dom::NodePtr & mySystemHaltNode = theConfigNode->childNode("SystemHalt");
        _mySystemHaltCommand = mySystemHaltNode->getAttributeString("command");
        AC_DEBUG <<"_mySystemHaltCommand: " << _mySystemHaltCommand;
    }

    // check for system reboot command configuration
    if (theConfigNode->childNode("SystemReboot")) {
        const dom::NodePtr & mySystemRebootNode = theConfigNode->childNode("SystemReboot");
        _mySystemRebootCommand = mySystemRebootNode->getAttributeString("command");
        AC_DEBUG <<"_mySystemRebootCommand: " << _mySystemRebootCommand;
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
        _myStopAppCommand = myStopAppNode->getAttributeString("command");
        AC_DEBUG <<"_myStopAppCommand: " << _myStopAppCommand;
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
}

UDPCommandListenerThread::~UDPCommandListenerThread() {
}

void
UDPCommandListenerThread::initiateShutdown() {

    if (_myShutdownCommand != "") {
        int myError = system(_myShutdownCommand.c_str());
        cerr << "shutdown command: \"" << _myShutdownCommand << "\" return with " << myError << endl;
    }

    initiateSystemShutdown();
}

void
UDPCommandListenerThread::initiateReboot() {

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
UDPCommandListenerThread::sendReturnMessage(asl::Unsigned32 theClientHost, asl::Unsigned16 theClientPort, const std::string & theMessage) {
    if (theMessage.empty() || !_myReturnMessageFlag) {
        return;
    }
    UDPSocket * myUDPClient = 0;
    try {
        for (unsigned int localPort = _myUDPPort+1; localPort <= MAX_PORT; localPort++)
        {
            try {
                myUDPClient = new UDPSocket(INADDR_ANY, localPort);
                break;
            }
            catch (SocketException & ) {
                myUDPClient = 0;
            }
        }
        if (myUDPClient) {
            myUDPClient->sendTo(theClientHost, theClientPort, theMessage.c_str(), theMessage.size());
            delete myUDPClient;
            cerr << "send message to client: " << theMessage << " to host:" << theClientHost << " on port #" << theClientPort << endl;
        }
    } catch (SocketException & se) {
        if (myUDPClient) {
            delete myUDPClient;
        }
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
    cout << "* UDP Listener return messages: " << _myReturnMessageFlag << " to sender port #" << _myReturnMessagePort << endl;
    cout << "* Commands:" << endl;
    cout << "    System Halt         : " << _mySystemHaltCommand << endl;
    cout << "    System Reboot       : " << _mySystemRebootCommand << endl;
    cout << "    Application restart : " << _myRestartAppCommand << endl;
    cout << "    Application stop    : " << _myStopAppCommand << endl;
    cout << "    Application start   : " << _myStartAppCommand << endl;
    cout << "    Application switch  : " << _mySwitchAppCommand << endl;

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
                if (_myReturnMessagePort != -1) {
                    clientPort = _myReturnMessagePort;
                }

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
                        sendReturnMessage(clientHost, clientPort, _mySystemHaltCommand);
                        initiateShutdown();
                    } else if (isCommand(myCommand, _mySystemRebootCommand)) {
                        cerr << "Client received reboot packet" << endl;
                        _myLogger.logToFile( string("Reboot from Network" ));
                        _myLogger.closeLogFile();
                        sendReturnMessage(clientHost, clientPort, _mySystemRebootCommand);
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
                        _myApplication.setPaused(true);
                        myMessage = _myStopAppCommand;
                    } else if (isCommand(myCommand, _myStartAppCommand)) {
                        cerr << "Client received start application packet" << endl;
                        _myLogger.logToFile( string( "Start application from Network" ));
                        _myApplication.setPaused(false);
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
                    } else {
                        ostringstream myOss;
                        myOss << "### UDPCommandListener: Unexpected packet '" << myCommand
                            << "'. Ignoring";
                        cerr << myOss.str() << endl;
                        _myLogger.logToFile( myOss.str() );
                    }
                    sendReturnMessage(clientHost, clientPort, myMessage);
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
