#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "colors.h"
#include "shared.cpp"
#include "d3d.h"

// -----------------------------------------------------------------------
// Window proc
// -----------------------------------------------------------------------
LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg)
    {
        case WM_KEYDOWN:
        {
            if(wparam == VK_ESCAPE) PostQuitMessage(0);
            if(wparam == VK_LEFT)   player_angle -= 1.0f;
            if(wparam == VK_RIGHT)  player_angle += 1.0f;
            if(wparam == VK_UP)
            {
                isWalking = true;
                player_y     += 0.1f;
            }
            if(wparam == VK_DOWN)   player_y     -= 0.1f;
        } break;
        
        case WM_KEYUP:
        {
            if(wparam == VK_UP)
                isWalking = false;
        } break;
        
        case WM_CREATE:
        {
        } break;
        
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        } break;
        
        case WM_SIZE:
        {
            // Could resize swapchain here if needed
        } break;
        
        default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return 0;
}

// -----------------------------------------------------------------------
// Entry point
// -----------------------------------------------------------------------
#ifdef DEBUG
int
main()
{
    HINSTANCE hinstance = GetModuleHandle(NULL);
    int show = SW_SHOW;
    return(0);
}
#else
int WINAPI
WinMain(HINSTANCE hinstance, HINSTANCE hprev, LPSTR cmdline, int show)
{
    WNDCLASS wc      = {0};
    MSG      msg     = {0};
    
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hinstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "GameWindow";
    
    RegisterClass(&wc);
    
    hwnd = CreateWindow(
                        "GameWindow", "aitd",
                        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                        100, 100, 1920, 1080,
                        NULL, NULL, hinstance, NULL
                        );
    
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);
    
    InitD3D11();
    QueryPerformanceFrequency(&perf_freq);
    QueryPerformanceCounter(&last_time);
    
    srv_background = LoadTextureFromFile("../data/textures/background.bmp");
    
    player = BuildModelFromGLB("../data/models/Arwin2.glb");
    
    if(player.image_data)
    {
        srv_model = CreateSRVFromPixels(player.image_data, player.image_width, player.image_height);
        stbi_image_free(player.image_data);
        player.image_data = NULL;
    }
    
    // debug - check idle animation translation keyframes
    Animation *idle = &player.animations[1];
    for(int c = 0; c < idle->channel_count; c++)
    {
        AnimChannel *ch = &idle->channels[c];
        if(ch->type == 0) // translation only
        {
            DebugLog("bone %d translation keyframes:\n", ch->bone_index);
            for(int k = 0; k < ch->keyframe_count; k++)
                DebugLog("  kf[%d] t=%f y=%f\n", k, ch->keyframes[k].time, ch->keyframes[k].value[1]);
        }
    }
    
    while(msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            //Update();
            Render();
        }
    }
    
    return (int)msg.wParam;
}
#endif