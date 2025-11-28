/*******************************************************************************
 * @file        test_buf_sort.cpp
 * @brief       This is a software only file meant to run on g++ to test sorting.
 * @details     Data needs to be loaded from a binary file into a buffer, and 
 * then unpacked, processed, and sorted into a 24x12 array of spherical patches.
 * This code tests the functionality of this process by carrying out this sorting
 * algorithm meant to place into the project. We verify the performance through
 * writing to a CSV file and then using a 3D plotter.
 * 
 * @author      LED Chasers
 * @date        2025-11-28
 * 
 * @note        This module is designed to be untouched by the microcontroller.
 * Check the memory usage, leaks, time, etc when running this to analyze 
 * performance.
 * 
 * @copyright   Copyright (c) 2025, LED Chasers. All rights reserved.
 ******************************************************************************/

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <iomanip>

// --------------------------------------------------------------------------
// Project Structs
// --------------------------------------------------------------------------

#define STAR_CATALOG_SIZE_MAX   15000
#define SKY_PATCH_RA_DIVISIONS  24
#define SKY_PATCH_DEC_DIVISIONS 12

// Header for file
typedef struct __attribute__((packed)) {
    uint32_t magic_number;
    uint16_t version;
    uint16_t header_size;
    uint32_t star_count;
} StarFileHeader_t;

// Raw star struct expected in bin file (14 bytes)
typedef struct __attribute__((packed)) {
    int32_t  ra_scaled;
    int32_t  dec_scaled;
    int16_t  pmra_scaled;
    int16_t  pmdec_scaled;
    int16_t  mag_scaled;
} PackedStar_t;

// Runtime struct
typedef struct {
    float x, y, z;
    float mag;
} Star_t;

// Runtime bins / patches of sphere
typedef struct {
    uint32_t start_index;
    uint16_t star_count;
} SkyPatch_t;

// --------------------------------------------------------------------------
// Global Buffers
// --------------------------------------------------------------------------

Star_t all_stars[STAR_CATALOG_SIZE_MAX];
SkyPatch_t sky_database[SKY_PATCH_RA_DIVISIONS][SKY_PATCH_DEC_DIVISIONS];
std::vector<PackedStar_t> sd_buffer; // Mock SD Input

// Constants from Python Script
const double SCALER_COORD = 1000000.0;
const double SCALER_MAG   = 1000.0;

// --------------------------------------------------------------------------
// Core Logic: Bin Calculation
// --------------------------------------------------------------------------

int get_ra_bin(int32_t ra_scaled) {
    double ra_deg = (double)ra_scaled / SCALER_COORD;
    while (ra_deg < 0) ra_deg += 360.0;
    while (ra_deg >= 360.0) ra_deg -= 360.0;
    return (int)(ra_deg / 15.0);
}

int get_dec_bin(int32_t dec_scaled) {
    double dec_deg = (double)dec_scaled / SCALER_COORD;
    if (dec_deg < -90.0) dec_deg = -90.0;
    if (dec_deg > 90.0) dec_deg = 90.0;
    
    // Bin 0 is South Pole (-90), Bin 11 is North Pole (+90)
    // Range is 180 degrees total. 12 bins = 15 deg per bin.
    double shifted = dec_deg + 90.0;
    int bin = (int)(shifted / 15.0);
    if (bin >= SKY_PATCH_DEC_DIVISIONS) bin = SKY_PATCH_DEC_DIVISIONS - 1;
    return bin;
}

// --------------------------------------------------------------------------
// Core Logic: Coordinate Conversion
// --------------------------------------------------------------------------

void convert_star(const PackedStar_t& src, Star_t& dst) {
    // Note: We ignore Proper Motion (pmra/pmdec) for static plotting
    // Ideally you propagate these based on current year (e.g. 2025.0)
    
    double ra_rad = ((double)src.ra_scaled / SCALER_COORD) * (M_PI / 180.0);
    double dec_rad = ((double)src.dec_scaled / SCALER_COORD) * (M_PI / 180.0);
    
    // Convert to Unit Vector (Z-Up)
    // X = cos(dec) * cos(ra)
    // Y = cos(dec) * sin(ra)
    // Z = sin(dec)
    
    dst.x = (float)(cos(dec_rad) * cos(ra_rad));
    dst.y = (float)(cos(dec_rad) * sin(ra_rad));
    dst.z = (float)(sin(dec_rad));
    dst.mag = (float)src.mag_scaled / SCALER_MAG;
}

// --------------------------------------------------------------------------
// THE SORTING ALGORITHM
// --------------------------------------------------------------------------

