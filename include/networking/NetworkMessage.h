//
// Created by Matthew.Sirman on 24/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_NETWORKMESSAGE_H
#define CONTRACTS_SITE_CLIENT_NETWORKMESSAGE_H

#include <memory>
#include <array>

#include <encrypt.h>

#define BUFFER_CHUNK_SIZE 128u

constexpr size_t paddedBufferSize(size_t size, size_t pad) {
    return ((size / pad) + (size % pad != 0)) * pad;
}

// TODO: This class needs reformatting and cleaning up

namespace networking {

    // Forward declare the TCPSocket class
    class TCPSocket;

    // Alias a byte
    using byte = unsigned char;
    // Alias a byte buffer as a unique pointer to a byte array
    using byte_buffer = std::unique_ptr<byte[]>;
    // Alias a shared byte buffer as a shared pointer to a byte array
    using shared_byte_buffer = std::shared_ptr<byte[]>;

    // NetworkMessage
    // A messaging interface for composing and buffering messages for sending over a socket
    class NetworkMessage {
        // Friend the socket class
        friend class TCPSocket;
    public:

//        // Header
//        // The header of a network message
//        struct Header {
//            friend class NetworkMessage;
//
//            // The size of the header
//            constexpr static size_t size = sizeof(unsigned);
//
//            // The amount of data the buffer contains
//            [[nodiscard]] constexpr size_t messageSize() const {
//                return sendMessageSize;
//            }
//
//            // Set the header's message size
//            void setMessageSize(size_t s) {
//                sendMessageSize = s;
//            }
//
//            // Create a bytestream representing the header
//            byte_buffer createHeaderBuffer();
//
//            // Load a bytestream into the header data
//            void loadFromBuffer(byte_buffer &&buffer);
//
//        private:
//            // The size of the sent message - note this will not necessarily be the
//            // buffer size, as the buffer will contain extra padding
//            unsigned sendMessageSize;
//        };

        // Default constructor
        NetworkMessage();

        // Constructor from a byte buffer and its size
        NetworkMessage(byte_buffer buffer, size_t bufferSize);

        // Deleted copy constructor
        NetworkMessage(const NetworkMessage &other) = delete;

        // Move constructor
        NetworkMessage(NetworkMessage &&other) noexcept;

        // Destructor
        ~NetworkMessage();

        // Deleted copy assignment operator
        NetworkMessage &operator=(const NetworkMessage &other) = delete;

        // Move assignment operator
        NetworkMessage &operator=(NetworkMessage &&other) noexcept;

        // Begin iterator for the start of the buffer
        const byte *begin();

        // End iterator for the end of the buffer
        const byte *end();

        // Clear the cache - it will be regenerated the next time it is implicitly requested
        void clearCache();

        constexpr static size_t _HeaderSize = sizeof(unsigned);

        virtual size_t headerSize() const;

    protected:
        virtual bool checkMessageValid() const;

        // Create the header object for the message
        virtual byte_buffer createHeader();

        virtual void reconstructHeader(byte_buffer headerBuffer);

        // Virtual method for copying the raw data into the cache. Can be overridden to manipulate the raw data,
        // for example by encrypting it
        virtual void copyDataToCache();

        // Virtual method for copying the cache data into the data buffer. Can be overridden to manipulate the data,
        // for example by decrypting it
        virtual void copyCacheToData();

        // Pointer to the logical beginning of the cache buffer, i.e. after the header
        byte *cacheDataBegin() const;

        // Pointer to the logical beginning of the data buffer
        byte *dataBegin() const;

        constexpr size_t messageSize() const;

        void setMessageSize(size_t size);

//        // The header for this message
//        Header header{};

    private:
        // Getter for the send stream
        shared_byte_buffer sendStream();

        // Getter for the size of the send stream
        size_t sendStreamSize();

        // Read the header data from a given array of bytes
        template<size_t _headerSize>
        void readHeader(const std::array<byte, _headerSize> &headerBuffer);

        // Read a buffer chunk into the internal data
        void readBuffer(const std::array<byte, BUFFER_CHUNK_SIZE> &buffer);

        // Get whether or not the message is expecting another chunk of data
        [[nodiscard]] bool expectingData() const;

        // Create the cache buffer from the header and data buffer
        void createBuffer();

        // Reconstruct the data buffer from the cache buffer and header
        void reconstruct();

        // Clear the data buffer
        void clearData();

        // Invalidate the message object (in the case of a move)
        void invalidate();

