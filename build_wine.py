#!/usr/bin/env python3
"""
Cross-platform build script for Windows application on Kali Linux using Wine
"""
import os
import subprocess
import sys
import pathlib
import hashlib
import json
import shutil
from concurrent.futures import ThreadPoolExecutor, as_completed

class WineBuilder:
    def __init__(self):
        self.wine_prefix = os.path.expanduser("~/.wine_smtpcracker")
        self.wine_cmd = ["wine", "cmd.exe", "/c"]
        self.wine_path = f"Z:{os.getcwd()}"
        
        # Compiler paths in Wine
        self.tcc_wine = ["wine", "C:\\tcc\\tcc.exe"]
        self.windres_wine = ["wine", "C:\\mingw32\\bin\\windres.exe"]
        
        # Fallback to native if Wine paths not found
        self.cc = "tcc"
        self.windres = "windres"
        
        # Build directories
        self.obj_dir = "obj"
        self.res_dir = "res"
        self.exe_name = "smtpcracker.exe"
        self.hash_file = os.path.join(self.obj_dir, "file_hashes.json")
        
        # Compiler flags
        self.cflags = [
            "-Iinclude",
            "-ISidebar",
            "-municode",
            "-mconsole",
            "-s",
            "-DUNICODE",
            "-D_UNICODE"
        ]
        
        self.libs = [
            "-lcomctl32", "-ldwmapi", "-lws2_32",
            "-lCabinet", "-loleacc", "-loleaut32", "-lcrypto",
            "-lgdi32", "-lshlwapi", "-lcomdlg32", "-lmsimg32",
            "-lwinmm", "-lgdiplus", "-lshell32", "-lDbghelp",
            "-lcurl", "-lversion", "-ladvapi32", "-lcredui",
            "-lole32", "-liphlpapi", "-ld2d1", "-lkernel32",
            "-ldnsapi", "-lssl", "-lcurl"
        ]
        
        # Source files (adjust based on your actual source structure)
        self.src = [
            "src/main.c",
            "src/main_proc.c",
            "src/utils/utils.c",
            "src/utils/err.c",
            "src/theme/theme.c",
            "src/theme/darkmode_dynamic.c",
            "src/sidebar/sidebar.c",
            "src/ui/sidebar_main.c",
            "src/ui/pages.c",
            "src/ui/settingspage.c",
            "src/ui/homepage.c",
            "src/ui/aboutpage.c",
            "src/core/settings.c",
            "src/core/size_msg.c",
            "src/core/notify_msg.c",
            "src/core/command_msg.c",
            "src/smtplib/smtp.c",
            "src/core/checker.c",
            "src/core/notifications.c",
        ]
        
    def setup_wine_env(self):
        """Setup Wine environment with required Windows tools"""
        print("[*] Setting up Wine environment...")
        
        # Check if Wine is installed
        if not shutil.which("wine"):
            print("[!] Wine not found. Installing...")
            subprocess.run(["sudo", "apt-get", "update"], check=True)
            subprocess.run(["sudo", "apt-get", "install", "-y", "wine", "wine32", "wine64"], check=True)
        
        # Set Wine prefix
        os.environ["WINEPREFIX"] = self.wine_prefix
        
        # Initialize Wine prefix if not exists
        if not os.path.exists(self.wine_prefix):
            print("[*] Initializing Wine prefix...")
            subprocess.run(["wineboot", "-u"], capture_output=True)
        
        # Install TCC in Wine
        tcc_path = os.path.join(self.wine_prefix, "drive_c", "tcc")
        if not os.path.exists(tcc_path):
            print("[*] Installing Tiny C Compiler in Wine...")
            self._install_tcc_wine()
        
        # Install MinGW for windres
        mingw_path = os.path.join(self.wine_prefix, "drive_c", "mingw32")
        if not os.path.exists(mingw_path):
            print("[*] Installing MinGW resources compiler in Wine...")
            self._install_mingw_wine()
        
        print("[+] Wine environment ready!")
    
    def _install_tcc_wine(self):
        """Download and install TCC in Wine"""
        # Download TCC Windows binary
        tcc_url = "https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27-win64-bin.zip"
        tcc_zip = "tcc.zip"
        
        subprocess.run(["wget", tcc_url, "-O", tcc_zip], check=True)
        subprocess.run(["unzip", "-q", tcc_zip, "-d", "tcc_temp"], check=True)
        
        # Copy to Wine drive_c
        wine_tcc_path = os.path.join(self.wine_prefix, "drive_c", "tcc")
        shutil.copytree("tcc_temp", wine_tcc_path, dirs_exist_ok=True)
        
        # Cleanup
        shutil.rmtree("tcc_temp", ignore_errors=True)
        os.remove(tcc_zip)
    
    def _install_mingw_wine(self):
        """Install MinGW-w64 resources compiler"""
        mingw_url = "https://github.com/niXman/mingw-builds-binaries/releases/download/13.2.0-rt_v11-rev1/x86_64-13.2.0-release-win32-seh-ucrt-rt_v11-rev1.7z"
        mingw_7z = "mingw.7z"
        
        subprocess.run(["wget", mingw_url, "-O", mingw_7z], check=True)
        subprocess.run(["7z", "x", mingw_7z, "-omingw_temp"], check=True)
        
        # Copy to Wine drive_c
        wine_mingw_path = os.path.join(self.wine_prefix, "drive_c", "mingw32")
        shutil.copytree("mingw_temp", wine_mingw_path, dirs_exist_ok=True)
        
        # Cleanup
        shutil.rmtree("mingw_temp", ignore_errors=True)
        os.remove(mingw_7z)
    
    def file_hash(self, path):
        """Compute SHA1 hash of a file"""
        h = hashlib.sha1()
        with open(path, "rb") as f:
            while chunk := f.read(8192):
                h.update(chunk)
        return h.hexdigest()
    
    def load_hash_cache(self):
        """Load previous compilation hashes"""
        if os.path.exists(self.hash_file):
            with open(self.hash_file, "r") as f:
                return json.load(f)
        return {}
    
    def save_hash_cache(self, hashes):
        """Save compilation hashes"""
        os.makedirs(self.obj_dir, exist_ok=True)
        with open(self.hash_file, "w") as f:
            json.dump(hashes, f, indent=2)
    
    def compile_source(self, src_file, old_hashes):
        """Compile a single .c file using Wine TCC"""
        obj_file = os.path.join(self.obj_dir, pathlib.Path(src_file).stem + ".o")
        current_hash = self.file_hash(src_file)
        
        # Skip if unchanged
        if old_hashes.get(src_file) == current_hash and os.path.exists(obj_file):
            print(f"[SKIP] {src_file} (unchanged)")
            return obj_file, current_hash
        
        # Convert to Windows path for Wine
        wine_src = f"C:\\{src_file.replace('/', '\\')}"
        wine_obj = f"C:\\{obj_file.replace('/', '\\')}"
        
        # Build command for Wine
        cmd = [
            "wine", "C:\\tcc\\tcc.exe", 
            "-c", wine_src, 
            "-o", wine_obj
        ] + self.cflags
        
        # Change to root directory for relative paths
        print(f"[COMPILE] {src_file}")
        
        try:
            # Run in the current directory with Wine
            result = subprocess.run(cmd, cwd=os.getcwd(), capture_output=True, text=True)
            if result.returncode != 0:
                print(f"[ERROR] Compilation failed for {src_file}")
                print(result.stderr)
                raise subprocess.CalledProcessError(result.returncode, cmd)
        except Exception as e:
            print(f"[ERROR] {e}")
            raise
        
        return obj_file, current_hash
    
    def compile_resources(self):
        """Compile Windows resources using windres in Wine"""
        res_src = os.path.join(self.res_dir, "resource.rc")
        res_obj = os.path.join(self.res_dir, "resource.o")
        
        # Convert to Windows paths
        wine_res_src = f"C:\\{res_src.replace('/', '\\')}"
        wine_res_obj = f"C:\\{res_obj.replace('/', '\\')}"
        
        cmd = [
            "wine", "C:\\mingw32\\bin\\windres.exe",
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
                print(f"[ERROR] Resource compilation failed")
                print(result.stderr)
                raise subprocess.CalledProcessError(result.returncode, cmd)
        except Exception as e:
            print(f"[ERROR] {e}")
            raise
        
        return res_obj
    
    def link_objects(self, objects):
        """Link object files using Wine TCC"""
        # Convert all objects to Windows paths
        wine_objects = [f"C:\\{obj.replace('/', '\\')}" for obj in objects]
        wine_exe = f"C:\\{self.exe_name}"
        
        cmd = ["wine", "C:\\tcc\\tcc.exe"] + wine_objects + ["-o", wine_exe] + self.cflags + self.libs
        
        print(f"[LINK] Creating {self.exe_name}...")
        try:
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                print("[ERROR] Linking failed")
                print(result.stderr)
                raise subprocess.CalledProcessError(result.returncode, cmd)
        except Exception as e:
            print(f"[ERROR] {e}")
            raise
    
    def build(self):
        """Main build process"""
        print("=" * 50)
        print("SMTP Cracker Builder for Kali Linux (Wine)")
        print("=" * 50)
        
        # Create directories
        os.makedirs(self.obj_dir, exist_ok=True)
        os.makedirs(self.res_dir, exist_ok=True)
        
        # Setup Wine environment
        self.setup_wine_env()
        
        # Load cache
        old_hashes = self.load_hash_cache()
        new_hashes = {}
        
        # Compile resources
        try:
            res_obj = self.compile_resources()
        except FileNotFoundError:
            print("[!] resource.rc not found, skipping resources")
            res_obj = None
        
        # Compile sources in parallel
        obj_files = []
        
        with ThreadPoolExecutor(max_workers=4) as executor:
            futures = {executor.submit(self.compile_source, src, old_hashes): src for src in self.src}
            
            for future in as_completed(futures):
                try:
                    obj_file, file_hash_value = future.result()
                    obj_files.append(obj_file)
                    new_hashes[futures[future]] = file_hash_value
                except Exception as e:
                    print(f"[ERROR] Compilation failed: {e}")
                    sys.exit(1)
        
        # Add resource object if compiled
        if res_obj and os.path.exists(res_obj):
            obj_files.append(res_obj)
        
        # Link everything
        self.link_objects(obj_files)
        
        # Save hash cache
        self.save_hash_cache(new_hashes)
        
        print("=" * 50)
        print(f"[SUCCESS] Build complete! Executable: {self.exe_name}")
        print("[INFO] Run with: ./run_wine.sh")
        print("=" * 50)

def main():
    builder = WineBuilder()
    builder.build()

if __name__ == "__main__":
    main()
