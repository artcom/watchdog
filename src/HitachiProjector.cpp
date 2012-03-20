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
// Hitachi Projector controller.
//
//=============================================================================

#include "HitachiProjector.h"

#include <asl/serial/SerialDevice.h>
#include <asl/base/Exception.h>
#include <asl/base/Time.h>
#include <asl/base/Logger.h>
#include <asl/base/string_functions.h>

#include <iostream>

const unsigned char LIES_POWER_UP[]        = { 0xBE, 0xEF, 0x03, 0x06, 0x00, 0xBA, 0xD2, 0x01, 0x00, 0x00, 0x60, 0x01, 0x00 };
const unsigned char LIES_POWER_DOWN[]      = { 0xBE, 0xEF, 0x03, 0x06, 0x00, 0x2A, 0xD3, 0x01, 0x00, 0x00, 0x60, 0x00, 0x00 };

const unsigned char LIES_INPUT_RGB_1[]     = { 0xBE, 0xEF, 0x03, 0x06, 0x00, 0xFE, 0xD2, 0x01, 0x00, 0x00, 0x20, 0x00, 0x00 };
const unsigned char LIES_INPUT_VIDEO[]     = { 0xBE, 0xEF, 0x03, 0x06, 0x00, 0x6E, 0xD3, 0x01, 0x00, 0x00, 0x20, 0x01, 0x00 };
const unsigned char LIES_INPUT_SVIDEO[]    = { 0xBE, 0xEF, 0x03, 0x06, 0x00, 0x9E, 0xD3, 0x01, 0x00, 0x00, 0x20, 0x02, 0x00 };
const unsigned char LIES_INPUT_M1[]        = { 0xBE, 0xEF, 0x03, 0x06, 0x00, 0x0E, 0xD2, 0x01, 0x00, 0x00, 0x20, 0x03, 0x00 };
const unsigned char LIES_INPUT_BNC[]       = { 0xBE, 0xEF, 0x03, 0x06, 0x00, 0x3E, 0xD0, 0x01, 0x00, 0x00, 0x20, 0x04, 0x00 };
//const unsigned char LIES_INPUT_COMPONENT[] = { 0xBE, 0xEF, 0x03, 0x06, 0x00, 0xAE, 0xD1, 0x01, 0x00, 0x00, 0x20, 0x05, 0x00 };

const unsigned char LIES_SHUTTER_OPEN[]    = { 0xBE, 0xEF, 0x03, 0x06, 0x00, 0xFB, 0xD8, 0x01, 0x00, 0x20, 0x30, 0x00, 0x00 };
const unsigned char LIES_SHUTTER_CLOSE[]   = { 0xBE, 0xEF, 0x03, 0x06, 0x00, 0x6B, 0xD9, 0x01, 0x00, 0x20, 0x30, 0x01, 0x00 };

using namespace std;

HitachiProjector::HitachiProjector(int thePortNum, int theBaud) : Projector(thePortNum, theBaud == -1 ? 19200 : theBaud)
{
    asl::SerialDevice * myDevice = getDevice();
    if (!myDevice) {
        throw asl::Exception("Failed to get serial device!", PLUS_FILE_LINE);
    }
    myDevice->open(getBaudRate(), 8, asl::SerialDevice::NO_PARITY, 1);
    _myDescription = "Hitachi on port: " + asl::as_string(thePortNum) + " ("+asl::as_string(getBaudRate())+"baud,8,N,1)";
}


void
HitachiProjector::power(bool thePowerFlag) {
    asl::SerialDevice * myDevice = getDevice();
    if (!myDevice)
        return;

    if (thePowerFlag) {
        AC_DEBUG << "HitachiProjector::powerUp - 1" ;
        myDevice->write((const char*) LIES_POWER_UP, 13);
        asl::msleep(1000);
        myDevice->close();
        myDevice->open(getBaudRate(), 8, asl::SerialDevice::NO_PARITY, 1);
        asl::msleep(1000);
        AC_DEBUG << "HitachiProjector::powerUp - 2" ;
        myDevice->write((const char*) LIES_POWER_UP, 13);
    }
    else {
        myDevice->write((const char*) LIES_POWER_DOWN, 13);
        AC_DEBUG << "HitachiProjector::powerDown" ;
    }
}


void
HitachiProjector::selectInput(VideoSource theVideoSource) {
    asl::SerialDevice * myDevice = getDevice();
    if (!myDevice)
        return;

    switch (theVideoSource) {
    case RGB_1 :
        myDevice->write((const char*) LIES_INPUT_RGB_1, 13);
        break;
    case BNC :
        myDevice->write((const char*) LIES_INPUT_BNC, 13);
        break;
    case VIDEO :
        myDevice->write((const char*) LIES_INPUT_VIDEO, 13);
        break;
    case SVIDEO :
        myDevice->write((const char*) LIES_INPUT_SVIDEO, 13);
        break;
    case DVI :
    case M1 :
        myDevice->write((const char*) LIES_INPUT_M1, 13);
        break;
    default:
        AC_WARNING << "Unknown projector input source '" << getStringFromEnum(theVideoSource) << "'.";
    }
}


void
HitachiProjector::shutter(bool theShutterOpenFlag) {
    asl::SerialDevice * myDevice = getDevice();
    if (!myDevice)
        return;

    if (theShutterOpenFlag) {
        myDevice->write((const char*) LIES_SHUTTER_OPEN, 13);
    } else {
        myDevice->write((const char*) LIES_SHUTTER_CLOSE, 13);
    }
}
