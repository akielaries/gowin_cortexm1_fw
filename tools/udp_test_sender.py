#!/usr/bin/env python3
"""
udp_test_sender.py -- send a UDP message to the FPGA and print the reply

usage:
    python3 tools/udp_test_sender.py [options] MESSAGE

options:
    --host IP       target IP address (default: 192.168.86.200)
    --port PORT     target UDP port   (default: 9999)
    --timeout SEC   how long to wait for a reply (default: 2.0)

examples:
    python3 tools/udp_test_sender.py "hello fpga"
    python3 tools/udp_test_sender.py --host 192.168.86.200 --port 9999 "ping"
"""

import argparse
import socket
import sys


def main():
    parser = argparse.ArgumentParser(description="send a UDP message to the FPGA")
    parser.add_argument("message", help="text to send")
    parser.add_argument("--host", default="192.168.86.200", help="target IP")
    parser.add_argument("--port", type=int, default=9999, help="target UDP port")
    parser.add_argument("--timeout", type=float, default=5.0,
                        help="reply wait timeout in seconds")
    args = parser.parse_args()

    payload = args.message.encode()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(args.timeout)

    print(f"tx -> {args.host}:{args.port}  {len(payload)} bytes  {payload!r}")

    sock.sendto(payload, (args.host, args.port))

    try:
        data, addr = sock.recvfrom(4096)
        print(f"rx <- {addr[0]}:{addr[1]}  {len(data)} bytes  {data!r}")
    except socket.timeout:
        print(f"no reply within {args.timeout}s")
        sys.exit(1)
    finally:
        sock.close()


if __name__ == "__main__":
    main()
