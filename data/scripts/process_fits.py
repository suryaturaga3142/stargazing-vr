#!/usr/bin/env python3
# -*- coding: utf-8 -*-

################################################################################
# @file        process_fits.py
# @brief       Compiles a scientific FITS star catalog into an optimized binary format.
# @author      LED Chasers
# @date        2025-10-15
# @version     1.0
################################################################################
#
# @details     This script serves as the "data compiler" for the project.
#              It reads a large, professional FITS file (e.g., from the Hipparcos or
#              Gaia catalog), filters the stars to a desired magnitude limit, and
#              then packs the essential astrometric data into a compact, custom
#              binary file (`stars.bin`). This output file is specifically
#              formatted to be loaded with maximum efficiency by the RP2350
#              microcontroller at startup.
#
# @note        This is an offline tool and is a mandatory step in the project's
#              data pipeline. It must be run on a PC to generate the `stars.bin`
#              file before the data can be copied to the device's SD card.
#
# @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
#
################################################################################
#
# DEPENDENCIES:
# =============
#   - Python 3.x
#   - Astropy: For reading the FITS file. (`pip install astropy`)
#   - NumPy: Astropy dependency, used for data filtering. (`pip install numpy`)
#
# USAGE:
# ======
#   Run from the command line in the `data/` directory:
#   > python scripts/process_fits.py
#
################################################################################

# --- Main script logic begins here ---

import struct
from astropy.io import fits
import numpy as np
import os
import math

# --- Define C Struct formats ---
# typedef struct {
#     uint32_t magic_number;    // <I (4 bytes)
#     uint16_t version;         // <H (2 bytes)
#     uint16_t header_size;     // <H (2 bytes)
#     uint32_t star_count;      // <I (4 bytes)
# } StarFileHeader_t;
HEADER_FORMAT = '<IHHI'
MAGIC_NUMBER = 0x53544152  # "STAR"

# typedef struct {
#     int32_t   ra_scaled;      // <l (4 bytes)
#     int32_t   dec_scaled;     // <l (4 bytes)
#     int16_t   pmra_scaled;    // <h (2 bytes)
#     int16_t   pmdec_scaled;   // <h (2 bytes)
#     int16_t   mag_scaled;     // <h (2 bytes)
# } PackedStar_t;
# Note: This struct is 14 bytes (4+4+2+2+2).
# Ensure your C compiler packs it tightly (e.g., #pragma pack(1)).
STAR_FORMAT = '<llhhh'

# --- Scaling Constants ---
# We want to store floats as integers, so we define scaling factors.
# int_val = float_val * SCALING_FACTOR
POS_SCALE = 1_000_000.0  # For RA/Dec: 1e-6 degree precision
PM_SCALE = 100.0         # For Proper Motion: 0.01 mas/yr precision
MAG_SCALE = 1000.0       # For Magnitude: 0.001 mag precision

# Clamping for int16_t range
PM_MIN = -32767
PM_MAX = 32767
MAG_MAX = 32767

def clamp(value, min_val, max_val):
    """Clamps a value to the (min_val, max_val) range."""
    return max(min_val, min(max_val, value))

