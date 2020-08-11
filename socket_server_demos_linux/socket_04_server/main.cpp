#include <iostream>
#include <cstring>
#include <thread>
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

void do_service(int conn, sockaddr_in& peer);
void do_recv(int conn, sockaddr_in& peer);

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

    // sockaddr_in peer_addr;
    // socklen_t peer_len = sizeof(peer_addr);

    // do_service(conn, peer_addr);

    int client[FD_SETSIZE];
    sockaddr_in peer_addr[FD_SETSIZE];
    socklen_t peer_len[FD_SETSIZE];
    for (int i = 0; i < FD_SETSIZE; i++) {
        client[i] = -1;
    }
    int max_fd = sock;
    fd_set rset;
    fd_set all_set;
    FD_ZERO(&rset);
    FD_ZERO(&all_set);

    FD_SET(sock, &all_set);

    while (true) {
        rset = all_set;
        int nready = select(max_fd + 1, &rset, nullptr, nullptr, nullptr);
        
        if (nready == -1) {
            int err = errno;
            if (err == EINTR) {
                continue;
            }
            std::cout << strerror(err) << std::endl;
            break;
        }

        if (nready == 0) {
            continue;
        }

        if (FD_ISSET(sock, &rset)) {
            char ip[INET_ADDRSTRLEN];
            sockaddr_in peer;
            socklen_t len = sizeof(peer);
            int conn = accept(sock, reinterpret_cast<sockaddr*>(&peer), &len);
            peer_addr[conn] = peer;
            peer_len[conn] = len;
            std::cout << "[" << inet_ntop(AF_INET, &peer_addr[conn].sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer_addr[conn].sin_port) << "]: "
                << "connected" << std::endl;
            for (int  i = 0; i < FD_SETSIZE; i++) {
                if (client[i] == -1) {
                    client[i] = conn;
                    break;
                }
            }
            FD_SET(conn, &all_set);
            if (conn > max_fd) {
                max_fd = conn;
            }
            if (--nready <= 0) {
                continue;
            }
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            int conn = client[i];
            if (conn == -1) {
                continue;
            }
            if (FD_ISSET(conn, &rset)) {
                Packet packet;
                char ip[INET_ADDRSTRLEN];

                int head_len = recv_packet(conn, &packet.len, sizeof(packet.len), 0);
                if (head_len == 0) {
                    std::cout << "[" << inet_ntop(AF_INET, &peer_addr[conn].sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer_addr[conn].sin_port) << "]: "
                     << "closed connection." << std::endl;
                    FD_CLR(conn, &all_set);
                    continue;
                }
                int body_len = recv_packet(conn, packet.buf, ntohl(packet.len), 0);
                if (body_len == 0) {
                    std::cout << "[" << inet_ntop(AF_INET, &peer_addr[conn].sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer_addr[conn].sin_port) << "]: "
                     << "closed connection." << std::endl;
                    FD_CLR(conn, &all_set);
                    client[i] = -1;
                    close(conn);
                    continue;
                }
           
                std::cout << "[" << inet_ntop(AF_INET, &peer_addr[conn].sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer_addr[conn].sin_port) << "]: "
                    <<  packet.buf << std::endl;
                send_packet(conn, &packet, sizeof(packet.len) + ntohl(packet.len), 0);
                if (--nready <= 0) {
                    break;
                }
            }
        }
    }
    close(sock);
    return 0;
}

void do_service(int conn, sockaddr_in& peer) {
    
}
void do_recv(int conn, sockaddr_in& peer) {
    Packet packet;
    char ip[INET_ADDRSTRLEN];

    while (recv_packet(conn, &packet.len, sizeof(packet.len), 0) >= 4 &&
           recv_packet(conn, packet.buf, ntohl(packet.len), 0)   >= ntohl(packet.len)) {
        std::cout << "[" << inet_ntop(AF_INET, &peer.sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer.sin_port) << "]:\t"
            << packet.buf << std::endl;
    }
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