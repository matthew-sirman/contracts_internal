//
// Created by Matthew.Sirman on 25/08/2020.
//

#include "../../include/networking/NetworkTranslator.h"

using namespace networking;

void NetworkTranslator::setConnectionProtocol(Protocol &&protocol) {
    connectionProtocol = std::move(protocol);
}
