# VidMetric

A GUI application for comprehensive video quality comparison using FFmpeg's SSIM, PSNR, and VMAF metrics.

<img width="1655" height="1534" alt="image" src="https://github.com/user-attachments/assets/c27b8f81-1037-49bb-8710-6accc62905e4" />


## Features

- **File Selection**: Browse and select original and comparison media files through intuitive file dialogs
  - **Automatic File Validation**: Ensures only valid video files are selected (MP4, AVI, MKV, MOV, WMV, FLV, etc.)
  - **Resolution Display**: Shows video resolution (e.g., 1920x1080) next to each selected file
- **Flexible Time Control**: 
  - Optional start time specification (defaults to beginning)
  - Optional duration specification (defaults to entire file)
- **Real-time Progress Tracking**: Live progress bar showing encoding progress and time remaining
- **Comprehensive Quality Analysis**: Three industry-standard metrics displayed simultaneously:
  - **SSIM (Structural Similarity Index)**: Perceptual similarity measurement (0-1 scale)
  - **PSNR (Peak Signal-to-Noise Ratio)**: Quality measurement in dB (higher is better)
  - **VMAF (Video Multimethod Assessment Fusion)**: Netflix's perceptual quality metric (0-100 scale)
- **Color-Coded Results**: Visual quality indicators (green = excellent, yellow = good, orange = fair, red = poor)
- **Smart CRF Prediction**:
  - Uses `ab-av1` to automatically find the optimal CRF value for a target VMAF score
  - Supports software (libsvtav1, libx265, etc.) and hardware encoders (QSV, NVENC, AMF)
  - Estimates final file size and encoding time
- **History Tracking**:
  - Automatically saves all comparison and prediction results
  - Exports data to CSV in your Documents folder for external analysis
- **Detailed Output Log**: Complete FFmpeg output for debugging and verification
- **Wide Format Support**: MP4, AVI, MKV, MOV, WMV, FLV, WebM, and more
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
- ab-av1.exe
- All required Qt DLLs and plugins
- MinGW runtime libraries

**Note**: Recipients must have FFmpeg installed and in their PATH to use the comparison features.

## Usage

The application is divided into three tabs: **Predict**, **Verify**, and **History**.

### Verify Tab (Comparison)
Compare two videos to analyze quality differences.

1. **Select files:**
   - Click "Browse..." next to "Original Media" to select your reference video
   - Click "Browse..." next to "Comparison Media" to select the video to compare
   - The video resolution (e.g., 1920x1080) will automatically display next to each file
   - Only valid video formats will be accepted (MP4, AVI, MKV, MOV, WMV, FLV, WebM, etc.)

2. **Configure time options (optional):**
   - Check "Start Time" and enter a timestamp (HH:MM:SS) to begin comparison at a specific point
   - Check "Duration" and enter a duration (HH:MM:SS) to limit comparison length
   - Leave unchecked to compare from beginning to end

3. **Run comparison:**
   - Click "Run Comparison" button
   - View real-time progress and output in the log area
   - Quality metrics (SSIM, PSNR, and VMAF) will be displayed with color-coded results
   - All three metrics provide complementary perspectives on video quality

### Predict Tab (CRF Search)
Determine the best encoding settings for a specific quality target using `ab-av1`.

1. **Select Input Video**: Choose the source file you intend to encode.
2. **Configure Settings**:
   - **Encoder**: Select your desired codec. Supports Software (libsvtav1, libx265) and Hardware (Intel QSV, NVIDIA NVENC, AMD AMF).
   - **Preset**: Set the encoding speed/efficiency balance (e.g., 8, medium, slow).
   - **Min VMAF**: Set your target quality score (default 95).
   - **Samples**: Number of video segments to analyze (more samples = higher accuracy but slower).
3. **Run**: Click "Run CRF Search". The tool will calculate the optimal CRF value, predicted file size, and encoding time.

### History Tab
View a persistent log of all your activities.
- Displays Date/Time, Operation Type, Details, and Results.
- Automatically saves to `Documents/FFmpegComparisonTool_History.csv`.

## Understanding Quality Metrics

The application displays three complementary quality metrics in a color-coded visual format:

### SSIM (Structural Similarity Index)
Measures perceptual similarity by comparing structure, luminance, and contrast.

**Component Scores:**
- **Y (Luminance)**: Measures brightness similarity between videos
- **U/V (Chrominance)**: Measures color similarity
- **Overall**: Combined SSIM score

**Quality Scale (0-1 range):**
- **0.99-1.00**: Excellent (green) - Nearly identical, visually lossless
- **0.95-0.99**: Very Good (light green) - High quality, minimal differences
- **0.90-0.95**: Good (yellow) - Noticeable but acceptable quality
- **0.80-0.90**: Fair (orange) - Visible quality differences
- **Below 0.80**: Poor (red) - Significant quality loss

### PSNR (Peak Signal-to-Noise Ratio)
Measures quality in decibels (dB) - a traditional objective quality metric.

**Component Scores:**
- **Y, U, V**: Individual channel measurements
- **Average**: Overall PSNR score

**Quality Scale (dB):**
- **>40 dB**: Excellent (green) - Very high quality
- **35-40 dB**: Very Good (light green) - High quality
- **30-35 dB**: Good (yellow) - Acceptable quality
- **25-30 dB**: Fair (orange) - Noticeable quality loss
- **<25 dB**: Poor (red) - Significant quality degradation
- **∞ dB**: Perfect match (identical frames)

### VMAF (Video Multimethod Assessment Fusion)
Developed by Netflix, combines multiple quality metrics to predict human perception.

**Quality Scale (0-100):**
- **95-100**: Excellent (green) - Transparent quality, indistinguishable from original
- **85-95**: Very Good (light green) - Very high quality, minimal artifacts
- **75-85**: Good (yellow) - Good quality, some artifacts may be visible
- **60-75**: Fair (orange) - Acceptable quality, artifacts visible
- **<60**: Poor (red) - Poor quality, significant artifacts

## Screenshots

*(Add screenshots here showing the main interface and results display)*

## Technical Details

### FFmpeg Command Generation

The tool generates comprehensive comparison commands using filter_complex:
```bash
ffmpeg -ss 00:08:00 -t 00:03:00 -i "original.mp4" \
       -ss 00:08:00 -t 00:03:00 -i "comparison.mp4" \
       -filter_complex "[0:v]split=3[ref1][ref2][ref3];[1:v]split=3[main1][main2][main3];[main1][ref1]ssim[stats_ssim];[main2][ref2]psnr[stats_psnr];[main3][ref3]libvmaf" \
       -map "[stats_ssim]" -map "[stats_psnr]" -f null -
```

**Note**: VMAF support requires FFmpeg to be compiled with libvmaf. If VMAF is not available, the tool will still display SSIM and PSNR results.

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

### VMAF results not showing
- VMAF requires FFmpeg to be compiled with libvmaf support
- Test by running `ffmpeg -filters | grep vmaf` to check if vmaf is available
- SSIM and PSNR will still work even without VMAF support
- To install FFmpeg with VMAF on Windows, use builds from https://github.com/BtbN/FFmpeg-Builds

## License

This project is provided as-is for video comparison purposes.

## Contributing

Feel free to submit issues and enhancement requests!
