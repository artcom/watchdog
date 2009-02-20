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
// projectiondesign F1 Projector controller.
//
//=============================================================================

#include "PdF1Projector.h"
#include "CRC16.h"

#include <asl/serial/SerialDevice.h>
#include <asl/base/string_functions.h>
#include <asl/base/Logger.h>
#include <asl/base/Exception.h>

#include <iostream>


// Packet size
const unsigned int PACKET_SIZE = 32;

/*
 * STATE OP_VALUE
 * ON    0x0001
 * OFF   0x0000
 */
const unsigned char F1_POWER_ON_OFF[] = {
    0xBE, 0xEF, 0x03, 0x19, 0x00, // header
    0x82, 0x14,                   // CRC lo,hi
    0x01,                         // SET
    0x9C, 0x02                    // operation id
};

/*
 * INPUT           OP_VALUE
 * VGA1            0x0000
 * VGA2            0x0001
 * DVI             0x0002
 * Component       0x0003
 * S-Video         0x0004
 * Composite Video 0x0005
 * Composite HD    0x0006
 */
const unsigned char F1_SELECT_INPUT[] = {
    0xBE, 0xEF, 0x03, 0x19, 0x00,
    0xEA, 0xE9,
    0x01,
    0x01, 0x44
};


PdF1Projector::PdF1Projector(int thePortNum, int theBaud) : Projector(thePortNum, theBaud == -1 ? 19200 : theBaud)
{
    asl::SerialDevice * myDevice = getDevice();
    if (!myDevice) {
        throw asl::Exception("Failed to get serial device!", PLUS_FILE_LINE);
    }
    myDevice->open(getBaudRate(), 8, asl::SerialDevice::NO_PARITY, 1);
    _myDescription = "Projection Design F1 on port: " + asl::as_string(thePortNum) + " ("+asl::as_string(getBaudRate())+"baud,8,N,1)"; 
}


void
PdF1Projector::power(bool thePowerFlag)
{
    std::cerr << "PdF1Projector::power " << (thePowerFlag ? "on" : "off") << std::endl;

    unsigned char packet[PACKET_SIZE];
    memset(packet, 0, sizeof(packet));
    memcpy(packet, F1_POWER_ON_OFF, sizeof(F1_POWER_ON_OFF));

    setOpValue(packet, (unsigned short) thePowerFlag);
    sendPacket(packet, sizeof(packet));
}

void
PdF1Projector::selectInput(VideoSource theVideoSource)
{
    unsigned char packet[PACKET_SIZE];
    memset(packet, 0, sizeof(packet));
    memcpy(packet, F1_SELECT_INPUT, sizeof(F1_SELECT_INPUT));

    unsigned short value = 0;
    switch (theVideoSource) {
    case RGB_1:
        value = 0x0000;
        break;
    case RGB_2:
        value = 0x0001;
        break;
    case DVI:
        value = 0x0002;
        break;
    case SVIDEO:
        value = 0x0004;
        break;
    case VIDEO:
        value = 0x0005;
        break;
    default:
        AC_WARNING << "Unknown projector input source '" << getStringFromEnum(theVideoSource) << "'.";
        return;
    }

    setOpValue(packet, value);
    sendPacket(packet, sizeof(packet));
}


/**********************************************************************
 *
 * Private
 *
 **********************************************************************/

bool PdF1Projector::checkHeader(const unsigned char* packet) const
{
    if (packet[0] != (unsigned char) 0xBE || packet[1] != (unsigned char) 0xEF) {
        std::cerr << "### Invalid packet header" << std::endl;
        return false;
    }

    return true;
}

void PdF1Projector::setOpValue(unsigned char* packet, unsigned short value)
{
    packet[16] = value & 0xff;
    packet[17] = (value >> 8);
}

void PdF1Projector::sendPacket(unsigned char* packet, unsigned int packetLen)
{
    asl::SerialDevice * myDevice = getDevice();
    if (!myDevice) {
        throw asl::Exception("I´m not even supposed to be here today.", PLUS_FILE_LINE);
    }

#if 1 
    // sanity checks
    if (packetLen != PACKET_SIZE) {
        std::cerr << "### Unexpected packet size" << std::endl;
    }
    if (!checkHeader(packet)) {
        std::cerr << "### Unexpected packet header" << std::endl;
    }
#endif

    // pre-computed CRC
#if 0
    unsigned short givenCrc = (packet[6] << 8) | packet[5];
#endif
    packet[5] = packet[6] = 0x00;

    // calculate CRC, check against pre-computed
    unsigned short crc = asl::CRC16(packet, packetLen);
#if 0
    if (givenCrc != 0 && crc != givenCrc) {
        std::cerr << "### CRC mismatch: given=0x" << std::hex << givenCrc << " calc=0x" << crc << std::dec << std::endl;
    }
#endif
    packet[5] = crc & 0xFF;
    packet[6] = (crc >> 8);

    myDevice->write((const char*) packet, packetLen);
}
