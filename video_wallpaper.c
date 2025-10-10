/*
 * Video Wallpaper Engine for Windows 10/11
 * Uses Microsoft Media Foundation for video playback and Windows API for wallpaper integration
 *
 * MIT License - Copyright (c) 2025 B.D.D.Devendra
 */

#include <windows.h>
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

#define CONFIG_FILE "config.txt"
#define MAX_PATH_LEN 512

HWND g_hwnd = NULL;
HWND g_videoHwnd = NULL;
IMFMediaSession* g_pSession = NULL;
IMFMediaSource* g_pSource = NULL;
IMFTopology* g_pTopology = NULL;
IMFVideoDisplayControl* g_pVideoDisplay = NULL;

int read_config(char* video_path, size_t max_len) {
    FILE* fp = fopen(CONFIG_FILE, "r");
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

    size_t len = strlen(video_path);
    if (len > 0 && video_path[len - 1] == '\n') {
        video_path[len - 1] = '\0';
    }
    if (len > 1 && video_path[len - 2] == '\r') {
        video_path[len - 2] = '\0';
    }

    fclose(fp);
    return 1;
}

BOOL CALLBACK EnumWindowsProc(HWND tophandle, LPARAM topparamhandle) {
    HWND shelldll_defview = FindWindowEx(tophandle, NULL, "SHELLDLL_DefView", NULL);
    if (shelldll_defview != NULL) {
        HWND* pWorkerw = (HWND*)topparamhandle;
        *pWorkerw = FindWindowEx(NULL, tophandle, "WorkerW", NULL);
    }
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (g_pVideoDisplay) {
            g_pVideoDisplay->RepaintVideo();
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SIZE:
        if (g_pVideoDisplay) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            g_pVideoDisplay->SetVideoPosition(NULL, &rc);
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HWND create_wallpaper_window(HINSTANCE hInstance) {
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "VideoWallpaperClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    if (!RegisterClassEx(&wc)) {
        fprintf(stderr, "Error: Failed to register window class\n");
        return NULL;
    }

    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);

    // Send message to Progman to spawn WorkerW
    HWND progman = FindWindow("Progman", NULL);
    SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, NULL);

    // Wait a bit for WorkerW to be created
    Sleep(100);

    // Find the WorkerW window
    HWND workerw = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&workerw);

    if (!workerw) {
        fprintf(stderr, "Warning: Could not find WorkerW, using Progman\n");
        workerw = progman;
    }

    // Create our window as a child of WorkerW
    HWND hwnd = CreateWindowEx(
        0, "VideoWallpaperClass", "Video Wallpaper",
        WS_CHILD | WS_VISIBLE,
        0, 0, screen_width, screen_height,
        workerw, NULL, hInstance, NULL
    );

    if (!hwnd) {
        fprintf(stderr, "Error: Failed to create window (Error: %d)\n", GetLastError());
        return NULL;
    }

    printf("Window created successfully\n");
    printf("Parent window: %p\n", workerw);
    printf("Video window: %p\n", hwnd);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

HRESULT CreateMediaSource(const WCHAR* sURL, IMFMediaSource** ppSource) {
    MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
    IMFSourceResolver* pSourceResolver = NULL;
    IUnknown* pSource = NULL;

    HRESULT hr = MFCreateSourceResolver(&pSourceResolver);
    if (FAILED(hr)) {
        return hr;
    }

    hr = pSourceResolver->CreateObjectFromURL(
        sURL,
        MF_RESOLUTION_MEDIASOURCE,
        NULL,
        &ObjectType,
        &pSource
    );

    if (SUCCEEDED(hr)) {
        hr = pSource->QueryInterface(IID_PPV_ARGS(ppSource));
    }

    if (pSource) {
        pSource->Release();
    }
    if (pSourceResolver) {
        pSourceResolver->Release();
    }

    return hr;
}

HRESULT AddSourceNode(IMFTopology* pTopology, IMFMediaSource* pSource,
    IMFPresentationDescriptor* pPD, IMFStreamDescriptor* pSD,
    IMFTopologyNode** ppNode) {
    IMFTopologyNode* pNode = NULL;

    HRESULT hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pNode);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pNode->SetUnknown(MF_TOPONODE_SOURCE, pSource);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPD);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pSD);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pTopology->AddNode(pNode);
    if (FAILED(hr)) {
        goto done;
    }

    *ppNode = pNode;
    (*ppNode)->AddRef();

