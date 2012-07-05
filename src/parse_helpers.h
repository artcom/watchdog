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


#ifndef INCL_PARSE_HELPERS
#define INCL_PARSE_HELPERS

#include <string>
#include <asl/dom/Nodes.h>

bool readConfigFile(dom::Document & theConfigDoc,  std::string theFileName);

#endif

