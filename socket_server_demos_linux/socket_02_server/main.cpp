#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

void do_service(int conn, sockaddr_in* peer);

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

    while (true) {
        int conn = accept(sock, reinterpret_cast<sockaddr*>(&peer_addr), &peer_len);
        pid_t pid = fork();
        if (pid == -1) {
            continue;
        }
        else if (pid == 0) {
            close(sock);
            do_service(conn, &peer_addr);
            close(conn);
            exit(EXIT_SUCCESS);
        }
        else {
            close(conn);
        }
    }
    close(sock);
    return 0;
}

void do_service(int conn, sockaddr_in* peer) {
    char recv_buf[1024] = { 0 };
    char ip[INET_ADDRSTRLEN] = { 0 };
    int recv_len;

    while ((recv_len = recv(conn, recv_buf, sizeof(recv_buf), 0)) > 0) {
        std::cout << inet_ntop(AF_INET, &peer->sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer->sin_port) <<
            ": " << recv_buf << std::endl;
        send(conn, recv_buf, strlen(recv_buf), 0);
        memset(recv_buf, 0, sizeof(recv_buf));
    }
    std::cout << inet_ntop(AF_INET, &peer->sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer->sin_port) <<
        "closed connection." << std::endl;
}
