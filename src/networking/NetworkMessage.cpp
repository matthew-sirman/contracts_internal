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
        : dataBuffer(nullptr), __cached(nullptr) {

}

NetworkMessage::NetworkMessage(byte_buffer buffer)
        : dataBuffer(std::move(buffer)), __cached(nullptr) {

}

NetworkMessage::NetworkMessage(NetworkMessage &&other) noexcept
        : dataBuffer(std::move(other.dataBuffer)),
          __cached(std::move(other.__cached)) {
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
    this->__cached = std::move(other.__cached);

    // Invalidate the original message
    other.invalidate();

    return *this;
}

const byte *NetworkMessage::begin() const {
    if (!dataBuffer) {
        throw NetworkMessageException("Tried to retrieve begin() from un-built message. Call build() first.");
    }
    // Return the start of the data buffer
    return dataBuffer.cbegin();
}

const byte *NetworkMessage::end() const {
    if (!dataBuffer) {
        throw NetworkMessageException("Tried to retrieve end() from un-built message. Call build() first.");
    }
    // Return the end of the data buffer
    return dataBuffer.cend();
}

void NetworkMessage::clearCache() {
    // Clear the cache data
    __cached = nullptr;
}

size_t NetworkMessage::headerSize() const {
    return _HeaderSize;
}

size_t NetworkMessage::messageSize() const {
    if (!dataBuffer) {
        throw NetworkMessageException("Tried to retrieve messageSize() from un-built message. Call build() first.");
    }
    return dataBuffer.size();
}

void NetworkMessage::build() {
    if ((!dataBuffer || !dataPopulated) && !__cached) {
        throw NetworkMessageException("No data in message to build from. Either the data buffer or cache buffer must be populated.");
    }

    if (!dataBuffer || !dataPopulated) {
        reconstruct();
    }
    if (!__cached) {
        copyDataToCache();
    }
}

bool NetworkMessage::checkMessageValid() {
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
    return __cached.size();
}

byte_buffer NetworkMessage::createHeader() {
    // Set the header's message size to the size of the data buffer
    byte_buffer buff(headerSize());

    unsigned dataBufferSize = dataBuffer.size();

    std::copy((byte *) &dataBufferSize, (byte *) &dataBufferSize + sizeof(unsigned), buff.begin());

    return std::move(buff);
}

void NetworkMessage::reconstructHeader(byte_buffer headerBuffer) {
    size_t dataBufferSize = 0;
    std::copy(headerBuffer.begin(), headerBuffer.begin() + sizeof(unsigned), (byte *) &dataBufferSize);

    setMessageSize(dataBufferSize);
}

void NetworkMessage::copyDataToCache() {
    // Copy the data buffer contents into the buffer
    std::copy(dataBuffer.begin(), dataBuffer.end(), __cached.begin() + _HeaderSize);
}

void NetworkMessage::copyCacheToData() {
    // Copy the data from the cache into the data buffer
    std::copy(cacheDataBegin(), cacheDataBegin() + dataBuffer.size(), dataBuffer.begin());
}

byte *NetworkMessage::cacheDataBegin() const {
    return __cached.begin() + headerSize();
}

byte *NetworkMessage::dataBegin() const {
    return dataBuffer.begin();
}

void NetworkMessage::setMessageSize(size_t size) {
    dataBuffer = byte_buffer(size);
    dataPopulated = false;
}

void NetworkMessage::readBuffer(const std::array<byte, 128u> &buffer) {
    // Copy this chunk into the cache buffer at the correct place, and increment the decoder index
    std::copy(buffer.begin(), buffer.end(), __cached.begin() + headerSize() + BUFFER_CHUNK_SIZE * (__decoderIndex++));
}

bool NetworkMessage::expectingData() const {
    // If the amount of data we've currently got in the cache (left had side) is less than the amount
    // we are expecting (right hand side) return true indicating that we need more chunks of data.
    // Otherwise, return false
    return (BUFFER_CHUNK_SIZE * __decoderIndex) + headerSize() < __cached.size();
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

    // Create the cache buffer with the calculated size
    __cached = shared_byte_buffer(headerSize() + paddedBufferSize(dataBuffer.size(), BUFFER_CHUNK_SIZE));

    byte_buffer headerStream = createHeader();

    // Copy the header into the start of the buffer
    std::copy(headerStream.begin(), headerStream.end(), __cached.begin());

    // Copy the data into the cache buffer
    copyDataToCache();
}

void NetworkMessage::reconstruct() {
    // If the data buffer already exists, just return - we have no work to do (note: there could in theory
    // be cases where the data buffer needs regenerating - for these, clearData() should be called before
    // reconstruct())
//    if (dataBuffer) {
//        return;
//    }

    if (!dataBuffer) {
        byte_buffer headerStream(headerSize());
        std::copy(__cached.begin(), __cached.begin() + headerSize(), headerStream.begin());
        reconstructHeader(std::move(headerStream));
    }
//    // Create the data buffer of the correct size
//    dataBuffer = byte_buffer(messageMeta.dataBufferSizeHint);

    // Copy the cache buffer into the data
    copyCacheToData();

    dataPopulated = true;
}

void NetworkMessage::clearData() {
    // Clear the data buffer
    dataBuffer = nullptr;
    dataPopulated = false;
}

void NetworkMessage::invalidate() {
    // Clear the buffers and reset the header
    clearData();
    clearCache();
}

RSAMessage::RSAMessage()
        : NetworkMessage() {

}

RSAMessage::RSAMessage(byte_buffer buffer)
        : NetworkMessage(std::move(buffer)) {

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

bool RSAMessage::checkMessageValid() {
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

AESMessage::AESMessage(byte_buffer buffer)
        : NetworkMessage(std::move(buffer)), initVector(0) {
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

bool AESMessage::checkMessageValid() {
    return NetworkMessage::checkMessageValid();
}

byte_buffer AESMessage::createHeader() {
    byte_buffer headerBuffer(_HeaderSize);

    unsigned mSize = messageSize();

    std::copy((byte *) &mSize, (byte *) &mSize + sizeof(unsigned), headerBuffer.begin());
    std::copy((byte *) &initVector, (byte *) &initVector + sizeof(uint64), headerBuffer.begin() + sizeof(unsigned));

    return std::move(headerBuffer);
}

void AESMessage::reconstructHeader(byte_buffer headerBuffer) {
    unsigned mSize;

    std::copy(headerBuffer.begin(), headerBuffer.begin() + sizeof(unsigned), (byte *) &mSize);
    std::copy(headerBuffer.begin() + sizeof(unsigned), headerBuffer.begin() + sizeof(unsigned) + sizeof(uint64),
              (byte *) &initVector);

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

NetworkMessageException::NetworkMessageException(const std::string &message)
        : std::exception(message.c_str()) {

}
