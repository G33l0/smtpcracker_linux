#!/bin/bash

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${GREEN}╔════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║     SMTP Tool for Kali Linux          ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════╝${NC}"

# Set Wine prefix
export WINEPREFIX=~/.wine_smtptool
export WINEDEBUG=-all

# Check if executable exists
if [ ! -f "smtptool.exe" ]; then
    echo -e "${RED}[!] Error: smtptool.exe not found!${NC}"
    echo -e "${YELLOW}[*] Please build first: ./build_linux.py${NC}"
    exit 1
fi

# Check if asset folder exists (your asset folder)
if [ ! -d "asset" ]; then
    echo -e "${YELLOW}[!] Warning: asset folder not found${NC}"
    echo -e "${YELLOW}[*] Some features may not work properly${NC}"
fi

# Run the application
echo -e "${GREEN}[*] Starting SMTP Tool...${NC}"
echo -e "${BLUE}[*] Press Ctrl+C to exit${NC}"
echo ""

# Run with Wine, passing all arguments
wine smtptool.exe "$@"

# Check exit code
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "\n${GREEN}[✓] Application closed successfully${NC}"
else
    echo -e "\n${RED}[✗] Application exited with error code: $EXIT_CODE${NC}"
    echo -e "${YELLOW}[*] Check Wine logs for details: wine smtptool.exe 2>&1 | tee error.log${NC}"
fi

echo ""
