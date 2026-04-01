#!/bin/bash
# Setup script for Wine environment on Kali Linux

set -e

echo "========================================="
echo "Setting up Wine for SMTP Cracker on Kali"
echo "========================================="

# Update system
echo "[*] Updating package list..."
sudo apt-get update

# Install required packages
echo "[*] Installing required packages..."
sudo apt-get install -y \
    wine \
    wine32 \
    wine64 \
    wine-binfmt \
    winbind \
    cabextract \
    p7zip-full \
    wget \
    curl \
    python3 \
    python3-pip \
    build-essential

# Install Winetricks for easier DLL management
echo "[*] Installing Winetricks..."
sudo apt-get install -y winetricks

# Set Wine prefix
export WINEPREFIX=~/.wine_smtpcracker
export WINEARCH=win64

# Initialize Wine prefix if not exists
if [ ! -d "$WINEPREFIX" ]; then
    echo "[*] Creating Wine prefix (this may take a while)..."
    wineboot -u 2>/dev/null || true
    sleep 5
fi

# Install required Windows DLLs and components
echo "[*] Installing Windows components with Winetricks..."
winetricks -q \
    vcrun2019 \
    dotnet48 \
    corefonts \
    gdiplus \
    comctl32 \
    comctl32.ocx

# Install libcurl and OpenSSL for Wine
echo "[*] Installing libcurl and OpenSSL..."
wine curl -o /dev/null 2>/dev/null || true

# Verify installation
echo "[*] Verifying Wine installation..."
wine --version

echo ""
echo "========================================="
echo "[✓] Setup complete!"
echo "[*] Next steps:"
echo "    1. Copy your source files to the src/ directory"
echo "    2. Run: python3 build_wine.py"
echo "    3. Run: ./run_wine.sh"
echo "========================================="