done:
    if (pNode) {
        pNode->Release();
    }
    return hr;
}

HRESULT AddOutputNode(IMFTopology* pTopology, IMFActivate* pActivate,
    DWORD dwId, IMFTopologyNode** ppNode) {
    IMFTopologyNode* pNode = NULL;

    HRESULT hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pNode);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pNode->SetObject(pActivate);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pNode->SetUINT32(MF_TOPONODE_STREAMID, dwId);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pTopology->AddNode(pNode);
    if (FAILED(hr)) {
        goto done;
    }

    *ppNode = pNode;
    (*ppNode)->AddRef();

done:
    if (pNode) {
        pNode->Release();
    }
    return hr;
}

HRESULT CreatePlaybackTopology(IMFMediaSource* pSource, IMFTopology** ppTopology, HWND hwndVideo) {
    IMFTopology* pTopology = NULL;
    IMFPresentationDescriptor* pPD = NULL;
    DWORD cSourceStreams = 0;

    HRESULT hr = MFCreateTopology(&pTopology);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pSource->CreatePresentationDescriptor(&pPD);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pPD->GetStreamDescriptorCount(&cSourceStreams);
    if (FAILED(hr)) {
        goto done;
    }

    printf("Found %d streams in video\n", cSourceStreams);

    for (DWORD i = 0; i < cSourceStreams; i++) {
        IMFStreamDescriptor* pSD = NULL;
        IMFActivate* pSinkActivate = NULL;
        IMFTopologyNode* pSourceNode = NULL;
        IMFTopologyNode* pOutputNode = NULL;
        BOOL fSelected = FALSE;

        hr = pPD->GetStreamDescriptorByIndex(i, &fSelected, &pSD);
        if (FAILED(hr)) {
            goto loop_done;
        }

        if (fSelected) {
            hr = AddSourceNode(pTopology, pSource, pPD, pSD, &pSourceNode);
            if (FAILED(hr)) {
                goto loop_done;
            }

            GUID majorType;
            IMFMediaTypeHandler* pHandler = NULL;
            hr = pSD->GetMediaTypeHandler(&pHandler);
            if (SUCCEEDED(hr)) {
                hr = pHandler->GetMajorType(&majorType);
                pHandler->Release();
            }

            if (FAILED(hr)) {
                goto loop_done;
            }

            if (majorType == MFMediaType_Video) {
                printf("Creating video renderer for stream %d\n", i);
                hr = MFCreateVideoRendererActivate(hwndVideo, &pSinkActivate);
            }
            else if (majorType == MFMediaType_Audio) {
                printf("Creating audio renderer for stream %d\n", i);
                hr = MFCreateAudioRendererActivate(&pSinkActivate);
            }
            else {
                printf("Skipping unknown stream type\n");
                hr = E_FAIL;
            }

            if (FAILED(hr)) {
                goto loop_done;
            }

            hr = AddOutputNode(pTopology, pSinkActivate, 0, &pOutputNode);
            if (FAILED(hr)) {
                goto loop_done;
            }

            hr = pSourceNode->ConnectOutput(0, pOutputNode, 0);
        }

    loop_done:
        if (pSD) pSD->Release();
        if (pSinkActivate) pSinkActivate->Release();
        if (pSourceNode) pSourceNode->Release();
        if (pOutputNode) pOutputNode->Release();

        if (FAILED(hr)) {
            break;
        }
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

HRESULT GetVideoDisplayControl(IMFMediaSession* pSession, IMFVideoDisplayControl** ppVideoDisplay) {
    HRESULT hr = S_OK;
    IMFGetService* pGetService = NULL;

    hr = MFGetService(pSession, MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(ppVideoDisplay));

    return hr;
}

int init_media_foundation_player(HWND hwnd, const char* video_path) {
    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to initialize COM (0x%08X)\n", hr);
        return 0;
    }

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to start Media Foundation (0x%08X)\n", hr);
        CoUninitialize();
        return 0;
    }

    int wide_len = MultiByteToWideChar(CP_UTF8, 0, video_path, -1, NULL, 0);
    WCHAR* wszURL = (WCHAR*)malloc(wide_len * sizeof(WCHAR));
    MultiByteToWideChar(CP_UTF8, 0, video_path, -1, wszURL, wide_len);

    printf("Loading video: %s\n", video_path);

    hr = CreateMediaSource(wszURL, &g_pSource);
    free(wszURL);

    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to create media source (0x%08X)\n", hr);
        fprintf(stderr, "Please check if the video file exists and is a valid format\n");
        MFShutdown();
        CoUninitialize();
        return 0;
    }

    printf("Media source created\n");

    hr = MFCreateMediaSession(NULL, &g_pSession);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to create media session (0x%08X)\n", hr);
        g_pSource->Release();
        MFShutdown();
        CoUninitialize();
        return 0;
    }

    printf("Media session created\n");

    hr = CreatePlaybackTopology(g_pSource, &g_pTopology, hwnd);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to create topology (0x%08X)\n", hr);
        g_pSession->Release();
        g_pSource->Release();
        MFShutdown();
        CoUninitialize();
        return 0;
    }

    printf("Topology created\n");

    hr = g_pSession->SetTopology(0, g_pTopology);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to set topology (0x%08X)\n", hr);
        g_pTopology->Release();
        g_pSession->Release();
        g_pSource->Release();
        MFShutdown();
        CoUninitialize();
        return 0;
    }

    printf("Topology set, waiting for ready...\n");
    Sleep(500);

    // Get video display control
    hr = GetVideoDisplayControl(g_pSession, &g_pVideoDisplay);
    if (SUCCEEDED(hr) && g_pVideoDisplay) {
        printf("Got video display control\n");
        RECT rc;
        GetClientRect(hwnd, &rc);
        g_pVideoDisplay->SetVideoPosition(NULL, &rc);
        g_pVideoDisplay->SetAspectRatioMode(MFVideoARMode_PreservePicture);
    }

    PROPVARIANT varStart;
    PropVariantInit(&varStart);
    hr = g_pSession->Start(&GUID_NULL, &varStart);
    PropVariantClear(&varStart);

    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to start playback (0x%08X)\n", hr);
        if (g_pVideoDisplay) g_pVideoDisplay->Release();
        g_pTopology->Release();
        g_pSession->Release();
        g_pSource->Release();
        MFShutdown();
        CoUninitialize();
        return 0;
    }

    printf("Playback started successfully!\n");
    return 1;
}

