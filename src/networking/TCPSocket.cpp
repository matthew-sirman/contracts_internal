//
// Created by Matthew.Sirman on 24/08/2020.
//

#include <array>

#include "../../include/networking/TCPSocket.h"

using namespace networking;

// Declare the static values
std::atomic_int TCPSocket::__globalSockUsage = 0;
WSADATA TCPSocket::__wsaData;

size_t std::hash<networking::TCPSocket>::operator()(const TCPSocket &sock) const {
    // Return the has of the socket file descriptor as this identifies a unique socket
    return std::hash<unsigned long long>()(sock.sock);
}

TCPSocketSet::TCPSocketSet() {
    // Zero each file descriptor set to initialise them
    FD_ZERO(&readFds);
    FD_ZERO(&writeFds);
    FD_ZERO(&exceptFds);
}

void TCPSocketSet::addReadSocket(const TCPSocket &sock) {
    // Add the socket to the read sockets set object
    readSockets.insert(sock);
    // Add the file descriptor to the internal set
    FD_SET(sock.sock, &readFds);
}

void TCPSocketSet::addWriteSocket(const TCPSocket &sock) {
    // Add the socket to the write sockets set object
    writeSockets.insert(sock);
    // Add the file descriptor to the internal set
    FD_SET(sock.sock, &writeFds);
}

void TCPSocketSet::addExceptSocket(const TCPSocket &sock) {
    // Add the socket to the except sockets set object
    exceptSockets.insert(sock);
    // Add the file descriptor to the internal set
    FD_SET(sock.sock, &exceptFds);
}

void TCPSocketSet::setAcceptSocket(const TCPSocket &sock) {
    acceptSocket = sock;
    FD_SET(sock.sock, &readFds);
}

std::unordered_set<TCPSocket> TCPSocketSet::reads() const {
    // Create an empty unordered set for each set socket
    std::unordered_set<TCPSocket> setSockets;

    // Loop over each socket
    for (const TCPSocket &sock : readSockets) {
        // If the socket is set (by select) add it to the set sockets
        if (FD_ISSET(sock.sock, &readFds)) {
            setSockets.insert(sock);
        }
    }

    // Return the set of set sockets
    return std::move(setSockets);
}

std::unordered_set<TCPSocket> TCPSocketSet::writes() const {
    // Create an empty unordered set for each set socket
    std::unordered_set<TCPSocket> setSockets;

    // Loop over each socket
    for (const TCPSocket &sock : writeSockets) {
        // If the socket is set (by select) add it to the set sockets
        if (FD_ISSET(sock.sock, &writeFds)) {
            setSockets.insert(sock);
        }
    }

    // Return the set of set sockets
    return std::move(setSockets);
}

std::unordered_set<TCPSocket> TCPSocketSet::excepts() const {
    // Create an empty unordered set for each set socket
    std::unordered_set<TCPSocket> setSockets;

    // Loop over each socket
    for (const TCPSocket &sock : exceptSockets) {
        // If the socket is set (by select) add it to the set sockets
        if (FD_ISSET(sock.sock, &exceptFds)) {
            setSockets.insert(sock);
        }
    }

    // Return the set of set sockets
    return std::move(setSockets);
}

bool TCPSocketSet::acceptReady() const {
    if (!acceptSocket.has_value()) {
        return false;
    }
    return FD_ISSET(acceptSocket->sock, &readFds);
}

TCPSocket::TCPSocket()
        : sock(INVALID_SOCK), __useCount(nullptr) {
    // If the global usage counter is 0, this indicates that WSA is not currently initialise,
    // so start it up
    if (__globalSockUsage == 0) {
        WSAStartup(MAKEWORD(2u, 2u), &__wsaData);
    }

    // Increment the global socket usage
    __globalSockUsage++;
}

TCPSocket::TCPSocket(const TCPSocket &socket) noexcept {
    // Copy the socket file descriptor
    this->sock = socket.sock;
    // Copy the pointer to the usage counter (we do not have to worry about the usage counter already having
    // a value as this is the constructor)
    this->__useCount = socket.__useCount;
    // If the usage counter is not null, increment it - we are making a copy of this object,
    // so there is now one extra reference to it
    if (this->__useCount) {
        (*this->__useCount)++;
    }

    // Increment the global socket usage
    __globalSockUsage++;
}

