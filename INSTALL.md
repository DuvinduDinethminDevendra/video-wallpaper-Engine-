---

## 📦 `INSTALL.md` (Updated)

# Installation and Setup Guide

## Quick Start Guide

### 1. Install a C Compiler

The project no longer requires VLC or a specific environment like MSYS2. You only need a C compiler for Windows.

**Option A: Visual Studio 2022 Build Tools (Recommended)**
1.  Download the "Build Tools for Visual Studio 2022" from [the official website](https://visualstudio.microsoft.com/visuals-downloads/#build-tools-for-visual-studio-2022).
2.  Run the installer and select the **"Desktop development with C++"** workload.
3.  After installation, use the **"Developer Command Prompt for VS 2022"** from your Start Menu for compiling.

**Option B: MinGW-w64**
1.  Download and install a MinGW-w64 toolchain (for example, from [MSYS2](https://www.msys2.org/)).
2.  Ensure `gcc.exe` is in your system's PATH.

### 2. Get the Source Code

Download or clone the project files to a folder on your computer.

### 3. Build the Application

1.  Open your chosen command prompt (Developer Prompt for VS or a standard Command Prompt/PowerShell for MinGW).
2.  Navigate to the folder containing `video_wallpaper.c`.
3.  Compile the source code.

    * **If using Visual Studio Compiler:**
        ```cmd
        cl video_wallpaper.c /link user32.lib gdi32.lib mf.lib mfplat.lib mfuuid.lib ole32.lib
        ```

    * **If using MinGW GCC:**
        ```bash
        gcc video_wallpaper.c -o video_wallpaper.exe -luser32 -lgdi32 -lmf -lmfplat -lmfuuid -lole32 -mwindows
        ```

This will create the `video_wallpaper.exe` file.

### 4. Configure and Run

1.  Create a `config.txt` file in the same folder.
2.  Add the full path to your video file inside `config.txt`, for example: `D:\Videos\MyWallpaper.mp4`.
3.  Double-click `run.bat` or `video_wallpaper.exe` to start.

## Troubleshooting Build Issues

### "cl' or 'gcc' is not recognized..."
- Your compiler is not in your system's PATH.
- **Solution**: If you installed Visual Studio Build Tools, make sure you are using the **"Developer Command Prompt for VS 2022"**, as it automatically sets up the correct paths for you. If using MinGW, add its `bin` folder to your PATH environment variable.

### Linker Errors (e.g., "unresolved external symbol")
- You forgot to link the necessary libraries.
- **Solution**: Ensure you are including all the libraries at the end of the compile command (`user32.lib`, `gdi32.lib`, `mf.lib`, etc.).

## Running at Windows Startup (Optional)

1.  Press `Win + R`, type `shell:startup`, and press Enter. This opens the Startup folder.
2.  Create a shortcut to `video_wallpaper.exe` inside this folder.
3.  Right-click the shortcut and go to **Properties**.
4.  In the **"Start in"** field, paste the path to the folder where `video_wallpaper.exe` and `config.txt` are located. This is crucial for the program to find its configuration file.