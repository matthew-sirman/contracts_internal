//
// Created by Matthew.Sirman on 27/08/2020.
//

#ifndef CONTRACTS_SERVER_RSAMESSAGELAYER_H
#define CONTRACTS_SERVER_RSAMESSAGELAYER_H

#include <encrypt.h>

#include "../../TCPSocket.h"
#include "../protocolInternal.h"

namespace networking {

    struct RSAMessageLayer : public internal::ProtocolLayer {
        friend class Protocol;

    public:
        // Create the slot for the public key parameter
        using RSAPublicKey = internal::Connector<0, RSAMessageLayer, RSAKeyPair::Public>;
        // Create the slot for the private key parameter
        using RSAPrivateKey = internal::Connector<1, RSAMessageLayer, RSAKeyPair::Private>;
        // Create the slot for the message parameter
        using Message = internal::Connector<2, RSAMessageLayer, uint2048>;
        // Create the slot for the socket parameter
        using Socket = internal::Connector<3, RSAMessageLayer, TCPSocket>;

        // Activate the layer
        void activate() override;

    private:
        // Anonymous enum for translating the sender and receiver tags
        enum {
            SENDER,
            RECEIVER
        } role;

        // Constructor for the sender tag
        explicit RSAMessageLayer(internal::role_sender_t);

        // Constructor for the receiver tag
        explicit RSAMessageLayer(internal::role_receiver_t);

        // Get the specified parameter value at the given slot
        template<typename _Param>
        constexpr _Param &param();

        // Public key parameter data
        RSAPublicKey publicKey;
        // Private key parameter data
        RSAPrivateKey privateKey;
        // Message parameter data
        Message message;
        // Socket parameter data
        Socket socket;
    };

    template<>
    constexpr RSAMessageLayer::RSAPublicKey &RSAMessageLayer::param<RSAMessageLayer::RSAPublicKey>() {
        return publicKey;
    }

    template<>
    constexpr RSAMessageLayer::RSAPrivateKey &RSAMessageLayer::param<RSAMessageLayer::RSAPrivateKey>() {
        return privateKey;
    }

    template<>
    constexpr RSAMessageLayer::Message &RSAMessageLayer::param<RSAMessageLayer::Message>() {
        return message;
    }

    template<>
    constexpr RSAMessageLayer::Socket &RSAMessageLayer::param<RSAMessageLayer::Socket>() {
        return socket;
    }

}

#endif //CONTRACTS_SERVER_RSAMESSAGELAYER_H
