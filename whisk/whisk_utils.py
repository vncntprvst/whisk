import os
import zipfile
import tarfile
import warnings
from io import BytesIO
import shutil

# The prebuilt whisk binaries link against different FFmpeg major versions per
# platform, so the auto-downloaded shared libraries MUST match the binary on
# THIS OS or whisk.dll / libwhisk.so / trace will fail to load:
#   - Windows .exe/.dll: built against FFmpeg 8.x (avcodec-62, avformat-62,
#     avutil-60, swscale-9, swresample-6, avdevice-62, avfilter-11)
#   - Linux .so:         built against FFmpeg 6.x (libavcodec.so.60, ...)
# Each entry maps to a GitHub release holding the matching lgpl-shared archive.
# When a platform's binaries are rebuilt against a different FFmpeg major, bump
# its tag here (AND the matching probe below) and upload the matching archive.
FFMPEG_DLL_RELEASE = {
    "nt": "ffmpeg_8.0_dlls",
    "posix": "ffmpeg_6.0_dlls",
}

# A marker shared-library, one per OS, used to detect whether the matching
# FFmpeg libs are already present so we don't re-download. Keep the major in
# sync with FFMPEG_DLL_RELEASE above (Windows FFmpeg 8 -> 62; Linux FFmpeg 6 -> 60).
FFMPEG_PROBE = {
    "nt": "avcodec-62.dll",
    "posix": "libavcodec.so.60",
}

# Set this env var (to any non-empty value) to disable the on-import auto-download.
SKIP_DOWNLOAD_ENV = "WHISK_SKIP_FFMPEG_DOWNLOAD"


def ffmpeg_dlls_present(bin_dir):
    """True if the matching FFmpeg marker library is already in ``bin_dir``.

    Checks ``bin_dir`` and its immediate ``ffmpeg_*_lgpl_shared`` subdir so both
    the flat layout (used here) and the setup.py subfolder layout are recognised.
    """
    probe = FFMPEG_PROBE.get(os.name)
    if probe is None:
        return True  # unknown OS: never block import on a download we can't do
    candidates = [bin_dir,
                  os.path.join(bin_dir, "ffmpeg_win64_lgpl_shared"),
                  os.path.join(bin_dir, "ffmpeg_linux64_lgpl_shared")]
    return any(os.path.isfile(os.path.join(d, probe)) for d in candidates)


def ensure_ffmpeg_dlls(bin_dir, *, force=False):
    """Best-effort: download the matching FFmpeg libs into ``bin_dir`` if missing.

    Returns True if the libs are present afterwards. Never raises: a failed or
    skipped download warns and returns False so ``import whisk`` still succeeds.
    Disable entirely with the ``WHISK_SKIP_FFMPEG_DOWNLOAD`` env var.
    """
    if not force and ffmpeg_dlls_present(bin_dir):
        return True
    if os.environ.get(SKIP_DOWNLOAD_ENV):
        return ffmpeg_dlls_present(bin_dir)
    try:
        download_and_extract_ffmpeg_dlls(destination_dir=bin_dir)
    except Exception as exc:  # network down, no requests, unsupported OS, ...
        warnings.warn(
            f"whisk: could not auto-download FFmpeg shared libraries ({exc}). "
            f"whisk binaries will fail to load until the matching FFmpeg "
            f"{'8.x' if os.name == 'nt' else '6.x'} libs are on the library path. "
            f"Fix: install with the [ffmpeg] extra from source, or call "
            f"whisk.whisk_utils.download_and_extract_ffmpeg_dlls(). "
            f"Set {SKIP_DOWNLOAD_ENV}=1 to silence this.")
        return False
    return ffmpeg_dlls_present(bin_dir)


def preload_bundled_ffmpeg(bin_dir):
    """Preload the bundled FFmpeg shared libraries into the global namespace (Linux).

    The Linux whisk .so are not built with an ``$ORIGIN`` rpath, and a runtime
    ``LD_LIBRARY_PATH`` edit does not affect the already-running interpreter's
    loader. So when WhiskiWrap later does ``ctypes.CDLL("libwhisk.so")`` the
    loader can't find its NEEDED ``libav*`` in ``bin/``. Loading them here first
    with ``RTLD_GLOBAL`` puts them in the process by soname, so the subsequent
    ``libwhisk.so`` load reuses them. No-op on Windows (``add_dll_directory``
    covers that) and silent if the libs are absent or already satisfied.
    """
    if os.name != "posix":
        return
    import ctypes
    import glob
    # versioned ffmpeg sonames only (e.g. libavcodec.so.60); the bare lib*.so
    # symlinks and libwhisk.so itself end in ".so" and are skipped by the glob.
    libs = [p for p in glob.glob(os.path.join(bin_dir, "lib*.so.*"))
            if os.path.basename(p).startswith(("libav", "libsw"))]
    # retry passes so interdependencies resolve regardless of load order
    for _ in range(len(libs)):
        remaining = []
        for p in libs:
            try:
                ctypes.CDLL(p, mode=ctypes.RTLD_GLOBAL)
            except OSError:
                remaining.append(p)
        if not remaining or remaining == libs:
            break
        libs = remaining


def download_and_extract_ffmpeg_dlls(destination_dir=None):
    """
    Downloads and extracts the FFmpeg DLLs from the GitHub releases page.

    The archives must contain the same FFmpeg major version the whisk binaries
    were built against (see ``FFMPEG_DLL_RELEASE``).

    Args:
        destination_dir (str): The directory where the DLLs should be extracted.
    """
    # requests is an optional ([ffmpeg]) dependency, so import it lazily here
    # rather than at module import — that keeps plain ``import whisk`` working
    # even when requests is not installed.
    import requests

    # FFmpeg DLLs download link (per-OS release tag must match this OS's binaries)
    try:
        release = FFMPEG_DLL_RELEASE[os.name]
    except KeyError:
        raise NotImplementedError(f"OS {os.name} not supported.")
    base = f"https://github.com/vncntprvst/whisk/releases/download/{release}"
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