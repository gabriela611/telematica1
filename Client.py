
import customtkinter as ctk
from datetime import datetime
import threading
import time
import socket

class ObserverInterface(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.server_socket = None
        self.host = "127.0.0.1"
        self.port = 5555
        self.is_connected = False
        self.receive_thread = None
        self.should_receive = False

        # telemetry data 
        self.telemetry_data = {
            "direction": "F",  
            "speed": "0.0",
            "battery": "0.0",
            "temperature": "0.0",
            "time": datetime.now().strftime("%H:%M:%S")
        }

        # configure window
        self.title("Vehicle Telemetry Observer")
        self.geometry("950x600")

        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("blue")

        self.create_layout()

        self.protocol("WM_DELETE_WINDOW", self.on_closing)

    def connect(self):
        if self.is_connected:
            return

        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.settimeout(5)
            self.server_socket.connect((self.host, self.port))
            self.server_socket.settimeout(None)

            # update estado
            self.is_connected = True
            self.status_label.configure(text="● Conectado", text_color="green")
            self.connect_btn.configure(state="disabled")
            self.disconnect_btn.configure(state="normal")

            # loop para recibir 
            self.start_receive_thread()

        except Exception as e:
            self.cleanup_socket()

    def disconnect(self):
        if not self.is_connected:
            return

        self.should_receive = False
        try:
            if self.server_socket:
                try:
                    self.server_socket.send("QUIT\n".encode())
                except:
                    pass
                time.sleep(0.1)
        except:
            pass

        self.cleanup_socket()

        self.is_connected = False
        self.status_label.configure(text="● Desconectado", text_color="red")
        self.connect_btn.configure(state="normal")
        self.disconnect_btn.configure(state="disabled")

    def cleanup_socket(self):
        if self.server_socket:
            try:
                self.server_socket.close()
            except:
                pass
            self.server_socket = None

    def start_receive_thread(self):
        self.should_receive = True
        self.receive_thread = threading.Thread(target=self.receive_loop, daemon=True)
        self.receive_thread.start()

    def receive_loop(self):
        buffer = ""
        while self.should_receive and self.is_connected:
            try:
                data = self.server_socket.recv(1024).decode()
                if not data:
                    self.after(0, self.disconnect)
                    break

                buffer += data

                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    line = line.strip()

                    if not line:
                        continue

                    if line.startswith("TLM"):
                        telemetry_str = line.replace("TLM ", "")
                        kvs = [x for x in telemetry_str.split(";") if '=' in x]
                        telemetry = dict([x.split("=", 1) for x in kvs])

                        self.telemetry_data["speed"] = telemetry.get("speed", "0.0")
                        self.telemetry_data["battery"] = telemetry.get("battery", "0.0")
                        ts = telemetry.get("ts")
                        if ts:
                            try:
                                t = datetime.fromtimestamp(int(ts))
                                self.telemetry_data["time"] = t.strftime("%H:%M:%S")
                            except:
                                self.telemetry_data["time"] = datetime.now().strftime("%H:%M:%S")
                        else:
                            self.telemetry_data["time"] = datetime.now().strftime("%H:%M:%S")
                        self.telemetry_data["temperature"] = telemetry.get("temp", "0.0")
                        self.telemetry_data["direction"] = telemetry.get("dir", "F")

                        self.after(0, self.update_telemetry_display)

            except Exception as e:
                if self.should_receive:
                    self.after(0, self.disconnect)
                break

    def on_closing(self):
        if self.is_connected:
            self.disconnect()
        time.sleep(0.2)
        self.destroy()

    def create_layout(self):
        #arriba
        top_bar = ctk.CTkFrame(self, height=60)
        top_bar.pack(fill="x")

        title = ctk.CTkLabel(top_bar, text="Observer Dashboard", font=ctk.CTkFont(size=22, weight="bold"))
        title.pack(side="left", padx=20, pady=10)

        self.status_label = ctk.CTkLabel(top_bar, text="● Desconectado", font=ctk.CTkFont(size=16), text_color="red")
        self.status_label.pack(side="right", padx=20, pady=10)
        #centro
        main_frame = ctk.CTkFrame(self)
        main_frame.pack(fill="both", expand=True, padx=20, pady=20)

        left_panel = ctk.CTkFrame(main_frame)
        left_panel.pack(side="left", fill="both", expand=True, padx=10)

        right_panel = ctk.CTkFrame(main_frame)
        right_panel.pack(side="right", fill="both", expand=True, padx=10)


        self.time_label = self.add_info_row(left_panel, "Time", "00:00:00")
        self.speed_label = self.add_info_row(left_panel, "Speed", "0.0 km/h")
        self.battery_label = self.add_info_row(left_panel, "Battery", "0%")
        self.temp_label = self.add_info_row(left_panel, "Temp", "0.0°C")

        self.battery_bar = ctk.CTkProgressBar(left_panel)
        self.battery_bar.pack(fill="x", padx=20, pady=10)
        self.battery_bar.set(1.0)

        # botones de direccion
        ctk.CTkLabel(right_panel, text="Direction", font=ctk.CTkFont(size=18, weight="bold")).pack(pady=10)
        dir_frame = ctk.CTkFrame(right_panel, fg_color="transparent")
        dir_frame.pack(expand=True, pady=20)

        self.left_btn = ctk.CTkButton(dir_frame, text="← Left", width=120, height=70,
                                      font=ctk.CTkFont(size=16, weight="bold"),
                                      fg_color="gray25", hover=False, state="disabled")
        self.forward_btn = ctk.CTkButton(dir_frame, text="↑ Forward", width=120, height=70,
                                         font=ctk.CTkFont(size=16, weight="bold"),
                                         fg_color="gray25", hover=False, state="disabled")
        self.right_btn = ctk.CTkButton(dir_frame, text="Right →", width=120, height=70,
                                       font=ctk.CTkFont(size=16, weight="bold"),
                                       fg_color="gray25", hover=False, state="disabled")

        self.left_btn.pack(side="left", padx=20, pady=10)
        self.forward_btn.pack(side="left", padx=20, pady=10)
        self.right_btn.pack(side="left", padx=20, pady=10)

        # botones de conexion
        connection_frame = ctk.CTkFrame(self)
        connection_frame.pack(fill="x", padx=15, pady=10)

        self.connect_btn = ctk.CTkButton(connection_frame, text="CONNECT", command=self.connect,
                                         height=45, font=ctk.CTkFont(size=15, weight="bold"),
                                         fg_color="green", hover_color="darkgreen")
        self.connect_btn.pack(side="left", expand=True, fill="x", padx=10, pady=5)

        self.disconnect_btn = ctk.CTkButton(connection_frame, text="DISCONNECT", command=self.disconnect,
                                            height=45, font=ctk.CTkFont(size=15, weight="bold"),
                                            fg_color="red", hover_color="darkred", state="disabled")
        self.disconnect_btn.pack(side="right", expand=True, fill="x", padx=10, pady=5)

    def add_info_row(self, parent, label, value):
        frame = ctk.CTkFrame(parent)
        frame.pack(fill="x", padx=10, pady=8)
        ctk.CTkLabel(frame, text=label + ":", font=ctk.CTkFont(size=15, weight="bold")).pack(side="left", padx=8, pady=8)
        lbl = ctk.CTkLabel(frame, text=value, font=ctk.CTkFont(size=15))
        lbl.pack(side="right", padx=8, pady=8)
        return lbl

    def update_telemetry_display(self):

        self.time_label.configure(text=self.telemetry_data.get("time", "00:00:00"))
        direction = self.telemetry_data.get("direction", "F").upper()

        # reset
        self.forward_btn.configure(fg_color="gray25")
        self.left_btn.configure(fg_color="gray25")
        self.right_btn.configure(fg_color="gray25")

        if direction in ["F", "FORWARD"]:
            self.forward_btn.configure(fg_color="green")
        elif direction in ["L", "LEFT"]:
            self.left_btn.configure(fg_color="blue")
        elif direction in ["R", "RIGHT"]:
            self.right_btn.configure(fg_color="orange")

        
        self.speed_label.configure(text=f"{self.telemetry_data.get('speed', '0.0')} km/h")

        try:
            battery = float(self.telemetry_data.get('battery', 0.0))
            self.battery_label.configure(text=f"{battery:.0f}%")
            self.battery_bar.set(max(0.0, min(1.0, battery / 100.0)))
        except:
            pass

        try:
            temp = float(self.telemetry_data.get('temperature', 0.0))
            self.temp_label.configure(text=f"{temp:.1f}°C")
        except:
            pass


if __name__ == "__main__":
    app = ObserverInterface()
    app.mainloop()
