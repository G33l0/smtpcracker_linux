# smtpcracker_linux
Originally created by @shoaibhassan2 but this is made to run on linux.... education purpose only.

# SMTP Tool for Kali Linux
A powerful multi-threaded SMTP checker with graphical user interface, ported to run seamlessly on Kali Linux using Wine.

## Features
- **Multi-Threaded**: Checks multiple SMTP accounts concurrently for maximum performance
- **User-Friendly GUI**: Easy-to-use graphical interface built with Win32 API, now running on Kali Linux
- **Dark Mode**: Modern dark theme support for comfortable extended use
- **SMTP Server Discovery**: Automatically discovers SMTP server settings (host, port, encryption) for various email providers
- **Intelligent Caching**: Caches discovered SMTP server configurations to significantly speed up subsequent checks
- **Retry Logic**: Implements exponential backoff for connection retries to handle temporary failures
- **Dual Notification System**: Sends notifications for valid credentials via Telegram and email
- **Results Management**: Saves validated accounts to a file with timestamp and statistics
- **Customizable Settings**: Configurable threads, retry attempts, timeout values, and notification settings
- **Kali Linux Optimized**: Specially adapted to run smoothly on Kali Linux using Wine emulation

## Kali Linux Installation & Setup

### Prerequisites
- Kali Linux (2020.1 or newer)
- Python 3.6 or higher
- Wine 5.0 or higher
- Active internet connection for downloading dependencies

### Quick Installation

