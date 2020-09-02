//
// Created by Matthew.Sirman on 26/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_PRIMITIVEEXCHANGE_H
#define CONTRACTS_SITE_CLIENT_PRIMITIVEEXCHANGE_H

#include "../../TCPSocket.h"
#include "../protocolInternal.h"

// Macros for defining the param methods for value and socket parameters for primitive exchanges,
// as the language does not support specialisation of only one template (i.e. specialising for a
// specific parameter but general _Ty)

#define PRIM_EXCHANGE_PARAM_VAL(type)                                               \
template<>                                                                          \
template<>                                                                          \
constexpr typename PrimitiveExchange<type>::Value &                                 \
PrimitiveExchange<type>::param<typename PrimitiveExchange<type>::Value>() {         \
    return val;                                                                     \
}

#define PRIM_EXCHANGE_PARAM_SOCKET_VAL(type)                                        \
template<>                                                                          \
template<>                                                                          \
constexpr typename PrimitiveExchange<type>::Socket &                                \
PrimitiveExchange<type>::param<typename PrimitiveExchange<type>::Socket>() {        \
    return socket;                                                                  \
}

namespace networking {

    // PrimitiveExchange
    // Layer for exchanging a primitive value of type _Ty
    template<typename _Ty>
    struct PrimitiveExchange : internal::ProtocolLayer {
        // Friend the protocol class so it can access internal param method
        friend class Protocol;

    public:
        // Create the slot for the value
        using Value = internal::Connector<0, PrimitiveExchange<_Ty>, _Ty>;
        // Create the slot for the socket
        using Socket = internal::Connector<1, PrimitiveExchange<_Ty>, TCPSocket>;

        // Activate the layer
        void activate() override;

    private:
        // Anonymous enum for translating the sender and receiver tags
        enum {
            SENDER,
            RECEIVER
        } role;

        // Constructor for the sender tag
        explicit PrimitiveExchange(internal::role_sender_t);

        // Constructor for the receiver tag
        explicit PrimitiveExchange(internal::role_receiver_t);

        // Get the specified parameter value at the given slot
        template<typename _Param>
        constexpr _Param &param();

        // Value parameter data
        Value val;
        // Socket parameter data
        Socket socket;
    };

    template<typename _Ty>
    PrimitiveExchange<_Ty>::PrimitiveExchange(internal::role_sender_t)
            : ProtocolLayer(Sender), role(SENDER) {
        // Sender constructor - note we set the role enum value to SENDER
    }

    template<typename _Ty>
    PrimitiveExchange<_Ty>::PrimitiveExchange(internal::role_receiver_t)
            : ProtocolLayer(Receiver), role(RECEIVER) {
        // Receiver constructor - note we set the role enum value to RECEIVER
    }

    template<typename _Ty>
    void PrimitiveExchange<_Ty>::activate() {
        // Switch the activation depending on the role
        switch (role) {
            case SENDER: {
                // Create the send buffer of the size of the primitive
                byte_buffer buff(sizeof(_Ty));
                // Copy the internal value into the send buffer
                std::copy(&val.get(), &val.get() + 1, (_Ty *) buff.begin());
                // Construct a message from the buffer and send it on the socket
                socket.get().send(NetworkMessage(std::move(buff)));
                break;
            }
            case RECEIVER: {
                // Receive a message from the socket
                NetworkMessage valMessage = socket.get().receive();
                // Copy from the message data into the internal value
                std::copy((const _Ty *) valMessage.begin(), (const _Ty *) valMessage.end(), &val.get());
                break;
            }
        }
    }

    // Create the various param methods for the different primitives

    PRIM_EXCHANGE_PARAM_VAL(char)

    PRIM_EXCHANGE_PARAM_SOCKET_VAL(char)

    PRIM_EXCHANGE_PARAM_VAL(unsigned char)

    PRIM_EXCHANGE_PARAM_SOCKET_VAL(unsigned char)

    PRIM_EXCHANGE_PARAM_VAL(short)

    PRIM_EXCHANGE_PARAM_SOCKET_VAL(short)

    PRIM_EXCHANGE_PARAM_VAL(unsigned short)

    PRIM_EXCHANGE_PARAM_SOCKET_VAL(unsigned short)

    PRIM_EXCHANGE_PARAM_VAL(int)

    PRIM_EXCHANGE_PARAM_SOCKET_VAL(int)

    PRIM_EXCHANGE_PARAM_VAL(unsigned int)

    PRIM_EXCHANGE_PARAM_SOCKET_VAL(unsigned int)

    PRIM_EXCHANGE_PARAM_VAL(long long)

    PRIM_EXCHANGE_PARAM_SOCKET_VAL(long long)

    PRIM_EXCHANGE_PARAM_VAL(unsigned long long)

    PRIM_EXCHANGE_PARAM_SOCKET_VAL(unsigned long long)

    // Create aliases for each of the primitives
    using Int8Exchange = PrimitiveExchange<char>;
    using UInt8Exchange = PrimitiveExchange<unsigned char>;
    using Int16Exchange = PrimitiveExchange<short>;
    using UInt16Exchange = PrimitiveExchange<unsigned short>;
    using Int32Exchange = PrimitiveExchange<int>;
    using UInt32Exchange = PrimitiveExchange<unsigned int>;
    using Int64Exchange = PrimitiveExchange<long long>;
    using UInt64Exchange = PrimitiveExchange<unsigned long long>;

}


#endif //CONTRACTS_SITE_CLIENT_PRIMITIVEEXCHANGE_H
