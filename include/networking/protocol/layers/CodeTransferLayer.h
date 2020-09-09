//
// Created by Matthew.Sirman on 09/09/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_CODETRANSFERLAYER_H
#define CONTRACTS_SITE_CLIENT_CODETRANSFERLAYER_H

#include "../../TCPSocket.h"
#include "../protocolInternal.h"


namespace networking {

    template<typename _EnumType>
    struct CodeTransferLayer : public internal::ProtocolLayer {
        friend class Protocol;

    public:
        using AESSymKey = internal::Connector<0, CodeTransferLayer<_EnumType>, AESKey>;
        using Code = internal::Connector<1, CodeTransferLayer<_EnumType>, _EnumType>;
        using Socket = internal::Connector<2, CodeTransferLayer<_EnumType>, TCPSocket>;

        void activate() override;

    private:
        enum {
            SENDER,
            RECEIVER
        } role;

        explicit CodeTransferLayer(internal::role_sender_t);

        explicit CodeTransferLayer(internal::role_receiver_t);

        template<typename _Param>
        struct ParameterDelegateSwitch {
            constexpr static _Param &get(AESSymKey &key, Code &code, Socket &socket) {
                static_assert(false, "Invalid parameter requested from delegate.");
            }
        };

        template<>
        struct ParameterDelegateSwitch<AESSymKey> {
            constexpr static AESSymKey &get(AESSymKey &key, Code &code, Socket &socket) {
                return key;
            }
        };

        template<>
        struct ParameterDelegateSwitch<Code> {
            constexpr static Code &get(AESSymKey &key, Code &code, Socket &socket) {
                return code;
            }
        };

        template<>
        struct ParameterDelegateSwitch<Socket> {
            constexpr static Socket &get(AESSymKey &key, Code &code, Socket &socket) {
                return socket;
            }
        };

        template<typename _Param>
        constexpr _Param &param();

        AESSymKey key;
        Code code;
        Socket socket;
    };

    template<typename _EnumType>
    CodeTransferLayer<_EnumType>::CodeTransferLayer(internal::role_sender_t)
            : ProtocolLayer(Sender), role(SENDER) {

    }

    template<typename _EnumType>
    CodeTransferLayer<_EnumType>::CodeTransferLayer(internal::role_receiver_t)
            : ProtocolLayer(Receiver), role(RECEIVER) {

    }

    template<typename _EnumType>
    inline void CodeTransferLayer<_EnumType>::activate() {
        switch (role) {
            case SENDER: {
                byte_buffer codeBuffer(sizeof(_EnumType));
                std::copy((byte *) &code.get(), (byte *) &code.get() + sizeof(_EnumType), codeBuffer.begin());
                socket.get().send(AESMessage(std::move(codeBuffer), key.get()));
                break;
            }
            case RECEIVER: {
                AESMessage aesMessage(socket.get().receive(), key.get());
                if (aesMessage.invalid()) {
                    markProtocolTermination();
                    return;
                }
                std::copy(aesMessage.begin(), aesMessage.end(), (byte *) &code.get());
                break;
            }
        }
    }

    template<typename _EnumType>
    template<typename _Param>
    inline constexpr _Param &CodeTransferLayer<_EnumType>::param() {
        return ParameterDelegateSwitch<_Param>::get(key, code, socket);
    }

}

#endif //CONTRACTS_SITE_CLIENT_CODETRANSFERLAYER_H
