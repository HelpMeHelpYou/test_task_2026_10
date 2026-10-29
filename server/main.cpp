#include <iostream>
#include <atomic>
#include <cstring>
#include <csignal>
#include <thread>

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

static std::atomic<bool> g_running{true};
static socket_t g_sock = INVALID_SOCK;

static void signal_handler(int) {
    g_running = false;
    // Закрываем сокет, чтобы разблокировать recvfrom
    if (g_sock != INVALID_SOCK) {
        CLOSE_SOCKET(g_sock);
        g_sock = INVALID_SOCK;
    }
}

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
    const int port = (argc > 1) ? std::atoi(argv[1]) : 9000;

    if (!init_sockets()) return 1;

    g_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_sock == INVALID_SOCK) {
        std::cerr << "socket() failed\n";
        cleanup_sockets();
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(static_cast<unsigned short>(port));

    if (bind(g_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind() failed on port " << port << "\n";
        CLOSE_SOCKET(g_sock);
        cleanup_sockets();
        return 1;
    }

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "Server listening on UDP port " << port << "\n";

    // Счётчик, увеличивается на 1 при каждом полученном пакете
    long long counter = 0;

    while (g_running) {
        sockaddr_in client_addr{};
#ifdef _WIN32
        int client_len = sizeof(client_addr);
#else
        socklen_t client_len = sizeof(client_addr);
#endif
        char buf[1024];

        // Синхронный приём: блокируется до прихода пакета
        int n = recvfrom(g_sock,
                         buf, sizeof(buf),
                         0,
                         reinterpret_cast<sockaddr*>(&client_addr),
                         &client_len);

        if (n < 0) {
            if (!g_running) break;   // штатное завершение по сигналу
            std::cerr << "recvfrom failed\n";
            continue;
        }

        ++counter;

        char ip[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        int cport = ntohs(client_addr.sin_port);

        std::cout << "Packet from " << ip << ":" << cport
                  << " (size=" << n << "), counter = " << counter
                  << std::endl;
    }

    if (g_sock != INVALID_SOCK) CLOSE_SOCKET(g_sock);
    cleanup_sockets();
    std::cout << "Server stopped. Total packets: " << counter << "\n";
    return 0;
}