TCPSocket::TCPSocket(TCPSocket &&socket) noexcept {
    // Copy the socket file descriptor
    this->sock = socket.sock;
    // Copy the pointer to the usage counter (we do not have to worry about the usage counter already having
    // a value as this is the constructor)
    this->__useCount = socket.__useCount;
    // Note: we do not increment the usage counter here because this is a move - the original socket object is
    // being invalidated and so the total usages doesn't change

    // Invalidate the original socket object - we have moved its contents into this object
    socket.invalidate();

    // Increment the global socket usage
    __globalSockUsage++;
}

TCPSocket::~TCPSocket() {
    // If the socket isn't invalid (i.e. has been created, or copied or moved from another socket)
    if (sock != INVALID_SOCK) {
        // We decrement the use count as this object is losing its reference. If the use count is now 0,
        // we destroy the internal socket as there are no longer any references to it
        if (--(*__useCount) == 0) {
            destroy();
        }
    }
    // We always decrement the global socket counter - this simply tracks the number of TCPSocket objects in
    // existence, and this is the destructor so one less will exist.
    if (!(--__globalSockUsage)) {
        // If there are no more TCPSocket objects, we cleanup WSA.
        WSACleanup();
    }
}

TCPSocket &TCPSocket::operator=(const TCPSocket &other) noexcept {
    // If the object we are trying to set to is actually just this object, we don't have to do anything
    if (this == &other) {
        return *this;
    }

    // If this socket is not invalid, we need to clean up the reference we have to it (as we are losing it by
    // changing to another socket)
    if (this->sock != INVALID_SOCK) {
        // Decrement the original usage counter. If the counter drops to 0, we have just lost the last reference
        // so destroy the socket.
        if (--(*this->__useCount) == 0) {
            destroy();
        }
    }

    // Copy across the file descriptor
    this->sock = other.sock;
    // Copy across the usage counter
    this->__useCount = other.__useCount;
    // If the usage counter isn't null (i.e. we are copying an actual socket rather than a null socket) then
    // we increment the usage counter
    if (this->__useCount) {
        (*this->__useCount)++;
    }

    return *this;
}

TCPSocket &TCPSocket::operator=(TCPSocket &&other) noexcept {
    // If the object we are trying to set to is actually just this object, we don't have to do anything
    if (this == &other) {
        return *this;
    }

    // If this socket is not invalid, we need to clean up the reference we have to it (as we are losing it by
    // changing to another socket)
    if (this->sock != INVALID_SOCK) {
        // Decrement the original usage counter. If the counter drops to 0, we have just lost the last reference
        // so destroy the socket.
        if (--(*this->__useCount) == 0) {
            destroy();
        }
    }

    // Copy across the file descriptor
    this->sock = other.sock;
    // Copy across the usage counter
    this->__useCount = other.__useCount;
    // Note: we do not increment the usage counter here because this is a move - the other socket object is
    // being invalidated and so the total usages doesn't change

    // Invalidate the other socket object - we have moved its contents into this object
    other.invalidate();

    return *this;
}

constexpr TCPSocket::operator bool() const noexcept {
    // Returns true if this socket is valid
    return sock != INVALID_SOCK;
}

bool TCPSocket::operator==(const TCPSocket &other) const {
    // Returns true if the internal socket file descriptors are equal
    return this->sock == other.sock;
}

void TCPSocket::create() {
    // Create the socket object. If there is an error, throw an exception
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCK) {
        throw SocketException("Failed to create socket");
    }

    BOOL reuseAddr = TRUE;
    // Set the socket to reuse the address if necessary
    if ((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuseAddr, sizeof(BOOL))) != 0) {
        throw SocketException("Failed to set socket option reuse address");
    }

    // Initialise the usage counter to 1 - we now have a live socket object with a single reference
    __useCount = new int(1);
}

void TCPSocket::bind(unsigned short port, const std::string &host) const {
    // Declare an address value to bind to
    sockaddr_in address{};
    // Set the family to AF_INET
    address.sin_family = AF_INET;
    // Set the port to the (converted) input port
    address.sin_port = htons(port);

    if (host.empty()) {
        // If the address is empty bind to INADDR_ANY which represents 0.0.0.0, or any address
        address.sin_addr.s_addr = INADDR_ANY;
    } else {
        // Otherwise translate the ip address string into a binary address and throw an error if this fails
        // (i.e. the ip string was invalid)
        if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) < 0) {
            throw SocketException("Failed to convert IP address string");
        }
    }

    // Call the socket interface's bind function to bind this socket object to the constructed address
    if (::bind(sock, (SOCKADDR *) &address, sizeof(address)) == SOCKET_ERROR) {
        throw SocketException("Failed to bind socket");
    }
}