void cleanup_media_foundation() {
    if (g_pSession) {
        g_pSession->Stop();
        Sleep(100);
        g_pSession->Close();
        g_pSession->Release();
        g_pSession = NULL;
    }
    if (g_pVideoDisplay) {
        g_pVideoDisplay->Release();
        g_pVideoDisplay = NULL;
    }
    if (g_pTopology) {
        g_pTopology->Release();
        g_pTopology = NULL;
    }
    if (g_pSource) {
        g_pSource->Release();
        g_pSource = NULL;
    }
    MFShutdown();
    CoUninitialize();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {

    // Attach to parent console if running from batch file
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        printf("\n"); // Move to new line after batch echo
    }

    char video_path[MAX_PATH_LEN];

    if (!read_config(video_path, MAX_PATH_LEN)) {
        MessageBox(NULL, "Failed to read config file. Please create config.txt with video path.", "Error", MB_ICONERROR);
        return 1;
    }

    printf("=================================\n");
    printf("Video Wallpaper Engine Starting\n");
    printf("=================================\n");
    printf("Video file: %s\n", video_path);

    g_hwnd = create_wallpaper_window(hInstance);
    if (!g_hwnd) {
        MessageBox(NULL, "Failed to create wallpaper window", "Error", MB_ICONERROR);
        return 1;
    }

    if (!init_media_foundation_player(g_hwnd, video_path)) {
        MessageBox(NULL, "Failed to initialize video player with Media Foundation", "Error", MB_ICONERROR);
        DestroyWindow(g_hwnd);
        return 1;
    }

    printf("\nVideo wallpaper is now running!\n");
    printf("Press Ctrl+C in this console to exit\n");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    cleanup_media_foundation();

    return (int)msg.wParam;
}