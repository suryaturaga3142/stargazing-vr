import sys
import serial
import re
import numpy as np
# Import from PySide6 instead of PyQt5
from PySide6.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QLabel, QHBoxLayout
from PySide6.QtCore import QThread, Signal, QTimer, Qt  # Changed pyqtSignal to Signal
import pyqtgraph.opengl as gl

# --- CONFIGURATION ---
SERIAL_PORT = "COM5" 
SERIAL_BAUDRATE = 115200
# ---------------------

def normalize(v):
    """Normalizes a numpy array."""
    norm = np.linalg.norm(v)
    if norm == 0:
        return v
    return v / norm

def quat_multiply(q1, q2):
    """Multiplies two quaternions in [w, x, y, z] order."""
    w1, x1, y1, z1 = q1
    w2, x2, y2, z2 = q2
    w = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2
    x = w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2
    y = w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2
    z = w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2
    return np.array([w, x, y, z])

def apply_quaternion_rotation(v, q):
    """
    Rotates vector v by quaternion q.
    q is [i, j, k, w] (from BNO085)
    v is [x, y, z]
    Returns rotated vector [x', y', z']
    """
    # Re-order q from [i, j, k, w] to [w, i, j, k] for calculations
    q_calc = np.array([q[3], q[0], q[1], q[2]])
    
    # Normalize the quaternion
    q_calc = normalize(q_calc)
    
    # Create pure vector quaternion
    v_quat = np.array([0, v[0], v[1], v[2]])
    
    # Get conjugate of q
    q_conj = np.array([q_calc[0], -q_calc[1], -q_calc[2], -q_calc[3]])
    
    # The rotation formula: v' = q * v * q_conjugate
    v_rotated_quat = quat_multiply(quat_multiply(q_conj, v_quat), q_calc)
    
    # Return just the vector part
    return v_rotated_quat[1:]

class SerialThread(QThread):
    """
    Reads serial data from the RP2350 in a separate thread
    to avoid blocking the UI.
    """
    # Signal to emit quaternion data [i, j, k, real]
    # Changed pyqtSignal to Signal
    data_received = Signal(list)

    def __init__(self, port, baudrate):
        super(SerialThread, self).__init__()
        self.port = port
        self.baudrate = baudrate
        self.is_running = True
        self.ser = None

    def run(self):
        """Main thread loop: read, parse, and emit data."""
        print(f"Attempting to connect to {self.port} at {self.baudrate}...")
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            print("Serial connection successful.")
        except serial.SerialException as e:
            print(f"Error: Could not open serial port {self.port}: {e}")
            return

        while self.is_running:
            try:
                line = self.ser.readline().decode('utf-8').strip()
                if line:
                    # Parse the line: "Game: <i> <j> <k> <real>"
                    match = re.search(r"Game: ([\d.-]+) ([\d.-]+) ([\d.-]+) ([\d.-]+)", line)
                    if match:
                        q_data = [
                            float(match.group(1)), # i
                            float(match.group(2)), # j
                            float(match.group(3)), # k
                            float(match.group(4))  # real (w)
                        ]
                        # Emit the parsed quaternion data
                        self.data_received.emit(q_data)
            except serial.SerialException as e:
                print(f"Serial error: {e}")
                break
            except Exception as e:
                print(f"Parsing error: {e}. Line: '{line}'")

    def stop(self):
        """Stops the thread and closes the serial port."""
        self.is_running = False
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.wait()

class MainWindow(QMainWindow):
    """Main application window with 3D visualization."""

    def __init__(self):
        super(MainWindow, self).__init__()
        self.setWindowTitle("BNO085 3D Visualization (PySide6)")
        self.setGeometry(100, 100, 800, 600)

        # 1. Create the central widget and layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout()
        central_widget.setLayout(main_layout)

        # 2. Create the 3D view widget
        self.view = gl.GLViewWidget()
        self.view.setCameraPosition(distance=3)
        main_layout.addWidget(self.view, 1) # Add view with stretch factor

        # 3. Add a 3D grid to the scene
        grid = gl.GLGridItem()
        grid.scale(1, 1, 1)
        self.view.addItem(grid)
        
        # 4. Add labels for status
        self.status_label = QLabel("Connecting to IMU...")
        self.quat_label = QLabel("Quaternion (i, j, k, w): N/A")
        font = self.status_label.font()
        font.setPointSize(10)
        self.status_label.setFont(font)
        self.quat_label.setFont(font)
        
        status_layout = QHBoxLayout()
        status_layout.addWidget(self.status_label)
        status_layout.addWidget(self.quat_label)
        main_layout.addLayout(status_layout)

        # 5. Define the vector to be rotated (points up Z-axis)
        self.base_vector = np.array([0, 0, 1])
        
        # 6. Create the 3D line item for the vector
        origin = np.array([0, 0, 0])
        vector_end = self.base_vector
        line_data = np.array([origin, vector_end])
        
        self.vector_line = gl.GLLinePlotItem(
            pos=line_data,
            color=(1.0, 0.0, 0.0, 1.0), # Red
            width=3,
            antialias=True
        )
        self.view.addItem(self.vector_line)
        
        # 7. Add axes lines (X=Green, Y=Blue, Z=Red-ish)
        x_axis = gl.GLLinePlotItem(pos=np.array([[0,0,0], [1,0,0]]), color=(0,1,0,1), width=1)
        y_axis = gl.GLLinePlotItem(pos=np.array([[0,0,0], [0,1,0]]), color=(0,0,1,1), width=1)
        z_axis = gl.GLLinePlotItem(pos=np.array([[0,0,0], [0,0,1]]), color=(1,0.5,0.5,1), width=1)
        self.view.addItem(x_axis)
        self.view.addItem(y_axis)
        self.view.addItem(z_axis)


        # 8. Start the serial reader thread
        self.serial_thread = SerialThread(SERIAL_PORT, SERIAL_BAUDRATE)
        self.serial_thread.data_received.connect(self.update_rotation)
        self.serial_thread.start()
        
        if self.serial_thread.ser and self.serial_thread.ser.is_open:
             self.status_label.setText(f"Connected to {SERIAL_PORT}")
        else:
             self.status_label.setText(f"Failed to connect to {SERIAL_PORT}. Check port.")

    def update_rotation(self, q):
        """
        Slot to receive quaternion data and update the 3D scene.
        q is [i, j, k, w]
        """
        # Update the text label
        self.quat_label.setText(f"Quaternion (i, j, k, w): {q[0]:.3f}, {q[1]:.3f}, {q[2]:.3f}, {q[3]:.3f}")
        
        # Calculate the new rotated vector
        rotated_vector = apply_quaternion_rotation(self.base_vector, q)
        
        # Update the 3D line in the scene
        origin = np.array([0, 0, 0])
        line_data = np.array([origin, rotated_vector])
        self.vector_line.setData(pos=line_data)

    def closeEvent(self, event):
        """Ensure the serial thread is stopped when the window closes."""
        self.serial_thread.stop()
        event.accept()


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    # Changed app.exec_() to app.exec()
    sys.exit(app.exec())