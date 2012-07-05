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


#include "parse_helpers.h"

#include <iostream>

#include <asl/base/file_functions.h>
#include <asl/base/os_functions.h>



bool
readConfigFile(dom::Document & theConfigDoc,  std::string theFileName) {
    AC_DEBUG << "Loading configuration data..." ;

    std::string myFileName = asl::expandEnvironment(theFileName);
    AC_DEBUG <<"fileName: " << myFileName;
    if (myFileName.empty()) {
        AC_DEBUG << "Watchdog::readConfigFile: Can't open configuration file "
             << myFileName << "." << std::endl;
        return false;
    }

    std::string myFileStr = asl::readFile(myFileName);
    if (myFileStr.empty()) {
        AC_DEBUG << "Watchdog::readConfigFile: Can't open configuration file "
             << myFileName << "." << std::endl;
        return false;
    }
    theConfigDoc.parseAll(myFileStr.c_str());
    if (!theConfigDoc) {
        AC_DEBUG << "Watchdog:::readConfigFile: Error reading configuration file "
             << myFileName << "." << std::endl;
        return false;
    }
    return true;
}

