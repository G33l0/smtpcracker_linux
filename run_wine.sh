#!/bin/bash
# Run the compiled Windows executable in Wine

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}Running SMTP Cracker on Kali Linux${NC}"
echo -e "${GREEN}================================${NC}"

# Check if Wine is installed
if ! command -v wine &> /dev/null; then
    echo -e "${RED}[!] Wine not found. Installing...${NC}"
    sudo apt-get update
    sudo apt-get install -y wine wine32 wine64
fi

# Set Wine prefix
export WINEPREFIX=~/.wine_smtpcracker

# Check if executable exists
if [ ! -f "smtpcracker.exe" ]; then
    echo -e "${RED}[!] smtpcracker.exe not found!${NC}"
    echo -e "${YELLOW}[*] Run build_wine.py first: python3 build_wine.py${NC}"
    exit 1
fi

# Check for required DLLs
echo -e "${YELLOW}[*] Checking dependencies...${NC}"

# Install common Windows DLLs if needed
if [ ! -f "$WINEPREFIX/drive_c/windows/system32/libcurl.dll" ]; then
    echo -e "${YELLOW}[*] Installing libcurl...${NC}"
    wine curl -o /dev/null 2>/dev/null || true
fi

# Run the application
echo -e "${GREEN}[*] Starting SMTP Cracker...${NC}"
echo -e "${YELLOW}[!] Note: This is a Windows application running in Wine${NC}"
echo -e "${YELLOW}[!] Performance may be slower than native Windows${NC}"
echo ""

# Run with Wine
wine smtpcracker.exe "$@"

# Check exit code
if [ $? -eq 0 ]; then
    echo -e "${GREEN}[✓] Application exited normally${NC}"
else
    echo -e "${RED}[✗] Application crashed with error code: $?${NC}"
    echo -e "${YELLOW}[*] Check Wine logs for more information${NC}"
fi
