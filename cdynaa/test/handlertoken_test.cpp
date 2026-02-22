#include "../include/handlertoken.h"
#include "catch.h"

using namespace std;
using namespace dynaa;

TEST_CASE("Test case for HandlerToken(s)", "[hregistry][stable]") {

    SECTION("Creation of HandlerTokens") {
        // Dummy handler tokens are equal to each other
        HandlerToken token_1;
        HandlerToken token_2;
        REQUIRE(token_1 == token_2);
        // Dummy handlers are the same in priority
        REQUIRE_FALSE(token_1 < token_2);

        // Factory (not for users!)
        HandlerToken token_3;
        REQUIRE_NOTHROW(token_3 = createHandlerToken(1,1));
        HandlerToken token_4;
        REQUIRE_NOTHROW(token_4 = createHandlerToken(2,2));
        REQUIRE(token_3 != token_4);
        REQUIRE(token_3 < token_4);
    }
}

TEST_CASE("Test case for HandlerToken(s) value semantics", "[hregistry][stable]") {
    HandlerToken token_1 = createHandlerToken(1,5);
    HandlerToken token_2 = createHandlerToken(2,5);
    HandlerToken token_copy;

    SECTION("HandlerToken copy assignment") {
        REQUIRE(token_1 != token_2);
        REQUIRE(token_1 != token_copy);
        REQUIRE_NOTHROW(token_copy = token_1);
        REQUIRE(token_1 == token_copy);
    }

    SECTION("HandlerToken copy construction") {
        HandlerToken copy_constructed_handler_token(token_1);
        REQUIRE(token_1 == copy_constructed_handler_token);
    }

    SECTION("Assignment copy construction") {
        HandlerToken assignment_constructed_handler_token = token_1;
        REQUIRE(assignment_constructed_handler_token == token_1);
    }

    SECTION("Move constructor and move assignment") {
        HandlerToken handler_token_copy(std::move(token_1));
        REQUIRE(handler_token_copy == token_1);
        REQUIRE(&handler_token_copy != &token_1);
    }


}
