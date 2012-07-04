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

#include <asl/base/string_functions.h>
#include <asl/base/file_functions.h>



void
readConfigFile(dom::Document & theConfigDoc,  std::string theFileName) {
    AC_DEBUG << "Loading configuration data..." ;
    std::string myFileStr = asl::readFile(theFileName);
    if (myFileStr.empty()) {
        std::cerr << "Watchdog::readConfigFile: Can't open configuration file "
             << theFileName << "." << std::endl;
        exit(-1);
    }
    theConfigDoc.parseAll(myFileStr.c_str());
    if (!theConfigDoc) {
        std::cerr << "Watchdog:::readConfigFile: Error reading configuration file "
             << theFileName << "." << std::endl;
        exit(-1);
    }
}

