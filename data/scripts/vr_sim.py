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

class VRSimulator(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Star Tracker VR Simulator")
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
        
        # --- VISUAL AIDS ---
        # 1. Screen Boundary (Green Dashed)
        screen_box = pg.PlotCurveItem(
            x=[-1, 1, 1, -1, -1], 
            y=[-1, -1, 1, 1, -1], 
            pen=pg.mkPen('g', width=2, style=Qt.DashLine)
        )
        self.plot_widget.addItem(screen_box)
        '''
        # 2. Center Crosshair (Red)
        self.plot_widget.addItem(pg.InfiniteLine(angle=0, pen=pg.mkPen('r', width=1, style=Qt.DotLine)))
        self.plot_widget.addItem(pg.InfiniteLine(angle=90, pen=pg.mkPen('r', width=1, style=Qt.DotLine)))

        # 3. "Up" Indicator (Triangle at Top)
        up_arrow = pg.PlotCurveItem(
            x=[-0.1, 0, 0.1], 
            y=[0.9, 1.0, 0.9], 
            pen=pg.mkPen('y', width=2),
            fillLevel=0, brush=pg.mkBrush('y')
        )
        self.plot_widget.addItem(up_arrow)
        '''

        # Scatter Item
        self.scatter = pg.ScatterPlotItem(pxMode=True)
        self.plot_widget.addItem(self.scatter)
        
        layout.addWidget(self.plot_widget)

        # 3. Serial Handling
        self.serial_port = None
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial_and_plot)

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
                self.serial_port = serial.Serial(port, BAUD_RATE, timeout=0.01)
                self.timer.start(30) 
                self.connect_btn.setText("Disconnect")
                print(f"Connected to {port}")
            except Exception as e:
                print(f"Error: {e}")

    def read_serial_and_plot(self):
        if not self.serial_port or not self.serial_port.is_open:
            return

        try:
            if self.serial_port.in_waiting > 0:
                raw_data = self.serial_port.read(self.serial_port.in_waiting)
                text_data = raw_data.decode('utf-8', errors='ignore')
                lines = text_data.split('\n')

                new_pos = []
                new_sizes = []
                new_brushes = []
                found_stars = False

                for line in lines:
                    line = line.strip()
                    if line.startswith('$') and len(line) >= 11:
                        try:
                            # Parse Hex
                            hex_x = line[1:5]
                            hex_y = line[5:9]
                            hex_m = line[9:11]

                            x_int = int(hex_x, 16)
                            if x_int > 32767: x_int -= 65536
                            
                            y_int = int(hex_y, 16)
                            if y_int > 32767: y_int -= 65536
                            
                            mag_int = int(hex_m, 16)

                            x = x_int / SCALE_XY
                            y = y_int / SCALE_XY
                            mag = mag_int / SCALE_MAG

                            new_pos.append([x, y])
                            size = max(2, (6.0 - mag) * 4) 
                            new_sizes.append(size)
                            new_brushes.append(pg.mkBrush(255, 255, 255, 200))
                            found_stars = True

                        except ValueError:
                            continue

                if found_stars:
                    self.scatter.setData(
                        pos=new_pos, 
                        size=new_sizes, 
                        brush=new_brushes,
                        pen=pg.mkPen(None)
                    )

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