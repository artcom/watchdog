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
// NEC Projector controller.
//
//=============================================================================

#ifndef _ac_watchdog_NecProjector_h_
#define _ac_watchdog_NecProjector_h_

#include "Projector.h"

class NecProjector : public Projector
{
public:
    explicit NecProjector(int thePortNumber, int theBaud);

    virtual void power(bool thePowerFlag);
    virtual void selectInput(VideoSource theSource);

    /**
     * Open/Close the shutter
     * \note For NEC projectors do not have a shutter
     *       this enables/disables the PICTURE MUTE
     * \param theShutterFlag
     */
    virtual void shutter(bool theShutterFlag);
};

#endif
