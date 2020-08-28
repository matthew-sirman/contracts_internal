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

}
