# video-wallpaper-Engine-
Custom video wallpaper engine for Windows 10/11 using C and libVLC for my personal uses

## Features

- 🎥 Play video files as your Windows desktop wallpaper
- 🔄 Automatic looping of video playback
- 🔇 Silent playback (no audio)
- 🖥️ Full screen coverage with proper wallpaper integration
- ⚡ Lightweight C implementation using libVLC

## Prerequisites

### Required Software

1. **MinGW-w64** (GCC compiler for Windows)
   - Download from: https://www.mingw-w64.org/
   - Or use MSYS2: https://www.msys2.org/

2. **VLC Media Player** (includes libVLC SDK)
   - Download from: https://www.videolan.org/vlc/
   - Make sure to install the full version (not the Windows Store version)

## Building

### Step 1: Install Dependencies

1. Install MinGW-w64 or MSYS2 with GCC
2. Install VLC Media Player to the default location (`C:\Program Files\VideoLAN\VLC`)

### Step 2: Configure Build

If VLC is installed in a different location, edit the `Makefile` and update these lines:
```makefile
VLC_INCLUDE = -I"C:/Program Files/VideoLAN/VLC/sdk/include"
VLC_LIB = -L"C:/Program Files/VideoLAN/VLC/sdk/lib"
```

### Step 3: Compile

Open a terminal (Command Prompt or MSYS2) in the project directory and run:
```bash
make
```

This will create `video_wallpaper.exe`

To clean build artifacts:
```bash
make clean
```

## Usage

### Step 1: Create Configuration File

Create a file named `config.txt` in the same directory as `video_wallpaper.exe`:
```
C:\Videos\your-video.mp4
```

You can use the example configuration as a template:
```bash
copy config.txt.example config.txt
```

Then edit `config.txt` with the full path to your video file.

### Step 2: Run the Application

Double-click `video_wallpaper.exe` or run from command line:
```bash
video_wallpaper.exe
```

The video should now be playing as your desktop wallpaper!

### Step 3: Stop the Application

To stop the video wallpaper:
- Press `Ctrl+Alt+Delete` and open Task Manager
- Find "video_wallpaper.exe" under processes
- Right-click and select "End Task"

Or use the command line:
```bash
taskkill /IM video_wallpaper.exe /F
```

## Supported Video Formats

Since this uses libVLC, it supports all formats that VLC supports, including:
- MP4 (H.264, H.265)
- AVI
- MKV
- WebM
- MOV
- And many more...

## Troubleshooting

### "Failed to create VLC instance"
- Make sure VLC is properly installed
- Check that the VLC SDK paths in the Makefile are correct
- Ensure `libvlc.dll` and `libvlccore.dll` are in your PATH or in the same directory as the executable

### "Failed to read config file"
- Ensure `config.txt` exists in the same directory as the executable
- Check that the file contains a valid video file path

### Video not displaying
- Verify the video file path in `config.txt` is correct
- Try using forward slashes or double backslashes in the path
- Make sure the video file format is supported by VLC

### Window appears but video doesn't play
- Check that the video file is not corrupted
- Try playing the video in VLC first to verify it works
- Ensure you have proper video codecs installed

## Technical Details

### How it Works

1. **Wallpaper Integration**: The application uses the Windows `Progman` and `WorkerW` windows to embed itself into the desktop wallpaper layer
2. **Video Playback**: libVLC handles all video decoding and rendering directly to the window handle
3. **Configuration**: Simple text-based configuration for easy customization

### Architecture

- `video_wallpaper.c`: Main application code
  - Window creation and management
  - libVLC integration
  - Configuration file parsing
  - Windows API wallpaper integration

## License

MIT License - See LICENSE file for details

## Author

B.D.D.Devendra

## Contributing

This is a personal project, but feel free to fork and modify for your own use!
