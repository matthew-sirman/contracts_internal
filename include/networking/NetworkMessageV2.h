//
// Created by Matthew on 03/09/2020.
//

#ifndef CONTRACTS_INTERNAL_NETWORKMESSAGEV2_H
#define CONTRACTS_INTERNAL_NETWORKMESSAGEV2_H

#include <encrypt.h>
#include <array>

#include "buffer.h"

namespace networking {

    class NetworkMessageDecoder;
    class NetworkMessageBuilder;

    constexpr size_t paddedSize(size_t size, size_t chunkSize) {
        return (size / chunkSize + (size % chunkSize != 0)) * chunkSize;
    }

    // Interfacing class for sending data across sockets - simply holds the data
    // and header, no knowledge of encryption
    // - Fixed size header, or variable?
    // - Or maybe fixed size header with optional added header from each message type?
    class NetworkMessage {
        friend class NetworkMessageDecoder;
        friend class NetworkMessageBuilder;

    public:
        constexpr static size_t HeaderSize { sizeof(unsigned) };
        constexpr static size_t BufferChunkSize { 128u };

        NetworkMessage();

        NetworkMessage(const byte_buffer &buffer);

        NetworkMessage(const shared_byte_buffer &&buffer);

        NetworkMessage(const NetworkMessage &other) = delete;

        NetworkMessage(NetworkMessage &&other) noexcept;

        ~NetworkMessage();

        NetworkMessage &operator=(const NetworkMessage &other) = delete;

        NetworkMessage &operator=(NetworkMessage &&other) noexcept;

        const byte *cbegin() const;

        const byte *cend() const;

        const byte *messageBegin() const;

        const byte *messageEnd() const;

        byte *begin();

        byte *end();

        constexpr size_t bufferSize() const {
            return sendBuffer.size();
        }

        constexpr size_t messageSize() const {
            return __messageSize;
        }

    private:
        constexpr static size_t calculateSendBufferSize(size_t bufferSize) {
            return HeaderSize + paddedSize(bufferSize, BufferChunkSize);
        }

        NetworkMessage(byte_buffer &&buffer, size_t messageSize);

        byte_buffer sendBuffer;
        size_t __messageSize;
    };

    // Helper class for building network messages
    class NetworkMessageBuilder {
    public:
        NetworkMessageBuilder(size_t requiredBufferSize);

        byte *begin();

        byte *end();

        NetworkMessage create();

    private:
        byte_buffer networkMessageBuffer;
        size_t messageSize;
    };

    // Helper class for decoding network messages
    class NetworkMessageDecoder {
    public:
        NetworkMessageDecoder();

        void decodeHeader(const std::array<byte, NetworkMessage::HeaderSize> &header);

        void decodeChunk(const std::array<byte, NetworkMessage::BufferChunkSize> &chunk);

        bool expectingData() const;

        NetworkMessage create();

    private:
        byte_buffer buff;
        size_t decoderStep;
        size_t messageSize;
    };

    // Base interface for different message types
    class MessageBase {
    public:
        MessageBase();

        MessageBase(byte_buffer &&buffer);

        virtual NetworkMessage message() const = 0;

        const byte *cbegin() const;

        const byte *cend() const;

        byte *begin();

        byte *end();

        constexpr size_t size() const {
            return buffer.size();
        }

    protected:
        byte_buffer buffer;
    };

    // Contains a raw, unencrypted message
    class RawMessage : public MessageBase {
    public:
        RawMessage();

        RawMessage(const byte_buffer &buffer);

        RawMessage(byte_buffer &&buffer);

        RawMessage(const shared_byte_buffer &buffer);

        RawMessage(const RawMessage &other) = delete;

        RawMessage(RawMessage &&other) noexcept;

        RawMessage(const NetworkMessage &message);

        ~RawMessage();

        RawMessage &operator=(const RawMessage &other) = delete;

        RawMessage &operator=(RawMessage &&other) noexcept;

        NetworkMessage message() const override;

    private:

    };

    // Contains a message encrypted under the RSA protocol
    class RSAMessage : public MessageBase {
    public:
        RSAMessage();

        RSAMessage(const byte_buffer &buffer, const RSAKeyPair::Public &encryptionKey);

        RSAMessage(const uint2048 &message, const RSAKeyPair::Public &encryptionKey);

        RSAMessage(const shared_byte_buffer &buffer, RSAKeyPair::Public encryptionKey);

        RSAMessage(const RSAMessage &other) = delete;

        RSAMessage(RSAMessage &&other) noexcept;

        RSAMessage(const NetworkMessage &message, RSAKeyPair keys);

        ~RSAMessage();

        RSAMessage &operator=(const RSAMessage &other) = delete;

        RSAMessage &operator=(RSAMessage &&other) noexcept;

        NetworkMessage message() const override;

    private:
        RSAKeyPair::Public encryptionKey;
    };

    // Contains a message encrypted under the AES protocol
    class AESMessage : public MessageBase {
    public:
        AESMessage();

        AESMessage(const byte_buffer &buffer, AESKey key);

        AESMessage(byte_buffer &&buffer, AESKey key);

        AESMessage(const shared_byte_buffer &buffer, AESKey key);

        AESMessage(const AESMessage &other) = delete;

        AESMessage(AESMessage &&other) noexcept;

        AESMessage(const NetworkMessage &message, AESKey key);

        ~AESMessage();

        AESMessage &operator=(const AESMessage &other) = delete;

        AESMessage &operator=(AESMessage &&other) noexcept;

        NetworkMessage message() const override;
    private:
        AESKey key;
    };

}

#endif //CONTRACTS_INTERNAL_NETWORKMESSAGEV2_H
