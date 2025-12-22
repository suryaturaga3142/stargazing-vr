#!/usr/bin/env python3
# -*- coding: utf-8 -*-

################################################################################
# @file        see_stars.py
# @brief       A script to visualize sorted and processed star data in a CSV.
# @author      LED Chasers
# @date        2025-11-28
# @version     1.0
################################################################################
#
# @details     This script serves as the verification for the project sorting algorithm.
#              It uses an existing CSV file and plots them in a 3D space using pyqtgraph,
#              which is better than matplotlib because of usage of OpenGL.
#
# @note        This is an offline tool and is a mandatory step in the project's
#              data pipeline. It must be run on a PC with an existing 'catalog_verification.csv'
#              that was produced by an existing C++ sorting algorithm in order to test it.
#
# @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
#
################################################################################
#
# DEPENDENCIES:
# =============
#   - Python 3.x
#   - NumPy: Astropy dependency, used for data filtering. (`pip install numpy`)
#   - Pandas: Calculations ('pip install pandas')
#   - PySide6: GUI framework ('pip install PySide6')
#   - PyQtGraph: Plotting ('pip install pyqtgraph')
#   - PyOpenGL: Required for 3D visualization ('pip install PyOpenGL PyOpenGL_accelerate')
#
# USAGE:
# ======
#   Run from the command line in the `stargazing_vr/` directory:
#   > python data/scripts/see_stars.py
#
################################################################################

# --- Main script logic begins here ---

import sys
import os
import pandas as pd
import numpy as np
from PySide6.QtWidgets import (QApplication, QMainWindow, QLabel, QVBoxLayout, 
                               QWidget, QComboBox, QHBoxLayout, QLineEdit)
from PySide6.QtCore import Qt, Signal, QEvent
from PySide6.QtGui import QPalette, QColor, QStandardItemModel, QStandardItem
import pyqtgraph as pg
import pyqtgraph.opengl as gl

# --- Constants for Binning ---
SKY_PATCH_RA_DIVISIONS = 24
SKY_PATCH_DEC_DIVISIONS = 12

class CheckableComboBox(QComboBox):
    # Signal to notify parent when selection changes
    selectionUpdated = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setEditable(True)
        self.lineEdit().setReadOnly(True)
        
        # Use a StandardItemModel to support checkable items
        self.setModel(QStandardItemModel(self))
        self.view().pressed.connect(self.handleItemPressed)
        
        # Update display text initially
        self.updateText()

    def handleItemPressed(self, index):
        item = self.model().itemFromIndex(index)
        
        # Toggle Check State
        if item.checkState() == Qt.Checked:
            item.setCheckState(Qt.Unchecked)
        else:
            item.setCheckState(Qt.Checked)
            
        self.updateText()
        self.selectionUpdated.emit()

    def addItem(self, text, data=None):
        item = QStandardItem(text)
        item.setFlags(Qt.ItemIsEnabled | Qt.ItemIsUserCheckable)
        item.setData(data)
        item.setCheckState(Qt.Unchecked) # Default Unchecked
        self.model().appendRow(item)
        self.updateText()

    def getCheckedData(self):
        res = []
        for i in range(self.model().rowCount()):
            item = self.model().item(i)
            if item.checkState() == Qt.Checked:
                res.append(item.data())
        return res

    def updateText(self):
        checked = self.getCheckedData()
        if not checked:
            self.lineEdit().setText("None Selected")
        elif len(checked) == self.model().rowCount():
            self.lineEdit().setText("All Selected")
        else:
            self.lineEdit().setText(f"{len(checked)} Bins Selected")

