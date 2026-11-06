/*
file: simenginetest.h

Definition of an EventHandlerTest class to facilitate testing in the SimEngine.
*/

#ifndef _INCLUDED_SIMENGINETEST_
#define _INCLUDED_SIMENGINETEST_

#include <iostream>
#include "../include/eventhandler.h"


using namespace dynaa;

/**
 * * Test class for EventHandler interface
 * TODO: Re-write test
 */

int a_callbackFunctionCounter = 0;

void a_callback_function(Event e) {
    a_callbackFunctionCounter++;
}

class SIMEventHandlerTest : public Entity {

public:

    int callbackMethodCounter = 0;

    void callback_method(Event event) {
        callbackMethodCounter++;
    }

    static void a_callback_static_method(Event event) {
        SIMEventHandlerTest::a_callbackStaticCounter++;
    }

    static void a_callback_that_stops_the_simulation(Event event) {
        DynAASim::getInstance().stop();
    }

    static int a_callbackStaticCounter;
};

int SIMEventHandlerTest::a_callbackStaticCounter = 0;

#endif
