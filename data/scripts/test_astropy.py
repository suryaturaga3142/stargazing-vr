import struct
import numpy as np
from astropy.io import fits
from astropy.utils.data import get_pkg_data_filename

def process_fits_to_binary(fits_filepath, bin_filepath):
    """
    Opens a FITS binary table containing star data, extracts key columns,
    and saves them as a packed binary file.

    The binary file format for each star will be:
    - Right Ascension (float32, 4 bytes)
    - Declination (float32, 4 bytes)
    - V-band Magnitude (float32, 4 bytes)

    Args:
        fits_filepath (str): Path to the input FITS file.
        bin_filepath (str): Path for the output binary (.bin) file.
    """
    print(f"Opening FITS file: {fits_filepath}")
    
    try:
        # Using a 'with' block ensures the file is properly closed
        with fits.open(fits_filepath) as hdul:
            # Star catalogs are typically in the first extension (index 1)
            # which is a BINTABLE HDU. The primary HDU (index 0) is often
            # just a header or an empty image.
            if len(hdul) < 2 or not isinstance(hdul[1], fits.BinTableHDU):
                print("Error: FITS file does not contain a binary table in the first extension.")
                return

            star_data_table = hdul[1].data
            print(f"Successfully loaded FITS table with {len(star_data_table)} rows.")
            print(f"Columns in table: {star_data_table.columns.names}")

            # Define the names of the columns we want to extract.
            # These names are specific to this sample FITS file.
            # You would change these to match the columns in your own file.
            RA_COL = 'RA'
            DEC_COL = 'DEC'
            MAG_COL = 'V_MAG'

            # --- Struct Packing ---
            # 'f' is the format character for a standard C float (4 bytes).
            # Since we are packing three floats (RA, Dec, Mag), our format is 'fff'.
            # The '<' specifies little-endian byte order, which is a common standard.
            star_struct_format = '<fff'
            struct_size = struct.calcsize(star_struct_format)
            print(f"Struct format: '{star_struct_format}' ({struct_size} bytes per star)")

            # Use a bytearray as a mutable, efficient buffer for binary data
            binary_buffer = bytearray()
            
            # Iterate over each row in the FITS table
            for star in star_data_table:
                # Extract the values and ensure they are float32
                ra = np.float32(star[RA_COL])
                dec = np.float32(star[DEC_COL])
                mag = np.float32(star[MAG_COL])

                # Pack the three float32 values into a binary string (bytes)
                packed_star_data = struct.pack(star_struct_format, ra, dec, mag)
                
                # Add the packed bytes to our buffer
                binary_buffer.extend(packed_star_data)

            print(f"\nProcessing complete. Total buffer size: {len(binary_buffer)} bytes.")

            # --- Verification ---
            num_stars = len(star_data_table)
            expected_size = num_stars * struct_size
            print(f"Expected buffer size for {num_stars} stars: {expected_size} bytes.")
            if len(binary_buffer) == expected_size:
                print("Verification successful: Buffer size matches expected size.")
            else:
                print("Verification failed: Buffer size mismatch!")

            # Unpack the first star from our buffer to prove the data is stored correctly
            if num_stars > 0:
                print("\n--- Verifying first star data ---")
                first_star_original = star_data_table[0]
                
                # Slice the buffer to get the bytes for the first star
                first_star_packed = binary_buffer[0:struct_size]
                
                # Unpack the bytes back into Python values
                unpacked_data = struct.unpack(star_struct_format, first_star_packed)

                print(f"Original: RA={first_star_original[RA_COL]:.6f}, "
                      f"DEC={first_star_original[DEC_COL]:.6f}, "
                      f"Mag={first_star_original[MAG_COL]:.6f}")
                print(f"Unpacked: RA={unpacked_data[0]:.6f}, "
                      f"DEC={unpacked_data[1]:.6f}, "
                      f"Mag={unpacked_data[2]:.6f}")

            # Save the binary buffer to a file
            print(f"\nSaving binary data to {bin_filepath}")
            with open(bin_filepath, 'wb') as f:
                f.write(binary_buffer)
            print("File saved successfully.")

    except FileNotFoundError:
        print(f"Error: FITS file not found at {fits_filepath}")
    except KeyError as e:
        print(f"Error: A required column was not found in the FITS table: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")


if __name__ == '__main__':
    # Astropy includes several sample data files. 'table.fits' is a simple
    # binary table that contains star catalog-like data.
    # This function finds the path to that file within your astropy installation.
    try:
        sample_fits_file = get_pkg_data_filename('data/table.fits')
        output_bin_file = 'star_data.bin'
        process_fits_to_binary(sample_fits_file, output_bin_file)
    except Exception as e:
        print(f"Could not find or access the astropy sample data file. Error: {e}")
        print("Please ensure you have astropy installed ('pip install astropy').")
