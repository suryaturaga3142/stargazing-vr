# --- Makefile for KiCad Git LFS Locking ---
#
# Use 'make lock_pcb' to lock files for editing.
# Use 'make unlock_pcb' to unlock files when you are done.

# --- Configuration ---
# The directory where your KiCad project is.
PROJECT_DIR := pcb-stargazing
# The base name of your project files.
PROJECT_NAME := pcb-stargazing

# --- File Lists ---
# Create the full paths for the core project files
CORE_EXTS := .kicad_prl .kicad_pro .kicad_sch .kicad_pcb .kicad_dru
CORE_FILES := $(addprefix $(PROJECT_DIR)/$(PROJECT_NAME), $(CORE_EXTS))

# Add other critical files that are often modified
OTHER_FILES := $(PROJECT_DIR)/fp-lib-table $(PROJECT_NAME)/breakout.kicad_sch

# Combine all files into one list
KICAD_FILES := $(CORE_FILES) $(OTHER_FILES)

# --- Targets ---

# This tells 'make' that these are commands, not files to be built.
.PHONY: lock_pcb unlock_pcb

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