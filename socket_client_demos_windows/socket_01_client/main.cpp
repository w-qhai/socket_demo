// link with Ws2_32.lib
#pragma comment(lib,"Ws2_32.lib")

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

int __cdecl wmain(int argc, char** argv) {
    //-----------------------------------------
    // Declare and initialize variables
    WSADATA wsaData = { 0 };
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        std::cerr << "WSAStartup WSAStartup " << std::endl;
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serve_addr;
    memset(&serve_addr, 0, sizeof(serve_addr));
    serve_addr.sin_family = AF_INET;
    serve_addr.sin_port = htons(5188);
    inet_pton(AF_INET, "127.0.0.1", &serve_addr.sin_addr.s_addr); /*server ip*/

    if (connect(sock, reinterpret_cast<sockaddr*>(&serve_addr), sizeof(serve_addr)) < 0) {
        std::cerr << "connect faild" << std::endl;
    }

    char send_buf[1024];
    char recv_buf[1024];

    while (std::cin.getline(send_buf, 1024)) {
        memset(recv_buf, 0, sizeof(recv_buf));
        send(sock, send_buf, strlen(send_buf), 0);
        recv(sock, recv_buf, sizeof(recv_buf), 0);
        std::cout << "recv: " << recv_buf << std::endl;
    }

    closesocket(sock);
    WSACleanup();

    return 0;
}