void sd_buf_sort() {
    std::cout << "[Sort] Clearing Database..." << std::endl;
    std::memset(sky_database, 0, sizeof(sky_database));

    std::cout << "[Sort] Counting Pass..." << std::endl;
    for (const auto& star : sd_buffer) {
        int r = get_ra_bin(star.ra_scaled);
        int d = get_dec_bin(star.dec_scaled);
        sky_database[r][d].star_count++;
    }

    std::cout << "[Sort] Prefix Sum Pass..." << std::endl;
    uint32_t current_idx = 0;
    for (int r = 0; r < SKY_PATCH_RA_DIVISIONS; r++) {
        for (int d = 0; d < SKY_PATCH_DEC_DIVISIONS; d++) {
            sky_database[r][d].start_index = current_idx;
            current_idx += sky_database[r][d].star_count;
        }
    }
    
    std::cout << "[Sort] Total Stars: " << current_idx << std::endl;
    if (current_idx > STAR_CATALOG_SIZE_MAX) {
        std::cerr << "ERROR: Buffer Overflow! Max " << STAR_CATALOG_SIZE_MAX << std::endl;
        return;
    }

    // Temporary copy of start indices to track insertion
    uint32_t temp_indices[SKY_PATCH_RA_DIVISIONS][SKY_PATCH_DEC_DIVISIONS];
    for (int r = 0; r < SKY_PATCH_RA_DIVISIONS; r++) {
        for (int d = 0; d < SKY_PATCH_DEC_DIVISIONS; d++) {
            temp_indices[r][d] = sky_database[r][d].start_index;
        }
    }

    std::cout << "[Sort] Population Pass..." << std::endl;
    for (const auto& star : sd_buffer) {
        int r = get_ra_bin(star.ra_scaled);
        int d = get_dec_bin(star.dec_scaled);
        
        uint32_t target_idx = temp_indices[r][d];
        convert_star(star, all_stars[target_idx]);
        temp_indices[r][d]++;
    }
}

// --------------------------------------------------------------------------
// File Loader
// --------------------------------------------------------------------------

void load_real_data(const char* filename) {
    std::ifstream f(filename, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "Failed to open " << filename << ". Generating fake data instead." << std::endl;
        // Fallback to fake data
        return;
    }

    StarFileHeader_t header;
    f.read((char*)&header, sizeof(header));
    
    if (header.magic_number != 0x53544152) {
        std::cerr << "Invalid Magic Number!" << std::endl;
        return;
    }

    std::cout << "Loading " << header.star_count << " stars from file..." << std::endl;
    
    // Resize vector
    sd_buffer.resize(header.star_count);
    
    // Bulk Read
    // Note: This relies on PackedStar_t being exactly 14 bytes packed
    f.read((char*)sd_buffer.data(), header.star_count * sizeof(PackedStar_t));
    
    std::cout << "Loaded successfully." << std::endl;
    f.close();
}

void generate_fake_data(int count) {
    std::cout << "Generating " << count << " random stars..." << std::endl;
    for (int i = 0; i < count; i++) {
        PackedStar_t p;
        p.ra_scaled = (int32_t)((double)rand()/RAND_MAX * 360.0 * SCALER_COORD);
        p.dec_scaled = (int32_t)(((double)rand()/RAND_MAX * 180.0 - 90.0) * SCALER_COORD);
        p.mag_scaled = (int16_t)((double)rand()/RAND_MAX * 6.0 * SCALER_MAG);
        sd_buffer.push_back(p);
    }
}

void export_csv() {
    std::ofstream f("../bin/debug_stars.csv");
    f << "RA_Idx,Dec_Idx,X,Y,Z,Mag" << std::endl;
    for (int r = 0; r < SKY_PATCH_RA_DIVISIONS; r++) {
        for (int d = 0; d < SKY_PATCH_DEC_DIVISIONS; d++) {
            uint32_t start = sky_database[r][d].start_index;
            uint32_t count = sky_database[r][d].star_count;
            for (uint32_t i = 0; i < count; i++) {
                Star_t& s = all_stars[start + i];
                f << r << "," << d << "," << s.x << "," << s.y << "," << s.z << "," << s.mag << std::endl;
            }
        }
    }
    std::cout << "Data exported to debug_stars.csv" << std::endl;
}

int main() {
    // Try to load real file, otherwise fake it
    sd_buffer.clear();
    load_real_data("../bin/stars.bin"); 
    
    if (sd_buffer.empty()) {
        generate_fake_data(5000);
    }

    sd_buf_sort();
    export_csv();
    
    return 0;
}