1. **Clone the repository:**
```bash
git clone https://github.com/shoaibhassan2/SMTPCracker
cd SMTPCracker

Run the automated setup script:

bash
chmod +x setup.sh
./setup.sh
This will automatically:

Install Wine and required dependencies

Set up a Wine prefix environment

Install the Tiny C Compiler (TCC) in Wine

Configure required Windows DLLs

Copy your source files (if you have existing Windows version):

bash
# Copy from your Windows source directory
cp -r /path/to/windows/SMTPCracker/src/* src/
cp -r /path/to/windows/SMTPCracker/res/* res/
cp -r /path/to/windows/SMTPCracker/include/* include/
cp -r /path/to/windows/SMTPCracker/asset/* asset/
Building from Source on Kali Linux
Run the Linux build script:

bash
python3 build_linux.py
The build process:

Automatically discovers all source files in the src/ directory

Compiles only changed files (incremental compilation)

Uses multiple CPU cores for parallel compilation

Creates smtptool.exe in the project root directory

Generates a run_tool.sh launcher script

Verify the build:

bash
ls -la smtptool.exe
# Should show the executable file
Running on Kali Linux
Using the launcher script (recommended):

bash
./run_tool.sh
Direct Wine execution:

bash
wine smtptool.exe
With custom arguments:

bash
./run_tool.sh --debug --threads 50
Usage Guide
Basic Workflow
Launch the application:

bash
./run_tool.sh
Load account list:

Click the "Upload Combo" button

Select a text file with credentials in format: email:password (one per line)

Example: user@gmail.com:password123

Configure settings (optional):

Navigate to the "Settings" tab

Adjust:

Thread Count: Number of concurrent checks (default: 20)

Timeout: Connection timeout in seconds (default: 10)

Retry Attempts: Number of retry attempts on failure (default: 3)

Retry Delay: Base delay between retries (default: 1 second)

Results File: Output file for valid accounts (default: results.txt)

Start checking:

Click "Start Checking" button

Monitor progress in real-time

Results appear in the main window as they're found

Save results:

Valid accounts are automatically saved to the specified results file

Results include timestamp, SMTP server, and status

Advanced Features
SMTP Server Discovery
The tool automatically discovers SMTP settings for common providers:

Gmail: smtp.gmail.com:587 (TLS)

Outlook/Hotmail: smtp-mail.outlook.com:587 (TLS)

Yahoo: smtp.mail.yahoo.com:587 (TLS)

Custom domains: Auto-detection via MX records

Notification Setup
Telegram Notifications:

Go to Settings → Telegram

Enter your Bot Token

Enter your Chat ID

Test connection with "Test Telegram" button

Email Notifications:

Go to Settings → Email

Configure SMTP settings

Set recipient email address

Test with "Test Email" button

Caching System
Discovered SMTP servers are cached in ~/.wine_smtptool/drive_c/users/kali/Application Data/SMTPTool/cache.json

Clear cache in Settings to force fresh discovery

Kali Linux Specific Configuration
Wine Performance Optimization
Add these to your .bashrc for better performance:

bash
# Wine optimizations for SMTP tool
export WINEPREFIX=~/.wine_smtptool
export WINEDEBUG=-all  # Disable debug output
export STAGING_SHARED_MEMORY=1  # Enable shared memory (Wine Staging)
Custom Wine Prefix Management
bash
# Backup working prefix
tar -czf wine_backup.tar.gz ~/.wine_smtptool

# Reset prefix if needed
rm -rf ~/.wine_smtptool
./setup.sh  # Recreate prefix
Running with Different Wine Versions
bash
# Install Wine Staging for better performance
sudo apt install wine-staging

# Run with specific Wine version
wine-staging smtptool.exe
Troubleshooting
Common Issues and Solutions
1. Compilation Errors
bash
# Check source files
find src/ -name "*.c"

# Verify include paths
grep -r "#include" src/ | head

# Run build with verbose output
python3 build_linux.py 2>&1 | tee build.log
2. Runtime Crashes
bash
# Run with debug output
export WINEDEBUG=+all
./run_tool.sh 2>&1 | tee wine_debug.log

# Check for missing DLLs
wine ./smtptool.exe 2>&1 | grep -i "dll"
3. Network Connection Issues
bash
# Test SMTP connectivity
telnet smtp.gmail.com 587

# Check Wine network configuration
winecfg  # Check network settings tab
4. Performance Problems
bash
# Monitor resource usage
htop  # Check CPU/RAM usage

# Reduce thread count in settings
# Disable animations in Windows settings
wine control  # System → Advanced → Performance
Dependency Management
Install additional Windows components if needed:

bash
# Install specific DLLs
winetricks vcrun2019
winetricks gdiplus
winetricks comctl32

# List installed components
winetricks list-installed
File Structure
text
SMTPCracker/
├── build_linux.py          # Linux/Wine build script
├── run_tool.sh            # Launcher script
├── setup.sh               # Initial setup script
├── smtptool.exe           # Compiled executable
├── src/                   # Source files
│   ├── main.c
│   ├── main_proc.c
│   ├── utils/
│   ├── theme/
│   ├── ui/
│   ├── core/
│   └── smtplib/
├── include/               # Header files
├── res/                   # Resources (icons, manifest)
├── asset/                 # Assets (images, themes)
├── obj/                   # Object files (auto-generated)
├── logs/                  # Build and runtime logs
└── results.txt            # Valid accounts (generated)
Performance Benchmarks
On a typical Kali Linux machine (4 cores, 8GB RAM):

Compilation time: 15-30 seconds (first build), 2-5 seconds (incremental)

Check speed: 50-100 accounts per second (depending on network)

Memory usage: 50-100 MB RAM

CPU usage: Variable based on thread count

Security Considerations
Credentials: Never share your combo files or results

Network: Use VPN if required by your testing environment

Rate Limiting: Respect email provider rate limits

Legal: Only test accounts you own or have explicit permission to test

Uninstallation
bash
# Remove the tool
cd ~
rm -rf ~/SMTPCracker

# Remove Wine prefix (optional)
rm -rf ~/.wine_smtptool

# Remove Wine (if no longer needed)
sudo apt remove wine wine32 wine64
sudo apt autoremove
Support & Resources
WineHQ Database: https://appdb.winehq.org/

SMTP Protocol: RFC 5321

Issue Tracker: GitHub Issues

License
This project is licensed under the MIT License - see the LICENSE file for details.

Note: This tool runs Windows binaries on Kali Linux via Wine. While functional, performance may vary compared to native Windows execution. For production use, consider running on Windows or using the native Linux version if available.
