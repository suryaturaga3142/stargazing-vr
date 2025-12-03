import sys
import time
import serial
import serial.tools.list_ports
import numpy as np
from PySide6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                               QHBoxLayout, QComboBox, QPushButton, QLabel)
from PySide6.QtCore import QTimer, Qt
from PySide6.QtGui import QColor, QPalette
import pyqtgraph as pg

# --- Configuration ---
SCALE_XY = 32000.0
SCALE_MAG = 10.0
BAUD_RATE = 115200

class VRSimulator(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Star Tracker VR Simulator (Explicit Sync)")
        self.resize(800, 800)

        # Main Layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)

        # 1. Control Bar
        control_layout = QHBoxLayout()
        self.port_combo = QComboBox()
        self.refresh_ports()
        control_layout.addWidget(QLabel("Port:"))
        control_layout.addWidget(self.port_combo)
        
        self.connect_btn = QPushButton("Connect")
        self.connect_btn.clicked.connect(self.toggle_connection)
        control_layout.addWidget(self.connect_btn)
        
        self.refresh_btn = QPushButton("Refresh")
        self.refresh_btn.clicked.connect(self.refresh_ports)
        control_layout.addWidget(self.refresh_btn)
        
        layout.addLayout(control_layout)

        # 2. Visualization Area
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setAspectLocked(True)
        self.plot_widget.setXRange(-1.1, 1.1) 
        self.plot_widget.setYRange(-1.1, 1.1)
        self.plot_widget.hideAxis('bottom')
        self.plot_widget.hideAxis('left')
        self.plot_widget.setBackground('k')
        
        # Screen Boundary
        screen_box = pg.PlotCurveItem(
            x=[-1, 1, 1, -1, -1], 
            y=[-1, -1, 1, 1, -1], 
            pen=pg.mkPen('g', width=2, style=Qt.DashLine)
        )
        self.plot_widget.addItem(screen_box)

        # Scatter Item
        self.scatter = pg.ScatterPlotItem(pxMode=True)
        self.plot_widget.addItem(self.scatter)
        
        self.star_brush = pg.mkBrush(255, 255, 255, 220)
        self.star_pen = None 

        layout.addWidget(self.plot_widget)

        # 3. Serial Handling
        self.serial_port = None
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial_chunk)
        
        # Double Buffering State
        self.back_buffer = []

    def refresh_ports(self):
        self.port_combo.clear()
        ports = serial.tools.list_ports.comports()
        for p in ports:
            self.port_combo.addItem(p.device)

    def toggle_connection(self):
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            self.timer.stop()
            self.connect_btn.setText("Connect")
            print("Disconnected.")
        else:
            port = self.port_combo.currentText()
            try:
                # Fast timeout, high speed
                self.serial_port = serial.Serial(port, BAUD_RATE, timeout=0.005)
                # Poll very fast (5ms) to clear UART FIFO quickly
                self.timer.start(5) 
                self.connect_btn.setText("Disconnect")
                print(f"Connected to {port} @ {BAUD_RATE}")
                self.back_buffer = []
            except Exception as e:
                print(f"Error: {e}")

    def read_serial_chunk(self):
        if not self.serial_port or not self.serial_port.is_open:
            return

        try:
            if self.serial_port.in_waiting > 0:
                raw_data = self.serial_port.read(self.serial_port.in_waiting)
                text_data = raw_data.decode('latin-1', errors='ignore')
                lines = text_data.split('\n')
                
                for line in lines:
                    line = line.strip()
                    if not line: continue

                    # --- FRAME SYNC LOGIC ---
                    if line == "xx":
                        # "xx" marks the START of a new frame / END of previous.
                        # 1. Commit the accumulated buffer to the screen
                        self.render_frame(self.back_buffer)
                        # 2. Clear buffer to start collecting the new frame
                        self.back_buffer = []
                    
                    # --- DATA PARSING ---
                    elif line.startswith('$') and len(line) >= 11:
                        star = self.parse_star_line(line)
                        if star:
                            self.back_buffer.append(star)

        except Exception as e:
            print(f"Serial Logic Error: {e}")

    def parse_star_line(self, line):
        try:
            hex_x = line[1:5]
            hex_y = line[5:9]
            hex_m = line[9:11]

            x_int = int(hex_x, 16)
            if x_int > 32767: x_int -= 65536
            
            y_int = int(hex_y, 16)
            if y_int > 32767: y_int -= 65536
            
            mag_int = int(hex_m, 16)

            x = x_int / SCALE_XY * 5
            y = y_int / SCALE_XY * 5
            
            size = max(2, (60 - mag_int) * 0.4)
            
            return {'x': x, 'y': y, 'size': size}
        except ValueError:
            return None

    def render_frame(self, stars):
        # If frame is empty, clear screen
        if not stars:
            self.scatter.setData(x=[], y=[], size=[])
            return

        # Unzip list of dicts to flat lists
        x = [s['x'] for s in stars]
        y = [s['y'] for s in stars]
        sizes = [s['size'] for s in stars]
        
        # Atomic Update: Replaces all points instantly
        self.scatter.setData(
            x=x, 
            y=y, 
            size=sizes, 
            brush=self.star_brush,
            pen=self.star_pen
        )

if __name__ == '__main__':
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    palette = app.palette()
    palette.setColor(QPalette.ColorRole.Window, QColor(10, 10, 10))
    palette.setColor(QPalette.ColorRole.WindowText, Qt.white)
    app.setPalette(palette)

    window = VRSimulator()
    window.show()
    sys.exit(app.exec())