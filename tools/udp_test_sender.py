#!/usr/bin/env python3
"""
usage:
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
    parser.add_argument(
        "--timeout", type=float, default=5.0, help="reply wait timeout in seconds"
    )
    args = parser.parse_args()

    msg = args.message
    if msg.startswith("0x") or msg.startswith("0X"):
        payload = bytes.fromhex(msg[2:])
    else:
        payload = msg.encode()

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
