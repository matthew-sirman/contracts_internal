//
// Created by Matthew.Sirman on 24/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_NETWORKMESSAGE_H
#define CONTRACTS_SITE_CLIENT_NETWORKMESSAGE_H

#include <memory>

namespace networking {

    using byte = unsigned char;
    using byte_buffer = std::unique_ptr<byte[]>;

    class NetworkMessage {
    public:
        NetworkMessage();

        NetworkMessage(byte_buffer buffer, size_t bufferSize);

        NetworkMessage(const NetworkMessage &other) = delete;

        NetworkMessage(NetworkMessage &&other) noexcept;

        ~NetworkMessage();

        NetworkMessage &operator=(const NetworkMessage &other) = delete;

        NetworkMessage &operator=(NetworkMessage &&other);

        byte_buffer sendStream() const;

    private:
        struct Header {
            unsigned sendBufferSize;

            constexpr size_t size() const {
                return sizeof(unsigned);
            }

            constexpr size_t dataSize() const {
                return sendBufferSize;
            }
        };

        Header createHeader() const;

        void invalidate();

        byte_buffer buff;
        size_t buffSize;
    };

    class RawMessage : public NetworkMessage {

    };

    class RSAMessage : public NetworkMessage {

    };

    class AESMessage : public NetworkMessage {

    };

}

#endif //CONTRACTS_SITE_CLIENT_NETWORKMESSAGE_H
