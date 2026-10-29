#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socket_t = SOCKET;
    #define CLOSE_SOCKET closesocket
    #define INVALID_SOCK INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    using socket_t = int;
    #define CLOSE_SOCKET close
    #define INVALID_SOCK (-1)
#endif

static bool init_sockets() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }
#endif
    return true;
}

static void cleanup_sockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}

int main(int argc, char* argv[]) {
    const char* server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    const int   server_port = (argc > 2) ? std::atoi(argv[2]) : 9000;

    if (!init_sockets()) return 1;

    socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCK) {
        std::cerr << "socket() failed\n";
        cleanup_sockets();
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<unsigned short>(server_port));
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) != 1) {
        std::cerr << "inet_pton failed\n";
        CLOSE_SOCKET(sock);
        cleanup_sockets();
        return 1;
    }

    std::cout << "Client -> " << server_ip << ":" << server_port
              << std::endl;


     
    int sent = sendto(sock,
                    nullptr, 0,
                    0,
                    reinterpret_cast<sockaddr*>(&addr),
                    sizeof(addr));
        if (sent < 0) {
            std::cerr << "sendto failed\n";
        } else {
            std::cout << "Sent packet #" << std::endl;
        }
  

    CLOSE_SOCKET(sock);
    cleanup_sockets();
    return 0;
}