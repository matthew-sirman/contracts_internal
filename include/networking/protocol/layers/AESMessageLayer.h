//
// Created by Matthew.Sirman on 28/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_AESMESSAGELAYER_H
#define CONTRACTS_SITE_CLIENT_AESMESSAGELAYER_H

#include "../../TCPSocket.h"
#include "../protocolInternal.h"

namespace networking {

    struct AESMessageLayer : internal::ProtocolLayer {
        friend class Protocol;

    public:
        using AESSymKey = internal::Connector<0, AESMessageLayer, AESKey>;
        using Message = internal::Connector<1, AESMessageLayer, shared_byte_buffer>;
        using Socket = internal::Connector<2, AESMessageLayer, TCPSocket>;

        void activate() override;

    private:
        enum {
            SENDER,
            RECEIVER
        } role;

        explicit AESMessageLayer(internal::role_sender_t);

        explicit AESMessageLayer(internal::role_receiver_t);

        template<typename _Param>
        constexpr _Param &param();

        AESSymKey key;
        Message message;
        Socket socket;
    };

    template<>
    constexpr AESMessageLayer::AESSymKey &AESMessageLayer::param<AESMessageLayer::AESSymKey>() {
        return key;
    }

    template<>
    constexpr AESMessageLayer::Message &AESMessageLayer::param<AESMessageLayer::Message>() {
        return message;
    }

    template<>
    constexpr AESMessageLayer::Socket &AESMessageLayer::param<AESMessageLayer::Socket>() {
        return socket;
    }

}


#endif //CONTRACTS_SITE_CLIENT_AESMESSAGELAYER_H
