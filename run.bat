@echo off
REM Simple batch file to run the video wallpaper engine
REM Make sure config.txt exists before running

if not exist config.txt (
    echo Error: config.txt not found!
    echo Please create config.txt with the path to your video file.
    echo Example: C:\Videos\wallpaper.mp4
    pause
    exit /b 1
)

echo Starting Video Wallpaper Engine...
video_wallpaper.exe
