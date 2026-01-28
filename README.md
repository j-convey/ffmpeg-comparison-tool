# FFmpeg Video Comparison Tool

A GUI application for comparing two video files using FFmpeg's SSIM (Structural Similarity Index) filter.

## Features

- **File Selection**: Browse and select original and comparison media files through intuitive file dialogs
- **Flexible Time Control**: 
  - Optional start time specification (defaults to beginning)
  - Optional duration specification (defaults to entire file)
- **Real-time Progress Tracking**: Live progress bar showing encoding progress and time remaining
- **Visual Results Display**: Color-coded SSIM results with easy-to-read scores:
  - Y (Luminance) similarity score
  - U/V (Chrominance) color similarity scores
  - Overall quality score with dB values
  - Color-coded display (green = excellent, yellow = good, orange = fair, red = poor)
- **Detailed Output Log**: Complete FFmpeg output for debugging and verification
- **Wide Format Support**: MP4, AVI, MKV, MOV, WMV, FLV, and more
- **Professional UI**: Modern Qt6-based interface with organized layout

## Prerequisites

### Required Software

1. **FFmpeg** - Must be installed and available in your system PATH
   - Download from: https://ffmpeg.org/download.html
   - Windows: Add FFmpeg bin folder to PATH environment variable

2. **Qt6** - Required for building the application
   - Download from: https://www.qt.io/download-open-source-software
   - Install Qt 6.x with the following components:
     - Desktop development with MinGW or MSVC
     - Qt Core, Qt Widgets

3. **CMake** - Version 3.16 or higher
   - Download from: https://cmake.org/download/

4. **C++ Compiler**
   - Windows: MinGW (included with Qt) or Visual Studio 2019/2022
   - Linux: GCC 7+ or Clang 5+
   - macOS: Xcode Command Line Tools

## Building the Application

### Windows (Qt with MinGW)

```powershell
# Create build directory
mkdir build
cd build

# Configure with CMake (adjust Qt path as needed)
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/mingw_64"

# Build
cmake --build .

# Run the application
./bin/FFmpegComparisonTool.exe
```

### Windows (Qt with Visual Studio)

```powershell
# Create build directory
mkdir build
cd build

# Configure with CMake (adjust Qt path as needed)
cmake .. -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64"

# Build
cmake --build . --config Release

# Run the application
./bin/Release/FFmpegComparisonTool.exe
```

### Linux

```bash
# Install Qt6 (Ubuntu/Debian)
sudo apt-get install qt6-base-dev

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build .

# Run the application
./bin/FFmpegComparisonTool
```

### macOS

```bash
# Install Qt6 via Homebrew
brew install qt@6

# Create build directory
mkdir build
cd build

# Configure and build
cmake .. -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build .

# Run the application
./bin/FFmpegComparisonTool
```

## Deployment (Windows)

To create a distributable version:

```powershell
# Build the application first
cd build
cmake --build .

# Create a ZIP file with all dependencies
cd ..
Compress-Archive -Path "build\bin\*" -DestinationPath "FFmpegComparisonTool-Release.zip" -Force
```

The ZIP file contains:
- FFmpegComparisonTool.exe
- All required Qt DLLs and plugins
- MinGW runtime libraries

**Note**: Recipients must have FFmpeg installed and in their PATH to use the comparison features.

## Usage

1. **Launch the application**

2. **Select files:**
   - Click "Browse..." next to "Original Media" to select your reference video
   - Click "Browse..." next to "Comparison Media" to select the video to compare

3. **Configure time options (optional):**
   - Check "Start Time" and enter a timestamp (HH:MM:SS) to begin comparison at a specific point
   - Check "Duration" and enter a duration (HH:MM:SS) to limit comparison length
   - Leave unchecked to compare from beginning to end

4. **Run comparison:**
   - Click "Run Comparison" button
   - View real-time output in the text area below
   - SSIM results will be displayed in the output

## Understanding SSIM Results

The application displays SSIM (Structural Similarity Index) scores in a color-coded visual format:

### Score Interpretation
- **Y (Luminance)**: Measures brightness similarity between videos
- **U/V (Chrominance)**: Measures color similarity
- **Overall**: Combined quality score

### Quality Scale (0-1 range)
- **0.99-1.00**: Excellent (green) - Nearly identical, visually lossless
- **0.95-0.99**: Very Good (light green) - High quality, minimal differences
- **0.90-0.95**: Good (yellow) - Noticeable but acceptable quality
- **0.80-0.90**: Fair (orange) - Visible quality differences
- **Below 0.80**: Poor (red) - Significant quality loss

### dB Values
- Higher dB values indicate better quality
- ∞ dB (infinity) = perfect match (identical frames)

## Screenshots

*(Add screenshots here showing the main interface and results display)*

## Technical Details

### FFmpeg Command Generation

The tool generates commands similar to:
```bash
ffmpeg -ss 00:08:00 -t 00:03:00 -i "original.mp4" -ss 00:08:00 -t 00:03:00 -i "comparison.mp4" -lavfi "ssim" -f null -
```

### Architecture
- **Framework**: Qt6 with C++17
- **Build System**: CMake 3.16+
- **Video Processing**: FFmpeg (external dependency)
- **Platform**: Cross-platform (Windows, Linux, macOS)

## Troubleshooting

### "Failed to start FFmpeg"
- Ensure FFmpeg is installed and in your system PATH
- Test by running `ffmpeg -version` in terminal/command prompt

### Build errors related to Qt
- Verify Qt6 is installed correctly
- Check that CMAKE_PREFIX_PATH points to your Qt installation
- Ensure all required Qt components (Core, Widgets) are installed

### Application doesn't launch
- Check that all Qt DLLs are accessible (Windows)
- On Linux, verify Qt libraries are in LD_LIBRARY_PATH
- On macOS, use `otool -L` to check library dependencies

## License

This project is provided as-is for video comparison purposes.

## Contributing

Feel free to submit issues and enhancement requests!
