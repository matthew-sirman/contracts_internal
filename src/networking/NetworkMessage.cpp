//
// Created by Matthew.Sirman on 24/08/2020.
//

#include "../../include/networking/NetworkMessage.h"

using namespace networking;

NetworkMessage::NetworkMessage() = default;

NetworkMessage::NetworkMessage(byte_buffer buffer, size_t bufferSize)
        : buff(std::move(buffer)), buffSize(bufferSize) {

}

NetworkMessage::NetworkMessage(NetworkMessage &&other) noexcept
        : buff(std::move(other.buff)), buffSize(other.buffSize) {
    other.invalidate();
}

NetworkMessage::~NetworkMessage() = default;

NetworkMessage &NetworkMessage::operator=(NetworkMessage &&other) {
    if (this == &other) {
        return *this;
    }

    this->buff = std::move(other.buff);
    this->buffSize = other.buffSize;
    other.invalidate();

    return *this;
}

byte_buffer NetworkMessage::sendStream() const {
    Header header = populateHeader();
    byte_buffer sendBuffer = std::make_unique<byte[]>(header.size() + header.dataSize());

    return std::move(sendBuffer);
}

NetworkMessage::Header NetworkMessage::populateHeader() const {
    Header header;
    header.sendBufferSize = 0;

    return header;
}

void NetworkMessage::invalidate() {
    buff = nullptr;
    buffSize = 0;
}
