// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_HANDLERTOKEN_
#define INCLUDED_HANDLERTOKEN_

#include "dynaatypes.h"
#include <cstddef>

/* Definition of the HandlerToken class */

/**
 * HandlerToken is simple token that gives access to a registered record in the core.
 *
 * HandlerToken is kept extremely simple so it is cheap to pass around.
 *
 * The internal identifier is a simple integer, but this is hidden from users.
 * Users are able to compare equality on HandlerTokens and equal HandlerTokens
 * give access to the same registered record.
 *
 * HandlerToken offers also some standard compare functions (<, hash) so they
 * can be used in sets, hashmaps, etc.
 *
 * HandlerTokens can be ordered according to the priority of their corresponding
 * handlers.
 *
 * The design of HandlerToken makes it also copiable, assignable, etc.
 */

namespace dynaa {

typedef long TokenIDType;

class HandlerToken {
    /**
     * Factory method - should be used only by restricted "friends", such as handlerrecord.
     */
    friend HandlerToken createHandlerToken(const TokenIDType& token_id, const int& priority_level);

private:
    //The identifier
    TokenIDType m_token;

    //The priority of this token, reflects the handler priority
    int m_priority;

public:
    static const TokenIDType INVALID_TOKEN_ID;
    static const int INVALID_PRIORITY_LEVEL;
    static const HandlerToken INVALID_TOKEN;

    /**
     * Default constructor
     * Copy constructor is trivial and implicitly declared.
     *
     */
    HandlerToken()
        : m_token(INVALID_TOKEN_ID)
        , m_priority(INVALID_PRIORITY_LEVEL) {};

    /**
    * Comparison operator (equality)
    *
    * @return true if tokens represent the same record, false otherwise
    */
    bool operator==(const HandlerToken& other) const;

    /**
    * Comparison operator (inequality)
    *
    * @return the negation of the equality operation.
    */
    bool operator!=(const HandlerToken& other) const;

    /**
     * Comparison operator (smaller than)
     *
     * Tokens can be ordered.
     * Utility function that allows use in containers.
     *
     * HandlerTokens are ordered reflecting the priority levels of their corresponding handlers.
     * That is, HandlerTokens with high priority handlers are "greater" than HandlerTokens
     * of lower priority handlers.
     * If HandlerTokens reflect handlers with the same priority level, they are ordered according to
     * an internal unique identifier.
     *
     * @return true if the token is
     */
    bool operator<(const HandlerToken& other) const;

    /**
     * Hash operator for HandlerToken.
     *
     * Hashing is important for using tokens as keys in the registry
     */
    size_t operator()(const HandlerToken& token) const;
};

/**
 *  This is a helper factory to create tokens with a given id
 *
 *  The idea is that this function should only be used by selected
 *  classes, such as HandlerRecord(s), which manage the creation and labeling of
 *  tokens.
 */
HandlerToken createHandlerToken(const TokenIDType& token_id, const int& priority_level);

//DESIGN_DECISION: Priority is not directly exposed
//  Though we have the priority associated to the handler in the token, we do not provide a method to retrieve it
//  We do that to make the way to get information about a handler uniform, that is, we use a query interface that
//  receives a token and returns a specific information.  If we do provide this method, we open exceptions on how to
//  get informations about the handler.  Note that we privately carry the information on priority so we are able to
//  order them according to their priority.

}

#endif