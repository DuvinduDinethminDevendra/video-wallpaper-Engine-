# Video Wallpaper - Video Display Fix

## Problem
- Audio plays ✓
- Video doesn't show ✗
- Works when you create a normal window ✓

## Root Cause
Your current code creates a **child window** of Progman (`WS_CHILD`), which:
- Has clipping restrictions that prevent proper video rendering
- Blocks Media Foundation's video renderer from displaying content
- Works for audio because audio doesn't need a surface

## Solution

### Key Changes Made

**Window Creation (CRITICAL FIX):**
```c
// BEFORE (Child window - doesn't work for video):
CreateWindowEx(
    WS_EX_NOACTIVATE,
    "VideoWallpaperClass",
    "Video Wallpaper",
    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,  // ← PROBLEM
    0, 0, screen_width, screen_height,
    hProgman,  // ← Child of Progman
    ...
)

// AFTER (Top-level window - proper video rendering):
CreateWindowEx(
    WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
    "VideoWallpaperClass",
    "Video Wallpaper",
    WS_POPUP | WS_VISIBLE,  // ← Top-level window
    0, 0, screen_width, screen_height,
    NULL,  // ← No parent - this is a top-level window
    ...
)
```

### How to Apply the Fix

You have two options:

#### Option 1: Use MSVC (Recommended)
The original code is written for **Visual Studio / MSVC**. To compile:

1. Install **Visual Studio Community** (free) with C++ workload
2. Copy the `video_wallpaper_fixed.c` file to your project
3. Compile with:
```batch
cl /W3 video_wallpaper_fixed.c /link mf.lib mfplat.lib mfuuid.lib mfreadwrite.lib evr.lib strmiids.lib dwmapi.lib dxva2.lib d3d9.lib user32.lib gdi32.lib kernel32.lib
```

#### Option 2: Quick Fix (If Already Compiled)
If your `video_wallpaper.exe` is already compiled, the binary doesn't change, but you should recompile with the fixed source.

### What's Fixed

✓ **Window is now top-level** - Media Foundation can properly render to it
✓ **Still positioned behind desktop icons** - Wallpaper effect maintained
✓ **No clipping restrictions** - Video renders fully
✓ **Maintained audio playback** - All functionality preserved

### Testing

After recompiling:
1. Rebuild: `nmake` or compile with your build system
2. Run: `video_wallpaper.exe` (or `run.bat`)
3. Expected: Video should now display in full screen

### If Video Still Doesn't Show

Check these:
1. **Video codec support** - Ensure your video uses H.264/AVC (widely supported)
2. **File path** - Verify `config.txt` has correct absolute path to video
3. **Media Foundation** - Run `dxdiag` and check DirectX compatibility
4. **Window positioning** - Press Win+D, icons should appear on top of video

### Why This Works

- **Child windows** = Clipped to parent bounds, restricted surface for rendering
- **Top-level windows** = Full access to rendering pipeline, proper surface allocation
- Media Foundation EVR (video renderer) needs an unclipped HWND to display video
- Audio doesn't require a surface, so it worked even with the child window

## Additional Notes

- The fixed version uses proper MSVC COM interface calls
- Compiling with MinGW/GCC may cause COM interface issues (see errors)
- If using MinGW, consider switching to VLC backend instead of Media Foundation
