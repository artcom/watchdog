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

//
// Logger.h : A simple Logger for watchdogs and bones
//
#ifndef INCL_LOGGER
#define INCL_LOGGER

#include <fstream>
#include <string>
#include <asl/base/ThreadLock.h>

class Logger {
    public:
        Logger();
        ~Logger();
        void openLogFile(const std::string & theLogFilename);
        void closeLogFile();
        void logToFile(const std::string& theMessage);
    private:
        std::ofstream * _myFile;
        asl::ThreadLock _myLock;
};
#endif // INCL_LOGGER