        // The data buffer (and its corresponding size)
        // The data buffer stores the raw byte data for this message
        byte_buffer dataBuffer;
        size_t dataBufferSize;

        // The cache buffer (and its corresponding size)
        // The cache buffer stores the complete buffer for communicating the data
        // for example the header and any encryption applied
        shared_byte_buffer __cached;
        size_t __cachedBufferSize;

        // The decoding index for tracking the current chunk of data being decoded
        size_t __decoderIndex{};
    };

    template<size_t _headerSize>
    void NetworkMessage::readHeader(const std::array<byte, _headerSize> &headerBuffer) {
        byte_buffer headerStream = std::make_unique<byte[]>(_headerSize);

        // Copy the data from the header buffer into the header object
        std::copy(headerBuffer.begin(), headerBuffer.end(), headerStream.get());

//    header.loadFromBuffer(std::move(headerStream));

        // Clear the cache in case it contained some data
        clearCache();

        reconstructHeader(std::move(headerStream));

        // Calculate the size of the cache based upon the header
        __cachedBufferSize = _headerSize + paddedBufferSize(dataBufferSize, BUFFER_CHUNK_SIZE);
        // Create the cache buffer of the correct size
        __cached = std::shared_ptr<byte[]>(new byte[__cachedBufferSize]);

        // Copy the header into the cache buffer
        std::copy(headerBuffer.begin(), headerBuffer.end(), __cached.get());

        // Initialise the decoder index to 0 - we have not actually decoded any chunks of data yet, just the
        // header
        __decoderIndex = 0;
    }

    // TODO: Distinguished classes for different types of messages

    class RawMessage : public NetworkMessage {

    };

    class RSAMessage : public NetworkMessage {
    public:
        // Getter for the size of an RSAMessage. They are required to always be exactly 2048 bits in size
        constexpr static size_t size() {
            return sizeof(uint2048);
        }

        // Default constructor
        RSAMessage();

        // Constructor from the data buffer
        RSAMessage(byte_buffer buffer, size_t bufferSize);

        // Deleted copy constructor
        RSAMessage(const RSAMessage &other) = delete;

        // Move constructor
        RSAMessage(RSAMessage &&other) noexcept;

        // Destructor
        ~RSAMessage();

        // Deleted copy assignment operator
        RSAMessage &operator=(const RSAMessage &other) = delete;

        // Move assignment operator
        RSAMessage &operator=(RSAMessage &&other) noexcept;

        // Setter for the public key used to encrypt messages
        void setPublicKey(const RSAKeyPair::Public &key);

        // Setter for the private key used to decrypt messages
        void setPrivateKey(const RSAKeyPair::Private &key);

        constexpr static size_t _HeaderSize = sizeof(unsigned);

        size_t headerSize() const override;

    protected:
        // Override for the validity checker for this message
        bool checkMessageValid() const override;

        // Override for copying the raw data buffer into the cache buffer - encrypts the message
        void copyDataToCache() override;

        // Override for copying the cache buffer into the raw data buffer - decrypts the message
        void copyCacheToData() override;

    private:
        // Const pointer to each key - these are left as null pointers if they aren't used
        const RSAKeyPair::Public *publicKey = nullptr;
        const RSAKeyPair::Private *privateKey = nullptr;
    };

    class AESMessage : public NetworkMessage {
    public:
        AESMessage();

        AESMessage(byte_buffer buffer, size_t bufferSize);

        AESMessage(const AESMessage &other) = delete;

        AESMessage(AESMessage &&other) noexcept;

        ~AESMessage();

        AESMessage &operator=(const AESMessage &other) = delete;

        AESMessage &operator=(AESMessage &&other) noexcept;

        void setAESKey(const AESKey &key);

        constexpr static size_t _HeaderSize = sizeof(unsigned) + sizeof(uint64);

        size_t headerSize() const override;

    protected:
        // Override for the validity checker for this message
        bool checkMessageValid() const override;

        // Override for header creation method
        byte_buffer createHeader() override;

        void reconstructHeader(byte_buffer headerBuffer) override;

        // Override for copying the raw data buffer into the cache buffer - encrypts the message
        void copyDataToCache() override;

        // Override for copying the cache buffer into the raw data buffer - decrypts the message
        void copyCacheToData() override;

    private:
        AESKey aesKey;

        uint64 initVector;
    };

}

#endif //CONTRACTS_SITE_CLIENT_NETWORKMESSAGE_H
