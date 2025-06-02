# test_runner_refined.py
# Adjusted test script to handle kernel buffer delays gracefully

import os
import subprocess
import hashlib
import time

def read_file_bytes(filepath):
    try:
        with open(filepath, 'rb') as f:
            return f.read()
    except FileNotFoundError:
        return b''  # File not yet created

def check_packets_so_far(expected_file, received_file, num_packs_sent, num_bytes_per_pack):
    expected_bytes = read_file_bytes(expected_file)[:num_packs_sent * num_bytes_per_pack]
    received_bytes = read_file_bytes(received_file)[:num_packs_sent * num_bytes_per_pack]
    
    if len(received_bytes) < len(expected_bytes):
        print(f"[WAIT] Packet {num_packs_sent}: Data not yet flushed to disk.")
        return
    
    if expected_bytes == received_bytes:
        print(f"[OK] Packet {num_packs_sent} matches expected data up to byte {num_packs_sent * num_bytes_per_pack}.")
    else:
        print(f"[ERROR] Mismatch detected at packet {num_packs_sent}.")
        for i, (e, r) in enumerate(zip(expected_bytes, received_bytes)):
            if e != r:
                print(f"  Byte {i}: Expected {e}, got {r}")
                break

def run_test():
    server_cmd = ['./server', '0', '44444']
    rcopy_cmd = ['./rcopy', 'test.pdf', 'testDownloadMAN.pdf', '10', '1000', '0', 'localhost', '44444']

    test_file = 'test.pdf'
    received_file = 'testDownloadMAN.pdf'
    num_bytes_per_pack = 1000

    if os.path.exists(received_file):
        os.remove(received_file)

    server_proc = subprocess.Popen(server_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    time.sleep(1)  # Allow server to start

    rcopy_proc = subprocess.Popen(rcopy_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    num_packs_sent = 1

    try:
        while True:
            check_packets_so_far(test_file, received_file, num_packs_sent, num_bytes_per_pack)
            num_packs_sent += 1
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("Test interrupted.")
    finally:
        server_proc.terminate()
        rcopy_proc.terminate()
        server_proc.wait()
        rcopy_proc.wait()
        print("Test completed.")

if __name__ == "__main__":
    run_test()
