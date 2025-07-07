Janelia Whisker Tracking
========================

Fully automated tracking of single rows of whiskers in high-speed video.

The original source code repository is https://github.com/nclack/whisk/.
The [original website](http://openwiki.janelia.org/wiki/display/MyersLab/Whisker+Tracking) for this program is now accessible [here](https://wikis.janelia.org/display/WT/Whisker+Tracking).
A copy of the instructions and the tutorial are available [here](https://github.com/wanglab-neuro/whisk-dockerfile/tree/main/context/wiki).

This package has been modernized with updated build systems, Python packaging standards (PEP 517/518), and improved compatibility.

Installation
============

## From PyPI (Recommended)

This package can be installed from PyPI with:

```bash
pip install whisk-janelia
```

For the latest features with FFmpeg support:
```bash
pip install whisk-janelia[ffmpeg]
```

It is recommended to use a virtual environment:

**Using venv:**
```bash
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install whisk-janelia
```

**Using uv (faster):**
```bash
uv venv
source .venv/bin/activate  # On Windows: .venv\Scripts\activate
uv pip install whisk-janelia
```

## From Source

To install from source, clone the repository and run the following commands:

```bash
git clone https://github.com/vncntprvst/whisk.git
cd whisk

# Create and activate virtual environment
uv venv  # or: python -m venv venv
source .venv/bin/activate  # On Windows: .venv\Scripts\activate

# Install in development mode
uv pip install -e .  # or: pip install -e .
```

## Post-Installation

After installation, the whisk executables will be available as command-line tools:
- `trace` - Main whisker tracing executable
- `classify` - Whisker classification
- `measure` - Whisker measurements
- `whisker_convert` - Convert between whisker file formats
- And more...

The Python package automatically handles binary permissions and PATH setup. You can verify the installation by running:

```bash
trace --help
```

The executables are also accessible through the `whisk` Python module for programmatic use.


## Building from Source

For development or custom builds, you can build from source using modern Python packaging tools.

### Prerequisites

- CMake 2.8+
- C++ compiler (gcc 12.2+ recommended)
- FFmpeg libraries
- Python 3.7+

### Quick Build

```bash
# Clone and navigate to the repository
git clone https://github.com/vncntprvst/whisk.git
cd whisk

# Build using modern Python tools
uv build  # or: python -m build

# Install the built wheel
pip install dist/*.whl
```

### Development Build

For development, you can build and install in editable mode:

```bash
# Create virtual environment
uv venv  # or: python -m venv venv
source .venv/bin/activate

# Install in development mode
uv pip install -e .  # or: pip install -e .
```

### Manual CMake Build

The package uses a CMake-based build system for the underlying C/C++ libraries:

**For Unix-like systems (Linux/macOS):**

1. Install build dependencies:

```bash
sudo apt update
sudo apt install cmake pkg-config bison gawk
sudo apt install g++ gdb make ninja-build rsync zip
```

2. Install FFmpeg development libraries:

```bash
sudo apt install libavdevice-dev libavfilter-dev libavformat-dev libavcodec-dev libswresample-dev libswscale-dev libavutil-dev
```

3. Build the project:

```bash
mkdir build
cd build
cmake ..
make
```

For WSL systems, see: [Build and debug C++ with WSL 2](https://devblogs.microsoft.com/cppblog/build-and-debug-c-with-wsl-2-distributions-and-visual-studio-2022/)

### FFmpeg Troubleshooting

If CMake cannot find FFmpeg libraries, you may need to:

1. Set the PKG_CONFIG_PATH: `export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH`
2. Update library path: `export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH`
3. Run `sudo ldconfig` to update the dynamic linker cache

**Windows Build Instructions:**

For Windows systems using MSYS2/MinGW:  

1. Install [MSYS2](https://www.msys2.org/) and set up the development environment
2. Install required packages in MSYS2 terminal:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc
pacman -S --needed base-devel mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ffmpeg
```

3. Build the project:

```bash
mkdir build
cd build
cmake ..
ninja
```

For detailed Windows setup instructions, see the [full documentation](https://github.com/vncntprvst/whisk/).


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

