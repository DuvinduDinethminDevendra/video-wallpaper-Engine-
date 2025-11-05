/*
 * Video Wallpaper Engine - AGGRESSIVE VISIBILITY METHOD
 * Forces window visibility with periodic refreshes
 *
 * MIT License - Copyright (c) 2025 B.D.D.Devendra
 */

#include <windows.h>
#include <dwmapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <evr.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "evr.lib")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "dwmapi.lib")

#define CONFIG_FILE "config.txt"
#define MAX_PATH_LEN 512
#define REFRESH_TIMER_ID 1

 // --- Globals ---
HWND g_hwnd = NULL;
HWND g_hShellDefView = NULL;
IMFMediaSession* g_pSession = NULL;
IMFMediaSource* g_pSource = NULL;
IMFTopology* g_pTopology = NULL;
IMFVideoDisplayControl* g_pVideoDisplay = NULL;
IMFMediaEventGenerator* g_pEventGenerator = NULL;
BOOL g_bPlaying = FALSE;

// --- Function Prototypes ---
int read_config(char*, size_t);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HWND create_wallpaper_window(HINSTANCE);
HRESULT CreateMediaSource(const WCHAR*, IMFMediaSource**);
HRESULT CreatePlaybackTopology(IMFMediaSource*, IMFTopology**, HWND);
void cleanup_media_foundation();
void force_window_refresh();

// --- Force window refresh ---
void force_window_refresh() {
    if (g_hwnd && g_pVideoDisplay) {
        // Force repaint
        InvalidateRect(g_hwnd, NULL, FALSE);
        g_pVideoDisplay->RepaintVideo();

        // Ensure proper Z-order
        if (g_hShellDefView) {
            SetWindowPos(g_hwnd, g_hShellDefView, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);
        }
    }
}

// --- Handle media events ---
HRESULT HandleMediaEvent(IMFMediaEvent* pEvent) {
    if (!pEvent) return E_POINTER;

    MediaEventType type;
    HRESULT hr = pEvent->GetType(&type);
    if (FAILED(hr)) return hr;

    if (type == MESessionEnded) {
        printf("Looping video...\n");
        PROPVARIANT varStart;
        PropVariantInit(&varStart);
        hr = g_pSession->Start(&GUID_NULL, &varStart);
        PropVariantClear(&varStart);
    }
    return S_OK;
}

// --- Initialize Media Foundation player ---
int init_media_foundation_player(HWND hwnd, const char* video_path) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return 0;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    if (FAILED(hr)) { CoUninitialize(); return 0; }

    int wide_len = MultiByteToWideChar(CP_UTF8, 0, video_path, -1, NULL, 0);
    WCHAR* wszURL = (WCHAR*)malloc(wide_len * sizeof(WCHAR));
    if (!wszURL) { MFShutdown(); CoUninitialize(); return 0; }
    MultiByteToWideChar(CP_UTF8, 0, video_path, -1, wszURL, wide_len);

    printf("Loading: %s\n", video_path);
    hr = CreateMediaSource(wszURL, &g_pSource);
    free(wszURL);
    if (FAILED(hr)) goto fail;

    hr = MFCreateMediaSession(NULL, &g_pSession);
    if (FAILED(hr)) goto fail;

    hr = g_pSession->QueryInterface(IID_PPV_ARGS(&g_pEventGenerator));

    hr = CreatePlaybackTopology(g_pSource, &g_pTopology, hwnd);
    if (FAILED(hr)) goto fail;

    hr = g_pSession->SetTopology(0, g_pTopology);
    if (FAILED(hr)) goto fail;

    return 1;

fail:
    cleanup_media_foundation();
    return 0;
}

