import os, stat
import sys
import sysconfig
from setuptools import setup, find_packages, Command
from setuptools.command.install import install
from setuptools.command.develop import develop

class CustomPostInstall:
    """Common post-installation tasks."""
    
    def run_custom_install(self):
        # Get the site-packages directory properly
        if hasattr(sysconfig, 'get_paths'):
            site_packages_dir = sysconfig.get_paths()['purelib']
        else:
            import distutils.sysconfig
            site_packages_dir = distutils.sysconfig.get_python_lib()
        
        print(f"Site packages directory: {site_packages_dir}")
        
        # Handle FFmpeg installation if requested
        if hasattr(self.distribution, 'extras_require') and 'ffmpeg' in self.distribution.extras_require:
            print("Downloading ffmpeg DLLs...")
            
            if os.name == 'posix':
                whisk_ffmpegbin_dir = os.path.join(site_packages_dir, 'whisk', 'bin', 'ffmpeg_linux64_lgpl_shared')
            elif os.name == 'nt':
                whisk_ffmpegbin_dir = os.path.join(site_packages_dir, 'whisk', 'bin', 'ffmpeg_win64_lgpl_shared')
            
            try:
                from whisk import whisk_utils
                whisk_utils.download_and_extract_ffmpeg_dlls(destination_dir=whisk_ffmpegbin_dir)
            except ImportError as e:
                print(f"Warning: Could not import whisk.whisk_utils: {e}")

        # Update the permissions of the files in the 'bin' directory
        bin_dir = os.path.join(site_packages_dir, 'whisk', 'bin')
        if os.path.exists(bin_dir):
            # Skip permission changes for development installs
            source_bin_dir = os.path.join(os.path.dirname(__file__), 'whisk', 'bin')
            
            # Check if this is a development install by comparing paths or checking if files are symlinks
            is_dev_install = False
            
            # Method 1: Check if the paths are the same
            if os.path.abspath(bin_dir) == os.path.abspath(source_bin_dir):
                is_dev_install = True
            else:
                # Method 2: Check if any file in bin_dir is a symlink pointing to source
                try:
                    for filename in os.listdir(bin_dir):
                        file_path = os.path.join(bin_dir, filename)
                        if os.path.islink(file_path):
                            link_target = os.readlink(file_path)
                            if os.path.abspath(link_target).startswith(os.path.abspath(source_bin_dir)):
                                is_dev_install = True
                                break
                except OSError:
                    pass
                
                # Method 3: Check if the parent directory contains a .egg-link file (development install indicator)
                if not is_dev_install:
                    whisk_dir = os.path.join(site_packages_dir, 'whisk')
                    parent_dir = os.path.dirname(whisk_dir)
                    for item in os.listdir(parent_dir):
                        if item.endswith('.egg-link') and 'whisk' in item:
                            is_dev_install = True
                            break
            
            if is_dev_install:
                print(f"Skipping permission changes for development install in {bin_dir}")
            else:
                print(f"Setting executable permissions for files in {bin_dir}")
                for filename in os.listdir(bin_dir):
                    file_path = os.path.join(bin_dir, filename)
                    if os.path.isfile(file_path):
                        try:
                            # For current user: read, write, execute; for group and others: read, execute
                            os.chmod(file_path, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
                        except OSError as e:
                            print(f"Warning: Could not set permissions for {file_path}: {e}")

class CustomInstall(install, CustomPostInstall):
    def run(self):
        super().run()  # Ensure the package is installed first
        self.run_custom_install()

class CustomDevelop(develop, CustomPostInstall):
    def run(self):
        super().run()  # Ensure the package is installed first
        self.run_custom_install()

with open('README.md', 'r') as f:
    long_description = f.read()
    
print("WARNING: This package requires ffmpeg and associated dynamic libraries. \n"
      "Recommended ffmpeg version is 6.0. \n"
      "Some library files from BtbN's FFmpeg-Builds (https://github.com/BtbN/FFmpeg-Builds)\n"
      " will be downloaded automatically to whisk/bin if specific versions are not found in LD_LIBRARY_PATH or PATH. \n "
      "Be aware however that multiple versions of ffmpeg and these libraries may cause conflicts.\n")

setup(
    name='whisk-janelia',
    version='1.2.1',
    author='Nathan Clack',
    maintainer=', '.join(['clackn','cxrodgers','mitchclough','vncntprvst']),
    url='https://github.com/nclack/whisk/',
    license='LICENSE.txt',
    description='Fully automated tracking of single rows of whiskers in high-speed video.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    install_requires=[],
    extras_require={
        'ffmpeg': ["requests>=2.24,<3"], # requests is used to download the ffmpeg DLLs. Trigger this by installing with `pip install whisk[ffmpeg]`
    },
    # packages=['whisk'],
    packages=find_packages(exclude=["whisk.bin.*", "whisk.bin", "pipeline.*", "pipeline", "ui.*", "ui"]),
    package_data={'whisk': ['bin/*']},
    include_package_data=True,
    cmdclass={
        'install': CustomInstall,
        'develop': CustomDevelop,
    },
    entry_points={
        'console_scripts': [
            'whisk-trace=whisk.bin:trace_main',
            'whisk-classify=whisk.bin:classify_main',
            'whisk-measure=whisk.bin:measure_main',
            'whisk-whisker-convert=whisk.bin:whisker_convert_main',
            'whisk-measurements-convert=whisk.bin:measurements_convert_main',
            'whisk-reclassify=whisk.bin:reclassify_main',
            'whisk-report=whisk.bin:report_main',
        ],
    },
)

# Get a list of all binary files 
# bin_files = [os.path.join('whisk', 'bin', f) for f in os.listdir(os.path.join('whisk', 'bin')) if os.path.isfile(os.path.join('whisk', 'bin', f))]


