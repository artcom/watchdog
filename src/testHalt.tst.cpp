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
#include <asl/net/UDPSocket.h>
#include <asl/net/net.h>
#include <asl/base/Logger.h>

using namespace std;
using namespace inet;

class TestUDPHalt: public UnitTest {
public:
    TestUDPHalt()
        : UnitTest ("TestUDPHalt")
    {}

    virtual void run() {
        int serverPort = 2342;
        UDPSocket myUDPClient = UDPSocket(INADDR_ANY, 2343);
        unsigned long inHostAddress = getHostAddress("localhost");

        try {
            myUDPClient.sendTo(inHostAddress, serverPort, "halt", 5);
            SUCCESS("sent UDP packet");
        } catch (SocketException & se) {
            cerr << se.what() << endl;
            FAILURE("could not send UDP packet");
        }
    }
};


int main( int argc, char *argv[] ) {
    UnitTestSuite mySuite("UDP halt tests", argc, argv);

    mySuite.addTest(new TestUDPHalt);
    mySuite.run();

    AC_DEBUG << ">> Finished test '" << argv[0] << "'"
          << ", return status = " << mySuite.returnStatus();

    return mySuite.returnStatus();
}