// --- MAIN ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }

    printf("\n=== Video Wallpaper - Aggressive Visibility ===\n\n");

    char video_path[MAX_PATH_LEN];
    if (!read_config(video_path, MAX_PATH_LEN)) {
        MessageBox(NULL, "Failed to read config.txt", "Error", MB_ICONERROR);
        return 1;
    }

    g_hwnd = create_wallpaper_window(hInstance);
    if (!g_hwnd) {
        MessageBox(NULL, "Failed to create window", "Error", MB_ICONERROR);
        return 1;
    }

    if (!init_media_foundation_player(g_hwnd, video_path)) {
        MessageBox(NULL, "Failed to initialize player", "Error", MB_ICONERROR);
        DestroyWindow(g_hwnd);
        return 1;
    }

    // Set up refresh timer (every 100ms)
    SetTimer(g_hwnd, REFRESH_TIMER_ID, 100, NULL);

    MSG msg = { 0 };
    BOOL bPlayerReady = FALSE;

    printf("Running... Press Ctrl+C to stop.\n\n");

    while (TRUE) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else if (g_pEventGenerator) {
            IMFMediaEvent* pEvent = NULL;
            HRESULT hrEvent = g_pEventGenerator->GetEvent(MF_EVENT_FLAG_NO_WAIT, &pEvent);

            if (SUCCEEDED(hrEvent)) {
                MediaEventType met;
                pEvent->GetType(&met);

                if (met == MESessionTopologyStatus && !bPlayerReady) {
                    UINT32 status;
                    pEvent->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &status);

                    if (status == MF_TOPOSTATUS_READY) {
                        printf("Starting playback...\n");

                        HRESULT hrService = MFGetService(g_pSession, MR_VIDEO_RENDER_SERVICE,
                            IID_PPV_ARGS(&g_pVideoDisplay));

                        if (SUCCEEDED(hrService) && g_pVideoDisplay) {
                            RECT rc;
                            GetClientRect(g_hwnd, &rc);

                            g_pVideoDisplay->SetVideoWindow(g_hwnd);
                            g_pVideoDisplay->SetVideoPosition(NULL, &rc);

                            // Try different aspect ratio modes
                            g_pVideoDisplay->SetAspectRatioMode(MFVideoARMode_None);

                            SIZE videoSize, aspectRatio;
                            if (SUCCEEDED(g_pVideoDisplay->GetNativeVideoSize(&videoSize, &aspectRatio))) {
                                printf("Video: %dx%d\n", videoSize.cx, videoSize.cy);
                            }

                            PROPVARIANT varStart;
                            PropVariantInit(&varStart);
                            g_pSession->Start(&GUID_NULL, &varStart);
                            PropVariantClear(&varStart);

                            g_bPlaying = TRUE;
                            printf("SUCCESS! Video is playing.\n");
                            printf("\nTroubleshooting:\n");
                            printf("  - Press Win+D to show desktop\n");
                            printf("  - Hide desktop icons (right-click > View)\n");
                            printf("  - The video refreshes every 100ms\n\n");
                        }

                        bPlayerReady = TRUE;
                    }
                }
                else {
                    HandleMediaEvent(pEvent);
                }
                pEvent->Release();
            }
            else {
                Sleep(5);
            }
        }
        else {
            Sleep(5);
        }
    }

    KillTimer(g_hwnd, REFRESH_TIMER_ID);
    cleanup_media_foundation();
    return (int)msg.wParam;
}

// --- Cleanup ---
void cleanup_media_foundation() {
    if (g_pSession) {
        g_pSession->Stop();
        g_pSession->Close();
        g_pSession->Release();
        g_pSession = NULL;
    }
    if (g_pVideoDisplay) { g_pVideoDisplay->Release(); g_pVideoDisplay = NULL; }
    if (g_pTopology) { g_pTopology->Release(); g_pTopology = NULL; }
    if (g_pSource) { g_pSource->Release(); g_pSource = NULL; }
    if (g_pEventGenerator) { g_pEventGenerator->Release(); g_pEventGenerator = NULL; }
    MFShutdown();
    CoUninitialize();
}

// --- Read config ---
int read_config(char* video_path, size_t max_len) {
    FILE* fp = fopen(CONFIG_FILE, "r");
    if (!fp) return 0;
    if (fgets(video_path, max_len, fp) == NULL) {
        fclose(fp);
        return 0;
    }
    size_t len = strlen(video_path);
    while (len > 0 && (video_path[len - 1] == '\n' || video_path[len - 1] == '\r')) {
        video_path[--len] = '\0';
    }
    fclose(fp);
    return 1;
}

