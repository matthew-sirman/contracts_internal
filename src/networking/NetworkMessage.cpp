//
// Created by Matthew.Sirman on 24/08/2020.
//

#include "../../include/networking/NetworkMessage.h"

using namespace networking;

//byte_buffer NetworkMessage::Header::createHeaderBuffer() {
//    // Create the buffer to return
//    byte_buffer buffer = std::make_unique<byte[]>(size);
//
//    // Copy the send message size into the buffer
//    std::copy((byte *) &sendMessageSize, (byte *) &sendMessageSize + sizeof(unsigned), buffer.get());
//
//    // Move out the buffer
//    return std::move(buffer);
//}
//
//
//void NetworkMessage::Header::loadFromBuffer(byte_buffer &&buffer) {
//    // Copy the message size out of the buffer into the send message variable
//    std::copy(buffer.get(), buffer.get() + sizeof(unsigned), (byte *) &sendMessageSize);
//}

NetworkMessage::NetworkMessage()
        : dataBuffer(nullptr), dataBufferSize(0), __cached(nullptr), __cachedBufferSize(0) {

}

NetworkMessage::NetworkMessage(byte_buffer buffer, size_t bufferSize)
        : dataBuffer(std::move(buffer)), dataBufferSize(bufferSize), __cached(nullptr), __cachedBufferSize(0) {

}

NetworkMessage::NetworkMessage(NetworkMessage &&other) noexcept
        : dataBuffer(std::move(other.dataBuffer)), dataBufferSize(other.dataBufferSize),
          __cached(std::move(other.__cached)), __cachedBufferSize(other.__cachedBufferSize) {
    // Invalidate the original message as we have moved its contents
    other.invalidate();
}

NetworkMessage::~NetworkMessage() = default;

NetworkMessage &NetworkMessage::operator=(NetworkMessage &&other) noexcept {
    // If this is a self assignment, do nothing
    if (this == &other) {
        return *this;
    }

    // Move the data buffer, cache buffer and header across
    this->dataBuffer = std::move(other.dataBuffer);
    this->dataBufferSize = other.dataBufferSize;
    this->__cached = std::move(other.__cached);
    this->__cachedBufferSize = other.__cachedBufferSize;

    // Invalidate the original message
    other.invalidate();

    return *this;
}

const byte *NetworkMessage::begin() {
    // Reconstruct the message (in case it has not been reconstructed from the cache)
    reconstruct();

    // Return the start of the data buffer
    return dataBuffer.get();
}

const byte *NetworkMessage::end() {
    // Reconstruct the message (in case it has not been reconstructed from the cache)
    reconstruct();

    // Return the end of the data buffer
    return dataBuffer.get() + dataBufferSize;
}

void NetworkMessage::clearCache() {
    // Clear the cache data
    __cached = nullptr;
    // Set the buffer size to 0
    __cachedBufferSize = 0;
}

size_t NetworkMessage::headerSize() const {
    return _HeaderSize;
}

bool NetworkMessage::checkMessageValid() const {
    return true;
}

shared_byte_buffer NetworkMessage::sendStream() {
    // Create the cache buffer (in case it has not been created)
    createBuffer();

    // Return the cache buffer - it is a shared buffer so we do not need to move it
    return __cached;
}

size_t NetworkMessage::sendStreamSize() {
    // Create the cache buffer (in case it has not been created)
    createBuffer();

    // Return the size of the cache buffer
    return __cachedBufferSize;
}

byte_buffer NetworkMessage::createHeader() {
    // Set the header's message size to the size of the data buffer
    byte_buffer buff = std::make_unique<byte[]>(headerSize());

    std::copy((byte *) &dataBufferSize, (byte *) &dataBufferSize + sizeof(unsigned), buff.get());

    return std::move(buff);
}

void NetworkMessage::reconstructHeader(byte_buffer headerBuffer) {
    std::copy(headerBuffer.get(), headerBuffer.get() + sizeof(unsigned), &dataBufferSize);
}

