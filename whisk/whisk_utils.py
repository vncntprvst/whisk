import requests
import os
import zipfile
import tarfile
from io import BytesIO
import shutil

def download_and_extract_ffmpeg_dlls(destination_dir=None):
    """
    Downloads and extracts the FFmpeg DLLs from the GitHub releases page.
    
    Args:
        destination_dir (str): The directory where the DLLs should be extracted.
    """
    # FFmpeg DLLs download link
    if os.name == 'posix':
        url = "https://github.com/vncntprvst/whisk/releases/download/ffmpeg_6.0_dlls/ffmpeg_linux64_lgpl_shared.tar.xz"
    elif os.name == 'nt':
        url = "https://github.com/vncntprvst/whisk/releases/download/ffmpeg_6.0_dlls/ffmpeg_win64_lgpl_shared.zip"
    else:
        raise NotImplementedError(f"OS {os.name} not supported.")
    
    # Send a GET request to fetch the archive content
    response = requests.get(url)
    print(f"Response status code: {response.status_code}")
    response.raise_for_status()  # Raise an exception if there's an error

    # Use BytesIO to treat the content as a file-like byte stream
    with BytesIO(response.content) as f:
        if os.name == 'posix':
            # Extract the tar.xz file
            with tarfile.open(fileobj=f, mode='r:xz') as tar:
                if destination_dir:
                    os.makedirs(destination_dir, exist_ok=True)
                    for member in tar.getmembers():
                        if member.isfile():
                            member.name = os.path.basename(member.name)  # Change the member name to only the basename
                            tar.extract(member, path=destination_dir)
                else:
                    tar.extractall()
                    destination_dir = os.getcwd()
                print(f"FFmpeg DLLs have been successfully downloaded and extracted to {destination_dir}")

        elif os.name == 'nt':
            # Extract the zip file
            with zipfile.ZipFile(f) as zipped:
                if destination_dir:
                    os.makedirs(destination_dir, exist_ok=True)
                    for member in zipped.namelist():
                        filename = os.path.basename(member)
                        if not filename:
                            continue
                        # Copy file (taken from zipfile's extract)
                        source = zipped.open(member)
                        target = open(os.path.join(destination_dir, filename), "wb")
                        with source, target:
                            shutil.copyfileobj(source, target)
                else:
                    destination_dir = os.getcwd()
                    zipped.extractall(path=destination_dir)
                print(f"FFmpeg DLLs have been successfully downloaded and extracted to {destination_dir}")

    return destination_dir