#!/usr/bin/env python3
"""
Linux Wine-based build script for Windows SMTP Tool
Optimized for Kali Linux
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
import platform

class LinuxWineBuilder:
    def __init__(self):
        self.project_root = os.getcwd()
        self.wine_prefix = os.path.expanduser("~/.wine_smtptool")
        
        # Build directories
        self.obj_dir = "obj"
        self.src_dir = "src"
        self.res_dir = "res"
        self.include_dir = "include"
        self.asset_dir = "asset"
        self.exe_name = "smtptool.exe"
        self.hash_file = os.path.join(self.obj_dir, "file_hashes.json")
        self.build_log = os.path.join("logs", f"build_{time.strftime('%Y%m%d_%H%M%S')}.log")
        
        # Compiler flags
        self.cflags = [
            "-I" + self.include_dir,
            "-I" + self.asset_dir,
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
        
        # Source files
        self.src_files = self.discover_source_files()
        self.res_file = os.path.join(self.res_dir, "resource.rc") if os.path.exists(os.path.join(self.res_dir, "resource.rc")) else None
        
    def discover_source_files(self):
        """Automatically discover all .c files"""
        src_files = []
        if os.path.exists(self.src_dir):
            for root, dirs, files in os.walk(self.src_dir):
                for file in files:
                    if file.endswith('.c'):
                        rel_path = os.path.relpath(os.path.join(root, file), self.project_root)
                        src_files.append(rel_path)
        
        # Also check root directory for .c files
        for file in os.listdir('.'):
            if file.endswith('.c') and os.path.isfile(file):
                src_files.append(file)
        
        if not src_files:
            print("[ERROR] No source files found in src/ directory!")
            print("[INFO] Please copy your .c files to the src/ folder")
            sys.exit(1)
        
        print(f"[INFO] Found {len(src_files)} source files")
        for src in src_files[:5]:  # Show first 5
            print(f"  - {src}")
        if len(src_files) > 5:
            print(f"  ... and {len(src_files) - 5} more")
        return sorted(src_files)
    
    def check_and_install_wine(self):
        """Check and install Wine for Kali Linux"""
        print("\n[*] Checking Wine installation...")
        
        # Check if wine is installed
        if shutil.which("wine"):
            wine_version = subprocess.run(["wine", "--version"], capture_output=True, text=True).stdout.strip()
            print(f"[+] Wine found: {wine_version}")
            return True
        
        print("[!] Wine not found. Installing for Kali Linux...")
        
        # Enable 32-bit architecture if not already enabled
        try:
            subprocess.run(["dpkg", "--print-foreign-architectures"], capture_output=True, text=True)
            result = subprocess.run(["dpkg", "--print-foreign-architectures"], capture_output=True, text=True)
            if "i386" not in result.stdout:
                print("[*] Enabling 32-bit architecture...")
                subprocess.run(["sudo", "dpkg", "--add-architecture", "i386"], check=True)
        except:
            pass
        
        # Update package list
        print("[*] Updating package list...")
        subprocess.run(["sudo", "apt-get", "update"], check=True)
        
        # Install Wine on Kali
        print("[*] Installing Wine packages...")
        try:
            # Try installing wine with recommended packages
            subprocess.run(["sudo", "apt-get", "install", "-y", "wine", "wine64"], check=True)
            
            # Try installing 32-bit support separately
            subprocess.run(["sudo", "apt-get", "install", "-y", "wine32"], check=False)
            
            # Install winetricks
            subprocess.run(["sudo", "apt-get", "install", "-y", "winetricks"], check=False)
            
        except subprocess.CalledProcessError as e:
            print(f"[!] Warning: Some Wine packages failed to install: {e}")
            print("[*] Trying alternative installation...")
            
            # Alternative: Install wine from Kali repositories
            subprocess.run(["sudo", "apt-get", "install", "-y", "wine-development", "wine32-development"], check=False)
        
        # Verify installation
        if shutil.which("wine"):
            print("[+] Wine installed successfully!")
            return True
        else:
            print("[ERROR] Wine installation failed!")
            print("[*] Please install Wine manually: sudo apt-get install wine wine32 wine64")
            return False
    
    def setup_wine_env(self):
        """Setup Wine environment"""
        # First check/install Wine
        if not self.check_and_install_wine():
            print("[ERROR] Cannot proceed without Wine")
            sys.exit(1)
        
        # Set Wine prefix
        os.environ["WINEPREFIX"] = self.wine_prefix
        
        # Initialize Wine prefix if needed
        if not os.path.exists(self.wine_prefix):
            print("[*] Initializing Wine prefix (this may take a few minutes)...")
            try:
                # Run wineboot to initialize
                subprocess.run(["wineboot", "-u"], capture_output=True, timeout=120)
                # Wait a bit for initialization
                time.sleep(3)
            except subprocess.TimeoutExpired:
                print("[!] Wine initialization timeout, but continuing...")
            except Exception as e:
                print(f"[!] Warning: {e}")
        
        # Install TCC compiler
        self.install_tcc()
        
        print("[+] Wine environment ready!")
    
    def install_tcc(self):
        """Install Tiny C Compiler in Wine"""
        tcc_path = os.path.join(self.wine_prefix, "drive_c", "tcc")
        
        if os.path.exists(tcc_path) and os.path.exists(os.path.join(tcc_path, "tcc.exe")):
            print("[+] TCC already installed")
            return
        
        print("[*] Installing Tiny C Compiler...")
        
        # Download TCC Windows binary
        tcc_url = "https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27-win64-bin.zip"
        tcc_zip = "/tmp/tcc.zip"
        
        try:
            # Download with wget
            subprocess.run(["wget", tcc_url, "-O", tcc_zip, "-q", "--show-progress"], check=True)
            
            # Extract
            temp_dir = "/tmp/tcc_temp"
            if os.path.exists(temp_dir):
                shutil.rmtree(temp_dir)
            
            subprocess.run(["unzip", "-q", tcc_zip, "-d", temp_dir], check=True)
            
            # Find tcc.exe
            tcc_source = None
            for root, dirs, files in os.walk(temp_dir):
                if "tcc.exe" in files:
                    tcc_source = root
                    break
            
            if tcc_source:
                # Copy to Wine
                os.makedirs(tcc_path, exist_ok=True)
                for item in os.listdir(tcc_source):
                    s = os.path.join(tcc_source, item)
                    d = os.path.join(tcc_path, item)
                    if os.path.isdir(s):
                        if os.path.exists(d):
                            shutil.rmtree(d)
                        shutil.copytree(s, d)
                    else:
                        shutil.copy2(s, d)
                print("[+] TCC installed successfully")
            else:
                print("[!] Could not find tcc.exe in extracted files")
            
            # Cleanup
            shutil.rmtree(temp_dir, ignore_errors=True)
            os.remove(tcc_zip)
            
        except Exception as e:
            print(f"[!] Failed to install TCC: {e}")
            print("[*] You may need to install TCC manually in Wine")
    
    def file_hash(self, filepath):
        """Compute SHA256 hash of file"""
        if not os.path.exists(filepath):
            return None
        sha256 = hashlib.sha256()
        try:
            with open(filepath, "rb") as f:
                for chunk in iter(lambda: f.read(8192), b""):
                    sha256.update(chunk)
            return sha256.hexdigest()
        except:
            return None
    
    def load_cache(self):
        """Load build cache"""
        if os.path.exists(self.hash_file):
            try:
                with open(self.hash_file, 'r') as f:
                    return json.load(f)
            except:
                return {}
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
        if current_hash and old_hashes.get(src_file) == current_hash and os.path.exists(obj_file):
            print(f"  [SKIP] {src_file}")
            return obj_file, current_hash
        
        # Convert paths for Wine
        abs_src = os.path.abspath(src_file)
        abs_obj = os.path.abspath(obj_file)
        
        wine_src = f"Z:{abs_src}".replace('\\', '/')
        wine_obj = f"Z:{abs_obj}".replace('\\', '/')
        
        # Build command
        tcc_path = os.path.join(self.wine_prefix, "drive_c", "tcc", "tcc.exe")
        if not os.path.exists(tcc_path):
            print(f"  [ERROR] TCC not found at {tcc_path}")
            return None, None
        
        cmd = [
            "wine", tcc_path,
            "-c", wine_src,
            "-o", wine_obj
        ] + self.cflags
        
        print(f"  [COMPILE] {src_file}")
        
        try:
            # Create obj directory
            os.makedirs(os.path.dirname(obj_file) if os.path.dirname(obj_file) else self.obj_dir, exist_ok=True)
            
            # Run compilation
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            
            if result.returncode != 0:
                print(f"  [ERROR] Failed to compile {src_file}")
                if result.stderr:
                    # Show first 200 chars of error
                    error_msg = result.stderr[:500]
                    print(f"    {error_msg}")
                return None, None
            
            return obj_file, current_hash
            
        except subprocess.TimeoutExpired:
            print(f"  [ERROR] Compilation timeout for {src_file}")
            return None, None
        except Exception as e:
            print(f"  [ERROR] {e}")
            return None, None
    
    def compile_resources(self):
        """Compile Windows resources"""
        if not self.res_file or not os.path.exists(self.res_file):
            return None
        
        res_obj = os.path.join(self.res_dir, "resource.o")
        abs_res = os.path.abspath(self.res_file)
        abs_obj = os.path.abspath(res_obj)
        
        wine_res_src = f"Z:{abs_res}".replace('\\', '/')
        wine_res_obj = f"Z:{abs_obj}".replace('\\', '/')
        
        # Try to find windres
        windres_path = None
        possible_paths = [
            os.path.join(self.wine_prefix, "drive_c", "mingw32", "bin", "windres.exe"),
            os.path.join(self.wine_prefix, "drive_c", "MinGW", "bin", "windres.exe"),
        ]
        
        for path in possible_paths:
            if os.path.exists(path):
                windres_path = path
                break
        
        if not windres_path:
            # Try system windres
            if shutil.which("windres"):
                windres_path = "windres"
            else:
                print("[!] windres not found, skipping resources")
                return None
        
        if windres_path != "windres":
            cmd = ["wine", windres_path, wine_res_src, "--input-format=rc", "--codepage=65001", "-O", "coff", "-o", wine_res_obj]
        else:
            cmd = [windres_path, self.res_file, "--input-format=rc", "--codepage=65001", "-O", "coff", "-o", res_obj]
        
        print("[RESOURCE] Compiling resources...")
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            if result.returncode != 0:
                print(f"[!] Resource compilation warning: {result.stderr[:100]}")
                return None
            return res_obj
        except Exception as e:
            print(f"[!] Resource compilation failed: {e}")
            return None
    
    def link_objects(self, objects):
        """Link all object files"""
        # Filter out None objects
        objects = [obj for obj in objects if obj and os.path.exists(obj)]
        
        if not objects:
            print("[ERROR] No object files to link")
            return False
        
        print(f"\n[LINK] Linking {len(objects)} object files...")
        
        # Convert paths for Wine
        wine_objects = [f"Z:{os.path.abspath(obj)}".replace('\\', '/') for obj in objects]
        wine_exe = f"Z:{os.path.abspath(self.exe_name)}".replace('\\', '/')
        
        tcc_path = os.path.join(self.wine_prefix, "drive_c", "tcc", "tcc.exe")
        if not os.path.exists(tcc_path):
            print("[ERROR] TCC not found")
            return False
        
        cmd = ["wine", tcc_path, "-o", wine_exe] + wine_objects + self.cflags + self.libs
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
            if result.returncode != 0:
                print("[ERROR] Linking failed")
                if result.stderr:
                    print(result.stderr[:1000])
                return False
            
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
        """Create run script"""
        run_script = '''#!/bin/bash
# Run script for SMTP Tool on Kali Linux

GREEN='\\033[0;32m'
YELLOW='\\033[1;33m'
RED='\\033[0;31m'
NC='\\033[0m'

echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}SMTP Tool for Kali Linux${NC}"
echo -e "${GREEN}================================${NC}"

export WINEPREFIX=~/.wine_smtptool
export WINEDEBUG=-all

if [ ! -f "smtptool.exe" ]; then
    echo -e "${RED}[!] smtptool.exe not found!${NC}"
    echo -e "${YELLOW}[*] Run: python3 build_linux.py${NC}"
    exit 1
fi

echo -e "${GREEN}[*] Starting SMTP Tool...${NC}"
wine smtptool.exe "$@"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}[✓] Application exited normally${NC}"
else
    echo -e "${RED}[✗] Application crashed${NC}"
fi
'''
        
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
            return False
        
        # Load build cache
        old_hashes = self.load_cache()
        new_hashes = {}
        
        # Compile resources
        res_obj = self.compile_resources()
        
        # Compile sources
        obj_files = []
        print("\n[COMPILING] Source files...")
        
        # Use sequential compilation for better error messages
        for src_file in self.src_files:
            result = self.compile_source(src_file, old_hashes)
            if result[0]:
                obj_file, file_hash = result
                obj_files.append(obj_file)
                new_hashes[src_file] = file_hash
            else:
                print(f"[ERROR] Failed to compile {src_file}")
                return False
        
        # Add resource object
        if res_obj and os.path.exists(res_obj):
            obj_files.append(res_obj)
        
        # Link objects
        if not self.link_objects(obj_files):
            return False
        
        # Save cache
        self.save_cache(new_hashes)
        
        # Create run script
        self.create_run_script()
        
        print("\n" + "=" * 60)
        print("[SUCCESS] Build completed successfully!")
        print("=" * 60)
        print(f"[INFO] Executable: {self.exe_name}")
        print(f"[INFO] Run with: ./run_tool.sh")
        print("\n[NOTE] Make sure your asset folder is in the same directory")
        print("=" * 60)
        
        return True

def main():
    builder = LinuxWineBuilder()
    success = builder.build()
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
