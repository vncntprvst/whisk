Janelia Whisker tracking
========================
Fully automated tracking of single rows of whiskers in high-speed video.  

The original source code repository is https://github.com/nclack/whisk/.    
The [original website](http://openwiki.janelia.org/wiki/display/MyersLab/Whisker+Tracking) for this program is now accessible [here](https://wikis.janelia.org/display/WT/Whisker+Tracking).
A copy of the instructions and the tutorial are available [here](https://github.com/wanglab-neuro/whisk-dockerfile/tree/main/context/wiki).

Downloading
===========
Pre-built binaries are available on [this forked repository](https://github.com/vncntprvst/whisk/tree/master/whisk/bin). Binaries are built against FFmpeg 6.0. 
Binary and library file can simply be installed from Pypi, with `pip install whisk-janelia`.  
> Note that on Linux, if not root, you may need to make some of the binary files executable, e.g. `chmod +x /path/to/whisk/bin/trace`. You can also use the post-installation script `update_permissions.py` to make all files in the `whisk/bin` directory executable.  
Check which version of FFmpeg is installed, just type `ffmpeg` in a terminal.  
You can also check which version of required libraries like `libavdevice` are installed with:  
`for dir in $(echo $LD_LIBRARY_PATH | tr ":" "\n"); do find $dir -name 'libavdevice.so*' 2>/dev/null; done`.  

See also the [README on the whisk-dockerfile repo](https://github.com/wanglab-neuro/whisk-dockerfile), in `whisk-base` section.  

Building
========
Updated build files are found [here](https://github.com/vncntprvst/whisk/).  
This package uses a [Cmake][] based build system.  The basic build steps are similar under Windows, OS X, and Linux.  
The build requirements are `cmake` `gcc-12.2+` `FFmpeg`.

**For Unix-y systems:**  

1. Install CMake 2.8+, a C++ compiler and other build tools.
```
sudo apt update
sudo apt install cmake pkg-config bison gawk
sudo apt install g++ gdb make ninja-build rsync zip
```
2. Install FFmpeg libraries
```
sudo apt install libavdevice-dev libavfilter-dev libavformat-dev libavcodec-dev libswresample-dev libswscale-dev libavutil-dev
```
* -- **Alternatively**  --  
  Build FFmpeg from source.    
  First, install the necessary dependencies:   
  ```bash
  sudo apt-get install -y autoconf automake build-essential cmake libass-dev libfreetype6-dev libsdl2-dev libtool libva-dev libvdpau-dev libvorbis-dev libxcb1-dev libxcb-shm0-dev libxcb-xfixes0-dev pkg-config texinfo zlib1g-dev yasm libx264-dev libx265-dev libnuma-dev libvpx-dev libfdk-aac-dev libmp3lame-dev libopus-dev
  ```
  Then, download the FFmpeg source code:  
  ```bash
  wget https://FFmpeg.org/releases/ffmpeg-6.0.tar.bz2
  tar xjf ffmpeg-6.0.tar.bz2
  cd ffmpeg-6.0
  ```
  Configure and compile FFmpeg with shared libraries (note the `--enable-pic`, `--enable-shared`, and `--disable-static` flags):
  ```bash
  CFLAGS="-fPIC" CXXFLAGS="-fPIC" ./configure \
    --enable-gpl \
    --enable-version3 \
    --enable-nonfree \
    --enable-small \
    --enable-libmp3lame \
    --enable-libx264 \
    --enable-libx265 \
    --enable-libvpx \
    --enable-libfdk-aac \
    --enable-libopus \
    --enable-pic \
    --enable-shared \
    --disable-static
  make -j$(nproc)
  ```
  Finally, install FFmpeg:  
  ```bash
  sudo make install
  ```

If cmake still fails because of it can't find FFmpeg libraries `libavdevice;libavfilter;libavformat;libavcodec;libswresample;libswscale;libavutil`, you need to have them somewhere, and make it find it, e.g. `export PKG_CONFIG_PATH=/home/$USER/ffmpeg_build/lib/pkgconfig`.  
If you built FFmpeg from source, you can add `/usr/local/lib` to `LD_LIBRARY_PATH` and update the dynamic linker cache with `sudo ldconfig`:  
```bash
echo 'export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
echo "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/FFmpeg.conf
sudo ldconfig
```

3. Install Qt5 and Qt5Svg
    * `sudo apt install qtbase5-dev libqt5svg5-dev` 
    * Check installation with `qmake -v`.
    * Set `Qt5_DIR` and `Qt5Svg_DIR` environment variables if cmake asks for it.
4. Start in the root of the source directory (in a terminal).
5. Type these commands (or use [VSCode's CMake extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) to configure and build):
```
    mkdir build
    cd build
    cmake ..
    make
```

For WSL systems, this page may be helpful:  
https://devblogs.microsoft.com/cppblog/build-and-debug-c-with-wsl-2-distributions-and-visual-studio-2022/

**For Windows systems** (ignore VS Code instructions if not using it):  

1. Install [Visual Studio Code](https://code.visualstudio.com/download).
2. Install the C/C++ extension for VS Code. 
3. Install Mingw-w64 via [MSYS2](https://www.msys2.org/). Prefer installing MSYS2 in a directory without space (e.g., not C:/Program Files/ ...). That will save you a lot of headaches.
4. Launch a MSYS2 terminal for the UCRT64 environment as an administrator and install the following:  
* Install gcc as mentioned in [MSYS2 installation page](https://www.msys2.org/): `pacman -S mingw-w64-ucrt-x86_64-gcc`   
* Install the Mingw-w64 toolchain: `pacman -S --needed base-devel mingw-w64-x86_64-toolchain`
* Install cmake and generators (ninja ... make is installed with the toolchain above, ): `pacman -S mingw-w64-x86_64-cmake`
* Install the FFmpeg libraries for the 64-bit MinGW toolchain: 
     To install the most recent version instead, run `pacman -S mingw-w64-x86_64-ffmpeg`.  
     If using python (see packages `whisk-janelia` and `whiskiwrap`), you need to ensure compatibility between the FFmpeg version used to build whisk (currently, version 6.0) and the FFmpeg library files that python will call with the package `ctypes`. Tarballs are available for download on [MSYS2's package page](https://packages.msys2.org/package/mingw-w64-x86_64-ffmpeg), or run `wget https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-ffmpeg-6.0-7-any.pkg.tar.zst`. Then, install the package with  `pacman -U mingw-w64-x86_64-ffmpeg-6.0-7-any.pkg.tar.zst`.    
* Update the PKG_CONFIG_PATH environment variable to include the path to the FFmpeg library .pc files. In the MSYS2 terminal, run: `export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/mingw64/lib/pkgconfig`, and add it to `~/.bashrc` as well to make this change permanent.
5. Open a terminal, navigate to the `whisk` repository directory.
6. Run the following commands:  
```bash
    mkdir build
    cd build
    cmake ..
    ninja
```  

CMake works by building the Makefile, or other required project files, in the "build" directory you created.  You don't have to use a Makefile; XCode projects,
Eclipse projects, Visual Studio projects, etc ... can be generated.  To generate these other project types replace the "cmake .." step with "cmake-gui ..".  The
GUI will show available options.  See the CMake documentation for more details.  

[Cmake]: http://www.cmake.org


File formats
============

### Whiskers

Whiskers can be stored in either a text or binary format.  There is a command line utility, `whisker_convert`, provided to convert between different
supported formats. `batch.py` can be used to speed up the process of converting many files.

The most detailed description of each format can be found in the corresponding `whisker_io_*.c` files implimenting the readers/writers for each format.

In general, each whisker segment in a file is comprised of a unique id and a number of data points describing the shape as output from the tracing program.

### Measurements 

The measurements file stores the features of traced curves used for classification, such as follicle position, angle and whisker length.
Additionally, after classification, this file records the determined identity of each traced curve.

Graphical interface
===================

See ui/README for instructions on how to use the graphical user interface.

      ui2.py --help

Community
=========

[Chat](https://discord.gg/Y7QJerr)

[Issue Tracker](https://github.com/nclack/whisk/issues)