// --- Window procedure ---
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TIMER:
        if (wParam == REFRESH_TIMER_ID && g_bPlaying) {
            force_window_refresh();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (g_pVideoDisplay) {
            // Only repaint video if playing
            if (g_bPlaying) {
                g_pVideoDisplay->RepaintVideo();
            }
        }
        else {
            // Draw black background while waiting for video
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &rc, brush);
            DeleteObject(brush);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_SIZE:
        if (g_pVideoDisplay) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            g_pVideoDisplay->SetVideoPosition(NULL, &rc);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_DISPLAYCHANGE:
        if (g_pVideoDisplay) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            g_pVideoDisplay->SetVideoPosition(NULL, &rc);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// --- Helper: find the WorkerW that hosts SHELLDLL_DefView
static BOOL CALLBACK EnumWindowsProc_FindWorkerW(HWND top, LPARAM lParam) {
    char className[64];
    if (!GetClassNameA(top, className, sizeof(className)))
        return TRUE;
    if (strcmp(className, "WorkerW") == 0) {
        HWND hDefView = FindWindowEx(top, NULL, "SHELLDLL_DefView", NULL);
        if (hDefView) {
            *(HWND*)lParam = top;
            return FALSE; // stop enumeration
        }
    }
    return TRUE; // continue
}

HWND find_workerw_with_defview() {
    HWND hProgman = FindWindowA("Progman", NULL);
    if (!hProgman) return NULL;

    // Ask Progman to spawn a WorkerW with the desktop view
    SendMessageTimeout(hProgman, 0x052C, 0, 0, SMTO_NORMAL, 1000, NULL);
    Sleep(100);

    HWND hWorkerW = NULL;
    EnumWindows(EnumWindowsProc_FindWorkerW, (LPARAM)&hWorkerW);
    return hWorkerW;
}

HWND create_wallpaper_window(HINSTANCE hInstance) {
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "VideoWallpaperClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassEx(&wc);

    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    printf("Screen: %dx%d\n", screen_width, screen_height);

    HWND hProgman = FindWindowA("Progman", NULL);
    if (!hProgman) {
        printf("ERROR: Cannot find Progman\n");
        return NULL;
    }

    printf("Found Progman: %p\n", hProgman);

    // Send magic message to create WorkerW
    HWND hWorkerW = NULL;
    SendMessageTimeout(hProgman, 0x052C, 0, (LPARAM)&hWorkerW, SMTO_NORMAL, 1000, NULL);
    Sleep(100);

    // Find or create WorkerW window
    hWorkerW = FindWindowEx(NULL, NULL, "WorkerW", NULL);
    if (!hWorkerW) {
        printf("WARNING: WorkerW not found, using Progman directly\n");
        hWorkerW = hProgman;
    }
    else {
        printf("Found WorkerW: %p\n", hWorkerW);
    }

    // Find SHELLDLL_DefView (desktop icons layer)
    g_hShellDefView = FindWindowEx(hProgman, NULL, "SHELLDLL_DefView", NULL);

    if (g_hShellDefView) {
        printf("Found desktop icons: %p\n", g_hShellDefView);
    }
    else {
        printf("WARNING: Desktop icons layer not found\n");
    }

    // Create window as TOP-LEVEL window (not child of Progman)
    // This allows Media Foundation to render properly
    HWND hwnd = CreateWindowEx(
        WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        "VideoWallpaperClass",
        "Video Wallpaper",
        WS_POPUP | WS_VISIBLE,
        0, 0, screen_width, screen_height,
        NULL,  // No parent window - top level window
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        printf("ERROR: Failed to create window\n");
        return NULL;
    }

    printf("Window created: %p\n", hwnd);

    // Disable DWM transparency
    BOOL disableTransparency = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_EXCLUDED_FROM_PEEK, &disableTransparency, sizeof(disableTransparency));

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Position behind desktop (on top of Progman/WorkerW, but below desktop icons)
    if (g_hShellDefView) {
        SetWindowPos(hwnd, g_hShellDefView, 0, 0, screen_width, screen_height,
            SWP_NOACTIVATE);
        printf("Positioned behind desktop icons\n");
    }
    else {
        SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, screen_width, screen_height,
            SWP_NOACTIVATE);
    }

    return hwnd;
}

