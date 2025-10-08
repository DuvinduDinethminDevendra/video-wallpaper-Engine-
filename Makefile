# Makefile for Video Wallpaper Engine
# Requires: MinGW-w64 (gcc) and libVLC

CC = gcc
CFLAGS = -Wall -O2 -mwindows
LIBS = -lvlc -luser32 -lgdi32

# VLC include and library paths (adjust if needed)
VLC_INCLUDE = -I"C:/Program Files/VideoLAN/VLC/sdk/include"
VLC_LIB = -L"C:/Program Files/VideoLAN/VLC/sdk/lib"

TARGET = video_wallpaper.exe
SOURCE = video_wallpaper.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(VLC_INCLUDE) -o $(TARGET) $(SOURCE) $(VLC_LIB) $(LIBS)

clean:
	del /Q $(TARGET) 2>nul || true

run: $(TARGET)
	$(TARGET)

.PHONY: all clean run
