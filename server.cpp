#include <iostream>
#include <string>
#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#endif

static void cleanupSocket();
static void closeSocket(int sock);

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    int listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket < 0) {
        std::cerr << "socket failed\n";
        cleanupSocket();
        return 1;
    }

    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = htonl(INADDR_ANY);
    service.sin_port = htons(54000);

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&service), sizeof(service)) < 0) {
        std::cerr << "bind failed\n";
        closeSocket(listenSocket);
        cleanupSocket();
        return 1;
    }

    if (listen(listenSocket, 1) < 0) {
        std::cerr << "listen failed\n";
        closeSocket(listenSocket);
        cleanupSocket();
        return 1;
    }

    std::cout << "Server listening on port 54000" << std::endl;

    sockaddr_in clientInfo{};
#ifdef _WIN32
    int clientSize = static_cast<int>(sizeof(clientInfo));
#else
    socklen_t clientSize = sizeof(clientInfo);
#endif
    int clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientInfo), &clientSize);
    if (clientSocket < 0) {
        std::cerr << "accept failed\n";
        closeSocket(listenSocket);
        cleanupSocket();
        return 1;
    }

    char clientIp[INET_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &clientInfo.sin_addr, clientIp, static_cast<socklen_t>(sizeof(clientIp)));
    std::cout << "Client connected: " << clientIp << ":" << ntohs(clientInfo.sin_port) << std::endl;

    constexpr int bufferSize = 4096;
    char buffer[bufferSize];

    int bytesReceived = recv(clientSocket, buffer, bufferSize - 1, 0);
    if (bytesReceived <= 0) {
        std::cerr << "recv failed or connection closed\n";
        closeSocket(clientSocket);
        closeSocket(listenSocket);
        cleanupSocket();
        return 1;
    }

    buffer[bytesReceived] = '\0';
    std::cout << "Received: " << buffer << std::endl;

    std::string reply = "Echo: ";
    reply += buffer;

    int bytesSent = send(clientSocket, reply.c_str(), static_cast<int>(reply.size()), 0);
    if (bytesSent < 0) {
        std::cerr << "send failed\n";
    }

    closeSocket(clientSocket);
    closeSocket(listenSocket);
    cleanupSocket();
    return 0;
}

static void closeSocket(int sock) {
#ifdef _WIN32
    closesocket(static_cast<SOCKET>(sock));
#else
    close(sock);
#endif
}

static void cleanupSocket() {
#ifdef _WIN32
    WSACleanup();
#endif
}