// --- Create media source ---
HRESULT CreateMediaSource(const WCHAR* sURL, IMFMediaSource** ppSource) {
    MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
    IMFSourceResolver* pSourceResolver = NULL;
    IUnknown* pSource = NULL;

    HRESULT hr = MFCreateSourceResolver(&pSourceResolver);
    if (FAILED(hr)) return hr;

    hr = pSourceResolver->CreateObjectFromURL(sURL, MF_RESOLUTION_MEDIASOURCE,
        NULL, &ObjectType, &pSource);

    if (SUCCEEDED(hr)) {
        hr = pSource->QueryInterface(IID_PPV_ARGS(ppSource));
    }

    if (pSource) pSource->Release();
    if (pSourceResolver) pSourceResolver->Release();
    return hr;
}

// --- Create playback topology ---
HRESULT CreatePlaybackTopology(IMFMediaSource* pSource, IMFTopology** ppTopology, HWND hwndVideo) {
    IMFTopology* pTopology = NULL;
    IMFPresentationDescriptor* pPD = NULL;
    DWORD cSourceStreams = 0;

    HRESULT hr = MFCreateTopology(&pTopology);
    if (FAILED(hr)) goto done;

    hr = pSource->CreatePresentationDescriptor(&pPD);
    if (FAILED(hr)) goto done;

    hr = pPD->GetStreamDescriptorCount(&cSourceStreams);
    if (FAILED(hr)) goto done;

    for (DWORD i = 0; i < cSourceStreams; i++) {
        IMFStreamDescriptor* pSD = NULL;
        IMFActivate* pSinkActivate = NULL;
        IMFTopologyNode* pSourceNode = NULL;
        IMFTopologyNode* pOutputNode = NULL;
        BOOL fSelected = FALSE;

        hr = pPD->GetStreamDescriptorByIndex(i, &fSelected, &pSD);
        if (FAILED(hr) || !fSelected) {
            if (pSD) pSD->Release();
            continue;
        }

        GUID majorType;
        IMFMediaTypeHandler* pHandler = NULL;
        pSD->GetMediaTypeHandler(&pHandler);
        pHandler->GetMajorType(&majorType);
        pHandler->Release();

        if (majorType == MFMediaType_Video) {
            hr = MFCreateVideoRendererActivate(hwndVideo, &pSinkActivate);
        }
        else if (majorType == MFMediaType_Audio) {
            hr = MFCreateAudioRendererActivate(&pSinkActivate);
        }
        else {
            pPD->DeselectStream(i);
            pSD->Release();
            continue;
        }

        if (FAILED(hr)) {
            pSD->Release();
            goto done;
        }

        MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pSourceNode);
        pSourceNode->SetUnknown(MF_TOPONODE_SOURCE, pSource);
        pSourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPD);
        pSourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pSD);
        pTopology->AddNode(pSourceNode);

        MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pOutputNode);
        pOutputNode->SetObject(pSinkActivate);
        pOutputNode->SetUINT32(MF_TOPONODE_STREAMID, 0);
        pTopology->AddNode(pOutputNode);

        pSourceNode->ConnectOutput(0, pOutputNode, 0);

        if (pSD) pSD->Release();
        if (pSinkActivate) pSinkActivate->Release();
        if (pSourceNode) pSourceNode->Release();
        if (pOutputNode) pOutputNode->Release();
    }

    if (SUCCEEDED(hr)) {
        *ppTopology = pTopology;
        (*ppTopology)->AddRef();
    }

done:
    if (pTopology) pTopology->Release();
    if (pPD) pPD->Release();
    return hr;
}