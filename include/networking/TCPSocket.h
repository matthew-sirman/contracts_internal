//
// Created by Matthew.Sirman on 24/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_TCPSOCKET_H
#define CONTRACTS_SITE_CLIENT_TCPSOCKET_H

#include <WinSock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#include <strsafe.h>

#include <string>
#include <unordered_set>
#include <exception>
#include <memory>
#include <atomic>
#include <optional>

#include "NetworkMessage.h"

#define BACKLOG_QUEUE_SIZE 8
#define INVALID_SOCK (SOCKET)(~0ULL)

// Forward declare TCP socket
namespace networking {
    class TCPSocket;
}

namespace std {

    // Template specialisation for the hash function for TCP sockets, so they can
    // be used in unordered maps.
    template<>
    struct hash<networking::TCPSocket> {
        size_t operator()(const networking::TCPSocket &sock) const;
    };

}

namespace networking {

    struct TCPSocketSet;

    // TCPSocket
    // Wraps a low level C socket in a C++ style object
    class TCPSocket {
        // Friend the TCPSocketSet so it can read from the internal file descriptors
        friend struct TCPSocketSet;
        // Friend the hash struct so it can access the private socket file descriptor
        friend struct std::hash<TCPSocket>;
    public:
        // Default constructor
        TCPSocket();

        // Copy constructor
        TCPSocket(const TCPSocket &socket) noexcept;

        // Move constructor
        TCPSocket(TCPSocket &&socket) noexcept;

        // Destructor
        ~TCPSocket();

        // Copy assignment operator
        TCPSocket &operator=(const TCPSocket &other) noexcept;

        // Move assignment operator
        TCPSocket &operator=(TCPSocket &&other) noexcept;

        // Bool cast operator
        constexpr explicit operator bool() const noexcept;

        // Equality operator between two sockets
        bool operator==(const TCPSocket &other) const;

        // Create the socket. This is done out of the constructor as it may throw exceptions
        void create();

        // Bind the socket to a specific port and optionally a specific address
        void bind(unsigned short port, const std::string &host = std::string()) const;

        // Connect to a remote socket with an IP address and port. If the IP address string is a hostname,
        // an attempt to resolve the hostname will be made before excepting
        void connect(const std::string &host, unsigned short port) const;

        // Set this socket to non blocking mode
        void setNonBlocking();

        // Close the socket
        void close();

        // Set the socket to listen for incoming connections
        void listen() const;

        // Accept an incoming socket connection
        [[nodiscard]] TCPSocket accept() const;

        // Send a message to this remote socket
        void send(NetworkMessage &&message) const;

        // Receive a message from this remote socket
        [[nodiscard]] NetworkMessage receive() const;

        RSAMessage receiveRSA() const;

        AESMessage receiveAES() const;

        // Select method to check a socket set for sockets ready to read, write and check for exceptions
        static void select(TCPSocketSet &socketSet);

    private:
        // Invalidate this socket object. Note that as these sockets are copyable, this only invalidates
        // the C++ object, not necessarily the socket itself
        void invalidate();

        // Destroy the actual internal socket
        void destroy();

        // Internal file descriptor for the socket
        SOCKET sock;

        // Shared usage counter for copied sockets. The internal socket will be destroyed when it has no
        // remaining references
        int *__useCount;

        // Static management system for global socket reference counting. This allows for the WSA system
        // to be started up when the first socket is created and cleaned up when all sockets go out of scope.
        static std::atomic_int __globalSockUsage;
        static WSADATA __wsaData;
    };

    // SocketException
    // Thrown when the TCPSocket class faces an error. This exception may be caught with a try statement
    // for error handling.
    class SocketException : public std::exception {
    public:
        // Constructor taking a string message
        explicit SocketException(const std::string &message);

    private:
        // Format the message string into a full error message with the formatted string
        // from the internal socket interface
        [[nodiscard]] std::string formatMessage(const std::string &userMessage) const;
    };

    // TCPSocketSet
    // Represents a set of sockets which may be used in select calls. Wraps low level C select interface
    struct TCPSocketSet {
        // Friend the TCPSocket class so it may access the internal file descriptor sets
        friend class TCPSocket;
    public:
        // Constructor
        TCPSocketSet();

        // Add a "read" socket to the set (i.e. a socket which may be checked for ability to read)
        void addReadSocket(const TCPSocket &sock);

        // Add a "write" socket to the set (i.e. a socket which may be checked for ability to write)
        void addWriteSocket(const TCPSocket &sock);

        // Add an "exception" socket to the set (i.e. a socket which may be checked for exceptions)
        void addExceptSocket(const TCPSocket &sock);

        void setAcceptSocket(const TCPSocket &sock);

        // Get the subset of read sockets which are ready to read
        [[nodiscard]] std::unordered_set<TCPSocket> reads() const;

        // Get the subset of write sockets which are ready to write
        [[nodiscard]] std::unordered_set<TCPSocket> writes() const;

        // Get the subset of exception sockets which are ready to be checked
        [[nodiscard]] std::unordered_set<TCPSocket> excepts() const;

        [[nodiscard]] bool acceptReady() const;

    private:
        std::optional<TCPSocket> acceptSocket;
        // Internal sets of socket objects
        std::unordered_set<TCPSocket> readSockets, writeSockets, exceptSockets;
        // Internal C interface sets of file descriptors
        FD_SET readFds{}, writeFds{}, exceptFds{};
    };

}

#endif //CONTRACTS_SITE_CLIENT_TCPSOCKET_H