def process_fits_to_custom_binary(fits_filepath, bin_filepath):
    """
    Opens the Hipparcos FITS file, extracts, sorts, and scales the
    9000 brightest stars, and saves them to a custom binary file.
    """
    
    if not os.path.exists(fits_filepath):
        print(f"Error: File not found at {fits_filepath}")
        print("Please download the FITS file from VizieR and")
        print(f"save it as '{fits_filepath}'")
        return

    print(f"Opening FITS file: {fits_filepath}")
    
    try:
        with fits.open(fits_filepath) as hdul:
            # Data is in the first extension (index 1)
            data = hdul[1].data
            
            # --- 1. Define required columns ---
            # The VizieR-generated FITS file uses '_RA_icrs' and '_DE_icrs'
            # for J2000 decimal degrees when 'Decimal °' is selected.
            COL_RA = '_RA_icrs' 
            COL_DEC = '_DE_icrs' 
            COL_MAG = 'Vmag'     # Johnson V-band magnitude
            COL_PMRA = 'pmRA'    # Proper motion in RA (mas/yr)
            COL_PMDE = 'pmDE'    # Proper motion in Dec (mas/yr)

            required_cols = [COL_RA, COL_DEC, COL_MAG, COL_PMRA, COL_PMDE]
            
            # Verify all columns exist
            for col in required_cols:
                if col not in data.columns.names:
                    print(f"Error: Required column '{col}' not found in FITS file.")
                    print(f"Available columns are: {data.columns.names}")
                    return

            print(f"Found {len(data)} total stars. Filtering and sorting...")

            # --- 2. Filter and Store Stars ---
            valid_stars = []
            for star in data:
                mag = star[COL_MAG]
                ra = star[COL_RA]
                dec = star[COL_DEC]
                
                # *** FIX: Check ALL float values for NaN/inf ***
                if (mag is not None and math.isfinite(mag) and
                    ra is not None and math.isfinite(ra) and
                    dec is not None and math.isfinite(dec)):
                    
                    pmra = star[COL_PMRA]
                    pmde = star[COL_PMDE]
                    
                    # Assume missing proper motion is 0.0
                    if not math.isfinite(pmra):
                        pmra = 0.0
                    if not math.isfinite(pmde):
                        pmde = 0.0

                    valid_stars.append(
                        (mag, ra, dec, pmra, pmde)
                    )
            
            print(f"Found {len(valid_stars)} stars with valid positions and magnitudes.")

            # --- 3. Sort by Brightness and Get Top 9000 ---
            valid_stars.sort(key=lambda x: x[0])
            brightest_stars = valid_stars[:9000]
            star_count = len(brightest_stars)

            print(f"Processing the {star_count} brightest stars.")

            # --- 4. Open Binary File and Write Header ---
            
            # Ensure the output directory exists
            output_dir = os.path.dirname(bin_filepath)
            if not os.path.exists(output_dir):
                print(f"Creating output directory: {output_dir}")
                os.makedirs(output_dir)
                
            print(f"Opening binary file for writing: {bin_filepath}")
            with open(bin_filepath, 'wb') as f:
                header_size = struct.calcsize(HEADER_FORMAT)
                
                header_data = struct.pack(
                    HEADER_FORMAT,
                    MAGIC_NUMBER,
                    1,  # version
                    header_size,
                    star_count
                )
                f.write(header_data)

                # --- 5. Pack and Write Each Star ---
                for star_data in brightest_stars:
                    mag, ra, dec, pmra, pmde = star_data
                    
                    ra_scaled = int(ra * POS_SCALE)
                    dec_scaled = int(dec * POS_SCALE)
                    mag_scaled = clamp(int(mag * MAG_SCALE), -MAG_MAX, MAG_MAX)
                    pmra_scaled = clamp(int(pmra * PM_SCALE), PM_MIN, PM_MAX)
                    pmdec_scaled = clamp(int(pmde * PM_SCALE), PM_MIN, PM_MAX)
                    
                    packed_star = struct.pack(
                        STAR_FORMAT,
                        ra_scaled,
                        dec_scaled,
                        pmra_scaled,
                        pmdec_scaled,
                        mag_scaled
                    )
                    f.write(packed_star)

            print("\nProcessing complete.")
            
            # --- Verification ---
            total_expected_size = header_size + (star_count * struct.calcsize(STAR_FORMAT))
            actual_size = os.path.getsize(bin_filepath)
            
            print(f"Wrote {star_count} stars.")
            print(f"Header size: {header_size} bytes")
            print(f"Data size:   {actual_size - header_size} bytes")
            print(f"Total file size: {actual_size} bytes")
            if actual_size == total_expected_size:
                print("Verification successful: File size matches expected size.")
            else:
                print(f"Verification FAILED: Expected {total_expected_size} bytes.")

            # --- Unpack first star to verify ---
            with open(bin_filepath, 'rb') as f:
                print("\n--- Verifying file header ---")
                header_bin = f.read(header_size)
                unpacked_header = struct.unpack(HEADER_FORMAT, header_bin)
                print(f"Magic: {hex(unpacked_header[0])} (Should be 0x53544152)")
                print(f"Version: {unpacked_header[1]}")
                print(f"Header Size: {unpacked_header[2]}")
                print(f"Star Count: {unpacked_header[3]}")
                
                if star_count > 0:
                    print("\n--- Verifying first star data ---")
                    first_star_original = brightest_stars[0]
                    star_bin = f.read(struct.calcsize(STAR_FORMAT))
                    unpacked_star = struct.unpack(STAR_FORMAT, star_bin)
                    
                    print(f"Original (float):  Mag={first_star_original[0]:.3f}, "
                          f"RA={first_star_original[1]:.6f}, "
                          f"Dec={first_star_original[2]:.6f}")
                    
                    print(f"Scaled (int):    Mag={unpacked_star[4]}, "
                          f"RA={unpacked_star[0]}, "
                          f"Dec={unpacked_star[1]}")
                          
                    print(f"Unscaled (float):  Mag={unpacked_star[4] / MAG_SCALE:.3f}, "
                          f"RA={unpacked_star[0] / POS_SCALE:.6f}, "
                          f"Dec={unpacked_star[1] / POS_SCALE:.6f}")


    except FileNotFoundError:
        print(f"Error: FITS file not found at {fits_filepath}")
    except KeyError as e:
        print(f"Error: A required column was not found in the FITS table: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")


if __name__ == '__main__':
    # --- Define File Paths Based on Script Location ---
    
    # Get the directory this script is in (e.g., .../data/scripts)
    SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
    
    # Get the parent 'data' directory (e.g., .../data)
    DATA_DIR = os.path.dirname(SCRIPT_DIR)
    
    # Define the input file path (e.g., .../data/sources/hipparcos.fit)
    fits_file = os.path.join(DATA_DIR, 'sources', 'hipparcos.fit')
    
    # Define the output directory (e.g., .../data/bin)
    output_dir = os.path.join(DATA_DIR, 'bin')
    
    # Define the output file path (e.g., .../data/bin/stars.bin)
    output_bin_file = os.path.join(output_dir, 'stars.bin')
    
    print(f"Script location: {SCRIPT_DIR}")
    print(f"Project root (data) dir: {DATA_DIR}")
    print(f"Input FITS file: {fits_file}")
    print(f"Output BIN file: {output_bin_file}")
    print("-" * 30)

    # Run the processing function
    process_fits_to_custom_binary(fits_file, output_bin_file)

