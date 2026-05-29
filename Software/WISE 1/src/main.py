import serial
import customtkinter as ctk
import threading
from datetime import datetime
from tkinter import scrolledtext

# ==========================================
# CONFIG
# ==========================================
HOST = "localhost"
PORT = 4000

# Set tema global
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("green")

class ModernMonitor(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("SMART FARMING DASHBOARD v1.0")
        self.geometry("950x650")
        
        # Grid Layout Configuration
        self.grid_columnconfigure(1, weight=1)
        self.grid_rowconfigure(0, weight=1)

        self.running = False
        self.total = 0
        self.ser = None # Inisialisasi variabel serial

        # ==================================
        # SIDEBAR
        # ==================================
        self.sidebar = ctk.CTkFrame(self, width=220, corner_radius=0)
        self.sidebar.grid(row=0, column=0, sticky="nsew")
        self.sidebar.pack_propagate(False) 
        
        self.logo_label = ctk.CTkLabel(
            self.sidebar, 
            text=" AG\nTECH MONITOR", 
            font=ctk.CTkFont(size=22, weight="bold")
        )
        self.logo_label.pack(pady=(30, 40))

        # TOMBOL CONNECT
        self.conn_btn = ctk.CTkButton(
            self.sidebar, 
            text="CONNECT", 
            command=self.connect, 
            fg_color="#2ecc71", 
            hover_color="#27ae60",
            font=ctk.CTkFont(weight="bold")
        )
        self.conn_btn.pack(pady=10, padx=20)

        # TOMBOL DISCONNECT (BARU)
        self.disconn_btn = ctk.CTkButton(
            self.sidebar, 
            text="DISCONNECT", 
            command=self.disconnect, 
            fg_color="#e74c3c", 
            hover_color="#c0392b",
            font=ctk.CTkFont(weight="bold"),
            state="disabled" # Nonaktif saat aplikasi baru dibuka
        )
        self.disconn_btn.pack(pady=10, padx=20)

        self.clear_btn = ctk.CTkButton(
            self.sidebar, 
            text="CLEAR LOG", 
            command=self.clear, 
            fg_color="transparent", 
            border_width=2,
            hover_color="#333333"
        )
        self.clear_btn.pack(pady=10, padx=20)

        self.status_indicator = ctk.CTkLabel(
            self.sidebar, 
            text="● DISCONNECTED", 
            text_color="#e74c3c", 
            font=ctk.CTkFont(size=12, weight="bold")
        )
        self.status_indicator.pack(side="bottom", pady=30)

        # ==================================
        # MAIN CONTENT
        # ==================================
        self.main_frame = ctk.CTkFrame(self, fg_color="transparent")
        self.main_frame.grid(row=0, column=1, padx=20, pady=20, sticky="nsew")
        self.main_frame.grid_columnconfigure((0, 1, 2), weight=1)

        # Card 1: Pump Status
        self.card_pump = ctk.CTkFrame(self.main_frame, height=160)
        self.card_pump.grid(row=0, column=0, padx=10, pady=10, sticky="nsew")
        self.card_pump.grid_propagate(False) 
        
        ctk.CTkLabel(self.card_pump, text="STATUS POMPA", font=ctk.CTkFont(size=13, weight="bold"), text_color="#aaaaaa").pack(pady=(25, 0), expand=True)
        self.lbl_pump = ctk.CTkLabel(
            self.card_pump, 
            text="OFF", 
            text_color="#e74c3c", 
            font=ctk.CTkFont(size=30, weight="bold"),
            width=180
        )
        self.lbl_pump.pack(pady=(0, 25), expand=True)

        # Card 2: Soil Humidity
        self.card_soil = ctk.CTkFrame(self.main_frame, height=160)
        self.card_soil.grid(row=0, column=1, padx=10, pady=10, sticky="nsew")
        self.card_soil.grid_propagate(False)
        
        ctk.CTkLabel(self.card_soil, text="KELEMBAPAN", font=ctk.CTkFont(size=13, weight="bold"), text_color="#aaaaaa").pack(pady=(25, 0), expand=True)
        self.lbl_soil = ctk.CTkLabel(
            self.card_soil, 
            text="---", 
            text_color="#95a5a6", 
            font=ctk.CTkFont(size=30, weight="bold"),
            width=180
        )
        self.lbl_soil.pack(pady=(0, 25), expand=True)

        # Card 3: Data Counter
        self.card_data = ctk.CTkFrame(self.main_frame, height=160)
        self.card_data.grid(row=0, column=2, padx=10, pady=10, sticky="nsew")
        self.card_data.grid_propagate(False)
        
        ctk.CTkLabel(self.card_data, text="TOTAL DATA", font=ctk.CTkFont(size=13, weight="bold"), text_color="#aaaaaa").pack(pady=(25, 0), expand=True)
        self.lbl_counter = ctk.CTkLabel(
            self.card_data, 
            text="0", 
            text_color="#f1c40f", 
            font=ctk.CTkFont(size=34, weight="bold")
        )
        self.lbl_counter.pack(pady=(0, 25), expand=True)

        # --- Console / Log ---
        self.log_label = ctk.CTkLabel(self.main_frame, text="LIVE SERIAL TERMINAL", font=ctk.CTkFont(size=14, weight="bold"))
        self.log_label.grid(row=1, column=0, columnspan=3, pady=(25, 5), sticky="w", padx=10)

        self.text_log = scrolledtext.ScrolledText(
            self.main_frame, 
            bg="#0f0f0f", 
            fg="#2ecc71", 
            font=("Consolas", 10),
            borderwidth=0,
            highlightthickness=1,
            highlightbackground="#333333"
        )
        self.text_log.grid(row=2, column=0, columnspan=3, padx=10, pady=10, sticky="nsew")
        self.main_frame.rowconfigure(2, weight=1)

    # ======================================
    # LOGIC FUNCTIONS
    # ======================================

    def connect(self):
        if self.running: return
        try:
            url = f"rfc2217://{HOST}:{PORT}?ign_set_control"
            self.ser = serial.serial_for_url(url, timeout=1)
            self.running = True

            # Update UI state
            self.status_indicator.configure(text="● CONNECTED", text_color="#2ecc71")
            self.conn_btn.configure(state="disabled", text="SYSTEM ONLINE")
            self.disconn_btn.configure(state="normal") # Aktifkan tombol disconnect
            self.log("Serial Connection.")

            threading.Thread(target=self.read_data, daemon=True).start()
        except Exception as e:
            self.log(f"CONNECTION ERROR: {e}")

    def disconnect(self):
        """Fungsi untuk memutus koneksi serial secara aman"""
        if not self.running: return
        
        self.running = False
        try:
            if self.ser and self.ser.is_open:
                self.ser.close()
            
            # Update UI state
            self.status_indicator.configure(text="● DISCONNECTED", text_color="#e74c3c")
            self.conn_btn.configure(state="normal", text="CONNECT")
            self.disconn_btn.configure(state="disabled") # Matikan tombol disconnect
            self.log("Serial Disconnect.")
        except Exception as e:
            self.log(f"DISCONNECT ERROR: {e}")

    def read_data(self):
        while self.running:
            try:
                if self.ser and self.ser.is_open and self.ser.in_waiting:
                    data = self.ser.readline().decode().strip()
                    if data:
                        self.after(0, self.update_gui, data)
            except Exception:
                self.running = False
                self.after(0, self.update_status_lost)

    def update_status_lost(self):
        self.status_indicator.configure(text="● DISCONNECTED", text_color="#e74c3c")
        self.conn_btn.configure(state="normal", text="CONNECT")
        self.disconn_btn.configure(state="disabled")
        self.log("Error: Connection Lost.")

    def update_gui(self, data):
        self.log(data)
        
        # Update Counter
        self.total += 1
        self.lbl_counter.configure(text=str(self.total))

        data_upper = data.upper()

        # Update Pump UI
        if "POMPA: ON" in data_upper or "POMPA ON" in data_upper:
            self.lbl_pump.configure(text=" ACTIVE ", text_color="#2ecc71")
        elif "POMPA: OFF" in data_upper or "POMPA OFF" in data_upper:
            self.lbl_pump.configure(text=" OFF ", text_color="#e74c3c")

        # Update Soil UI
        if "RENDAH" in data_upper:
            self.lbl_soil.configure(text=" RENDAH ", text_color="#e67e22")
        elif "TINGGI" in data_upper:
            self.lbl_soil.configure(text=" TINGGI ", text_color="#3498db")
        elif "MANUAL" in data_upper:
            self.lbl_soil.configure(text=" MANUAL ", text_color="#9b59b6")

    def log(self, text):
        waktu = datetime.now().strftime("%H:%M:%S")
        self.text_log.insert("end", f"[{waktu}] {text}\n")
        self.text_log.see("end")

    def clear(self):
        self.text_log.delete(1.0, "end")
        self.total = 0
        self.lbl_counter.configure(text="0")
        self.lbl_pump.configure(text="OFF", text_color="#e74c3c")
        self.lbl_soil.configure(text="---", text_color="#95a5a6")
        self.log("System dashboard reset.")

if __name__ == "__main__":
    app = ModernMonitor()
    app.mainloop()