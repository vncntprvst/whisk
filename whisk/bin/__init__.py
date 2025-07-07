"""
Whisk binary executables wrapper module.

This module provides Python entry points for the whisk binary executables.
"""
import os
import sys
import subprocess
from pathlib import Path

def get_bin_path():
    """Get the path to the whisk bin directory."""
    return Path(__file__).parent

def run_executable(exe_name, args=None):
    """Run a whisk executable with given arguments."""
    bin_path = get_bin_path()
    exe_path = bin_path / exe_name
    
    if not exe_path.exists():
        raise FileNotFoundError(f"Executable '{exe_name}' not found in {bin_path}")
    
    if not os.access(exe_path, os.X_OK):
        # Try to fix permissions
        try:
            os.chmod(exe_path, 0o755)
        except OSError:
            pass
    
    cmd = [str(exe_path)]
    if args:
        cmd.extend(args)
    
    try:
        result = subprocess.run(cmd, check=False)
        return result.returncode
    except OSError as e:
        print(f"Error running {exe_name}: {e}", file=sys.stderr)
        return 1

def trace_main():
    """Entry point for trace executable."""
    return run_executable('trace', sys.argv[1:])

def classify_main():
    """Entry point for classify executable."""
    return run_executable('classify', sys.argv[1:])

def measure_main():
    """Entry point for measure executable."""
    return run_executable('measure', sys.argv[1:])

def whisker_convert_main():
    """Entry point for whisker_convert executable."""
    return run_executable('whisker_convert', sys.argv[1:])

def measurements_convert_main():
    """Entry point for measurements_convert executable."""
    return run_executable('measurements_convert', sys.argv[1:])

def reclassify_main():
    """Entry point for reclassify executable."""
    return run_executable('reclassify', sys.argv[1:])

def report_main():
    """Entry point for report executable."""
    return run_executable('report', sys.argv[1:])
