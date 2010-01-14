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
//  Description: A little logger class for dogs and bones
//
//=============================================================================

#include "Logger.h"

#include <ctime>
#include <iostream>
#include <asl/base/Auto.h>

using namespace std;

Logger::Logger() : _myFile(0) {}
Logger::~Logger() {
    if (_myFile) {
        delete _myFile;
    }
}
void
Logger::openLogFile(const std::string & theLogFilename) {
    if (_myFile) {
        delete _myFile;
    }
    if (!theLogFilename.empty()) {
         _myFile = new ofstream(theLogFilename.c_str(), ios::out|ios::app);
        if (!_myFile) {
            cerr << string("Watchdog - Could not open log file ") + theLogFilename << endl;
        } else {
            (*_myFile) << "---------------------------------------------------------------------" << endl;
        }
    }
}

void
Logger::closeLogFile() {
    asl::AutoLocker<asl::ThreadLock> myLocker(_myLock);
    if (_myFile) {
        _myFile->close();
    }
}

void
Logger::logToFile(const string& theMessage) {
    asl::AutoLocker<asl::ThreadLock> myLocker(_myLock);
    if (_myFile) {
        time_t myTime;
        time(&myTime);
        struct tm * myPrintableTime = localtime(&myTime);
        (*_myFile) << asctime(myPrintableTime)
                   << "threadid: " << std::hex << (unsigned int)pthread_self() << std::dec << "\n"
                   << theMessage << "\n"
                   << "---------------------------------------------------------------------" << endl;
    }
}


