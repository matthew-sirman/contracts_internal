//
// Created by Matthew.Sirman on 27/08/2020.
//

#include "../../../../include/networking/protocol/layers/RSAMessageLayer.h"

using namespace networking;


RSAMessageLayer::RSAMessageLayer(internal::role_sender_t)
        : ProtocolLayer(Sender), role(SENDER) {
    // Sender constructor - note we set the role enum value to SENDER
}

RSAMessageLayer::RSAMessageLayer(internal::role_receiver_t)
        : ProtocolLayer(Receiver), role(RECEIVER) {
    // Sender constructor - note we set the role enum value to SENDER
}

void RSAMessageLayer::activate() {
    switch (role) {
        case SENDER: {
            // Send the message on the socket
            socket.get().send(std::move(RSAMessage(message.get(), publicKey.get())));
            break;
        }
        case RECEIVER: {
            // Receive a message from the socket
            RSAMessage rsaMessage(socket.get().receive(), { publicKey.get(), privateKey.get() });
            if (rsaMessage.invalid()) {
                markProtocolTermination();
                return;
            }
            // Copy from the message data into the internal value
            std::copy((uint2048 *) rsaMessage.begin(), (uint2048 *) rsaMessage.end(), &message.get());
            break;
        }
    }
}
