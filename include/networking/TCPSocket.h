//
// Created by Matthew.Sirman on 24/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_TCPSOCKET_H
#define CONTRACTS_SITE_CLIENT_TCPSOCKET_H

#include <Windows.h>
#include <ws2tcpip.h>
#include <strsafe.h>

#include <string>
#include <unordered_set>
#include <exception>
#include <memory>

#include "NetworkMessage.h"

#define BACKLOG_QUEUE_SIZE 8

namespace networking {

    class TCPSocket;

    struct TCPSocketSet {
        friend class TCPSocket;
    public:
        TCPSocketSet();

        void addReadSocket(const TCPSocket &sock);

        void addWriteSocket(const TCPSocket &sock);

        void addExceptSocket(const TCPSocket &sock);

        std::unordered_set<TCPSocket> reads() const;

        std::unordered_set<TCPSocket> writes() const;

        std::unordered_set<TCPSocket> excepts() const;

    private:
        std::unordered_set<TCPSocket> readSockets, writeSockets, exceptSockets;
        FD_SET readFds, writeFds, exceptFds;
    };

    class TCPSocket {
        friend struct TCPSocketSet;
    public:
        TCPSocket();

        TCPSocket(const TCPSocket &socket) noexcept;

        TCPSocket(TCPSocket &&socket) noexcept;

        ~TCPSocket();

        TCPSocket &operator=(const TCPSocket &other) noexcept;

        TCPSocket &operator=(TCPSocket &&other) noexcept;

        constexpr operator bool() noexcept;

        void create();

        void bind(unsigned short port, const std::string &ip = std::string());

        void connect(const std::string &ip, unsigned short port);

        void setNonBlocking();

        void close();

        void listen();

        TCPSocket accept() const;

        void send(NetworkMessage &&message) const;

        NetworkMessage receive() const;

        static void select(TCPSocketSet &socketSet);

    private:
        void invalidate();

        void destroy();

        SOCKET sock;

        int *__useCount;
    };

    class SocketException : public std::exception {
    public:
        SocketException(const std::string &message);

    private:
        std::string formatMessage(const std::string &userMessage) const;
    };

    class WinSockManager {
    public:
        WinSockManager();

        ~WinSockManager();

        bool start();

    private:
        WSADATA wsaData;
    };

}

#endif //CONTRACTS_SITE_CLIENT_TCPSOCKET_H
