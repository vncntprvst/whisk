import os, stat, atexit
import distutils.sysconfig
from setuptools import setup, find_packages, Command
from setuptools.command.install import install

def post_install():
    print("Running post-install script...")
    # Update the permissions of the files in the 'bin' directory
    print("Updating permissions of files in the 'bin' directory...")
    site_packages_dir = distutils.sysconfig.get_python_lib()
    bin_dir = os.path.join(site_packages_dir, 'whisk', 'bin')
    for filename in os.listdir(bin_dir):
        file_path = os.path.join(bin_dir, filename)
        # For all users: read, write, execute
        # os.chmod(file_path, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)
        # For current user: read, write, execute
        os.chmod(file_path, stat.S_IRWXU | stat.S_IRGRP | stat.S_IROTH)
        print(f"New permissions: {os.stat(file_path).st_mode}")
    
class CustomInstall(install):
    def run(self):
        print("Running custom install...")
        super().run()  # Ensure the package is installed first
        
        if 'ffmpeg' in self.distribution.extras_require:
            print("Downloading ffmpeg DLLs...")
            
            # Determine the destination directory
            site_packages_dir = distutils.sysconfig.get_python_lib()
            print(f"Site packages directory: {site_packages_dir}")
            whisk_ffmpegbin_dir = os.path.join(site_packages_dir, 'whisk', 'bin', 'ffmpeg_win64_lgpl_shared')
            
            from whisk import whisk_utils
            whisk_utils.download_and_extract_ffmpeg_dlls(destination_dir=whisk_ffmpegbin_dir)

        atexit.register(post_install)

with open('README.md', 'r') as f:
    long_description = f.read()

setup(
    name='whisk-janelia',
    version='1.1.6',
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
    }
)

# Get a list of all binary files 
# bin_files = [os.path.join('whisk', 'bin', f) for f in os.listdir(os.path.join('whisk', 'bin')) if os.path.isfile(os.path.join('whisk', 'bin', f))]


