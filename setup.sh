#!/bin/bash

echo "========================================="
echo "SMTP Tool Setup for Kali Linux"
echo "========================================="

# Update system
echo "[1/4] Updating system packages..."
sudo apt-get update

# Install dependencies
echo "[2/4] Installing dependencies..."
sudo apt-get install -y \
    wine \
    wine32 \
    wine64 \
    wine-binfmt \
    winetricks \
    python3 \
    python3-pip \
    wget \
    unzip \
    p7zip-full \
    curl

# Create directories
echo "[3/4] Creating directories..."
mkdir -p src res include asset obj logs

# Make scripts executable
echo "[4/4] Making scripts executable..."
chmod +x build_linux.py setup.sh

echo ""
echo "========================================="
echo "Setup Complete!"
echo "========================================="
echo ""
echo "Next steps:"
echo "1. Copy your Windows source files:"
echo "   cp -r /path/to/windows/src/* src/"
echo "   cp -r /path/to/windows/res/* res/"
echo "   cp -r /path/to/windows/include/* include/"
echo "   cp -r /path/to/windows/asset/* asset/"
echo ""
echo "2. Build the tool:"
echo "   ./build_linux.py"
echo ""
echo "3. Run the tool:"
echo "   ./run_tool.sh"
echo ""
echo "========================================="
