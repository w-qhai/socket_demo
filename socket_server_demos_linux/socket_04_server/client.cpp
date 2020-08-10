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

void do_recv(int conn, sockaddr_in& peer);

int send_packet(SOCKET sock, void* buf, int len, int flag);
int recv_packet(SOCKET sock, void* buf, int len, int flag);

int main(int argc, char** argv) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serve_addr;
    memset(&serve_addr, 0, sizeof(serve_addr));
    serve_addr.sin_family = AF_INET;
    serve_addr.sin_port = htons(5188);
    inet_pton(AF_INET, "127.0.0.1", &serve_addr.sin_addr.s_addr); /*server ip*/

    if (connect(sock, reinterpret_cast<sockaddr*>(&serve_addr), sizeof(serve_addr)) < 0) {
        std::cerr << "connect faild" << std::endl;
    }

    //std::thread recv_thread(do_recv, sock, std::ref(serve_addr));
    //recv_thread.detach();

    //Packet packet;
    //while (std::cin.getline(packet.buf, BUF_SIZE)) {
    //    packet.len = htonl(strlen(packet.buf) + 1);
    //    send_packet(sock, &packet, sizeof(packet.len) + ntohl(packet.len), 0);
    //}

    fd_set rest;
    FD_ZERO(&rest);

    int fd_stdin = fileno(stdin);
    int max_fd = std::max(fd_stdin, sock);

    while (true) {
        FD_SET(fd_stdin, &rest);
        FD_SET(sock, &rest);

        int nready = select(sock + 1, &rest, nullptr, nullptr, nullptr);
        if (nready == -1) {
            int e = errno;
            std::cout << e << std::endl;
            std::wcout << strerror(e) << std::endl;
            break;
        }

        if (nready == 0) {
            continue;
        }

        if (FD_ISSET(sock, &rest)) {
            Packet packet;
            char ip[INET_ADDRSTRLEN];
            sockaddr_in peer = serve_addr;

            recv_packet(sock, &packet.len, sizeof(packet.len), 0);
            recv_packet(sock, packet.buf, ntohl(packet.len), 0) >= ntohl(packet.len);
            std::cout << "[" << inet_ntop(AF_INET, &peer.sin_addr.s_addr, ip, sizeof(ip)) << ":" << ntohs(peer.sin_port) << "]:\t"
                << packet.buf << std::endl;
        }

        if (FD_ISSET(fd_stdin, &rest)) {
            Packet packet;
            std::cin.getline(packet.buf, BUF_SIZE);
            packet.len = htonl(strlen(packet.buf) + 1);
            send_packet(sock, &packet, sizeof(packet.len) + ntohl(packet.len), 0);
        }
    }

    close(sock);

    return 0;
}

void do_recv(int conn, sockaddr_in& peer) {
    Packet packet;
    char ip[INET_ADDRSTRLEN];

    while (recv_packet(conn, &packet.len, sizeof(packet.len), 0) >= 4 &&
        recv_packet(conn, packet.buf, ntohl(packet.len), 0) >= ntohl(packet.len)) {
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