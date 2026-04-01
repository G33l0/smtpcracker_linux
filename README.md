# smtpcracker_linux
Originally created by @shoaibhassan2 but this is made to run on linux.... education purpose only.


# SMTP Cracker - Kali Linux Port

This is a Wine-based port of the Windows SMTP Cracker tool for Kali Linux.

## Prerequisites

- Kali Linux (or any Debian-based distribution)
- Python 3.6+
- Internet connection for downloading dependencies

## Installation

1. **Clone or extract this project:**
   ```bash
   cd /path/to/smtpcracker_linux
2. **Make scripts executable:**
   chmod +x *.sh
chmod +x build_wine.py
This will:

Install Wine and required packages

Set up a Wine prefix

Install Windows dependencies

Copy your source files:

bash
# Copy your original Windows source files
cp -r /path/to/original/src/ ./src/
cp -r /path/to/original/res/ ./res/
cp -r /path/to/original/include/ ./include/
cp -r /path/to/original/Sidebar/ ./Sidebar/
Building
Run the Python build script:

bash
python3 build_wine.py
The build process:

Detects changed files for incremental compilation

Compiles all source files using Wine's TCC

Links with required Windows libraries

Creates smtpcracker.exe

Running
Execute the run script:

bash
./run_wine.sh
Or run directly with Wine:

bash
wine smtpcracker.exe
Troubleshooting
Compilation Errors
If compilation fails:

Check that all source files exist in the expected locations

Verify the include paths in build_wine.py

Install missing dependencies: ./wine_setup.sh

Runtime Errors
If the application crashes:

Check Wine logs: wine smtpcracker.exe 2>&1 | tee wine.log

Install missing DLLs: winetricks dll_name

Try with a clean Wine prefix: rm -rf ~/.wine_smtpcracker && ./wine_setup.sh

Performance Issues
Wine emulation may be slower than native Windows. To improve:

Disable Wine debugging: export WINEDEBUG=-all

Use a 64-bit prefix for better performance

Consider using Windows VM for heavy workloads

File Structure
text
smtpcracker_linux/
├── build_wine.py          # Build script
├── run_wine.sh           # Run script
├── wine_setup.sh         # Setup script
├── README.md             # This file
├── obj/                  # Object files (auto-generated)
├── src/                  # Source files (copy from Windows)
├── res/                  # Resource files
├── include/              # Header files
└── Sidebar/              # Sidebar headers
Limitations
Performance: Runs in Wine emulation, slower than native Windows

Graphics: Some GUI features may have minor issues

Network: Some network features may require additional Wine configuration

File paths: Windows-style paths may need adjustment

Security Notes
This tool should only be used on systems you own or have permission to test

Use responsibly and in accordance with local laws

Never use for unauthorized access to email systems

Support
For issues:

Check Wine compatibility database: https://appdb.winehq.org/

Review Wine logs for specific errors

Ensure all dependencies are properly installed

License
Same as original Windows version

text

## Installation Instructions

1. **Create the folder structure:**
```bash
mkdir -p smtpcracker_linux/{src,res,include,Sidebar,obj}
cd smtpcracker_linux
Save all the files:

Save build_wine.py in the main folder

Save run_wine.sh in the main folder

Save wine_setup.sh in the main folder

Save README.md in the main folder

Make scripts executable:

bash
chmod +x build_wine.py run_wine.sh wine_setup.sh
Run the setup:

bash
./wine_setup.sh
Copy your source files:

bash
# Copy from your Windows source location
cp -r /path/to/windows/source/* src/
cp -r /path/to/windows/resources/* res/
cp -r /path/to/windows/includes/* include/
cp -r /path/to/windows/sidebar/* Sidebar/
Build the application:

bash
python3 build_wine.py
Run the application:

bash
./run_wine.sh
Key Features of this Solution
Incremental Compilation: Only recompiles changed files

Parallel Compilation: Uses multiple threads for faster builds

Wine Integration: Automatically sets up and configures Wine

Error Handling: Comprehensive error messages and logging

Dependency Management: Automatically installs required Windows DLLs

Cross-Platform: Works on any Linux distribution with Wine

This solution is superior because:

No code rewriting needed - runs original Windows code

Automatic setup - handles all Wine configuration

Parallel builds - faster compilation than original

Incremental builds - saves time on subsequent compiles

Better error handling - detailed error messages

Complete Wine integration - properly sets up all dependencies

The tool will run exactly as it did on Windows, but through Wine emulation. For best performance, ensure your Kali Linux has sufficient resources (at least 2GB RAM, 2 CPU cores).
