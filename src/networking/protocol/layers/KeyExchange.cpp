//
// Created by Matthew.Sirman on 26/08/2020.
//

#include "../../../../include/networking/protocol/layers/KeyExchange.h"

using namespace networking;

KeyExchange::KeyExchange(internal::role_sender_t)
        : ProtocolLayer(Sender), role(SENDER) {
    // Sender constructor - note we set the role enum value to SENDER
}

KeyExchange::KeyExchange(internal::role_receiver_t)
        : ProtocolLayer(Receiver), role(RECEIVER) {
    // Receiver constructor - note we set the role enum value to RECEIVER
}

void KeyExchange::activate() {
    // Switch the activation depending on the role
    switch (role) {
        case SENDER: {
            // Create the send buffer of the size of the key
            byte_buffer buff(sizeof(RSAPublicKey::ValueType));
            // Copy the key data into the buffer
            std::copy((byte *) &key.get(), (byte *) &key.get() + sizeof(RSAPublicKey::ValueType), buff.begin());
            // Construct a message from the buffer and send it on the socket
            socket.get().send(std::move(RawMessage(std::move(buff))));
            break;
        }
        case RECEIVER: {
            // Receive a message from the socket
            RawMessage keyMessage(socket.get().receive());
            if (keyMessage.invalid()) {
                markProtocolTermination();
                return;
            }
            // Copy from the message data into the internal key value
            std::copy(keyMessage.begin(), keyMessage.end(), (byte *) &key.get());
            break;
        }
    }
}