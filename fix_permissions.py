#!/usr/bin/env python3
"""
Post-installation script to ensure whisk binaries have proper permissions.
Run this after installing whisk if you encounter permission issues.
"""
import os
import stat
import sys
from pathlib import Path

def fix_whisk_permissions():
    """Fix permissions for whisk binary files."""
    try:
        import whisk
        bin_path = whisk.get_whisk_bin_path()
    except ImportError:
        print("Error: whisk package not found. Please install it first.")
        return False
    
    if not bin_path.exists():
        print(f"Error: Whisk bin directory not found at {bin_path}")
        return False
    
    print(f"Fixing permissions for binaries in {bin_path}")
    
    fixed_count = 0
    for exe_file in bin_path.iterdir():
        if exe_file.is_file() and not exe_file.suffix in ['.dll', '.so', '.exp', '.lib', '.ilk']:
            try:
                # Make executable for owner, readable/executable for group and others
                os.chmod(exe_file, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
                print(f"  Fixed: {exe_file.name}")
                fixed_count += 1
            except OSError as e:
                print(f"  Warning: Could not fix {exe_file.name}: {e}")
    
    print(f"Fixed permissions for {fixed_count} files.")
    return True

if __name__ == "__main__":
    success = fix_whisk_permissions()
    sys.exit(0 if success else 1)