void NetworkMessage::copyDataToCache() {
    // Copy the data buffer contents into the buffer
    std::copy(dataBuffer.get(), dataBuffer.get() + dataBufferSize, __cached.get() + _HeaderSize);
}

void NetworkMessage::copyCacheToData() {
    // Copy the data from the cache into the data buffer
    std::copy(cacheDataBegin(), __cached.get() + _HeaderSize + dataBufferSize, dataBuffer.get());
}

byte *NetworkMessage::cacheDataBegin() const {
    return __cached.get() + headerSize();
}

byte *NetworkMessage::dataBegin() const {
    return dataBuffer.get();
}

constexpr size_t NetworkMessage::messageSize() const {
    return dataBufferSize;
}

void NetworkMessage::setMessageSize(size_t size) {
    dataBufferSize = size;
}

void NetworkMessage::readBuffer(const std::array<byte, 128u> &buffer) {
    // Copy this chunk into the cache buffer at the correct place, and increment the decoder index
    std::copy(buffer.begin(), buffer.end(), __cached.get() + headerSize() + BUFFER_CHUNK_SIZE * (__decoderIndex++));
}

bool NetworkMessage::expectingData() const {
    // If the amount of data we've currently got in the cache (left had side) is less than the amount
    // we are expecting (right hand side) return true indicating that we need more chunks of data.
    // Otherwise, return false
    return (BUFFER_CHUNK_SIZE * __decoderIndex) + headerSize() < __cachedBufferSize;
}

void NetworkMessage::createBuffer() {
    // If the cache buffer already exists, just return - we have no work to do (note: there could
    // in theory be cases where the cache needs regenerating - for these, clearCache() should be called
    // before createBuffer())
    if (__cached) {
        return;
    }

    // Create the header
    createHeader();

    // Calculate the padded size this buffer will occupy
    __cachedBufferSize = headerSize() + paddedBufferSize(dataBufferSize, BUFFER_CHUNK_SIZE);
    // Create the cache buffer with the calculated size
    __cached = std::shared_ptr<byte[]>(new byte[__cachedBufferSize]);

    byte_buffer headerStream = createHeader();

    // Copy the header into the start of the buffer
    std::copy(headerStream.get(), headerStream.get() + headerSize(), __cached.get());

    // Copy the data into the cache buffer
    copyDataToCache();
}

void NetworkMessage::reconstruct() {
    // If the data buffer already exists, just return - we have no work to do (note: there could in theory
    // be cases where the data buffer needs regenerating - for these, clearData() should be called before
    // reconstruct())
    if (dataBuffer) {
        return;
    }

    byte_buffer headerStream = std::make_unique<byte[]>(headerSize());
    std::copy(__cached.get(), __cached.get() + headerSize(), headerStream.get());
    reconstructHeader(std::move(headerStream));
    // Create the data buffer of the correct size
    dataBuffer = std::make_unique<byte[]>(dataBufferSize);

    // Copy the cache buffer into the data
    copyCacheToData();
}

void NetworkMessage::clearData() {
    // Clear the data buffer
    dataBuffer = nullptr;
    // Set the buffer size to 0
    dataBufferSize = 0;
}

void NetworkMessage::invalidate() {
    // Clear the buffers and reset the header
    clearData();
    clearCache();
}

RSAMessage::RSAMessage()
        : NetworkMessage() {

}

RSAMessage::RSAMessage(byte_buffer buffer, size_t bufferSize)
        : NetworkMessage(std::move(buffer), bufferSize) {

}

RSAMessage::RSAMessage(RSAMessage &&other) noexcept
        : NetworkMessage(std::move(other)) {
    this->publicKey = other.publicKey;
    this->privateKey = other.privateKey;
    other.publicKey = nullptr;
    other.privateKey = nullptr;
}

RSAMessage::~RSAMessage() = default;

