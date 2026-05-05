# multi_node_udp.py
import socket
import struct
import threading
import queue
import tkinter as tk
from tkinter import scrolledtext, ttk
import time

# ----- Constants -----
SERVER_IP = "127.0.0.1"
SERVER_PORT = 12345
BUFFER_SIZE = 512

# Message types
TYPE_REGISTER = 0x00
TYPE_PING     = 0x01
TYPE_PONG     = 0x02
TYPE_NOTIFY   = 0x03

# Node types
NODE_BASE         = 0x00
NODE_DATA_PLANE   = 0x01
NODE_CONTROL_PLANE= 0x02
NODE_OAM          = 0x03

# Port assignment for local nodes (each node gets a unique port)
BASE_LOCAL_PORT = 20000


class Node:
    """Represents a single UDP node with its own socket, listener thread, and auto‑pong timer."""
    def __init__(self, node_id, node_type, local_port, gui_callback):
        self.node_id = node_id
        self.node_type = node_type
        self.local_port = local_port
        self.gui_callback = gui_callback   # function to append messages to the GUI

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('', self.local_port))
        self.sock.settimeout(0.5)
        self.running = True
        self.paused = False          # pause auto-pong
        self.active_timers = []      # list of threading.Timer objects
        self.timer_lock = threading.Lock()

        self.server_addr = (SERVER_IP, SERVER_PORT)
        self.recv_thread = threading.Thread(target=self.udp_listener, daemon=True)
        self.recv_thread.start()

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

    def parse_incoming_packet(self, data):
        if len(data) < 2:
            return None
        pkt_type = data[0]
        total_len = data[1]
        if len(data) < total_len or total_len < 2:
            return None
        payload = data[2:total_len]

        if pkt_type == TYPE_PING:
            message = payload.decode('utf-8', errors='replace')
            return f"Ping: {message}"
        elif pkt_type == TYPE_NOTIFY:
            if len(payload) < 4:
                return None
            type_node = payload[0]
            port = struct.unpack('<H', payload[1:3])[0]
            state = payload[3]
            msg_bytes = payload[4:]
            message = msg_bytes.decode('utf-8', errors='replace') if msg_bytes else ""
            return f"Notify: NodeType={type_node}, Port={port}, State={state}\nMessage: {message}"
        else:
            return f"Unknown packet (type={pkt_type:02X}): {payload.hex()}"

    def udp_listener(self):
        """Background thread: receive packets and handle PING (auto-pong)."""
        while self.running:
            try:
                data, addr = self.sock.recvfrom(BUFFER_SIZE)
                if data:
                    parsed = self.parse_incoming_packet(data)
                    if parsed:
                        self.gui_callback(f"[From {addr[0]}:{addr[1]}]\n{parsed}")
                        # If it's a PING, schedule a PONG after 300ms (unless paused)
                        if data[0] == TYPE_PING:
                            with self.timer_lock:
                                if not self.paused:
                                    timer = threading.Timer(0.3, self.send_pong)
                                    timer.daemon = True
                                    timer.start()
                                    self.active_timers.append(timer)
                    else:
                        self.gui_callback(f"[From {addr[0]}:{addr[1]}] Invalid packet")
            except socket.timeout:
                continue
            except OSError:
                break

    def send_register(self):
        """Send registration packet."""
        try:
            packet = self.build_register_packet(self.node_type, "REGISTER")
            self.sock.sendto(packet, self.server_addr)
            self.gui_callback("[ME] Register sent")
        except Exception as e:
            self.gui_callback(f"[ERROR] Register failed: {e}")

    def send_pong(self):
        """Send a PONG packet (called by timer or manually)."""
        # Check again if paused (timer may have been scheduled before pause)
        if self.paused:
            return
        try:
            packet = self.build_pong_packet("PONG")
            self.sock.sendto(packet, self.server_addr)
            self.gui_callback("[ME] Pong sent")
        except Exception as e:
            self.gui_callback(f"[ERROR] Pong send failed: {e}")
        finally:
            # Remove this timer from the list (if it's there)
            with self.timer_lock:
                for t in self.active_timers[:]:
                    if not t.is_alive():
                        self.active_timers.remove(t)

    def set_pause(self, paused):
        """Pause or resume auto‑reply to PING."""
        self.paused = paused
        if paused:
            # Cancel all pending timers
            with self.timer_lock:
                for t in self.active_timers:
                    t.cancel()
                self.active_timers.clear()

    def stop(self):
        """Stop the node and clean up resources."""
        self.running = False
        self.set_pause(True)   # cancel any pending timers
        self.sock.close()