class StarVisualizer(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Star Catalog Visualizer (PySide6 + PyQtGraph)")
        self.resize(1200, 900)

        # Main layout container
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        
        # 1. Setup OpenGL View
        self.view = gl.GLViewWidget()
        self.view.opts['distance'] = 2.5  # Initial camera distance
        self.view.opts['fov'] = 60        # Field of view
        main_layout.addWidget(self.view, stretch=1) # Give view mostly all space

        # 2. Add Controls Area (Bottom)
        controls_layout = QHBoxLayout()
        controls_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.addLayout(controls_layout)

        # RA Filter
        self.ra_combo = CheckableComboBox()
        for i in range(SKY_PATCH_RA_DIVISIONS):
            self.ra_combo.addItem(f"RA Bin {i} ({i}h)", i)
        self.ra_combo.selectionUpdated.connect(self.update_plot)
        
        controls_layout.addWidget(QLabel("Filter RA:"))
        controls_layout.addWidget(self.ra_combo, stretch=1)

        # Dec Filter
        self.dec_combo = CheckableComboBox()
        for i in range(SKY_PATCH_DEC_DIVISIONS):
            self.dec_combo.addItem(f"Dec Bin {i}", i)
        self.dec_combo.selectionUpdated.connect(self.update_plot)
        
        controls_layout.addWidget(QLabel("Filter Dec:"))
        controls_layout.addWidget(self.dec_combo, stretch=1)

        # 3. Add Axes (Red=X, Green=Y, Blue=Z)
        axis = gl.GLAxisItem()
        axis.setSize(1.5, 1.5, 1.5)
        self.view.addItem(axis)

        # Placeholder for Scatter Item
        self.scatter_item = None

        # Data Storage
        self.df = None
        self.full_pos = None
        self.full_sizes = None
        self.full_colors = None

        # 4. Load Data
        self.load_data()

    def load_data(self):
        # --- PATH LOGIC ---
        script_dir = os.path.dirname(os.path.abspath(__file__))
        data_dir = os.path.dirname(script_dir)
        csv_path = os.path.join(data_dir, 'outputs', 'catalog_verification.csv')

        if not os.path.exists(csv_path):
            print(f"Error: Could not find {csv_path}")
            return

        print(f"Loading {csv_path}...")
        self.df = pd.read_csv(csv_path)
        
        # --- Prepare Full Arrays ---
        # Positions
        self.full_pos = np.vstack([self.df['X'], self.df['Y'], self.df['Z']]).transpose()

        # Metadata (Magnitude)
        mags = self.df['Mag'].to_numpy()
        
        # Sizes
        sizes = (6.5 - mags) * 5 
        self.full_sizes = np.clip(sizes, 2, 20) 

        # Colors (Viridis)
        min_mag = mags.min()
        max_mag = mags.max()
        norm_mags = 1.0 - (mags - min_mag) / (max_mag - min_mag)
        cm = pg.colormap.get('viridis')
        self.full_colors = cm.map(norm_mags, mode='float')

        # --- Init Scatter Plot (Empty initially) ---
        self.scatter_item = gl.GLScatterPlotItem(pos=np.zeros((0,3)), size=np.zeros(0), color=np.zeros((0,4)), pxMode=True)
        self.scatter_item.setGLOptions('translucent')
        self.view.addItem(self.scatter_item)
        
        # Initial Plot Update
        self.update_plot()

    def update_plot(self):
        if self.df is None:
            return

        # Get checked bins
        checked_ra = self.ra_combo.getCheckedData()
        checked_dec = self.dec_combo.getCheckedData()

        # Logic: If either RA list or Dec list is empty, show NOTHING.
        # This matches "checking a box will only show stars of that ra and dec set"
        if not checked_ra or not checked_dec:
            self.scatter_item.setData(pos=np.zeros((0,3)), size=np.zeros(0), color=np.zeros((0,4)))
            return

        # Filter
        mask = self.df['RA_Idx'].isin(checked_ra) & self.df['Dec_Idx'].isin(checked_dec)
        
        visible_pos = self.full_pos[mask]
        visible_sizes = self.full_sizes[mask]
        visible_colors = self.full_colors[mask]

        self.scatter_item.setData(pos=visible_pos, size=visible_sizes, color=visible_colors)
        # print(f"Showing {len(visible_pos)} stars in selected bins.")

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