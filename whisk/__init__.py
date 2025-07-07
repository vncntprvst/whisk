"""
Whisk - Fully automated tracking of single rows of whiskers in high-speed video.

This package provides Python bindings and utilities for the Janelia Whisker Tracking software.
"""
import os
import sys
from pathlib import Path

__version__ = "1.1.10"
__author__ = "Nathan Clack"

def get_whisk_bin_path():
    """Get the path to the whisk bin directory containing executables."""
    return Path(__file__).parent / "bin"

def get_trace_executable():
    """Get the full path to the trace executable."""
    bin_path = get_whisk_bin_path()
    trace_path = bin_path / "trace"
    if not trace_path.exists():
        raise FileNotFoundError(f"trace executable not found at {trace_path}")
    return str(trace_path)

def ensure_executable_permissions():
    """Ensure all binary files in the bin directory have execute permissions."""
    bin_path = get_whisk_bin_path()
    if not bin_path.exists():
        return
    
    for exe_file in bin_path.iterdir():
        if exe_file.is_file() and not exe_file.suffix in ['.dll', '.so', '.exp', '.lib', '.ilk']:
            try:
                # Make executable for owner, readable for group and others
                os.chmod(exe_file, 0o755)
            except OSError:
                pass

# Ensure executables have proper permissions on import
ensure_executable_permissions()

# Add bin directory to PATH if not already there
bin_path_str = str(get_whisk_bin_path())
if bin_path_str not in os.environ.get('PATH', ''):
    if 'PATH' in os.environ:
        os.environ['PATH'] = bin_path_str + os.pathsep + os.environ['PATH']
    else:
        os.environ['PATH'] = bin_path_str
