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
// Panasonic PT-D5500 projector
//
//=============================================================================

#include "PanasonicProjector.h"
#include "Logger.h"

#include <asl/serial/SerialDevice.h>
#include <asl/base/string_functions.h>
#include <asl/base/Exception.h>
#include <asl/base/Time.h>
#include <asl/dom/Nodes.h>

#include <iostream>
#include <sstream>

using namespace std;

PanasonicProjector::PanasonicProjector(int thePortNum, int theBaud) : Projector(thePortNum, theBaud == -1 ? 9600 : theBaud),
    _myNumProjectors(2),
    _myFirstID(1),
    _myPowerDelay(2000),
    _myReadTimeout(250)
{
    asl::SerialDevice * myDevice = getDevice();
    if (!myDevice) {
        throw asl::Exception("Failed to get serial device!", PLUS_FILE_LINE);
    }
    myDevice->open(getBaudRate(), 8, asl::SerialDevice::NO_PARITY, 1);
    _myDescription = "Panasonic PT-D5500 on port : " + asl::as_string(thePortNum) + " ("+asl::as_string(getBaudRate())+"baud,8,N,1)";
}


void
PanasonicProjector::configure(const dom::NodePtr & theConfigNode)
{
    Projector::configure(theConfigNode);
    //cerr << "PanasonicProjector::configure " << *theConfigNode << endl;

    dom::NodePtr myNode;
    if ((myNode = theConfigNode->childNode("NumProjectors"))) {
        _myNumProjectors = asl::as<int>(myNode->childNode(0)->nodeValue());
    }

    if ((myNode = theConfigNode->childNode("FirstID"))) {
        _myFirstID = asl::as<int>(myNode->childNode(0)->nodeValue());
    }

    if ((myNode = theConfigNode->childNode("PowerDelay"))) {
        _myPowerDelay = asl::as<int>(myNode->childNode(0)->nodeValue());
    }

    if ((myNode = theConfigNode->childNode("ReadTimeout"))) {
        _myReadTimeout = asl::as<int>(myNode->childNode(0)->nodeValue());
    }

    if ((myNode = theConfigNode->childNode("Input"))) {
        Projector::selectInput(myNode->childNode(0)->nodeValue());
    }

    if ((myNode = theConfigNode->childNode("Lamps"))) {
        lamps(asl::as<int>(myNode->childNode(0)->nodeValue()));
    }

    if ((myNode = theConfigNode->childNode("LampPower"))) {
        // OLP:0 = High, OLP:1 = Low
    }
}

void
PanasonicProjector::power(bool thePowerFlag)
{
    cerr << "PanasonicProjector::power " << (thePowerFlag ? "on" : "off") << endl;
    sendCommand((thePowerFlag ? "PON" : "POFF"), "", _myPowerDelay);
}

void
PanasonicProjector::selectInput(VideoSource theVideoSource)
{
    std::string myParams;
    switch (theVideoSource) {
    case RGB_1:
        myParams = "RG1";
        break;
    case RGB_2:
        myParams = "RG2";
        break;
    case VIDEO:
        myParams = "VID";
        break;
    case SVIDEO:
        myParams = "SVD";
        break;
    case DVI:
        myParams = "DVI";
        break;
    default:
        AC_WARNING << "Unknown projector input source '" << getStringFromEnum(theVideoSource) << "'.";
        return;
    }
    sendCommand("IIS", myParams);
}

void
PanasonicProjector::lamps(unsigned theLampsMask)
{
    cerr << "PanasonicProjector::lamps " << theLampsMask << endl;
    std::string myParams;
    switch (theLampsMask) {
    case 0:
        myParams = "1"; // SINGLE
        break;
    case 1:
        myParams = "2"; // LAMP 1
        break;
    case 2:
        myParams = "3"; // LAMP 2
        break;
    case 3:
        myParams = "0"; // DUAL
        break;
    }
    sendCommand("LPM", myParams);
}

void
PanasonicProjector::lampPower(bool thePowerHighFlag)
{
    cerr << "PanasonicProjector::lampPower power=" << thePowerHighFlag << endl;
    sendCommand("OLP", (thePowerHighFlag ? "0" : "1"));
}

void
PanasonicProjector::shutter(bool theShutterOpenFlag)
{
    cerr << "PanasonicProjector::shutter open=" << theShutterOpenFlag << endl;
    sendCommand("OSH", (theShutterOpenFlag ? "0" : "1"));
    asl::msleep(1000);
    sendCommand("OSH", (theShutterOpenFlag ? "0" : "1"));
}

