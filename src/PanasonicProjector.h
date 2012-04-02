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
// Panasonic PT-D5500 projector
//
//=============================================================================

#ifndef _ac_watchdog_Panasonic_h_
#define _ac_watchdog_Panasonic_h_

#include "Projector.h"

#include <vector>
#include <string>

class PanasonicProjector : public Projector
{
public:
    explicit PanasonicProjector(int thePortNumber, int theBaud);

    virtual void configure(const dom::NodePtr & theConfigNode);

    virtual void power(bool thePowerFlag);
    virtual void selectInput(VideoSource theSource);
    virtual void lamps(unsigned theLampsMask);
    virtual void lampPower(bool thePowerHighFlag);
    virtual void shutter(bool theShutterFlag);
    virtual void update();

private:
    unsigned _myNumProjectors;
    unsigned _myFirstID;
    unsigned _myPowerDelay;
    unsigned _myReadTimeout;

    std::vector<std::string> _myResponses;

    void sendCommand(const std::string & theCommand, const std::string & theParams = "", unsigned theTimeout = 0);
    void sendCommandSingle(const std::string & theCommand, const std::string & theParams, unsigned theAddress);
    void readFromDevice(unsigned theTimeout = 300);

    void parseResponse(const std::string & thePattern, const std::string & theParamName, const std::string & theUnit = "");
};

#endif
