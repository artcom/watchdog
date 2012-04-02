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
// projectiondesign F1 Projector controller.
//
//=============================================================================

#ifndef _ac_watchdog_PdF1Projector_h_
#define _ac_watchdog_PdF1Projector_h_

#include "Projector.h"

class PdF1Projector : public Projector
{
public:
    explicit PdF1Projector(int thePortNumber, int theBaud);

    virtual void power(bool thePowerFlag);
    virtual void selectInput(VideoSource theSource);
    virtual void shutter(bool theShutterFlag) {};

private:
    bool checkHeader(const unsigned char* packet) const;

    void setOpValue(unsigned char* packet, unsigned short value);
    void sendPacket(unsigned char* packet, unsigned int packetLen);
};

#endif
