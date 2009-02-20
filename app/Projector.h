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
// Projector controller class.
//
//=============================================================================

#ifndef _ac_watchdog_Projector_h_
#define _ac_watchdog_Projector_h_

#include <string>
#include <asl/dom/Nodes.h>
#include <y60/base/typedefs.h>

namespace asl {
    class SerialDevice;
}

class Logger;

class Projector {
public:
    /**
     * Factory method to get a projector
     * \note DEPRECATED! Only still used by the tests.
     * \param theType Type string i.e. nec, panasonic
     * \param thePort UDP-port
     * \return 
     */                            
    static Projector* getProjector(const std::string& theType, int thePort, int theBaud = -1);
    
    /**
     * Factory method to get a projector
     * \param theProjectorNode XML-Node from the config file
     * \param theLogger 
     * \return 
     */                            
    static Projector* getProjector(const dom::NodePtr & theProjectorNode, Logger* theLogger);

    explicit Projector(int thePortNumber, unsigned theBaud);
    virtual ~Projector();

    const std::string & getDescription() const { return _myDescription; }

    /// Logger.
    void setLogger(Logger* theLogger) { _myLogger = theLogger; }
    Logger* getLogger() const { return _myLogger; }
    
    /// Configure projector.
    virtual void configure(const dom::NodePtr & theConfigNode);

    /// Turn projector on/off.
    void powerUp() { power(true); }
    void powerDown() { power(false); }

    virtual void power(bool thePowerFlag) = 0;

    /// Input sources.
    enum VideoSource {
        NONE = 0,
        RGB_1,
        RGB_2,
        VIDEO,
        SVIDEO,
        DVI,
        M1,
        VIEWER,
        BNC
    };
    
    /**
     * Select the projector's input source
     * \param theSource A video source
     */                  
    void selectInput(const std::string& theSource);

    /**
     * Set input to initial value
     */         
    virtual void selectInput() {
        if (_myInitialInputSource != NONE) {
            selectInput(_myInitialInputSource);
        }
    };
    
    virtual void selectInput(VideoSource theSource) = 0;

    /**
     * Set the lamps mode
     * \param theLampsMask 
     */                  
    virtual void lamps(unsigned theLampsMask) {}

    /// Lamp power.
    virtual void lampPower(bool thePowerHighFlag) {}

    /// Shutter mode.
    virtual void shutter(bool theShutterOpenFlag) = 0;

    /// Handle command.
    void setCommandEnable(bool theEnableFlag) { _myCommandEnable = theEnableFlag; }
    bool getCommandEnable() const { return _myCommandEnable; }
    void setInitialInputSource(const VideoSource theInput) { _myInitialInputSource = theInput; }
    
    virtual bool command(const std::string & theCommand);

    /// Projector status update.
    virtual void update() {}
    

protected:
    std::string _myDescription;

    asl::SerialDevice* getDevice() {
        return _mySerialDevice;
    }

    VideoSource getEnumFromString(const std::string& theSource);
    std::string getStringFromEnum(const Projector::VideoSource theSource);
    unsigned getBaudRate() const { return _myBaud; }
    
private:
    asl::SerialDevice * _mySerialDevice;
    Logger *            _myLogger;
    bool                _myCommandEnable;
    VideoSource         _myInitialInputSource;
    unsigned            _myBaud;

    //Projector();
};

#endif
