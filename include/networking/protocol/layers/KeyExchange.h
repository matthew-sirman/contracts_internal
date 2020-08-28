//
// Created by Matthew.Sirman on 26/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_KEYEXCHANGE_H
#define CONTRACTS_SITE_CLIENT_KEYEXCHANGE_H

#include <encrypt.h>

#include "../../TCPSocket.h"
#include "../protocolInternal.h"

namespace networking {

    // KeyExchange
    // Layer for exchanging RSA Public keys
    struct KeyExchange : public internal::ProtocolLayer {
        // Friend the protocol class so it can access internal param method
        friend class Protocol;

    public:
        // Create the slot for the key parameter
        using RSAPublicKey = internal::Connector<0, KeyExchange, RSAKeyPair::Public>;
        // Create the slot for the socket parameter
        using Socket = internal::Connector<1, KeyExchange, TCPSocket>;

        // Activate the layer
        void activate() override;

    private:
        // Anonymous enum for translating the sender and receiver tags
        enum {
            SENDER,
            RECEIVER
        } role;

        // Constructor for the sender tag
        explicit KeyExchange(internal::role_sender_t);

        // Constructor for the receiver tag
        explicit KeyExchange(internal::role_receiver_t);

        // Get the specified parameter value at the given slot
        template<typename _Param>
        constexpr _Param &param();

        // Key parameter data
        RSAPublicKey key;
        // Socket parameter data
        Socket socket;
    };

    // Specialise for the key parameter
    template<>
    constexpr KeyExchange::RSAPublicKey &KeyExchange::param<KeyExchange::RSAPublicKey>() {
        return key;
    }

    // Specialise for the socket parameter
    template<>
    constexpr KeyExchange::Socket &KeyExchange::param<KeyExchange::Socket>() {
        return socket;
    }

}


#endif //CONTRACTS_SITE_CLIENT_KEYEXCHANGE_H
