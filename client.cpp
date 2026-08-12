#include <iostream>
#include <string>
#include <sstream>
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

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    unsigned short port = 54000;
    std::string message = "Hello from client!";

    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        message = argv[2];
        for (int i = 3; i < argc; ++i) {
            message += " ";
            message += argv[i];
        }
    }

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    int connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectSocket < 0) {
        std::cerr << "socket failed\n";
        cleanupSocket();
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
#ifdef _WIN32
    InetPton(AF_INET, host.c_str(), &serverAddress.sin_addr);
#else
    if (inet_pton(AF_INET, host.c_str(), &serverAddress.sin_addr) != 1) {
        std::cerr << "Invalid address: " << host << std::endl;
        closeSocket(connectSocket);
        cleanupSocket();
        return 1;
    }
#endif

    std::cout << "Connecting to " << host << ":" << port << std::endl;
    if (connect(connectSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) < 0) {
        std::cerr << "connect failed\n";
        closeSocket(connectSocket);
        cleanupSocket();
        return 1;
    }

    int bytesSent = send(connectSocket, message.c_str(), static_cast<int>(message.size()), 0);
    if (bytesSent < 0) {
        std::cerr << "send failed\n";
        closeSocket(connectSocket);
        cleanupSocket();
        return 1;
    }

    constexpr int bufferSize = 4096;
    char buffer[bufferSize];
    int bytesReceived = recv(connectSocket, buffer, bufferSize - 1, 0);
    if (bytesReceived <= 0) {
        std::cerr << "recv failed or connection closed\n";
        closeSocket(connectSocket);
        cleanupSocket();
        return 1;
    }
    buffer[bytesReceived] = '\0';
    std::cout << "Server replied: " << buffer << std::endl;

    closeSocket(connectSocket);
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
