//
// Created by Matthew.Sirman on 24/08/2020.
//

#include "../../include/networking/TCPSocket.h"

using namespace networking;

TCPSocketSet::TCPSocketSet() {
    FD_ZERO(&readFds);
    FD_ZERO(&writeFds);
    FD_ZERO(&exceptFds);
}

void TCPSocketSet::addReadSocket(const TCPSocket &sock) {
    readSockets.insert(sock);
    FD_SET(sock.sock, &readFds);
}

void TCPSocketSet::addWriteSocket(const TCPSocket &sock) {
    writeSockets.insert(sock);
    FD_SET(sock.sock, &writeFds);
}

void TCPSocketSet::addExceptSocket(const TCPSocket &sock) {
    exceptSockets.insert(sock);
    FD_SET(sock.sock, &exceptFds);
}

std::unordered_set<TCPSocket> TCPSocketSet::reads() const {
    std::unordered_set<TCPSocket> setSockets;

    for (const TCPSocket &sock : readSockets) {
        if (FD_ISSET(sock.sock, &readFds)) {
            setSockets.insert(sock);
        }
    }

    return std::move(setSockets);
}

std::unordered_set<TCPSocket> TCPSocketSet::writes() const {
    std::unordered_set<TCPSocket> setSockets;

    for (const TCPSocket &sock : writeSockets) {
        if (FD_ISSET(sock.sock, &writeFds)) {
            setSockets.insert(sock);
        }
    }

    return std::move(setSockets);
}

std::unordered_set<TCPSocket> TCPSocketSet::excepts() const {
    std::unordered_set<TCPSocket> setSockets;

    for (const TCPSocket &sock : exceptSockets) {
        if (FD_ISSET(sock.sock, &exceptFds)) {
            setSockets.insert(sock);
        }
    }

    return std::move(setSockets);
}

TCPSocket::TCPSocket()
        : sock(INVALID_SOCKET), __useCount(new int(1)) {

}

TCPSocket::TCPSocket(const TCPSocket &socket) noexcept {
    this->sock = socket.sock;
    this->__useCount = socket.__useCount;
    (*this->__useCount)++;
}

TCPSocket::TCPSocket(TCPSocket &&socket) noexcept {
    this->sock = socket.sock;
    this->__useCount = socket.__useCount;
    socket.invalidate();
}

TCPSocket::~TCPSocket() {
    if ((*__useCount)-- == 0) {
        destroy();
    }
}

TCPSocket &TCPSocket::operator=(const TCPSocket &other) noexcept {
    if (this == &other) {
        return *this;
    }

    this->sock = other.sock;
    this->__useCount = other.__useCount;
    (*this->__useCount)++;

    return *this;
}

TCPSocket &TCPSocket::operator=(TCPSocket &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    this->sock = other.sock;
    other.invalidate();

    return *this;
}

constexpr TCPSocket::operator bool() noexcept {
    return sock != INVALID_SOCKET;
}

void TCPSocket::create() {
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        throw SocketException("Failed to create socket.");
    }

    BOOL reuseAddr = TRUE;
    if ((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuseAddr, sizeof(BOOL))) != 0) {
        throw SocketException("Failed to set socket option reuse address.");
    }
}

void TCPSocket::bind(unsigned short port, const std::string &ip) {
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (ip.empty()) {
        address.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) < 0) {
            throw SocketException("Failed to convert IP address string.");
        }
    }

    if (::bind(sock, (SOCKADDR *) &address, sizeof(address)) == SOCKET_ERROR) {
        throw SocketException("Failed to bind socket.");
    }
}

void TCPSocket::connect(const std::string &ip, unsigned short port) {
    sockaddr_in serverAddress;

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddress.sin_addr) < 0) {
        throw SocketException("Failed to convert IP address string.");
    }

    if (::connect(sock, (SOCKADDR *) &serverAddress, sizeof(sockaddr_in)) == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEADDRNOTAVAIL) {
            addrinfo *result = NULL;
            addrinfo hints;

            ZeroMemory(&hints, sizeof(addrinfo));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            if (getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
                throw SocketException("Failed to resolve host name.");
            }

            serverAddress.sin_addr = ((sockaddr_in *) result)->sin_addr;

            if (::connect(sock, (SOCKADDR *) &serverAddress, sizeof(sockaddr_in)) != SOCKET_ERROR) {
                return;
            }
        }

        throw SocketException("Failed to connect to server.");
    }
}

void TCPSocket::setNonBlocking() {
    unsigned socketFlags;
    unsigned long nonBlocking = 1;

    if (ioctlsocket(sock, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        throw SocketException("Failed to set socket to non-blocking mode.");
    }
}

void TCPSocket::close() {
    destroy();
}

void TCPSocket::listen() {
    if (::listen(sock, BACKLOG_QUEUE_SIZE) == SOCKET_ERROR) {
        throw SocketException("Failed to set socket to listen.");
    }
}

TCPSocket TCPSocket::accept() const {
    sockaddr_in clientAddress;
    socklen_t clientAddressSize;

    TCPSocket acceptedSocket;

    SOCKET clientSocket = INVALID_SOCKET;

    if ((clientSocket = ::accept(sock, (SOCKADDR *) &clientAddress, &clientAddressSize)) == INVALID_SOCKET) {
        throw SocketException("Failed to accept socket.");
    }

    acceptedSocket.sock = clientSocket;

    return std::move(acceptedSocket);
}

void TCPSocket::send(NetworkMessage &&message) const {

}

NetworkMessage TCPSocket::receive() const {

}

void TCPSocket::select(TCPSocketSet &socketSet) {
    if (::select(0, &socketSet.readFds, &socketSet.writeFds, &socketSet.exceptFds, nullptr) == SOCKET_ERROR) {
        throw SocketException("Failed to select socket file descriptors.");
    }
}

void TCPSocket::invalidate() {
    sock = INVALID_SOCKET;
}

void TCPSocket::destroy() {
    *__useCount = 0;
    if (sock != INVALID_SOCKET) {
        shutdown(sock, SD_BOTH);
        closesocket(sock);
        invalidate();
    }
}

SocketException::SocketException(const std::string &message)
        : std::exception(formatMessage(message).c_str()) {

}

std::string SocketException::formatMessage(const std::string &userMessage) const {
    LPVOID messageBuffer;
    LPVOID displayBuffer;
    DWORD code = WSAGetLastError();

    FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR) &messageBuffer,
            0, NULL
    );

    displayBuffer = (LPVOID) LocalAlloc(
            LMEM_ZEROINIT,
            (lstrlen((LPCTSTR) messageBuffer) + userMessage.size() + 40) * sizeof(TCHAR)
    );
    StringCchPrintf((LPTSTR) displayBuffer,
                    LocalSize(displayBuffer) / sizeof(TCHAR),
                    TEXT("%s. %s (%d)"),
                    (LPTSTR) userMessage.c_str(), messageBuffer, code
    );

    std::string message((const char *) displayBuffer, lstrlen((LPCTSTR) displayBuffer));

    LocalFree(messageBuffer);
    LocalFree(displayBuffer);

    return message;
}

WinSockManager::WinSockManager() = default;

WinSockManager::~WinSockManager() {
    WSACleanup();
}

bool WinSockManager::start() {
    if (WSAStartup(MAKEWORD(2u, 2u), &wsaData) != 0) {
        return false;
    }
    return true;
}
