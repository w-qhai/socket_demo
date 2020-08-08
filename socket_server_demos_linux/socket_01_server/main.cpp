#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char** argv) {

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serve_addr;
    memset(&serve_addr, 0, sizeof(serve_addr));
    serve_addr.sin_family = AF_INET;
    serve_addr.sin_port = htons(5188);
    inet_pton(AF_INET, "0.0.0.0", &serve_addr.sin_addr.s_addr); /*server ip*/

    int on = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    bind(sock, reinterpret_cast<sockaddr*>(&serve_addr), sizeof(serve_addr));
    listen(sock, SOMAXCONN);

    sockaddr_in peer_addr;
    socklen_t peer_len = sizeof(peer_addr);

    int conn = accept(sock, reinterpret_cast<sockaddr*>(&peer_addr), &peer_len);
    char recv_buf[1024] = { 0 };
    char ip[INET_ADDRSTRLEN];

    std::cout << inet_ntop(AF_INET, &peer_addr.sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer_addr.sin_port) << ": " << std::endl;
    while (true) {
        int recv_len = recv(conn, recv_buf, sizeof(recv_buf), 0);
        std::cout << recv_len << std::endl;
        if (recv_len <= 0) {
            std::cout << "closed connection." << std::endl;
            break;
        }
        else {
            std::cout << recv_buf << std::endl;
        }
        send(conn, recv_buf, recv_len, 0);
        memset(recv_buf, 0, sizeof(recv_buf));
    }

    close(conn);
    close(sock);
    return 0;
}
