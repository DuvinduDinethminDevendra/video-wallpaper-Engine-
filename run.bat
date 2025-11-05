@echo off
cd /d "%~dp0"
REM Video Wallpaper Engine - DEBUG MODE
REM This keeps the console open to see all debug messages

echo =======================================
echo Video Wallpaper Engine - DEBUG MODE
echo =======================================
echo.

if not exist config.txt (
    echo [ERROR] config.txt not found!
    echo Please create config.txt with the full path to your video file.
    pause
    exit /b 1
)

if not exist video_wallpaper.exe (
    echo [ERROR] video_wallpaper.exe not found!
    pause
    exit /b 1
)

echo Video file: 
type config.txt
echo.
echo.
echo Starting wallpaper with debug output...
echo Keep this window open to see debug messages.
echo Close this window to stop the wallpaper.
echo.
echo =======================================
echo.

REM Run with console visible
video_wallpaper.exe

echo.
echo =======================================
echo Wallpaper stopped.
echo =======================================
pause