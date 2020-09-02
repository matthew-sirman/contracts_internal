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
            // Create the message buffer
            byte_buffer messageBuffer(sizeof(uint2048));
            // Copy the message into the buffer
            std::copy((byte *) &message.get(), (byte *) &message.get() + sizeof(uint2048), messageBuffer.begin());
            // Create the RSAMessage from this buffer
            RSAMessage rsaMessage(std::move(messageBuffer));
            // Give the message the public key to encrypt under
            rsaMessage.setPublicKey(publicKey.get());
            // Send the message on the socket
            socket.get().send(std::move(rsaMessage));
            break;
        }
        case RECEIVER: {
            // Receive a message from the socket
            RSAMessage rsaMessage = socket.get().receiveRSA();
            // Give the message the private key to decrypt with
            rsaMessage.setPrivateKey(privateKey.get());
            // Copy from the message data into the internal value
            std::copy((uint2048 *) rsaMessage.begin(), (uint2048 *) rsaMessage.end(), &message.get());
            break;
        }
    }
}
