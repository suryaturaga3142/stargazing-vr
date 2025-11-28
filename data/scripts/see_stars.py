import sys
import os
import pandas as pd
import numpy as np
from PySide6.QtWidgets import QApplication, QMainWindow, QLabel, QVBoxLayout, QWidget
from PySide6.QtGui import QPalette, QColor
from PySide6.QtCore import Qt
import pyqtgraph as pg
import pyqtgraph.opengl as gl

class StarVisualizer(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Star Catalog Visualizer (PySide6 + PyQtGraph)")
        self.resize(1200, 900)

        # Main layout container
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)
        
        # 1. Setup OpenGL View
        self.view = gl.GLViewWidget()
        self.view.opts['distance'] = 2.5  # Initial camera distance
        self.view.opts['fov'] = 60        # Field of view
        layout.addWidget(self.view)

        # Instructions Label
        # info_label = QLabel("Left Click: Rotate | Right Click: Pan | Scroll: Zoom")
        # info_label.setAlignment(Qt.AlignCenter)
        # info_label.setStyleSheet("color: gray; font-size: 12px; padding: 5px;")
        # layout.addWidget(info_label)

        # 2. Add Reference Grid (Optional, helps visual orientation)
        # g = gl.GLGridItem()
        # g.scale(0.1, 0.1, 0.1)
        # g.setDepthValue(10) 
        # self.view.addItem(g)

        # 3. Add Axes (Red=X, Green=Y, Blue=Z)
        axis = gl.GLAxisItem()
        axis.setSize(1.5, 1.5, 1.5)
        self.view.addItem(axis)

        # 4. Load and Plot Data
        self.load_data()

    def load_data(self):
        script_dir = os.path.dirname(os.path.abspath(__file__))
        data_dir = os.path.dirname(script_dir)
        csv_path = os.path.join(data_dir, 'bin', 'debug_stars.csv')

        if not os.path.exists(csv_path):
            print(f"Error: Could not find {csv_path}")
            print("Expected path structure: {common}/data/scripts/this_script.py")
            print("                         {common}/data/bin/debug_stars.csv")
            return

        print(f"Loading {csv_path}...")
        df = pd.read_csv(csv_path)
        
        # --- Prepare Coordinates ---
        # PyQtGraph expects a (N, 3) numpy array of floats
        pos = np.vstack([df['X'], df['Y'], df['Z']]).transpose()

        # --- Prepare Metadata (Magnitude) ---
        mags = df['Mag'].to_numpy()
        
        # --- Calculate Sizes ---
        # Brighter stars (lower mag) should be larger.
        # Formula: Size = Base + (MaxMag - Mag) * Scale
        # We clip sizes to keep them visible but not overwhelming
        sizes = (6.5 - mags) * 5 
        sizes = np.clip(sizes, 2, 20) 

        # --- Calculate Colors ---
        # Map magnitude to a colormap (Viridis: Yellow=Bright, Purple=Dim)
        # Normalize mags to 0.0 - 1.0 range
        min_mag = mags.min()
        max_mag = mags.max()
        
        # Normalize: 0.0 (dimmest) to 1.0 (brightest)
        # We invert it because viridis yellow (bright) is at 1.0
        norm_mags = 1.0 - (mags - min_mag) / (max_mag - min_mag)

        # Get colormap from pyqtgraph
        cm = pg.colormap.get('viridis')
        # Map normalized values to RGBA colors (N, 4)
        colors = cm.map(norm_mags, mode='float')

        # --- Create Scatter Plot Item ---
        # pxMode=True: Points stay the same pixel size regardless of zoom (good for stars)
        # pxMode=False: Points scale with the world (good for physical spheres)
        sp = gl.GLScatterPlotItem(
            pos=pos, 
            size=sizes, 
            color=colors, 
            pxMode=True
        )
        
        # Optimize transparency rendering
        sp.setGLOptions('translucent')

        self.view.addItem(sp)
        print(f"Successfully plotted {len(df)} stars.")

if __name__ == '__main__':
    app = QApplication(sys.argv)
    
    # Dark theme for space vibes
    app.setStyle('Fusion')
    palette = app.palette()
    palette.setColor(QPalette.ColorRole.Window, Qt.black)
    palette.setColor(QPalette.ColorRole.WindowText, Qt.white)
    app.setPalette(palette)

    window = StarVisualizer()
    window.show()
    sys.exit(app.exec())