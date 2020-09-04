//
// Created by Matthew on 03/09/2020.
//

#include "../../include/networking/NetworkMessageV2.h"

using namespace networking;

NetworkMessage::NetworkMessage()
        : sendBuffer(nullptr) {

}

NetworkMessage::NetworkMessage(const byte_buffer &buffer)
        : sendBuffer(calculateSendBufferSize(buffer.size())), __messageSize(buffer.size()) {
    std::copy((byte *) &__messageSize, (byte *) &__messageSize + sizeof(unsigned), sendBuffer.begin());
    std::copy(buffer.cbegin(), buffer.cend(), sendBuffer.begin() + sizeof(unsigned));
}

NetworkMessage::NetworkMessage(const shared_byte_buffer &&buffer)
        : sendBuffer(calculateSendBufferSize(buffer.size())), __messageSize(buffer.size()) {
    std::copy((byte *) &__messageSize, (byte *) &__messageSize + sizeof(unsigned), sendBuffer.begin());
    std::copy(buffer.cbegin(), buffer.cend(), sendBuffer.begin() + sizeof(unsigned));
}

NetworkMessage::NetworkMessage(NetworkMessage &&other) noexcept
        : sendBuffer(std::move(other.sendBuffer)), __messageSize(other.__messageSize) {
    other.__messageSize = 0;
}

NetworkMessage::~NetworkMessage() = default;

NetworkMessage &NetworkMessage::operator=(NetworkMessage &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    this->sendBuffer = std::move(other.sendBuffer);
    this->__messageSize = other.__messageSize;
    other.__messageSize = 0;

    return *this;
}

const byte *NetworkMessage::cbegin() const {
    return sendBuffer.cbegin();
}

const byte *NetworkMessage::cend() const {
    return sendBuffer.cend();
}

const byte *NetworkMessage::messageBegin() const {
    return sendBuffer.cbegin() + HeaderSize;
}

const byte *NetworkMessage::messageEnd() const {
    return sendBuffer.cbegin() + HeaderSize + __messageSize;
}

byte *NetworkMessage::begin() {
    return sendBuffer.begin();
}

byte *NetworkMessage::end() {
    return sendBuffer.end();
}

NetworkMessage::NetworkMessage(byte_buffer &&buffer, size_t messageSize)
        : sendBuffer(std::move(buffer)), __messageSize(messageSize) {

}

NetworkMessageBuilder::NetworkMessageBuilder(size_t requiredBufferSize)
        : networkMessageBuffer(NetworkMessage::calculateSendBufferSize(requiredBufferSize)),
          messageSize(requiredBufferSize) {
    std::copy((byte *) &requiredBufferSize, (byte *) &requiredBufferSize + sizeof(unsigned),
              networkMessageBuffer.begin());
}

byte *NetworkMessageBuilder::begin() {
    return networkMessageBuffer.begin() + NetworkMessage::HeaderSize;
}

byte *NetworkMessageBuilder::end() {
    return networkMessageBuffer.end();
}

NetworkMessage NetworkMessageBuilder::create() {
    return std::move(NetworkMessage(std::move(networkMessageBuffer), messageSize));
}

NetworkMessageDecoder::NetworkMessageDecoder()
        : buff(nullptr), decoderStep(0), messageSize(0) {

}

void NetworkMessageDecoder::decodeHeader(const std::array<byte, NetworkMessage::HeaderSize> &header) {
    std::copy(header.begin(), header.begin() + sizeof(unsigned), (byte *) &messageSize);

    buff = byte_buffer(NetworkMessage::calculateSendBufferSize(messageSize));

    std::copy(header.begin(), header.end(), buff.begin());
}

void NetworkMessageDecoder::decodeChunk(const std::array<byte, NetworkMessage::BufferChunkSize> &chunk) {
    std::copy(chunk.begin(), chunk.end(),
              buff.begin() + NetworkMessage::HeaderSize + NetworkMessage::BufferChunkSize * (decoderStep++));
}

bool NetworkMessageDecoder::expectingData() const {
    return NetworkMessage::HeaderSize + NetworkMessage::BufferChunkSize * decoderStep < buff.size();
}

NetworkMessage NetworkMessageDecoder::create() {
    return std::move(NetworkMessage(std::move(buff), messageSize));
}

MessageBase::MessageBase()
        : buffer(nullptr) {

}

MessageBase::MessageBase(byte_buffer &&buffer)
        : buffer(std::move(buffer)) {

}

const byte *MessageBase::cbegin() const {
    return buffer.cbegin();
}

const byte *MessageBase::cend() const {
    return buffer.cend();
}

byte *MessageBase::begin() {
    return buffer.begin();
}

byte *MessageBase::end() {
    return buffer.end();
}

RawMessage::RawMessage()
        : MessageBase() {

}

RawMessage::RawMessage(const byte_buffer &buffer)
        : MessageBase(std::move(buffer.copy())) {

}

RawMessage::RawMessage(byte_buffer &&buffer)
        : MessageBase(std::move(buffer)) {

}

RawMessage::RawMessage(const shared_byte_buffer &buffer)
        : MessageBase(std::move(buffer.uniqueCopy())) {

}

RawMessage::RawMessage(RawMessage &&other) noexcept
        : MessageBase(std::move(other.buffer)) {

}

RawMessage::RawMessage(const NetworkMessage &message) {
    buffer = byte_buffer(message.messageSize());
    std::copy(message.messageBegin(), message.messageEnd(), buffer.begin());
}

RawMessage::~RawMessage() = default;

