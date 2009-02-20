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
// NEC Projector controller.
//
//=============================================================================

#include "NecProjector.h"

#include <asl/serial/SerialDevice.h>
#include <asl/base/Exception.h>
#include <asl/base/Time.h>
#include <asl/base/Logger.h>
#include <asl/base/string_functions.h>

#include <iostream>

const char NEC_POWER_UP[]      = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x02 };
const char NEC_POWER_DOWN[]    = { 0x02, 0x01, 0x00, 0x00, 0x00, 0x03 };
const char NEC_INPUT_SELECT[]  = { 0x02, 0x03, 0x00, 0x00, 0x02, 0x01 };
const char NEC_INPUT_RGB_1[]   = { 0x01, 0x09 };
const char NEC_INPUT_RGB_2[]   = { 0x02, 0x0A };
const char NEC_INPUT_VIDEO[]   = { 0x06, 0x0E };
const char NEC_INPUT_SVIDEO[]  = { 0x0B, 0x13 };
const char NEC_INPUT_DVI[]     = { 0x1A, 0x22 };
const char NEC_INPUT_VIEWER[]  = { 0x1F, 0x27 };
const char NEC_SHUTTER_OPEN[]  = { 0x02, 0x11, 0x00, 0x00, 0x00, 0x13 };
const char NEC_SHUTTER_CLOSE[] = { 0x02, 0x10, 0x00, 0x00, 0x00, 0x12 };

using namespace std;

NecProjector::NecProjector(int thePortNum, int theBaud) : Projector(thePortNum, theBaud == -1 ? 38400 : theBaud)
{
    asl::SerialDevice * myDevice = getDevice();
    if (!myDevice) {
        throw asl::Exception("Failed to get serial device!", PLUS_FILE_LINE);
    }
    myDevice->open(getBaudRate(), 8, asl::SerialDevice::NO_PARITY, 1);
    _myDescription = "NEC on port: " + asl::as_string(thePortNum) + " ("+asl::as_string(getBaudRate())+"baud,8,N,1)"; 
}


void
NecProjector::power(bool thePowerFlag) {
    asl::SerialDevice * myDevice = getDevice();
    if (!myDevice)
        return;

    if (thePowerFlag) {
        AC_DEBUG << "NecProjector::powerUp - 1" ;
        myDevice->write(NEC_POWER_UP, 6);
        asl::msleep(1000);
        myDevice->close();
        myDevice->open(getBaudRate(), 8, asl::SerialDevice::NO_PARITY, 1);
        asl::msleep(1000);
        AC_DEBUG << "NecProjector::powerUp - 2" ;
        myDevice->write(NEC_POWER_UP, 6);
    }
    else {
        myDevice->write(NEC_POWER_DOWN, 6);
        AC_DEBUG << "NecProjector::powerDown" ;
    }
}


void
NecProjector::selectInput(VideoSource theVideoSource) {
    asl::SerialDevice * myDevice = getDevice();
    if (!myDevice)
        return;

    AC_PRINT << "NecProjector::selectInput " << getStringFromEnum(theVideoSource);

    myDevice->write(NEC_INPUT_SELECT, 6);
    switch (theVideoSource) {
    case RGB_1 :
        myDevice->write(NEC_INPUT_RGB_1, 2);
        break;
    case RGB_2 :
        myDevice->write(NEC_INPUT_RGB_2, 2);
        break;
    case VIDEO :
        myDevice->write(NEC_INPUT_VIDEO, 2);
        break;
    case SVIDEO :
        myDevice->write(NEC_INPUT_SVIDEO, 2);
        break;
    case DVI :
        myDevice->write(NEC_INPUT_DVI, 2);
        break;
    case VIEWER :
        myDevice->write(NEC_INPUT_VIEWER, 2);
        break;
    default:
        AC_WARNING << "Unknown projector input source '" << getStringFromEnum(theVideoSource) << "'.";
    }
}


void
NecProjector::shutter(bool theShutterOpenFlag) {
    asl::SerialDevice * myDevice = getDevice();
    if (!myDevice)
        return;

    if (theShutterOpenFlag) {
        myDevice->write(NEC_SHUTTER_OPEN, 6);
    } else {
        myDevice->write(NEC_SHUTTER_CLOSE, 6);
    }
}
