import sys,os
from ctypes import *
from ctypes.util import find_library
from numpy import zeros, float32, uint8, array, hypot, arctan2, pi, concatenate, float64, ndarray, int32
from numpy import where, cos, sin, sum
from warnings import warn

import whisk
import pdb

def get_all_subdirectories(path):
    subdirs = [d for d, _, _ in os.walk(path)]
    return subdirs

# Find the base directory of the whisk package
whisk_base_dir = os.path.dirname(whisk.__file__)
whisk_bin_dir = os.path.join(whisk_base_dir, 'bin')
# build_dir = 'C:\\Code\\WhiskerTracking\\whisk\\build'

if sys.platform == 'win32':
    lib = os.path.join(whisk_bin_dir, 'whisk.dll')
else:
    lib = os.path.join(whisk_bin_dir, 'libwhisk.so')
    
# Append both whisk base directory and bin directory to PATH
os.environ['PATH'] += os.pathsep + os.pathsep.join(['.', '..', whisk_base_dir, whisk_bin_dir]) #build_dir

all_directories = get_all_subdirectories(whisk_bin_dir)

# Append all directories to PATH
os.environ['PATH'] = os.environ['PATH'] + ';' + ';'.join(all_directories)

name = lib
if not name:
  name = find_library('whisk')

def load_ffmpeg_dlls(whisk_bin_dir):
    """
    Load ffmpeg DLLs required for cWhisk.
    
    Args:
    - whisk_bin_dir: the imported whisk module's bin directory
    """
    ffmpeg_dll_dir = os.path.join(whisk_bin_dir, 'ffmpeg_win64_lgpl_shared')

    ffmpeg_dll_names = [
        "avcodec-60.dll",
        "avdevice-60.dll",
        "avformat-60.dll",
        "avutil-58.dll",
        "swscale-7.dll",
    ]

    ffmpeg_dlls = [os.path.join(ffmpeg_dll_dir, dll_name) for dll_name in ffmpeg_dll_names]

    # Load each ffmpeg DLL
    for dll in ffmpeg_dlls:
        CDLL(dll)
        # print(f"Loaded {dll} successfully!")

# ffmpeg_dll_dir = os.path.join(whisk_bin_dir, 'ffmpeg_win64_lgpl_shared')

# ffmpeg_dll_names = [
#     "avcodec-60.dll",
#     "avdevice-60.dll",
#     "avformat-60.dll",
#     "avutil-58.dll",
#     "swscale-7.dll",
# ]

# ffmpeg_dlls = [os.path.join(ffmpeg_dll_dir, dll_name) for dll_name in ffmpeg_dll_names]

# for dll in ffmpeg_dlls:
#     try:
#         CDLL(dll)
#         print(f"Loaded {dll} successfully!")
#     except Exception as e:
#         print(f"Failed to load {dll}. Error: {e}")

try:
    cWhisk = CDLL(name)
except OSError:
    # If loading cWhisk failed, attempt to load ffmpeg DLLs and then try again
    load_ffmpeg_dlls(whisk_bin_dir)
    cWhisk = CDLL(name)

