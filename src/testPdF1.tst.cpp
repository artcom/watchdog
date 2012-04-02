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


#include <asl/base/UnitTest.h>
#include <asl/base/Ptr.h>

#include "Projector.h"


const int myPort = 0; // if this doesn´t exist under Windows the test will fail


class TestPowerUp: public UnitTest {
public:
    TestPowerUp() : UnitTest ("TestPowerUp") {}

    virtual void run() {

	asl::Ptr<Projector> myProjector(Projector::getProjector("pdf1", myPort));
        ENSURE(myProjector != 0);
	myProjector->powerUp();
    }
};

class TestInput: public UnitTest {
public:
    TestInput() : UnitTest ("TestInput") {}

    virtual void run() {

	asl::Ptr<Projector> myProjector(Projector::getProjector("pdf1", myPort));
        ENSURE(myProjector != 0);
	myProjector->selectInput("RGB_1");
    }
};

class TestPowerDown: public UnitTest {
public:
    TestPowerDown() : UnitTest ("TestPowerDown") {}

    virtual void run() {

	asl::Ptr<Projector> myProjector(Projector::getProjector("pdf1", myPort));
        ENSURE(myProjector != 0);
	myProjector->powerDown();
    }
};


int main( int argc, char *argv[] )
{
    UnitTestSuite mySuite ("PdF1 Projector tests", argc, argv);

    try {
       //mySuite.addTest(new TestPowerUp);
       mySuite.addTest(new TestInput);
       mySuite.addTest(new TestPowerDown);
       mySuite.run();
    }
    catch (const asl::Exception& e) {
        std::cerr << "!!! " << e << std::endl;
    }

    return mySuite.returnStatus();
}
