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

    sockaddr_in peer_addr;
    socklen_t peer_len = sizeof(peer_addr);

    char ip[INET_ADDRSTRLEN];
    int conn = accept(sock, reinterpret_cast<sockaddr*>(&peer_addr), &peer_len);
    std::cout << "[" << inet_ntop(AF_INET, &peer_addr.sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer_addr.sin_port) << "]: "
            << "connected" << std::endl;

    do_service(conn, peer_addr);
    close(conn);
    close(sock);
    return 0;
}

void do_service(int conn, sockaddr_in& peer) {
    std::thread recv_thread(do_recv, conn, std::ref(peer));
    recv_thread.detach();

    Packet packet;
    while (std::cin.getline(packet.buf, BUF_SIZE)) {
        packet.len = htonl(strlen(packet.buf) + 1);
        send_packet(conn, &packet, sizeof(packet.len) + ntohl(packet.len), 0);
    }
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