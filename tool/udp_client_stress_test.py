#!/usr/bin/env python3
# stress_tool.py - 50-node UDP stress test for NoM app
# Usage: python stress_tool.py
# Now uses a GUI dialog for base port input and a main window with Stop button.

import socket
import struct
import threading
import time
import sys
import csv
from datetime import datetime
import tkinter as tk
from tkinter import ttk, simpledialog, messagebox

# ----- Constants -----
SERVER_IP = "127.0.0.1"
SERVER_PORT = 12345
BUFFER_SIZE = 512
NUM_NODES = 50
PONG_DELAY_MS = 50           # 50 milliseconds
PONG_DELAY_SEC = PONG_DELAY_MS / 1000.0
SOCKET_TIMEOUT = 1.0          # seconds

TYPE_REGISTER = 0x00
TYPE_PING     = 0x01
TYPE_PONG     = 0x02
TYPE_NOTIFY   = 0x03

NODE_CONTROL_PLANE = 0x02   # using control plane type for all nodes

# Global list to keep node threads
nodes = []
stop_event = threading.Event()

# CSV log file setup
log_file = open("latency_log.csv", "w", newline='')
csv_writer = csv.writer(log_file)
csv_writer.writerow(["timestamp", "node_id", "latency_ms"])

def log_latency(node_id, latency_ms):
    """Write latency to CSV file (thread-safe)"""
    with threading.Lock():
        csv_writer.writerow([datetime.now().isoformat(), node_id, f"{latency_ms:.3f}"])

class StressNode:
    def __init__(self, node_id, local_port):
        self.node_id = node_id
        self.local_port = local_port
        self.sock = None
        self.running = True
        self.server_addr = (SERVER_IP, SERVER_PORT)

    def start(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('', self.local_port))
        self.sock.settimeout(SOCKET_TIMEOUT)
        self.send_register()
        self.recv_thread = threading.Thread(target=self.udp_listener, daemon=True)
        self.recv_thread.start()
        print(f"[Node {self.node_id}] Started on port {self.local_port}")

    def send_register(self):
        packet = self.build_register_packet(NODE_CONTROL_PLANE, "REGISTER")
        self.sock.sendto(packet, self.server_addr)

    def build_register_packet(self, node_type, message="REGISTER"):
        msg_bytes = message.encode('utf-8')
        payload = struct.pack('!B', node_type) + msg_bytes
        payload_len = 2 + len(payload)
        if payload_len > 255:
            raise ValueError("Packet too long")
        header = struct.pack('!B B', TYPE_REGISTER, payload_len)
        return header + payload

    def build_pong_packet(self, message="PONG"):
        msg_bytes = message.encode('utf-8')
        payload_len = 2 + len(msg_bytes)
        if payload_len > 255:
            raise ValueError("Packet too long")
        header = struct.pack('!B B', TYPE_PONG, payload_len)
        return header + msg_bytes

    def send_pong(self):
        if not self.running or stop_event.is_set():
            return
        try:
            packet = self.build_pong_packet("PONG")
            self.sock.sendto(packet, self.server_addr)
        except Exception as e:
            print(f"[Node {self.node_id}] Error sending PONG: {e}")

    def udp_listener(self):
        while self.running and not stop_event.is_set():
            try:
                data, addr = self.sock.recvfrom(BUFFER_SIZE)
                if data and data[0] == TYPE_PING:
                    recv_time = time.time()
                    def delayed_pong():
                        if self.running and not stop_event.is_set():
                            send_time = time.time()
                            latency_ms = (send_time - recv_time) * 1000.0
                            log_latency(self.node_id, latency_ms)
                            self.send_pong()
                    timer = threading.Timer(PONG_DELAY_SEC, delayed_pong)
                    timer.daemon = True
                    timer.start()
            except socket.timeout:
                continue
            except OSError:
                break

    def stop(self):
        self.running = False
        if self.sock:
            self.sock.close()

def get_base_port_via_dialog():
    """Show a Tkinter dialog to get the base port."""
    root = tk.Tk()
    root.withdraw()
    root.attributes('-topmost', True)
    port = simpledialog.askinteger(
        "Input Base Port",
        "Enter base port (e.g., 20000):",
        initialvalue=20000,
        parent=root
    )
    root.destroy()
    if port is None:
        port = 20000
    return port

class StressTestGUI:
    def __init__(self, master, base_port):
        self.master = master
        self.base_port = base_port
        master.title("NoM Stress Test Controller")
        master.geometry("400x200")
        master.resizable(False, False)

        # Info frame
        info_frame = ttk.Frame(master, padding=10)
        info_frame.pack(fill=tk.BOTH, expand=True)
        ttk.Label(info_frame, text=f"Stress Test Running", font=('Arial', 12, 'bold')).pack(pady=5)
        ttk.Label(info_frame, text=f"Number of nodes: {NUM_NODES}").pack()
        ttk.Label(info_frame, text=f"Base port: {base_port}").pack()
        ttk.Label(info_frame, text=f"PONG delay: {PONG_DELAY_MS} ms").pack()

        # Stop button
        self.stop_btn = ttk.Button(master, text="Stop Test", command=self.stop_test)
        self.stop_btn.pack(pady=20)

        # Status label
        self.status_label = ttk.Label(master, text="All nodes started. Press Stop to exit.")
        self.status_label.pack()

        # Start nodes
        print(f"Using base port: {base_port}")
        print(f"Creating {NUM_NODES} nodes...")
        for i in range(NUM_NODES):
            node = StressNode(node_id=i, local_port=base_port + i)
            node.start()
            nodes.append(node)
        print("All nodes started. Use GUI to stop.")
        self.master.protocol("WM_DELETE_WINDOW", self.stop_test)

    def stop_test(self):
        """Stop all nodes, close log, and exit."""
        self.status_label.config(text="Stopping...")
        self.stop_btn.config(state=tk.DISABLED)
        print("\nStopping all nodes...")
        stop_event.set()
        for node in nodes:
            node.stop()
        log_file.close()
        print("Latency log saved to latency_log.csv")
        self.master.quit()
        self.master.destroy()
        sys.exit(0)

def main():
    # First get base port via dialog
    base_port = get_base_port_via_dialog()
    # Then create main window
    root = tk.Tk()
    app = StressTestGUI(root, base_port)
    root.mainloop()

if __name__ == "__main__":
    main()