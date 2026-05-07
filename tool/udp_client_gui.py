# multi_node_udp.py
import socket
import struct
import threading
import tkinter as tk
from tkinter import scrolledtext, ttk
import tkinter.simpledialog
import gc
from datetime import datetime   # <-- NEW import for timestamp

# ----- Constants -----
SERVER_IP = "127.0.0.1"
SERVER_PORT = 12345
BUFFER_SIZE = 512
MAX_LOG_LINES = 500
LINES_TO_DELETE = 100

TYPE_REGISTER = 0x00
TYPE_PING     = 0x01
TYPE_PONG     = 0x02
TYPE_NOTIFY   = 0x03

NODE_BASE         = 0x00
NODE_DATA_PLANE   = 0x01
NODE_CONTROL_PLANE= 0x02
NODE_OAM          = 0x03


class Node:
    # ... (giữ nguyên như cũ) ...
    def __init__(self, node_id, node_type, local_port, gui_callback):
        self.node_id = node_id
        self.node_type = node_type
        self.local_port = local_port
        self.gui_callback = gui_callback

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('', self.local_port))
        self.sock.settimeout(0.5)
        self.running = True
        self.paused = False
        self.active_timers = []
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
        while self.running:
            try:
                data, addr = self.sock.recvfrom(BUFFER_SIZE)
                if data:
                    parsed = self.parse_incoming_packet(data)
                    if parsed:
                        self.gui_callback(f"[From {addr[0]}:{addr[1]}]\n{parsed}")
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
        try:
            packet = self.build_register_packet(self.node_type, "REGISTER")
            self.sock.sendto(packet, self.server_addr)
            self.gui_callback("[ME] Register sent")
        except Exception as e:
            self.gui_callback(f"[ERROR] Register failed: {e}")

    def send_pong(self):
        if self.paused:
            return
        try:
            packet = self.build_pong_packet("PONG")
            self.sock.sendto(packet, self.server_addr)
            self.gui_callback("[ME] Pong sent")
        except Exception as e:
            self.gui_callback(f"[ERROR] Pong send failed: {e}")
        finally:
            with self.timer_lock:
                self.active_timers = [t for t in self.active_timers if t.is_alive()]

    def set_pause(self, paused):
        self.paused = paused
        if paused:
            with self.timer_lock:
                for t in self.active_timers:
                    t.cancel()
                self.active_timers.clear()

    def stop(self):
        self.running = False
        self.set_pause(True)
        self.sock.close()


