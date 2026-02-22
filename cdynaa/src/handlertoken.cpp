/*
file: handlertoken.cpp
*/

#include "../include/handlertoken.h"
#include <iostream>

using namespace std;
using namespace dynaa;

// Identifier for an invalid (dummy) handler token
const TokenIDType HandlerToken::INVALID_TOKEN_ID = -1;
const HandlerToken HandlerToken::INVALID_TOKEN;
const int HandlerToken::INVALID_PRIORITY_LEVEL = -1;

HandlerToken dynaa::createHandlerToken(const TokenIDType& token_id, const int& priority_level) {
    HandlerToken token;
    token.m_token = token_id;
    token.m_priority = priority_level;
    return token;
}

/**
* Overrides the comparison (equality) operator
*/
bool HandlerToken::operator==(const HandlerToken& other) const {
    return m_token == other.m_token;
}

/**
* Overrides the comparison (difference) operator
*/
bool HandlerToken::operator!=(const HandlerToken& other) const {
    return !(*this == other);
}

/**
 * Overrides the comparison (smaller than) operator
 */
bool HandlerToken::operator<(const HandlerToken& other) const {
    if (m_token == other.m_token)
        return false;
    return (m_priority == other.m_priority) ? (m_token < other.m_token) : (m_priority < other.m_priority);
}

/**
 * Hashing function
 */
size_t HandlerToken::operator()(const HandlerToken& token) const {
    return token.m_token;
};
