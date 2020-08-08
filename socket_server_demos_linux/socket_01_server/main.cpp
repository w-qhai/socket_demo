#include <iostream>
#include <cstring>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

void do_service(int conn, sockaddr_in& peer);
void do_recv(int conn, sockaddr_in& peer);

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
    do_service(conn, peer_addr);
    close(conn);
    close(sock);
    return 0;
}

void do_service(int conn, sockaddr_in& peer) {
    std::thread recv_thread(do_recv, conn, std::ref(peer));
    recv_thread.detach();

    char send_buf[1024];
    while (std::cin.getline(send_buf, 1024)) {
        send(conn, send_buf, strlen(send_buf), 0);
    }

}

void do_recv(int conn, sockaddr_in& peer) {
    char recv_buf[1024] = { 0 };
    char ip[INET_ADDRSTRLEN];
    int recv_len;
    while ((recv_len = recv(conn, recv_buf, sizeof(recv_buf), 0)) > 0) {
        std::cout << "[" << inet_ntop(AF_INET, &peer.sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer.sin_port) <<
            "]:\t" << recv_buf << std::endl;
        memset(recv_buf, 0, sizeof(recv_buf));
    }
}
