Janelia Whisker tracking
========================
Fully automated tracking of single rows of whiskers in high-speed video.  

The original source code repository is https://github.com/nclack/whisk/.    
The [associated website](http://openwiki.janelia.org/wiki/display/MyersLab/Whisker+Tracking) for this program is not accessible anymore outside of Janelia. 
A copy of the instructions and the tutorial are available [here](https://github.com/wanglab-neuro/whisk-dockerfile/tree/main/context/wiki).

Downloading
===========
Pre-built binaries are available on [this forked repository](https://github.com/vncntprvst/whisk/tree/master/whisk/bin).  
Binary and library file can simply be installed from Pypi, with `pip install whisk-janelia`.  
See also the [README on the whisk-dockerfile repo](https://github.com/wanglab-neuro/whisk-dockerfile), in `whisk-base` section.  

Building
========
Updated build files are found [here](https://github.com/vncntprvst/whisk/).  
This package uses a [Cmake][] based build system.  The basic build steps are similar under Windows, OS X, and Linux.  
The build requirements are `cmake` `gcc-12.2+` `ffmpeg`.

For Unix-y systems:

1. Install CMake 2.8+, a C++ compiler and other build tools.
```
sudo apt update
sudo cmake pkg-config bison gawk
sudo apt install g++ gdb make ninja-build rsync zip
```
2. Install ffmpeg libraries
```
sudo apt install libavdevice-dev libavfilter-dev libavformat-dev libavcodec-dev libswresample-dev libswscale-dev libavutil-dev
```
If cmake still fails because of it can't find ffmpeg libraries `libavdevice;libavfilter;libavformat;libavcodec;libswresample;libswscale;libavutil`, you need to have them somewhere, and make it find it, e.g. `export PKG_CONFIG_PATH=/home/jovyan/ffmpeg_build/lib/pkgconfig`.

3. Start in the root of the source directory (in a terminal).
6. Type these commands (or use [VSCode's CMake extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) to configure and build):
```
    mkdir build
    cd build
    cmake ..
    make
```

For WSL systems, this page may be helpful:
https://devblogs.microsoft.com/cppblog/build-and-debug-c-with-wsl-2-distributions-and-visual-studio-2022/

For Windows systems (ignore VS Code instructions if not using it):

1. Install [Visual Studio Code](https://code.visualstudio.com/download).
2. Install the C/C++ extension for VS Code. 
3. Install Mingw-w64 via [MSYS2](https://www.msys2.org/). Prefer installing MSYS2 in a directory without space (e.g., not C:/Program Files/ ...). That will save you a lot of headaches.
4. Launch a MSYS2 terminal for the UCRT64 environment as an administrator and install the following:  
* Install gcc as mentioned in [MSYS2 installation page](https://www.msys2.org/): `pacman -S mingw-w64-ucrt-x86_64-gcc`   
* Install the Mingw-w64 toolchain: `pacman -S --needed base-devel mingw-w64-x86_64-toolchain`
* Install cmake and generators (ninja ... make is installed with the toolchain above, ): `pacman -S mingw-w64-x86_64-cmake`
* Install the FFmpeg libraries for the 64-bit MinGW toolchain: 
     To install the most recent version instead, run `pacman -S mingw-w64-x86_64-ffmpeg`.  
     If using python (see packages `whisk-janelia` and `whiskiwrap`), you need to ensure compatibility between the ffmpeg version used to build whisk (currently, version 6.0) and the ffmpeg library files that python will call with the package `ctypes`. Tarballs are available for download on [MSYS2's package page](https://packages.msys2.org/package/mingw-w64-x86_64-ffmpeg), or run `wget https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-ffmpeg-6.0-7-any.pkg.tar.zst`. Then, install the package with  `pacman -U mingw-w64-x86_64-ffmpeg-6.0-7-any.pkg.tar.zst`.    
* Update the PKG_CONFIG_PATH environment variable to include the path to the FFmpeg library .pc files. In the MSYS2 terminal, run: `export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/mingw64/lib/pkgconfig`, and add it to `~/.bashrc` as well to make this change permanent.
5. Open a terminal, navigate to the `whisk` repository directory.
6. Run the following commands:  
```
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

