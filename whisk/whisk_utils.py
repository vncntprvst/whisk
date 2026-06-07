import requests
import os
import zipfile
import tarfile
from io import BytesIO
import shutil

# The prebuilt whisk binaries are now compiled against FFmpeg 8.x (avcodec-62,
# avformat-62, avutil-60, swscale-9, swresample-6, avdevice-62, avfilter-11).
# The auto-downloaded DLLs MUST match this major version or whisk.dll / trace
# will fail to load. Bump this tag whenever the binaries are rebuilt against a
# different FFmpeg release, and upload matching archives to that GitHub release.
FFMPEG_DLL_RELEASE = "ffmpeg_8.0_dlls"

def download_and_extract_ffmpeg_dlls(destination_dir=None):
    """
    Downloads and extracts the FFmpeg DLLs from the GitHub releases page.

    The archives must contain the same FFmpeg major version the whisk binaries
    were built against (see ``FFMPEG_DLL_RELEASE``).

    Args:
        destination_dir (str): The directory where the DLLs should be extracted.
    """
    # FFmpeg DLLs download link
    base = f"https://github.com/vncntprvst/whisk/releases/download/{FFMPEG_DLL_RELEASE}"
    if os.name == 'posix':
        url = f"{base}/ffmpeg_linux64_lgpl_shared.tar.xz"
    elif os.name == 'nt':
        url = f"{base}/ffmpeg_win64_lgpl_shared.zip"
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