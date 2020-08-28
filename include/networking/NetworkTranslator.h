//
// Created by Matthew.Sirman on 25/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_NETWORKTRANSLATOR_H
#define CONTRACTS_SITE_CLIENT_NETWORKTRANSLATOR_H

#include "protocol/Protocol.h"
#include "TCPSocket.h"

namespace networking {

    class NetworkTranslator {
    public:
        void setConnectionProtocol(Protocol &&protocol);

    private:
        TCPSocket socket;
        Protocol connectionProtocol;

    };

}

#endif //CONTRACTS_SITE_CLIENT_NETWORKTRANSLATOR_H