class NodeFrame(tk.Frame):
    """GUI frame for a single node: register/pause buttons, message log."""
    def __init__(self, parent, node_id, node_type, local_port):
        super().__init__(parent, bd=2, relief=tk.RIDGE)
        self.node = None
        self.node_id = node_id
        self.node_type = node_type
        self.local_port = local_port

        # Node label
        type_names = {NODE_DATA_PLANE: "DATA", NODE_CONTROL_PLANE: "CTRL", NODE_OAM: "OAM"}
        label_text = f"Node {node_id} ({type_names.get(node_type, '?')}) Port:{local_port}"
        self.label = tk.Label(self, text=label_text, font=('Arial', 10, 'bold'))
        self.label.pack(pady=2)

        # Buttons
        btn_frame = tk.Frame(self)
        btn_frame.pack(pady=2)
        self.register_btn = tk.Button(btn_frame, text="Register", width=8, command=self.on_register)
        self.register_btn.pack(side=tk.LEFT, padx=2)
        self.pause_btn = tk.Button(btn_frame, text="Pause Auto", width=12, command=self.toggle_pause)
        self.pause_btn.pack(side=tk.LEFT, padx=2)
        self.paused_state = False

        # Message display
        self.msg_display = scrolledtext.ScrolledText(self, wrap=tk.WORD, width=50, height=12,
                                                     state='disabled', font=('Courier', 8))
        self.msg_display.pack(padx=5, pady=5, fill=tk.BOTH, expand=True)

        # Create the Node object (starts its own thread)
        self.node = Node(node_id, node_type, local_port, self.append_message)

    def append_message(self, text):
        """Thread‑safe: called from Node's background thread to add a message."""
        def _update():
            self.msg_display.configure(state='normal')
            self.msg_display.insert(tk.END, text + '\n' + '-'*40 + '\n')
            self.msg_display.see(tk.END)
            self.msg_display.configure(state='disabled')
        # Schedule on the main thread (tkinter)
        self.master.after(0, _update)

    def on_register(self):
        self.node.send_register()

    def toggle_pause(self):
        self.paused_state = not self.paused_state
        self.node.set_pause(self.paused_state)
        self.pause_btn.config(text="Resume Auto" if self.paused_state else "Pause Auto")

    def stop(self):
        if self.node:
            self.node.stop()


class MultiNodeApp:
    def __init__(self, master):
        self.master = master
        master.title("Multi‑Node UDP Client - 9 Nodes (3x3)")
        # Maximize window
        master.state('zoomed')
        # master.attributes('-fullscreen', True)   # optional

        # Define node composition: 3 CONTROL, 4 DATA, 2 OAM (total 9)
        node_types = ([NODE_CONTROL_PLANE] * 3 +
                      [NODE_DATA_PLANE] * 4 +
                      [NODE_OAM] * 2)
        # Assign unique local ports
        local_ports = [BASE_LOCAL_PORT + i for i in range(9)]

        main_frame = ttk.Frame(master)
        main_frame.pack(fill=tk.BOTH, expand=True)

        self.node_frames = []
        rows = 3
        cols = 3
        for idx, (node_type, port) in enumerate(zip(node_types, local_ports)):
            row = idx // cols
            col = idx % cols
            frame = NodeFrame(main_frame, idx, node_type, port)
            frame.grid(row=row, column=col, padx=5, pady=5, sticky="nsew")
            self.node_frames.append(frame)
            # Configure grid weights
            main_frame.grid_rowconfigure(row, weight=1)
            main_frame.grid_columnconfigure(col, weight=1)

        # Handle window close
        master.protocol("WM_DELETE_WINDOW", self.quit_app)

    def quit_app(self):
        for nf in self.node_frames:
            nf.stop()
        self.master.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = MultiNodeApp(root)
    root.mainloop()