void
PanasonicProjector::update()
{
    //cerr << "PanasonicProjector::update" << endl;

    // power mode
    sendCommand("QPW","");
    parseResponse("%03d", "Power");

    // runtime
    sendCommand("QST","");
    parseResponse("%05d", "Runtime", "h");

    // temp input air
    sendCommand("QTM","0");
    parseResponse("%04d", "Input Air", "C");

#if 0 // NOT AVAILABLE ON PT-D5500
    // temp output air
    sendCommand("QTM","1",0);
    parseResponse("%04d", "Output Air", "C");
#endif

    // temp optical module
    sendCommand("QTM","2",0);
    parseResponse("%04d", "Optical Module", "C");
}


/**********************************************************************
 *
 * Private
 *
 **********************************************************************/

#ifdef WIN32
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif

const char STX[] = "\002";
const char ETX[] = "\003";

void
PanasonicProjector::sendCommand(const string & theCommand, const string & theParams, unsigned theTimeout)
{
    asl::SerialDevice * myDevice = getDevice();
    if (!myDevice) {
        return;
    }

    _myResponses.clear();
    for (unsigned i = 0; i < _myNumProjectors; ++i) {
        sendCommandSingle(theCommand, theParams, i);
        asl::msleep(theTimeout);
    }
}

void
PanasonicProjector::sendCommandSingle(const string & theCommand, const string & theParams, unsigned theAddress)
{
    // address
    char myAddress[] = "ZZ";
    if (theAddress != ~0u) {
        SNPRINTF(myAddress, sizeof(myAddress), "%02d", theAddress+_myFirstID);
    }

    // assemble buffer
    char myBuf[256];
    SNPRINTF(myBuf, sizeof(myBuf), "%sAD%s;%s", STX, myAddress, theCommand.c_str());
    if (!theParams.empty()) {
        strcat(myBuf, ":");
        strcat(myBuf, theParams.c_str());
    }
    strcat(myBuf, ETX);

    //cerr << "PanasonicProjector::sendCommand buf='" << myBuf << "'" << endl;
    getDevice()->write(myBuf, strlen(myBuf));
    readFromDevice(_myReadTimeout);
}

void
PanasonicProjector::readFromDevice(unsigned theTimeout)
{
    //cerr << "PanasonicProjector::readFromDevice" << endl;

    char myReadBuffer[1024];
    unsigned myReadBufferIndex = 0;

    char myBuf[256] = "";
    unsigned myTimeout = 0;
    while (myTimeout < theTimeout) {

        size_t myBufLen = sizeof(myBuf);
        bool myReadErr = getDevice()->read(myBuf, myBufLen);
        if (myReadErr) {
            cerr << "Error reading from device" << endl;
            break;
        }
        //cerr << "len=" << myBufLen << endl;
        if (myBufLen == 0) {
            const unsigned myDelay = 100;
            asl::msleep(myTimeout);
            myTimeout += myDelay;
            continue;
        }
        for (unsigned i = 0; i < myBufLen; ++i) {
            char myChar = myBuf[i];
            if (myChar == 0) {
                continue;
            }
            //cerr << hex << (int) myChar << dec << "=" << myChar << endl;
            if (myChar == STX[0]) {
                myReadBufferIndex = 0;
            }
            myReadBuffer[myReadBufferIndex++] = myChar;
            if (myChar == ETX[0]) {
                myReadBuffer[myReadBufferIndex++] = '\0';
                _myResponses.push_back(std::string(myReadBuffer));
                myReadBufferIndex = 0;
                continue;
            }
        }
		// XXX MAYBE RESET myTimeout=0
    }
}

void
PanasonicProjector::parseResponse(const std::string & thePattern, const std::string & theParamName, const std::string & theUnit)
{
	std::stringstream s;
    s << "PanasonicProjector " << theParamName;
    for (unsigned i = 0; i < _myResponses.size(); ++i) {

        const char* myData = _myResponses[i].c_str() + 1;
        int myValue;

        s << " ID" << (i + _myFirstID) << ":";
        if (sscanf((char*) myData, thePattern.c_str(), &myValue) == 1) {
			s << myValue << theUnit;
        }
        else {
            s << "??? (" << _myResponses[i] << ")";
        }
    }
    //cerr << s.str() << endl;
#if 1
	if (getLogger()) {
		getLogger()->logToFile(s.str());
	}
#endif
    _myResponses.clear();
}
