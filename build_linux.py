#!/usr/bin/env python3
"""
Linux Wine-based build script for Windows SMTP Tool
Handles actual folder structure (asset folder instead of Sidebar)
"""
import os
import sys
import subprocess
import shutil
import hashlib
import json
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed
import time

class LinuxWineBuilder:
    def __init__(self):
        self.project_root = os.getcwd()
        self.wine_prefix = os.path.expanduser("~/.wine_smtptool")
        
        # Build directories
        self.obj_dir = "obj"
        self.src_dir = "src"
        self.res_dir = "res"
        self.include_dir = "include"
        self.asset_dir = "asset"  # Your asset folder
        self.exe_name = "smtptool.exe"
        self.hash_file = os.path.join(self.obj_dir, "file_hashes.json")
        self.build_log = os.path.join("logs", f"build_{time.strftime('%Y%m%d_%H%M%S')}.log")
        
        # Compiler flags (matching your original)
        self.cflags = [
            "-I" + self.include_dir,
            "-I" + self.asset_dir,  # Include asset folder for headers if any
            "-municode",
            "-mconsole",
            "-s",
            "-DUNICODE",
            "-D_UNICODE"
        ]
        
        # Libraries
        self.libs = [
            "-lcomctl32", "-ldwmapi", "-lws2_32",
            "-lCabinet", "-loleacc", "-loleaut32", "-lcrypto",
            "-lgdi32", "-lshlwapi", "-lcomdlg32", "-lmsimg32",
            "-lwinmm", "-lgdiplus", "-lshell32", "-lDbghelp",
            "-lcurl", "-lversion", "-ladvapi32", "-lcredui",
            "-lole32", "-liphlpapi", "-ld2d1", "-lkernel32",
            "-ldnsapi", "-lssl", "-lcurl"
        ]
        
        # Source files - AUTO DISCOVER from your src folder
        self.src_files = self.discover_source_files()
        
        # Resource file
        self.res_file = os.path.join(self.res_dir, "resource.rc") if os.path.exists(os.path.join(self.res_dir, "resource.rc")) else None
        
    def discover_source_files(self):
        """Automatically discover all .c files in src directory"""
        src_files = []
        if os.path.exists(self.src_dir):
            for root, dirs, files in os.walk(self.src_dir):
                for file in files:
                    if file.endswith('.c'):
                        rel_path = os.path.relpath(os.path.join(root, file), self.project_root)
                        src_files.append(rel_path)
        
        # Also check for .c files in root
        for file in os.listdir('.'):
            if file.endswith('.c'):
                src_files.append(file)
        
        if not src_files:
            print("[ERROR] No source files found! Please copy your source files to src/ directory")
            sys.exit(1)
        
        print(f"[INFO] Found {len(src_files)} source files")
        return sorted(src_files)
    
    def setup_wine_env(self):
        """Setup Wine environment"""
        print("\n[*] Setting up Wine environment...")
        
        # Check Wine installation
        if not shutil.which("wine"):
            print("[!] Wine not found. Installing...")
            subprocess.run(["sudo", "apt-get", "update"], check=True)
            subprocess.run(["sudo", "apt-get", "install", "-y", "wine", "wine32", "wine64", "wine-binfmt"], check=True)
        
        # Set Wine prefix
        os.environ["WINEPREFIX"] = self.wine_prefix
        
        # Initialize Wine if needed
        if not os.path.exists(self.wine_prefix):
            print("[*] Initializing Wine prefix (this may take a minute)...")
            subprocess.run(["wineboot", "-u"], capture_output=True, timeout=60)
        
        # Install TCC compiler
        self.install_tcc()
        
        # Install required DLLs
        self.install_windows_dlls()
        
        print("[+] Wine environment ready!")
    
    def install_tcc(self):
        """Install Tiny C Compiler in Wine"""
        tcc_path = os.path.join(self.wine_prefix, "drive_c", "tcc")
        
        if os.path.exists(tcc_path):
            return
        
        print("[*] Installing Tiny C Compiler...")
        
        # Download TCC Windows binary
        tcc_url = "https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27-win64-bin.zip"
        tcc_zip = "/tmp/tcc.zip"
        
        subprocess.run(["wget", tcc_url, "-O", tcc_zip, "-q"], check=True)
        
        # Extract and copy to Wine
        temp_dir = "/tmp/tcc_temp"
        if os.path.exists(temp_dir):
            shutil.rmtree(temp_dir)
        
        subprocess.run(["unzip", "-q", tcc_zip, "-d", temp_dir], check=True)
        
        # Find the actual tcc.exe
        for root, dirs, files in os.walk(temp_dir):
            if "tcc.exe" in files:
                source_path = root
                break
        else:
            source_path = temp_dir
        
        # Copy to Wine
        shutil.copytree(source_path, tcc_path, dirs_exist_ok=True)
        
        # Cleanup
        shutil.rmtree(temp_dir, ignore_errors=True)
        os.remove(tcc_zip)
        
        print("[+] TCC installed")
    
    def install_windows_dlls(self):
        """Install required Windows DLLs using winetricks"""
        # Check if winetricks is installed
        if not shutil.which("winetricks"):
            print("[*] Installing winetricks...")
            subprocess.run(["sudo", "apt-get", "install", "-y", "winetricks"], check=True)
        
        # Set environment for winetricks
        env = os.environ.copy()
        env["WINEPREFIX"] = self.wine_prefix
        
        # Install required components (quiet mode)
        required_dlls = ["vcrun2019", "gdiplus", "comctl32"]
        
        for dll in required_dlls:
            print(f"[*] Installing {dll}...")
            try:
                subprocess.run(["winetricks", "-q", dll], env=env, capture_output=True, timeout=120)
            except:
                print(f"[!] Warning: Could not install {dll}, continuing anyway")
    
    def file_hash(self, filepath):
        """Compute SHA256 hash of file"""
        if not os.path.exists(filepath):
            return None
        sha256 = hashlib.sha256()
        with open(filepath, "rb") as f:
            for chunk in iter(lambda: f.read(8192), b""):
                sha256.update(chunk)
        return sha256.hexdigest()
    
    def load_cache(self):
        """Load build cache"""
        if os.path.exists(self.hash_file):
            with open(self.hash_file, 'r') as f:
                return json.load(f)
        return {}
    
    def save_cache(self, cache):
        """Save build cache"""
        os.makedirs(self.obj_dir, exist_ok=True)
        with open(self.hash_file, 'w') as f:
            json.dump(cache, f, indent=2)
    
    def compile_source(self, src_file, old_hashes):
        """Compile a single source file"""
        obj_file = os.path.join(self.obj_dir, Path(src_file).stem + ".o")
        
        # Check if recompilation is needed
        current_hash = self.file_hash(src_file)
        if old_hashes.get(src_file) == current_hash and os.path.exists(obj_file):
            print(f"  [SKIP] {src_file}")
            return obj_file, current_hash
        
        # Convert paths for Wine
        wine_src = f"Z:{os.path.abspath(src_file)}".replace('\\', '/')
        wine_obj = f"Z:{os.path.abspath(obj_file)}".replace('\\', '/')
        
        # Build command
        cmd = [
            "wine", f"{self.wine_prefix}/drive_c/tcc/tcc.exe",
            "-c", wine_src,
            "-o", wine_obj
        ] + self.cflags
        
        print(f"  [COMPILE] {src_file}")
        
        try:
            # Create obj directory if needed
            os.makedirs(os.path.dirname(obj_file) if os.path.dirname(obj_file) else self.obj_dir, exist_ok=True)
            
            # Run compilation
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            
            if result.returncode != 0:
                print(f"  [ERROR] Failed to compile {src_file}")
                if result.stderr:
                    print(f"    {result.stderr[:200]}")
                raise subprocess.CalledProcessError(result.returncode, cmd)
            
            return obj_file, current_hash
            
        except subprocess.TimeoutExpired:
            print(f"  [ERROR] Compilation timeout for {src_file}")
            raise
        except Exception as e:
            print(f"  [ERROR] {e}")
            raise
    
    def compile_resources(self):
        """Compile Windows resources if resource.rc exists"""
        if not self.res_file or not os.path.exists(self.res_file):
            print("[INFO] No resource file found, skipping")
            return None
        
        res_obj = os.path.join(self.res_dir, "resource.o")
        wine_res_src = f"Z:{os.path.abspath(self.res_file)}".replace('\\', '/')
        wine_res_obj = f"Z:{os.path.abspath(res_obj)}".replace('\\', '/')
        
        # Try to find windres in Wine
        windres_paths = [
            f"{self.wine_prefix}/drive_c/mingw32/bin/windres.exe",
            f"{self.wine_prefix}/drive_c/MinGW/bin/windres.exe",
            "windres"  # Try system windres if available
        ]
        
        windres_cmd = None
        for path in windres_paths:
            if os.path.exists(path) and "wine" not in path:
                windres_cmd = ["wine", path]
                break
            elif shutil.which(path):
                windres_cmd = [path]
                break
        
        if not windres_cmd:
            print("[WARNING] windres not found, resources will be skipped")
            return None
        
        cmd = windres_cmd + [
            wine_res_src,
            "--input-format=rc",
            "--codepage=65001",
            "-O", "coff",
            "-o", wine_res_obj
        ]
        
        print("[RESOURCE] Compiling resources...")
        try:
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                print(f"[WARNING] Resource compilation failed: {result.stderr[:100]}")
                return None
            return res_obj
        except Exception as e:
            print(f"[WARNING] Could not compile resources: {e}")
            return None
    
    def link_objects(self, objects):
        """Link all object files"""
        if not objects:
            print("[ERROR] No object files to link")
            return False
        
        # Convert paths for Wine
        wine_objects = [f"Z:{os.path.abspath(obj)}".replace('\\', '/') for obj in objects if obj]
        wine_exe = f"Z:{os.path.abspath(self.exe_name)}".replace('\\', '/')
        
        cmd = [
            "wine", f"{self.wine_prefix}/drive_c/tcc/tcc.exe",
            "-o", wine_exe
        ] + wine_objects + self.cflags + self.libs
        
        print(f"\n[LINK] Creating {self.exe_name}...")
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
            if result.returncode != 0:
                print("[ERROR] Linking failed")
                if result.stderr:
                    print(result.stderr)
                return False
            
            # Make executable
            if os.path.exists(self.exe_name):
                os.chmod(self.exe_name, 0o755)
                print(f"[SUCCESS] Created {self.exe_name}")
                return True
            else:
                print("[ERROR] Executable not created")
                return False
                
        except subprocess.TimeoutExpired:
            print("[ERROR] Linking timeout")
            return False
        except Exception as e:
            print(f"[ERROR] {e}")
            return False
    
    def create_run_script(self):
        """Create a run script for easy execution"""
        run_script = """#!/bin/bash
# Run script for SMTP Tool on Kali Linux

# Colors
GREEN='\\033[0;32m'
YELLOW='\\033[1;33m'
RED='\\033[0;31m'
NC='\\033[0m'

echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}SMTP Tool for Kali Linux${NC}"
echo -e "${GREEN}================================${NC}"

# Set Wine prefix
export WINEPREFIX=~/.wine_smtptool

# Check if executable exists
if [ ! -f "smtptool.exe" ]; then
    echo -e "${RED}[!] smtptool.exe not found!${NC}"
    echo -e "${YELLOW}[*] Run: python3 build_linux.py${NC}"
    exit 1
fi

# Disable Wine debug output for cleaner display
export WINEDEBUG=-all

# Run the application
echo -e "${GREEN}[*] Starting SMTP Tool...${NC}"
wine smtptool.exe "$@"

# Check exit code
if [ $? -eq 0 ]; then
    echo -e "${GREEN}[✓] Application exited normally${NC}"
else
    echo -e "${RED}[✗] Application crashed${NC}"
fi
"""
        
        with open("run_tool.sh", "w") as f:
            f.write(run_script)
        os.chmod("run_tool.sh", 0o755)
    
    def build(self):
        """Main build process"""
        print("=" * 60)
        print("SMTP Tool Builder for Kali Linux")
        print("=" * 60)
        
        # Create directories
        os.makedirs(self.obj_dir, exist_ok=True)
        os.makedirs("logs", exist_ok=True)
        
        # Setup Wine
        self.setup_wine_env()
        
        # Check source files
        if not self.src_files:
            print("\n[ERROR] No source files found!")
            print("[INFO] Please copy your source files to the 'src/' directory")
            return False
        
        print(f"\n[INFO] Found {len(self.src_files)} source files to compile")
        
        # Load build cache
        old_hashes = self.load_cache()
        new_hashes = {}
        
        # Compile resources
        res_obj = self.compile_resources()
        
        # Compile sources in parallel
        obj_files = []
        print("\n[COMPILING] Source files...")
        
        with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as executor:
            futures = {executor.submit(self.compile_source, src, old_hashes): src for src in self.src_files}
            
            for future in as_completed(futures):
                try:
                    obj_file, file_hash = future.result()
                    if obj_file:
                        obj_files.append(obj_file)
                        new_hashes[futures[future]] = file_hash
                except Exception as e:
                    print(f"[ERROR] Compilation failed: {e}")
                    return False
        
        # Add resource object if compiled
        if res_obj and os.path.exists(res_obj):
            obj_files.append(res_obj)
        
        # Link objects
        if not self.link_objects(obj_files):
            return False
        
        # Save cache
        self.save_cache(new_hashes)
        
        # Create run script
        self.create_run_script()
        
        # Print success message
        print("\n" + "=" * 60)
        print("[SUCCESS] Build completed successfully!")
        print("=" * 60)
        print(f"[INFO] Executable: {self.exe_name}")
        print(f"[INFO] Run with: ./run_tool.sh")
        print(f"[INFO] Or directly: wine {self.exe_name}")
        print("\n[NOTE] Asset folder should be in the same directory as the executable")
        print("=" * 60)
        
        return True

def main():
    builder = LinuxWineBuilder()
    success = builder.build()
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