RawMessage &RawMessage::operator=(RawMessage &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    this->buffer = std::move(other.buffer);

    return *this;
}

NetworkMessage RawMessage::message() const {
    return std::move(NetworkMessage(buffer));
}

RSAMessage::RSAMessage()
        : MessageBase() {

}

RSAMessage::RSAMessage(const byte_buffer &buffer, const RSAKeyPair::Public &encryptionKey)
        : MessageBase(buffer.copy()), encryptionKey(encryptionKey) {

}

RSAMessage::RSAMessage(const uint2048 &message, const RSAKeyPair::Public &encryptionKey) {
    this->buffer = byte_buffer(sizeof(uint2048));
    std::copy((byte *) &message, (byte *) &message + sizeof(uint2048), this->buffer.begin());

    this->encryptionKey = encryptionKey;
}

RSAMessage::RSAMessage(const shared_byte_buffer &buffer, RSAKeyPair::Public encryptionKey)
        : MessageBase(buffer.uniqueCopy()), encryptionKey(encryptionKey) {

}

RSAMessage::RSAMessage(RSAMessage &&other) noexcept
        : MessageBase(std::move(other.buffer)) {
    this->encryptionKey = other.encryptionKey;
}

RSAMessage::RSAMessage(const NetworkMessage &message, RSAKeyPair keys) {
    size_t rawMessageSize = 0;
    std::copy(message.messageBegin(), message.messageBegin() + sizeof(unsigned), (byte *) &rawMessageSize);
    uint2048 encrypted;
    std::copy(message.messageBegin() + sizeof(unsigned), message.messageEnd(), (byte *) &encrypted);

    uint2048 decrypted = decrypt(encrypted, keys.privateKey);
    buffer = byte_buffer(sizeof(uint2048));
    std::copy((byte *) &decrypted, (byte *) &decrypted + sizeof(uint2048), buffer.begin());

    this->encryptionKey = keys.publicKey;
}

RSAMessage::~RSAMessage() = default;

RSAMessage &RSAMessage::operator=(RSAMessage &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    this->buffer = std::move(other.buffer);
    this->encryptionKey = other.encryptionKey;

    return *this;
}

NetworkMessage RSAMessage::message() const {
    uint2048 messageValue;
    size_t messageSize = buffer.size();
    if (messageSize > sizeof(uint2048)) {
        messageSize = sizeof(uint2048);
        std::copy(buffer.cbegin(), buffer.cbegin() + sizeof(uint2048), (byte *) &messageValue);
    } else {
        std::copy(buffer.cbegin(), buffer.cend(), (byte *) &messageValue);
    }
    uint2048 encrypted = encrypt(messageValue, encryptionKey);

    NetworkMessageBuilder messageBuilder(sizeof(unsigned) + sizeof(uint2048));
    std::copy((byte *) &messageSize, (byte *) &messageSize + sizeof(unsigned), messageBuilder.begin());
    std::copy((byte *) &encrypted, (byte *) &encrypted + sizeof(uint2048),
              messageBuilder.begin() + sizeof(unsigned));

    return std::move(messageBuilder.create());
}

AESMessage::AESMessage()
        : MessageBase() {

}

AESMessage::AESMessage(const byte_buffer &buffer, AESKey key)
        : MessageBase(buffer.copy()), key(key) {

}

AESMessage::AESMessage(byte_buffer &&buffer, AESKey key)
        : MessageBase(std::move(buffer)), key(key) {

}

AESMessage::AESMessage(const shared_byte_buffer &buffer, AESKey key)
        : MessageBase(buffer.uniqueCopy()), key(key) {

}

AESMessage::AESMessage(AESMessage &&other) noexcept
        : MessageBase(std::move(other.buffer)), key(other.key) {

}

AESMessage::AESMessage(const NetworkMessage &message, AESKey key)
        : key(key) {
    uint64 initialisationVector;
    size_t messageSize = 0;

    std::copy(message.messageBegin(), message.messageBegin() + sizeof(unsigned), (byte *) &messageSize);
    std::copy(message.messageBegin() + sizeof(unsigned),
              message.messageBegin() + sizeof(unsigned) + sizeof(uint64),
              (byte *) &initialisationVector);

    buffer = byte_buffer(paddedSize(messageSize, 16u));
    const byte *messageStart = message.messageBegin() + sizeof(unsigned) + sizeof(uint64);

    decrypt(messageStart, message.messageEnd() - messageStart, buffer.begin(), initialisationVector, key);
}

AESMessage::~AESMessage() = default;

AESMessage &AESMessage::operator=(AESMessage &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    this->buffer = std::move(other.buffer);
    this->key = other.key;

    return *this;
}

NetworkMessage AESMessage::message() const {
    size_t encryptedSize = paddedSize(buffer.size(), 16u);
    NetworkMessageBuilder messageBuilder(sizeof(unsigned) + sizeof(uint64) + encryptedSize);
    size_t messageSize = buffer.size();
    uint64 initialisationVector;
    CryptoSafeRandom::random(&initialisationVector, sizeof(uint64));
    std::copy((byte *) &messageSize, (byte *) &messageSize + sizeof(unsigned), messageBuilder.begin());
    std::copy((byte *) &initialisationVector, (byte *) &initialisationVector + sizeof(uint64),
              messageBuilder.begin() + sizeof(unsigned));
    encrypt(buffer.cbegin(), buffer.size(),
            messageBuilder.begin() + sizeof(unsigned) + sizeof(uint64), initialisationVector, key);

    return std::move(messageBuilder.create());
}
