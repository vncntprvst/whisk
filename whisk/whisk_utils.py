import requests
import os
import zipfile
from io import BytesIO

def download_and_extract_ffmpeg_dlls(destination_dir=None):
    """
    Downloads and extracts the FFmpeg DLLs from the GitHub releases page.
    
    Args:
        destination_dir (str): The directory where the DLLs should be extracted.
    """
    # FFmpeg DLLs download link
    url = "https://github.com/vncntprvst/whisk/releases/download/ffmpeg_6.0_dlls/ffmpeg_win64_lgpl_shared.zip"
    
    # Send a GET request to fetch the zip content
    response = requests.get(url)
    print(f"Response status code: {response.status_code}")
    response.raise_for_status()  # Raise an exception if there's an error
    
    # Use BytesIO to treat the content as a file-like byte stream
    with BytesIO(response.content) as f:
        with zipfile.ZipFile(f) as zipped:
            # Define a directory to save the DLLs
            if destination_dir is None:
                destination_dir = os.getcwd()  # Default to the current directory
            zipped.extractall(path=destination_dir)
            print(f"FFmpeg DLLs have been successfully downloaded and extracted to {destination_dir}")