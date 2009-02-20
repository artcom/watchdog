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
// Projector factory
//
//=============================================================================

#include "Projector.h"

#include <string>

#include "NecProjector.h"
#include "PdF1Projector.h"
#include "PanasonicProjector.h"
#include "HitachiProjector.h"

#include <asl/serial/SerialDeviceFactory.h>
#include <asl/serial/SerialDevice.h>
#include <asl/base/Exception.h>
#include <asl/dom/Nodes.h>


Projector* Projector::getProjector(const std::string& theType, int thePortNum, int theBaud)
{
    Projector* projector = 0;

    if (theType.size() == 0 || theType == "nec") {
        projector = new NecProjector(thePortNum, theBaud);
    }
    else if (theType == "pdf1") {
        projector = new PdF1Projector(thePortNum, theBaud);
    }
    else if (theType == "panasonic" || theType == "pt-d5500") {
        projector = new PanasonicProjector(thePortNum, theBaud);
    }
    else if (theType == "liesegang" || theType == "hitachi" || theType == "infocus") {
        projector = new HitachiProjector(thePortNum, theBaud);
    }
    else {
        throw asl::Exception(std::string("Unknown projector: ") + theType);
    }

    return projector;
}

Projector* Projector::getProjector(const dom::NodePtr & theProjectorNode, Logger* theLogger)
{
    Projector*  projector = 0;
    std::string myType  = "";
    int         myPort = -1;
    int         myBaud = -1;

    if (theProjectorNode->nodeType() == dom::Node::ELEMENT_NODE) {
        if (theProjectorNode->getAttribute("type")) {
            myType = theProjectorNode->getAttribute("type")->nodeValue();
        }

        if (theProjectorNode->getAttribute("port")) {
            myPort = asl::as<int>(theProjectorNode->getAttribute("port")->nodeValue());
        }

        if (theProjectorNode->getAttribute("baud")) {
            myBaud = asl::as<int>(theProjectorNode->getAttribute("baud")->nodeValue());
        }

        if (myPort != -1) {
            if (myType.size() == 0 || myType == "nec") {
                projector = new NecProjector(myPort, myBaud);
            }
            else if (myType == "pdf1") {
                projector = new PdF1Projector(myPort, myBaud);
            }
            else if (myType == "panasonic" || myType == "pt-d5500") {
                projector = new PanasonicProjector(myPort, myBaud);
            }
            else if (myType == "liesegang" || myType == "hitachi" || myType == "infocus") {
                projector = new HitachiProjector(myPort, myBaud);
            }
            else {
                throw asl::Exception(std::string("Unknown projector type: ") + myType);
            }
            projector->setLogger(theLogger);
            projector->configure(theProjectorNode);
        }
    }

    return projector;
}

void
Projector::configure(const dom::NodePtr & theConfigNode)
{
    AC_PRINT << "Projector::configure " << *theConfigNode;
    
    if (theConfigNode->getAttribute("input")) {
        _myInitialInputSource = getEnumFromString(theConfigNode->getAttribute("input")->nodeValue());
        AC_DEBUG << "_myInitialInputSource: " << getStringFromEnum(_myInitialInputSource);
    }
}

void
Projector::selectInput(const std::string& theSource)
{
    VideoSource mySource = getEnumFromString(theSource);
    if (mySource != NONE) {
	    AC_PRINT << "Projector::selectInput " << theSource;
        selectInput(mySource);
    }
}

bool
Projector::command(const std::string & theCommand)
{
    if (!_myCommandEnable) {
        AC_PRINT << "Projector::command disabled, ignoring '" << theCommand << "'";
        return true;
    }

    AC_PRINT << "Projector::command '" << theCommand << "'";
    if (theCommand == "projector_on") {
        // power on: I tell you three times...
        for (unsigned i = 0; i < 3; ++i) {
            power(true);
        }
    }
    else if (theCommand == "projector_off") {
        // power off: I tell you three times...
        for (unsigned i = 0; i < 3; ++i) {
            power(false);
        }
    }
    else if (theCommand == "projector_shutter_open") {
        shutter(true);
    }
    else if (theCommand == "projector_shutter_close") {
        shutter(false);
    }
    else if (theCommand == "projector_power_low") {
        lampPower(false);
    }
    else if (theCommand == "projector_power_high") {
        lampPower(true);
    }
    else if (theCommand.substr(0, 22) == "projector_input_select") {
        std::string::size_type myIndex = theCommand.find("=", 0);
        if (myIndex != std::string::npos) {
            std::string mySource = theCommand.substr(myIndex+1);
            selectInput(mySource);
        }
    }
    else {
        AC_PRINT << "Projector::command unknown '" << theCommand << "'";
        return false;
    }
    return true;
}


/*
 * Private
 */
Projector::Projector(int thePortNum, unsigned theBaud) :
    _mySerialDevice(0), _myLogger(0), _myCommandEnable(true),
    _myInitialInputSource(NONE), _myBaud(theBaud)
{
    if (thePortNum != -1) {
        _mySerialDevice = asl::getSerialDevice(thePortNum);
        if (!_mySerialDevice) {
            throw asl::Exception("Failed to get serial device", PLUS_FILE_LINE);
        }
	    //_mySerialDevice->setNoisy(true);
    }
}

Projector::~Projector()
{
    if (_mySerialDevice) {
        _mySerialDevice->close();
        delete _mySerialDevice;
        _mySerialDevice = 0;
    }
}


Projector::VideoSource
Projector::getEnumFromString(const std::string& theSource)
{
    if (theSource == "RGB_1" || theSource == "RGB") {
        return RGB_1;
    }
    if (theSource == "RGB_2") {
        return RGB_2;
    }
    if (theSource == "VIDEO") {
        return VIDEO;
    }
    if (theSource == "SVIDEO") {
        return SVIDEO;
    }
    if (theSource == "DVI") {
        return DVI;
    }
    if (theSource == "M1" ||
        theSource == "M1D" ||
        theSource == "M1-D") {
        return M1;
    }
    if (theSource == "VIEWER") {
        return VIEWER;
    }
    if (theSource == "BNC") {
        return BNC;
    }
    return NONE;
}

std::string
Projector::getStringFromEnum(const Projector::VideoSource theSource)
{
    if (theSource == RGB_1) {
        return "RGB_1";
    }
    if (theSource == RGB_2) {
        return "RGB_2";
    }
    if (theSource == VIDEO) {
        return "VIDEO";
    }
    if (theSource == SVIDEO) {
        return "SVIDEO";
    }
    if (theSource == DVI) {
        return "DVI";
    }
    if (theSource == M1) {
        return "M1";
    }
    if (theSource == VIEWER) {
        return "VIEWER";
    }
    if (theSource == BNC) {
        return "BNC";
    }
    return "NONE";
}
