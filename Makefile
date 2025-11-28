# Makefile for stargazing-vr

# --- Configuration ---
PROJECT_DIR := pcb-stargazing
PROJECT_NAME := pcb-stargazing

# Compiler settings for PC-side verification tools
CXX := g++
CXXFLAGS := -Wall -Wextra -g -O0
LDFLAGS := -lm

# Python Interpreter
PYTHON := py

# --- File Paths ---
DATA_DIR := data
SCRIPTS_DIR := $(DATA_DIR)/scripts
OUTPUTS_DIR := $(DATA_DIR)/outputs
SOURCES_DIR := $(DATA_DIR)/sources

# Input/Output Files
FITS_FILE := $(SOURCES_DIR)/hipparcos.fit
BIN_FILE := $(OUTPUTS_DIR)/stars.bin
CSV_FILE := $(OUTPUTS_DIR)/catalog_verification.csv

# Scripts
SCRIPT_PROCESS_FITS := $(SCRIPTS_DIR)/process_fits.py
SCRIPT_SEE_STARS := $(SCRIPTS_DIR)/see_stars.py
SCRIPT_GAME_ROT := $(SCRIPTS_DIR)/game_rot.py
TEST_SRC := $(SCRIPTS_DIR)/test_buf_sort.cpp
TEST_EXE := $(SCRIPTS_DIR)/test_buf_sort

# KiCad Files
CORE_EXTS := .kicad_prl .kicad_pro .kicad_sch .kicad_pcb .kicad_dru
CORE_FILES := $(addprefix $(PROJECT_DIR)/$(PROJECT_NAME), $(CORE_EXTS))
OTHER_FILES := $(PROJECT_DIR)/fp-lib-table $(PROJECT_NAME)/breakout.kicad_sch
KICAD_FILES := $(CORE_FILES) $(OTHER_FILES)

# --- Targets ---
.PHONY: all help lock_pcb unlock_pcb see-imu see-stars clean analyze-valgrind analyze-callgrind analyze-massif analyze-gcov debug-gdb

# Default target just builds the test executable
all: $(TEST_EXE)

help:
	@echo "Available commands:"
	@echo "  make help              - Show this help message"
	@echo "  make                   - Compile test_buf_sort.cpp"
	@echo "  make see-imu           - Run the IMU visualization script (game_rot.py)"
	@echo "  make see-stars         - Run the Star visualization pipeline (Generate data -> Sort -> Visualize)"
	@echo "  make analyze-valgrind  - Run Valgrind Memcheck on test_buf_sort"
	@echo "  make analyze-callgrind - Run Valgrind Callgrind on test_buf_sort"
	@echo "  make analyze-massif    - Run Valgrind Massif on test_buf_sort"
	@echo "  make analyze-gcov      - Run Code Coverage analysis on test_buf_sort"
	@echo "  make debug-gdb         - Run GDB on test_buf_sort"
	@echo "  make lock_pcb          - Lock KiCad files with Git LFS"
	@echo "  make unlock_pcb        - Unlock KiCad files with Git LFS"
	@echo "  make clean             - Remove generated binaries and analysis files"

# --- PCB Management ---
lock_pcb:
	@echo "Requesting Git LFS lock for KiCad project files..."
	@git lfs lock $(KICAD_FILES)
	@echo "---"
	@echo "Lock request complete. You can now edit the PCB project."

unlock_pcb:
	@echo "Unlocking KiCad project files..."
	@git lfs unlock $(KICAD_FILES)
	@echo "---"
	@echo "Unlock request complete."

# --- Verification & Pipeline ---

# Rule to compile the C++ test harness
$(TEST_EXE): $(TEST_SRC)
	@echo "Compiling $(TEST_SRC)..."
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

# Rule to generate stars.bin from FITS (Dependency: FITS file must exist)
$(BIN_FILE): $(SCRIPT_PROCESS_FITS)
	@if [ ! -f "$(FITS_FILE)" ]; then \
		echo "Error: Source file $(FITS_FILE) not found!"; \
		exit 1; \
	fi
	@echo "Generating binary from FITS..."
	$(PYTHON) $(SCRIPT_PROCESS_FITS)

# Rule to run the sorting test and generate CSV (Dependency: stars.bin must exist)
# This runs the compiled C++ executable
$(CSV_FILE): $(TEST_EXE) $(BIN_FILE)
	@echo "Running C++ Sorting Test..."
	# We run it from the script dir so it finds the bin file correctly if coded relatively, 
	# or purely rely on absolute paths. Adjust working dir if needed.
	cd $(SCRIPTS_DIR) && ./test_buf_sort

see-imu:
	@echo "Running IMU Visualization..."
	$(PYTHON) $(SCRIPT_GAME_ROT)

see-stars:
	@if [ -f "$(CSV_FILE)" ]; then \
		echo "CSV verification file exists. Skipping generation."; \
	else \
		echo "CSV missing. Triggering generation pipeline..."; \
		$(MAKE) $(CSV_FILE); \
	fi
	@echo "Launching Star Visualizer..."
	$(PYTHON) $(SCRIPT_SEE_STARS)

# --- Analysis Tools ---

analyze-valgrind: $(TEST_EXE)
	@echo "Running Valgrind Memcheck..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(TEST_EXE)

analyze-callgrind: $(TEST_EXE)
	@echo "Running Valgrind Callgrind..."
	valgrind --tool=callgrind $(TEST_EXE)
	@echo "Callgrind output generated."

analyze-massif: $(TEST_EXE)
	@echo "Running Valgrind Massif..."
	valgrind --tool=massif $(TEST_EXE)
	@echo "Massif output generated. Use 'ms_print massif.out.PID' to view."

analyze-gcov:
	@echo "Compiling with coverage flags..."
	$(CXX) -fprofile-arcs -ftest-coverage -g $(TEST_SRC) -o $(TEST_EXE)_cov $(LDFLAGS)
	@echo "Running executable..."
	$(TEST_EXE)_cov
	@echo "Generating coverage report..."
	gcov -n -o . $(TEST_SRC)

debug-gdb: $(TEST_EXE)
	@echo "Starting GDB..."
	gdb $(TEST_EXE)

clean:
	rm -f $(TEST_EXE) $(TEST_EXE)_cov
	rm -f *.gcno *.gcda *.gcov
	rm -f callgrind.out.* massif.out.*