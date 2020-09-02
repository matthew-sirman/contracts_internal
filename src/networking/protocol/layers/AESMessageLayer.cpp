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
            byte_buffer messageBuffer(message.get().size());
            std::copy(message.get().begin(), message.get().end(), messageBuffer.begin());
            AESMessage aesMessage(std::move(messageBuffer));
            aesMessage.setAESKey(key.get());
            socket.get().send(std::move(aesMessage));
            break;
        }
        case RECEIVER: {
            AESMessage aesMessage = socket.get().receiveAES();
            aesMessage.setAESKey(key.get());
            message.get() = shared_byte_buffer(aesMessage.messageSize());
            std::copy(aesMessage.begin(), aesMessage.end(), message.get().begin());
            break;
        }
    }
}
