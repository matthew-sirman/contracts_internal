//
// Created by Matthew.Sirman on 28/08/2020.
//

#include "../../../../include/networking/protocol/layers/AESMessageLayer.h"

using namespace networking;

AESMessageLayer::AESMessageLayer(internal::role_sender_t)
        : ProtocolLayer(Sender), role(SENDER) {

}

AESMessageLayer::AESMessageLayer(internal::role_receiver_t)
        : ProtocolLayer(Receiver), role(RECEIVER) {

}

void AESMessageLayer::activate() {
    switch (role) {
        case SENDER: {
            AESMessage aesMessage(message.get(), key.get());
            socket.get().send(std::move(aesMessage));
            break;
        }
        case RECEIVER: {
            AESMessage aesMessage(socket.get().receive(), key.get());
            if (aesMessage.invalid()) {
                markProtocolTermination();
                return;
            }
            message.get() = shared_byte_buffer(aesMessage.size());
            std::copy(aesMessage.begin(), aesMessage.end(), message.get().begin());
            break;
        }
    }
}
