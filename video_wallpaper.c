/*
 * Video Wallpaper Engine for Windows 10/11
 * Uses libVLC for video playback and Windows API for wallpaper integration
 * 
 * MIT License - Copyright (c) 2025 B.D.D.Devendra
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vlc/vlc.h>

#define CONFIG_FILE "config.txt"
#define MAX_PATH_LEN 512

// Global variables
HWND g_hwnd = NULL;
libvlc_instance_t *g_vlc = NULL;
libvlc_media_player_t *g_mp = NULL;
libvlc_media_t *g_media = NULL;

// Function to read video path from config file
int read_config(char *video_path, size_t max_len) {
    FILE *fp = fopen(CONFIG_FILE, "r");
    if (!fp) {
        fprintf(stderr, "Error: Could not open config file '%s'\n", CONFIG_FILE);
        fprintf(stderr, "Please create a config.txt file with the video file path\n");
        return 0;
    }
    
    if (fgets(video_path, max_len, fp) == NULL) {
        fprintf(stderr, "Error: Could not read video path from config file\n");
        fclose(fp);
        return 0;
    }
    
    // Remove trailing newline
    size_t len = strlen(video_path);
    if (len > 0 && video_path[len-1] == '\n') {
        video_path[len-1] = '\0';
    }
    if (len > 1 && video_path[len-2] == '\r') {
        video_path[len-2] = '\0';
    }
    
    fclose(fp);
    return 1;
}

// Callback function for EnumWindows
BOOL CALLBACK EnumWindowsProc(HWND tophandle, LPARAM topparamhandle) {
    HWND shelldll_defview = FindWindowEx(tophandle, NULL, "SHELLDLL_DefView", NULL);
    if (shelldll_defview != NULL) {
        HWND *pWorkerw = (HWND*)topparamhandle;
        *pWorkerw = FindWindowEx(NULL, tophandle, "WorkerW", NULL);
    }
    return TRUE;
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            // Paint black background
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Create wallpaper window
HWND create_wallpaper_window(HINSTANCE hInstance) {
    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "VideoWallpaperClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    
    if (!RegisterClassEx(&wc)) {
        fprintf(stderr, "Error: Failed to register window class\n");
        return NULL;
    }
    
    // Get screen dimensions
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    
    // Create window
    HWND hwnd = CreateWindowEx(
        0,
        "VideoWallpaperClass",
        "Video Wallpaper",
        WS_POPUP | WS_VISIBLE,
        0, 0,
        screen_width, screen_height,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hwnd) {
        fprintf(stderr, "Error: Failed to create window\n");
        return NULL;
    }
    
    // Find the "Progman" window
    HWND progman = FindWindow("Progman", NULL);
    
    // Send message to spawn a WorkerW window
    SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, NULL);
    
    // Find the WorkerW window that was created
    HWND workerw = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&workerw);
    
    // Set our window as child of WorkerW to make it part of the wallpaper
    if (workerw != NULL) {
        SetParent(hwnd, workerw);
    } else {
        // Fallback: Set as child of Progman
        SetParent(hwnd, progman);
    }
    
    // Make sure window is positioned correctly
    SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, screen_width, screen_height, 
                 SWP_SHOWWINDOW);
    
    return hwnd;
}

// Initialize VLC and start playback
int init_vlc_player(HWND hwnd, const char *video_path) {
    // Create VLC instance
    const char *vlc_args[] = {
        "--no-xlib",
        "--loop",
        "--no-video-title-show",
        "--no-audio"
    };
    
    g_vlc = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
    if (!g_vlc) {
        fprintf(stderr, "Error: Failed to create VLC instance\n");
        return 0;
    }
    
    // Create media from file
    g_media = libvlc_media_new_path(g_vlc, video_path);
    if (!g_media) {
        fprintf(stderr, "Error: Failed to create media from path: %s\n", video_path);
        libvlc_release(g_vlc);
        return 0;
    }
    
    // Create media player
    g_mp = libvlc_media_player_new_from_media(g_media);
    if (!g_mp) {
        fprintf(stderr, "Error: Failed to create media player\n");
        libvlc_media_release(g_media);
        libvlc_release(g_vlc);
        return 0;
    }
    
    // Set the window for video output
    libvlc_media_player_set_hwnd(g_mp, hwnd);
    
    // Start playback
    if (libvlc_media_player_play(g_mp) == -1) {
        fprintf(stderr, "Error: Failed to start playback\n");
        libvlc_media_player_release(g_mp);
        libvlc_media_release(g_media);
        libvlc_release(g_vlc);
        return 0;
    }
    
    printf("Video playback started successfully\n");
    return 1;
}

// Cleanup VLC resources
void cleanup_vlc() {
    if (g_mp) {
        libvlc_media_player_stop(g_mp);
        libvlc_media_player_release(g_mp);
        g_mp = NULL;
    }
    if (g_media) {
        libvlc_media_release(g_media);
        g_media = NULL;
    }
    if (g_vlc) {
        libvlc_release(g_vlc);
        g_vlc = NULL;
    }
}

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    char video_path[MAX_PATH_LEN];
    
    // Read configuration
    if (!read_config(video_path, MAX_PATH_LEN)) {
        MessageBox(NULL, "Failed to read config file. Please create config.txt with video path.", 
                   "Error", MB_ICONERROR);
        return 1;
    }
    
    printf("Video Wallpaper Engine starting...\n");
    printf("Video file: %s\n", video_path);
    
    // Create wallpaper window
    g_hwnd = create_wallpaper_window(hInstance);
    if (!g_hwnd) {
        MessageBox(NULL, "Failed to create wallpaper window", "Error", MB_ICONERROR);
        return 1;
    }
    
    // Initialize VLC and start playback
    if (!init_vlc_player(g_hwnd, video_path)) {
        MessageBox(NULL, "Failed to initialize video player", "Error", MB_ICONERROR);
        DestroyWindow(g_hwnd);
        return 1;
    }
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    cleanup_vlc();
    
    return (int)msg.wParam;
}
