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

inline int a_callbackFunctionCounter = 0;

inline void a_callback_function(HandlerReport e) {
    a_callbackFunctionCounter++;
}

class SIMEventHandlerTest : public Entity {

public:

    int callbackMethodCounter = 0;

    void callback_method(HandlerReport event) {
        callbackMethodCounter++;
    }

    static void a_callback_static_method(HandlerReport event) {
        SIMEventHandlerTest::a_callbackStaticCounter++;
    }

    static void a_callback_that_stops_the_simulation(HandlerReport event) {
        DynAASim::getInstance().stop();
    }

    static int a_callbackStaticCounter;
};

inline int SIMEventHandlerTest::a_callbackStaticCounter = 0;

#endif
