import sys
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
DEFAULT_PORT = "COM5" # Hardcoded default port

class VRSimulator(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Star Tracker VR Simulator (Frame Sync)")
        self.resize(800, 800)

        # Main Layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)

        # 1. Control Bar
        control_layout = QHBoxLayout()
        self.port_combo = QComboBox()
        self.refresh_ports()
        
        # Hardcode selection if available
        index = self.port_combo.findText(DEFAULT_PORT)
        if index >= 0:
            self.port_combo.setCurrentIndex(index)
        
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
        
        layout.addWidget(self.plot_widget)

        # 3. Serial Handling
        self.serial_port = None
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial_and_plot)
        
        # Buffers for Frame Synchronization
        self.serial_buffer = ""   # Raw text accumulator
        self.frame_buffer = []    # List of stars for the current frame building

        # Auto-connect if the default port exists
        if index >= 0:
            self.toggle_connection()

    def refresh_ports(self):
        self.port_combo.clear()
        ports = serial.tools.list_ports.comports()
        
        # Add hardcoded port manually if not found automatically (sometimes needed for virtual ports)
        found_default = False
        for p in ports:
            self.port_combo.addItem(p.device)
            if p.device == DEFAULT_PORT:
                found_default = True
        
        if not found_default:
             self.port_combo.addItem(DEFAULT_PORT)

    def toggle_connection(self):
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            self.timer.stop()
            self.connect_btn.setText("Connect")
            print("Disconnected.")
        else:
            port = self.port_combo.currentText()
            try:
                self.serial_port = serial.Serial(port, BAUD_RATE, timeout=0.01)
                self.timer.start(10) # Fast polling (10ms)
                self.connect_btn.setText("Disconnect")
                self.serial_buffer = "" # Reset buffers
                self.frame_buffer = []
                print(f"Connected to {port}")
            except Exception as e:
                print(f"Error: {e}")

    def parse_star_line(self, line):
        try:
            # Format: $XXXXYYYMMM
            hex_x = line[1:5]
            hex_y = line[5:9]
            hex_m = line[9:11]

            x_int = int(hex_x, 16)
            if x_int > 32767: x_int -= 65536
            
            y_int = int(hex_y, 16)
            if y_int > 32767: y_int -= 65536
            
            mag_int = int(hex_m, 16)

            # Standard Normalize (-1.0 to 1.0)
            x = x_int / SCALE_XY * 5
            y = y_int / SCALE_XY * 5
            mag = mag_int / SCALE_MAG

            # Prepare visual properties
            size = max(2, (6.0 - mag) * 4)
            brush = pg.mkBrush(255, 255, 255, 200)
            
            return {'pos': [x, y], 'size': size, 'brush': brush}
            
        except ValueError:
            return None

    def read_serial_and_plot(self):
        if not self.serial_port or not self.serial_port.is_open:
            return

        try:
            if self.serial_port.in_waiting > 0:
                # Read chunk
                raw_data = self.serial_port.read(self.serial_port.in_waiting)
                # Accumulate text (handle split lines)
                self.serial_buffer += raw_data.decode('utf-8', errors='ignore')

                # Process line by line
                while '\n' in self.serial_buffer:
                    line, self.serial_buffer = self.serial_buffer.split('\n', 1)
                    line = line.strip()

                    # --- FRAME SYNCHRONIZATION LOGIC ---
                    if line == "xx": 
                        # 'xx' indicates the C code just finished (or started) a frame.
                        # This is the "VSync" signal.
                        
                        # 1. Commit the buffer to the screen (Atomic Update)
                        if self.frame_buffer:
                            # Convert list of dicts to flat arrays for performance
                            pos = np.array([s['pos'] for s in self.frame_buffer])
                            sizes = [s['size'] for s in self.frame_buffer]
                            brushes = [s['brush'] for s in self.frame_buffer]
                            
                            # setData replaces OLD points with NEW points instantly
                            self.scatter.setData(
                                pos=pos, 
                                size=sizes, 
                                brush=brushes,
                                pen=pg.mkPen(None)
                            )
                        else:
                            # Empty frame (e.g. looking at ground) -> Clear screen
                            self.scatter.setData(pos=np.zeros((0,2)))

                        # 2. Reset the buffer for the NEXT frame
                        self.frame_buffer = []

                    elif line.startswith('$') and len(line) >= 11:
                        # Parse star and add to BACK BUFFER
                        # Do NOT update screen yet
                        star = self.parse_star_line(line)
                        if star:
                            self.frame_buffer.append(star)

        except Exception as e:
            print(f"Serial Error: {e}")

if __name__ == '__main__':
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    palette = app.palette()
    palette.setColor(QPalette.ColorRole.Window, QColor(20, 20, 20))
    palette.setColor(QPalette.ColorRole.WindowText, Qt.white)
    app.setPalette(palette)

    window = VRSimulator()
    window.show()
    sys.exit(app.exec())