class NodeFrame(tk.Frame):
    def __init__(self, parent, node_id, node_type, local_port):
        super().__init__(parent, bd=2, relief=tk.RIDGE)
        self.node = None
        self.node_id = node_id
        self.node_type = node_type
        self.local_port = local_port

        type_names = {NODE_DATA_PLANE: "DATA", NODE_CONTROL_PLANE: "CTRL", NODE_OAM: "OAM"}
        label_text = f"Node {node_id} ({type_names.get(node_type, '?')}) Port:{local_port}"
        self.label = tk.Label(self, text=label_text, font=('Arial', 10, 'bold'))
        self.label.pack(pady=2)

        btn_frame = tk.Frame(self)
        btn_frame.pack(pady=2)
        self.register_btn = tk.Button(btn_frame, text="Register", width=8, command=self.on_register)
        self.register_btn.pack(side=tk.LEFT, padx=2)
        self.pause_btn = tk.Button(btn_frame, text="Pause Auto", width=12, command=self.toggle_pause)
        self.pause_btn.pack(side=tk.LEFT, padx=2)
        self.clear_btn = tk.Button(btn_frame, text="Clear Log", width=8, command=self.clear_log)
        self.clear_btn.pack(side=tk.LEFT, padx=2)
        self.paused_state = False

        # --- SỬA: luôn ở chế độ normal, nhưng chặn chỉnh sửa bằng bind ---
        self.msg_display = scrolledtext.ScrolledText(self, wrap=tk.WORD, width=50, height=12,
                                                     state='normal', font=('Courier', 8))
        # Chặn mọi thao tác sửa đổi
        self.msg_display.bind('<Key>', lambda e: 'break')
        self.msg_display.bind('<BackSpace>', lambda e: 'break')
        self.msg_display.bind('<Delete>', lambda e: 'break')
        self.msg_display.bind('<Control-v>', lambda e: 'break')  # chặn dán
        # Vẫn cho phép copy (Ctrl+C, Ctrl+A) và chọn text
        self.msg_display.pack(padx=5, pady=5, fill=tk.BOTH, expand=True)

        self.node = Node(node_id, node_type, local_port, self.append_message)

    def append_message(self, text):
        """Append a line of text with a timestamp prefix (MM:SS.mmm)."""
        def _update():
            # Generate timestamp: minutes:seconds.milliseconds (3 digits)
            now = datetime.now()
            timestamp = now.strftime("%M:%S.") + f"{now.microsecond // 1000:03d}"
            # Insert timestamp + original message
            self.msg_display.insert(tk.END, f"[{timestamp}] {text}\n" + '-'*40 + '\n')
            total_lines = int(self.msg_display.index('end-1c').split('.')[0])
            if total_lines > MAX_LOG_LINES:
                self.msg_display.delete(1.0, f"{LINES_TO_DELETE + 1}.0")
                gc.collect()
            self.msg_display.see(tk.END)
        self.master.after(0, _update)

    def on_register(self):
        self.node.send_register()

    def toggle_pause(self):
        self.paused_state = not self.paused_state
        self.node.set_pause(self.paused_state)
        self.pause_btn.config(text="Resume Auto" if self.paused_state else "Pause Auto")

    def clear_log(self):
        self.msg_display.delete(1.0, tk.END)
        self.clear_btn.config(text="Cleared!", relief=tk.SUNKEN)
        self.master.after(1000, lambda: self.clear_btn.config(text="Clear Log", relief=tk.RAISED))
        gc.collect()

    def stop(self):
        if self.node:
            self.node.stop()


class MultiNodeApp:
    def __init__(self, master, base_port):
        self.master = master
        master.title(f"Multi‑Node UDP Client - 9 Nodes - Base Port {base_port}")
        master.state('zoomed')

        node_types = ([NODE_CONTROL_PLANE] * 3 +
                      [NODE_DATA_PLANE] * 4 +
                      [NODE_OAM] * 2)
        local_ports = [base_port + i for i in range(9)]

        main_frame = ttk.Frame(master)
        main_frame.pack(fill=tk.BOTH, expand=True)

        self.node_frames = []
        rows, cols = 3, 3
        for idx, (node_type, port) in enumerate(zip(node_types, local_ports)):
            row = idx // cols
            col = idx % cols
            frame = NodeFrame(main_frame, idx, node_type, port)
            frame.grid(row=row, column=col, padx=5, pady=5, sticky="nsew")
            self.node_frames.append(frame)
            main_frame.grid_rowconfigure(row, weight=1)
            main_frame.grid_columnconfigure(col, weight=1)

        master.protocol("WM_DELETE_WINDOW", self.quit_app)

    def quit_app(self):
        for nf in self.node_frames:
            nf.stop()
        self.master.destroy()


if __name__ == "__main__":
    temp_root = tk.Tk()
    temp_root.withdraw()
    base_port = tkinter.simpledialog.askinteger(
        "Input",
        "Enter base port (e.g., 20000):",
        initialvalue=20000,
        parent=temp_root
    )
    temp_root.destroy()
    if base_port is None:
        base_port = 20000

    root = tk.Tk()
    app = MultiNodeApp(root, base_port)

    def periodic_gc():
        gc.collect()
        root.after(30000, periodic_gc)
    root.after(30000, periodic_gc)

    root.mainloop()