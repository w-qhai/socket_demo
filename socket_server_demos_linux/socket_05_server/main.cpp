#include <iostream>
#include <cstring>
#include <array>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SOCKET int

const int BUF_SIZE = 1024;
struct Packet {
    size_t len = 0;
    char buf[BUF_SIZE] = { 0 };
};

int send_packet(SOCKET sock, void* buf, int len, int flag);
int recv_packet(SOCKET sock, void* buf, int len, int flag);

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

    std::array<pollfd, 1024> client;
    sockaddr_in peer_addr[FD_SETSIZE];
    socklen_t peer_len[FD_SETSIZE];
    for (int i = 0; i < FD_SETSIZE; i++) {
        client[i].fd = -1;
    }
    client[0].fd = sock;
    client[0].events = POLLIN;

    while (true) {
        int nready = poll(client.data(), client.size(), -1);
        if (nready == -1) {
            int err = errno;
            if (err == EINTR) {
                continue;
            }
            std::cout << strerror(err) << std::endl;
            break;
        }

        if (nready > 0 && client[0].revents & POLLIN) {
            char ip[INET_ADDRSTRLEN];
            sockaddr_in peer;
            socklen_t len = sizeof(peer);
            int conn = accept(sock, reinterpret_cast<sockaddr*>(&peer), &len);
            peer_addr[conn] = peer;
            peer_len[conn] = len;
            std::cout << "[" << inet_ntop(AF_INET, &peer_addr[conn].sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer_addr[conn].sin_port) << "]: "
                << "connected" << std::endl;
            for (int  i = 0; i < client.size(); i++) {
                if (client[i].fd == -1) {
                    client[i].fd = conn;
                    client[i].events = POLLIN;
                    break;
                }
            }
            nready--;
        }

        for (int i = 0; nready > 0 && i <  client.size(); i++) {
            if (client[i].fd != -1 && client[i].revents & POLLIN) {
                int conn = client[i].fd;
                Packet packet;
                char ip[INET_ADDRSTRLEN];

                int head_len = recv_packet(conn, &packet.len, sizeof(packet.len), 0);
                if (head_len == 0) {
                    std::cout << "[" << inet_ntop(AF_INET, &peer_addr[conn].sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer_addr[conn].sin_port) << "]: "
                     << "closed connection." << std::endl;
                    continue;
                }
                int body_len = recv_packet(conn, packet.buf, ntohl(packet.len), 0);
                if (body_len == 0) {
                    std::cout << "[" << inet_ntop(AF_INET, &peer_addr[conn].sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer_addr[conn].sin_port) << "]: "
                     << "closed connection." << std::endl;
                    client[i].fd = -1;
                    close(conn);
                    continue;
                }
           
                std::cout << "[" << inet_ntop(AF_INET, &peer_addr[conn].sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer_addr[conn].sin_port) << "]: "
                    <<  packet.buf << std::endl;
                // 返回给客户端
                send_packet(conn, &packet, sizeof(packet.len) + ntohl(packet.len), 0);
                nready--;
            }
        }
    }
    close(sock);
    return 0;
}

int send_packet(SOCKET sock, void* buf, int packet_size, int flag) {
    char* buf_ptr = reinterpret_cast<char*>(buf);
    int   sent_size = 0;

    while (sent_size < packet_size) {
        int len = send(sock, buf_ptr + sent_size, packet_size - sent_size, flag);
        if (len < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        else if (len == 0) {
            break;
        }
        sent_size += len;
    }
    return sent_size;
}

int recv_packet(SOCKET sock, void* buf, int packet_size, int flag) {
    char* buf_ptr = reinterpret_cast<char*>(buf);
    int   recv_size = 0;

    while (recv_size < packet_size) {
        int len = recv(sock, buf_ptr + recv_size, packet_size - recv_size, flag);
        if (len < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        else if (len == 0) {
            break;
        }
        recv_size += len;
    }
    return recv_size;
}