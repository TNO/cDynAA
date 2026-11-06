/*
file: eventhandlertest.h

Definition of an EventHandlerTest class to facilitate testing.
*/

#ifndef _INCLUDED_EVENTHANDLERTEST_
#define _INCLUDED_EVENTHANDLERTEST_

#include <iostream>
#include "../include/eventhandler.h"


using namespace dynaa;

/**
 * * Test class for EventHandler interface
 * TODO: Re-write test
 */

int callbackFunctionCounter = 0;

inline void callback_function(Event e) {
    callbackFunctionCounter++;
}

class EventHandlerTest : public Entity {

public:

    int callbackMethodCounter = 0;

    void callback_method(Event event) {
        callbackMethodCounter++;
    }

    static void callback_static_method(Event event) {
        EventHandlerTest::callbackStaticCounter++;
    }

    static int callbackStaticCounter;
};

int EventHandlerTest::callbackStaticCounter = 0;

#endif
