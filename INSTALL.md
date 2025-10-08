# Installation and Setup Guide

## Quick Start Guide

### 1. Install Prerequisites

#### Install MinGW-w64 (GCC Compiler)

**Option A: Using MSYS2 (Recommended)**
1. Download MSYS2 from https://www.msys2.org/
2. Install MSYS2 to default location
3. Open MSYS2 MINGW64 terminal
4. Update package database:
   ```bash
   pacman -Syu
   ```
5. Install gcc and make:
   ```bash
   pacman -S mingw-w64-x86_64-gcc make
   ```

**Option B: Standalone MinGW-w64**
1. Download from https://www.mingw-w64.org/downloads/
2. Install to a directory (e.g., `C:\mingw64`)
3. Add `C:\mingw64\bin` to your system PATH

#### Install VLC Media Player

1. Download VLC from https://www.videolan.org/vlc/
2. **Important**: Download the installer version (not Microsoft Store version)
3. Install to default location: `C:\Program Files\VideoLAN\VLC`
4. During installation, make sure to install the SDK components

### 2. Download or Clone This Repository

```bash
git clone https://github.com/DuvinduDinethminDevendra/video-wallpaper-Engine-.git
cd video-wallpaper-Engine-
```

### 3. Configure Video Path

Create `config.txt` from the example:
```bash
copy config.txt.example config.txt
```

Edit `config.txt` with your video file path:
```
C:\Videos\my-wallpaper.mp4
```

### 4. Build the Application

#### Using MSYS2:
```bash
make
```

#### Using Command Prompt with MinGW:
```cmd
mingw32-make
```

If VLC is installed in a non-default location, edit `Makefile` first to update the paths.

### 5. Run the Application

#### Option A: Use the batch file
```cmd
run.bat
```

#### Option B: Run directly
```cmd
video_wallpaper.exe
```

## Verifying Installation

### Check if VLC SDK is available

The libVLC SDK should be in one of these locations:
- `C:\Program Files\VideoLAN\VLC\sdk\`
- `C:\Program Files (x86)\VideoLAN\VLC\sdk\`

If not found, you may need to:
1. Reinstall VLC and ensure SDK is included
2. Or download VLC development files separately from the VLC website

### Check if MinGW is in PATH

Open Command Prompt and run:
```cmd
gcc --version
```

You should see GCC version information.

## Troubleshooting Build Issues

### "gcc: command not found"
- MinGW is not in your PATH
- Solution: Add MinGW bin directory to system PATH or use MSYS2 terminal

### "vlc/vlc.h: No such file or directory"
- VLC SDK not found
- Solution: Update VLC_INCLUDE path in Makefile to point to your VLC installation

### "cannot find -lvlc"
- VLC library not found
- Solution: Update VLC_LIB path in Makefile to point to your VLC installation

### Linker errors with libvlc
- Make sure both `libvlc.dll` and `libvlccore.dll` are accessible
- These are typically in `C:\Program Files\VideoLAN\VLC\`
- Copy them to the same directory as `video_wallpaper.exe` or add VLC directory to PATH

## Running at Windows Startup (Optional)

To run the video wallpaper automatically when Windows starts:

1. Press `Win + R` and type `shell:startup`
2. Create a shortcut to `video_wallpaper.exe` in the Startup folder
3. Right-click the shortcut → Properties
4. Set "Start in" to the directory containing your exe and config.txt

## Updating to a New Video

1. Edit `config.txt` with new video path
2. Kill the current video_wallpaper.exe process
3. Run video_wallpaper.exe again

Or use this command to restart:
```cmd
taskkill /IM video_wallpaper.exe /F && video_wallpaper.exe
```

## System Requirements

- **OS**: Windows 10 or Windows 11
- **RAM**: 2GB minimum (4GB+ recommended for HD videos)
- **CPU**: Any modern x64 processor
- **Video**: Any video supported by VLC
  - Recommended: H.264 MP4 for best performance
  - Resolution: Match your screen resolution for best quality

## Performance Tips

1. Use videos encoded with H.264 for best performance
2. Match video resolution to your screen resolution
3. Lower bitrate videos use less CPU
4. If video stutters, try:
   - Converting video to lower resolution
   - Using a more efficient codec (H.264)
   - Closing other applications