RSAMessage &RSAMessage::operator=(RSAMessage &&other) noexcept {
    // If this is a self assignment, just return
    if (this == &other) {
        return *this;
    }

    // Copy across the public and private keys, and null them on the original
    this->publicKey = other.publicKey;
    this->privateKey = other.privateKey;
    other.publicKey = nullptr;
    other.privateKey = nullptr;
    // Call the base move assignment operator
    this->NetworkMessage::operator=(std::move(other));

    return *this;
}

void RSAMessage::setPublicKey(const RSAKeyPair::Public &key) {
    publicKey = &key;
}

void RSAMessage::setPrivateKey(const RSAKeyPair::Private &key) {
    privateKey = &key;
}

size_t RSAMessage::headerSize() const {
    return _HeaderSize;
}

bool RSAMessage::checkMessageValid() const {
    // Return that the message size is equal to the expected size
    return messageSize() == size();
}

void RSAMessage::copyDataToCache() {
    // Encrypt the data buffer
    uint2048 encrypted = encrypt(*(uint2048 *) dataBegin(), *publicKey);
    // Copy the encrypted value into the cache buffer
    std::copy((byte *) &encrypted, (byte *) &encrypted + sizeof(uint2048), cacheDataBegin());
}

void RSAMessage::copyCacheToData() {
    // Decrypt the cache buffer
    uint2048 decrypted = decrypt(*(uint2048 *) cacheDataBegin(), *privateKey);
    // Copy the decrypted value into the data buffer
    std::copy((byte *) &decrypted, (byte *) &decrypted + sizeof(uint2048), dataBegin());
}

AESMessage::AESMessage()
        : NetworkMessage(), initVector(0) {
    CryptoSafeRandom::random(&initVector, sizeof(uint64));
}

AESMessage::AESMessage(byte_buffer buffer, size_t bufferSize)
        : NetworkMessage(std::move(buffer), bufferSize), initVector(0) {
    CryptoSafeRandom::random(&initVector, sizeof(uint64));
}

AESMessage::AESMessage(AESMessage &&other) noexcept
        : NetworkMessage(std::move(other)) {
    this->aesKey = other.aesKey;
    this->initVector = other.initVector;
    other.initVector = 0;
}

AESMessage::~AESMessage() = default;

AESMessage &AESMessage::operator=(AESMessage &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    this->aesKey = other.aesKey;
    this->initVector = other.initVector;
    other.initVector = 0;
    this->NetworkMessage::operator=(std::move(other));

    return *this;
}

void AESMessage::setAESKey(const AESKey &key) {
    aesKey = key;
}

size_t AESMessage::headerSize() const {
    return _HeaderSize;
}

bool AESMessage::checkMessageValid() const {
    return NetworkMessage::checkMessageValid();
}

byte_buffer AESMessage::createHeader() {
    byte_buffer headerBuffer = std::make_unique<byte[]>(_HeaderSize);

    unsigned mSize = messageSize();

    std::copy((byte *) &mSize, (byte *) &mSize + sizeof(unsigned), headerBuffer.get());
    std::copy((byte *) &initVector, (byte *) &initVector + sizeof(uint64), headerBuffer.get() + sizeof(unsigned));

    return std::move(headerBuffer);
}

void AESMessage::reconstructHeader(byte_buffer headerBuffer) {
    unsigned mSize;

    std::copy(headerBuffer.get(), headerBuffer.get() + sizeof(unsigned), &mSize);
    std::copy(headerBuffer.get() + sizeof(unsigned), headerBuffer.get() + sizeof(unsigned) + sizeof(uint64), &initVector);

    setMessageSize(mSize);
}

void AESMessage::copyDataToCache() {
    // Encrypt the data buffer into the cache buffer
    encrypt(dataBegin(), messageSize(), cacheDataBegin(), initVector, aesKey);
}

void AESMessage::copyCacheToData() {
    // Decrypt the cache buffer into the data buffer
    decrypt(cacheDataBegin(), messageSize(), dataBegin(), initVector, aesKey);
}
