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
                   << "threadid: " << std::hex << (size_t)pthread_self() << std::dec << "\n"
                   << theMessage << "\n"
                   << "---------------------------------------------------------------------" << endl;
    }
}