void TCPSocket::connect(const std::string &host, unsigned short port) const {
    // Declare an address value to connect to
    sockaddr_in serverAddress{};
    // Set the family to AF_INET
    serverAddress.sin_family = AF_INET;
    // Set the port to the (converted) input port
    serverAddress.sin_port = htons(port);

    // First try resolving the host as an IP address
    if (inet_pton(AF_INET, host.c_str(), &serverAddress.sin_addr) == 0) {
        // If that failed, we next try resolving as a host name

        // Declare a result pointer for the linked list of hosts
        addrinfo *result = nullptr;
        // Declare hints for the type of host we are looking for
        addrinfo hints{};

        // Initialise the hints and set the family type an protocol to what we are looking for
        ZeroMemory(&hints, sizeof(addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        // Attempt to get the address info from the specified host name
        if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
            throw SocketException("Failed to resolve host name");
        }

        // Set the server address to the first hostname in the linked list
        serverAddress.sin_addr = ((sockaddr_in *) result->ai_addr)->sin_addr;
    }

    // Attempt to connect to whichever host address we ended up with by this point. We know the address is
    // valid, but not necessarily that we will be able to connect
    if (::connect(sock, (SOCKADDR *) &serverAddress, sizeof(sockaddr_in)) == SOCKET_ERROR) {
        throw SocketException("Failed to connect to server");
    }
}

void TCPSocket::setNonBlocking() {
    // Create a value to set the non blocking property to - 1 for true
    unsigned long nonBlocking = 1;

    // Set the socket object's non blocking property to true
    if (ioctlsocket(sock, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        throw SocketException("Failed to set socket to non-blocking mode");
    }
}

void TCPSocket::close() {
    // If the socket has been explicitly closed, destroy it
    destroy();
}

void TCPSocket::listen() const {
    // Call the listen interface on this socket
    if (::listen(sock, BACKLOG_QUEUE_SIZE) == SOCKET_ERROR) {
        throw SocketException("Failed to set socket to listen");
    }
}

TCPSocket TCPSocket::accept() const {
    // Declare an address variable for the accepted client's address
    sockaddr_in clientAddress{};
    socklen_t clientAddressSize = sizeof(sockaddr_in);

    // Declare the socket object for this incoming client
    TCPSocket acceptedSocket;

    // Create an internal socket file descriptor initialised to invalid
    SOCKET clientSocket = INVALID_SOCK;

    // Accept the client socket
    if ((clientSocket = ::accept(sock, (SOCKADDR *) &clientAddress, &clientAddressSize)) == INVALID_SOCK) {
        throw SocketException("Failed to accept socket");
    }

    // Set the socket object's file descriptor to the accepted socket
    acceptedSocket.sock = clientSocket;
    // This is a new socket, but is not created internally, so we explicitly initialise the usage counter
    // to 1
    acceptedSocket.__useCount = new int(1);

    // Return the accepted socket by moving it
    return std::move(acceptedSocket);
}

void TCPSocket::send(MessageBase &&message) const {
    // Create the network message to send
    NetworkMessage networkMessage = message.message();
    // Send the raw message data from the message object
    ::send(sock, (const char *) networkMessage.begin(), networkMessage.bufferSize(), 0);
}

NetworkMessage TCPSocket::receive() const {
    // Create an empty message object
    NetworkMessageDecoder decoder;

    // Declare a fixed size array for the header of the message. The header is sent
    // initially, followed by the body
    std::array<byte, NetworkMessage::HeaderSize> header{};

    // Receive the header data
    size_t testSize = ::recv(sock, (char *) header.data(), header.size(), 0);
    // Pass the header data to the message so it can decode it
    decoder.decodeHeader(header);

    // Declare an array for the message chunks
    std::array<byte, NetworkMessage::BufferChunkSize> chunk{};

    bool test = false;

    if (test) {
        ::recv(sock, (char *) chunk.data(), chunk.size(), 0);
    }

    // For as long as the message object is expecting data
    while (decoder.expectingData()) {
        // Receive the next fixed size chunk
        ::recv(sock, (char *) chunk.data(), chunk.size(), 0);
        // Pass the chunk to the message so it can decode it
        decoder.decodeChunk(chunk);
    }

    // Return the message by transferring ownership
    return decoder.create();
}

//RSAMessage TCPSocket::receiveRSA() const {
//    // Create an empty message object
//    RSAMessage message;
//
//    // Declare a fixed size array for the header of the message. The header is sent
//    // initially, followed by the body
//    std::array<byte, RSAMessage::_HeaderSize> header{};
//
//    // Receive the header data
//    ::recv(sock, (char *) header.data(), header.size(), 0);
//    // Pass the header data to the message so it can decode it
//    message.readHeader(header);
//
//    // Declare an array for the message chunks
//    std::array<byte, BUFFER_CHUNK_SIZE> chunk{};
//
//    // For as long as the message object is expecting data
//    while (message.expectingData()) {
//        // Receive the next fixed size chunk
//        ::recv(sock, (char *) chunk.data(), chunk.size(), 0);
//        // Pass the chunk to the message so it can decode it
//        message.readBuffer(chunk);
//    }
//
//    // Return the message by transferring ownership
//    return std::move(message);
//}
//
//AESMessage TCPSocket::receiveAES() const {
//    // Create an empty message object
//    AESMessage message;
//
//    // Declare a fixed size array for the header of the message. The header is sent
//    // initially, followed by the body
//    std::array<byte, AESMessage::_HeaderSize> header{};
//
//    // Receive the header data
//    ::recv(sock, (char *) header.data(), header.size(), 0);
//    // Pass the header data to the message so it can decode it
//    message.readHeader(header);
//
//    // Declare an array for the message chunks
//    std::array<byte, BUFFER_CHUNK_SIZE> chunk{};
//
//    // For as long as the message object is expecting data
//    while (message.expectingData()) {
//        // Receive the next fixed size chunk
//        ::recv(sock, (char *) chunk.data(), chunk.size(), 0);
//        // Pass the chunk to the message so it can decode it
//        message.readBuffer(chunk);
//    }
//
//    // Return the message by transferring ownership
//    return std::move(message);
//}

void TCPSocket::select(TCPSocketSet &socketSet) {
    // Call the select interface with the socket sets defined in the passed in object
    if (::select(0, &socketSet.readFds, &socketSet.writeFds, &socketSet.exceptFds, nullptr) == SOCKET_ERROR) {
        throw SocketException("Failed to select socket file descriptors");
    }
}

void TCPSocket::invalidate() {
    // Set the socket to be the invalid socket
    sock = INVALID_SOCK;
    // Set the usage counter to be a null pointer
    __useCount = nullptr;
}

void TCPSocket::destroy() {
    // Delete the usage counter - we are destroying this socket and so there should be no
    // usages
    delete __useCount;
    // If the socket was actually set
    if (sock != INVALID_SOCK) {
        // Shut it down and close it
        shutdown(sock, SD_BOTH);
        closesocket(sock);
        // Invalidate the socket object (in case the object still exists, to notify that the actual socket
        // is closed)
        invalidate();
    }
}

SocketException::SocketException(const std::string &message)
        : std::exception(formatMessage(message).c_str()) {

}

std::string SocketException::formatMessage(const std::string &userMessage) const {
    // Declare buffers for the error message and the return display buffer
    LPVOID messageBuffer;
    LPVOID displayBuffer;
    // Get the error code
    DWORD code = WSAGetLastError();

    // Format the message into the message buffer string
    FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR) &messageBuffer,
            0, nullptr
    );

    // Create the buffer for the display message
    displayBuffer = (LPVOID) LocalAlloc(
            LMEM_ZEROINIT,
            (lstrlen((LPCTSTR) messageBuffer) + userMessage.size() + 40) * sizeof(TCHAR)
    );
    // Print the passed in message, the error message and the error code into the buffer
    StringCchPrintf((LPTSTR) displayBuffer,
                    LocalSize(displayBuffer) / sizeof(TCHAR),
                    TEXT("%s (%d): %s"),
                    (LPTSTR) userMessage.c_str(), code, messageBuffer
    );

    // Convert the raw string back into a C++ string
    std::string message((const char *) displayBuffer, lstrlen((LPCTSTR) displayBuffer));

    // Free the two local buffers
    LocalFree(messageBuffer);
    LocalFree(displayBuffer);

    // Return the actual complete error message string
    return message;
}