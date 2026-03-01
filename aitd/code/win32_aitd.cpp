#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "colors.h"
#include "shared.cpp"

// Globals
HWND hwnd;
HDC hdc;
HGLRC hglrc;
GLuint background;
Model player;
float anim_time = 0.0f;
float player_angle = -90.0f;
float player_y = -0.6f;

// Delta time
LARGE_INTEGER perf_freq;
LARGE_INTEGER last_time;
float delta_time;

void
RenderModel(Model *model)
{
    if(model->texture)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, model->texture);
    }
    
    glBegin(GL_TRIANGLES);
    for(int i = 0; i < model->face_count; i++)
    {
        MDLFace *face = &model->faces[i];
        
        for(int c = 0; c < 3; c++)
        {
            unsigned int vi = (c == 0) ? face->v0 : (c == 1) ? face->v1 : face->v2;
            MDLVertex *v = &model->vertices[vi];
            
            float px = v->x, py = v->y, pz = v->z;
            float ox = 0, oy = 0, oz = 0;
            
            if(model->bone_count > 0 && model->bone_weights)
            {
                for(int b = 0; b < 4; b++)
                {
                    float w = model->bone_weights[vi].weights[b];
                    if(w == 0.0f) continue;
                    int bi = model->bone_weights[vi].joints[b];
                    if(bi < 0 || bi >= model->bone_count) continue;
                    Mat4 *bm = &model->bone_matrices[bi];
                    
                    ox += w * (bm->m[0]*px + bm->m[4]*py + bm->m[8]*pz  + bm->m[12]);
                    oy += w * (bm->m[1]*px + bm->m[5]*py + bm->m[9]*pz  + bm->m[13]);
                    oz += w * (bm->m[2]*px + bm->m[6]*py + bm->m[10]*pz + bm->m[14]);
                }
            }
            else
            {
                ox = px; oy = py; oz = pz;
            }
            
            glColor3f(1.0f, 1.0f, 1.0f);
            glTexCoord2f(face->u[c], face->v[c]);
            glVertex3f(ox, oy, oz);
        }
    }
    
    glEnd();
    
    if(model->texture)
        glDisable(GL_TEXTURE_2D);
}

void
SetupPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd = {};
    int pixelformat;
    
    pfd.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.dwLayerMask  = PFD_MAIN_PLANE;
    pfd.iPixelType   = PFD_TYPE_RGBA;
    pfd.cColorBits   = 32;
    pfd.cDepthBits   = 16;
    
    pixelformat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelformat, &pfd);
}

void
InitOpenGL(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    //glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_FLAT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1920.0/1080.0, 0.1, 100.0);
    //gluPerspective(60.0, 320.0/200.0, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

GLuint
LoadTexture(const char *filename)
{
    GLuint texture;
    int width, height, channels;
    unsigned char *data;
    
    data = stbi_load(filename, &width, &height, &channels, 3);
    if(!data)
    {
        // Failed to load
        OutputDebugStringA("Failed to load texture\n");
        return(0);
    }
    
    char debug[128];
    sprintf_s(debug, sizeof(debug), "Loaded: %dx%d channels:%d\n", width, height, channels);
    OutputDebugStringA(debug);
    
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, data);
    
    GLenum err = glGetError();
    if(err != GL_NO_ERROR)
    {
        OutputDebugStringA("Texture upload error\n");
    }
    
    stbi_image_free(data);
    
    return(texture);
}

void
Render(void)
{
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    delta_time = (float)(current_time.QuadPart - last_time.QuadPart) / (float)perf_freq.QuadPart;
    last_time = current_time;
    
    //anim_time += 1.0f / 24.0f;
    anim_time += delta_time;
    if(anim_time > 1.666f)
        anim_time = 0.0f;
    UpdateAnimation(&player, 2, anim_time);
    
    static bool anim_printed = false;
    if(!anim_printed)
    {
        DebugLog("anim[2] duration=%f channels=%d\n", player.animations[2].duration, player.animations[2].channel_count);
        AnimChannel *ch = &player.animations[2].channels[0];
        for(int i = 0; i < ch->keyframe_count; i++)
            DebugLog("kf[%d] t=%f\n", i, ch->keyframes[i].time);
        anim_printed = true;
    }
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Draw background
    glDisable(GL_DEPTH_TEST);
    glColor3f(COLOR_WHITE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, background);
    
    glBegin(GL_QUADS);
    {
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  1.0f);
    }
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    // Draw player
    glLoadIdentity();
    glTranslatef(0.0f, player_y, -1.5f);
    glRotatef(player_angle, 0.0f, 1.0f, 0.0f);
    
    RenderModel(&player);
    
    SwapBuffers(hdc);
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT Result = 0;
    
    switch(msg)
    {
        case WM_KEYDOWN:
        {
            if(wparam == VK_ESCAPE)
                PostQuitMessage(0);
            if(wparam == VK_LEFT)
                player_angle -= 5.0f;
            if(wparam == VK_RIGHT)
                player_angle += 5.0f;
            if(wparam == VK_UP)
                player_y += 0.1f;
            if(wparam == VK_DOWN)
                player_y -= 0.1f;
        } break;
        
        case WM_CREATE:
        {
            hdc = GetDC(hwnd);
            SetupPixelFormat(hdc);
            hglrc = wglCreateContext(hdc);
            wglMakeCurrent(hdc, hglrc);
            InitOpenGL();
            QueryPerformanceFrequency(&perf_freq);
            QueryPerformanceCounter(&last_time);
            
            background = LoadTexture("../data/textures/background.bmp");
            player = BuildModelFromGLB("../data/models/player.glb");
            
            cgltf_options options = {};
            cgltf_data* data = 0;
            cgltf_result result = cgltf_parse_file(&options, "../data/models/player.glb", &data);
            
            if(result != cgltf_result_success)
            {
                OutputDebugStringA("Failed to parse glb\n");
                return(0);
            }
            
            // Load actual binary
            cgltf_load_buffers(&options, data, "player.glb");
            
            // Get mesh
            cgltf_mesh *mesh = &data->meshes[0];
            cgltf_primitive *prim = &mesh->primitives[0];
            
            // Walk attributes
            for(int i = 0;
                i < (int)prim->attributes_count;
                i++)
            {
                cgltf_attribute *attr = &prim->attributes[i];
                
                if(attr->type == cgltf_attribute_type_position)
                {
                    // attr->data is a cfltf_accessor* with the vertex positions
                }
                else if(attr->type == cgltf_attribute_type_texcoord)
                {
                    // UV coords
                }
                else if(attr->type == cgltf_attribute_type_joints)
                {
                    // Which bones influence each vertex ( 4 per vertex )
                }
                else if(attr->type == cgltf_attribute_type_weights)
                {
                    // How much each bone influences each vertex (4 floas per vertex)
                }
            }
            
            // Animations
            for(int i = 0;
                i < (int)data->animations_count;
                i++)
            {
                cgltf_animation *anim = &data->animations[i];
                // anim->name, anim->channels, anim->samplers
                // each channel drives one bone's translation/rotation/scale over time
            }
            
            // Skeleton
            for(int i = 0;
                i < (int)data->skins_count;
                i++)
            {
                cgltf_skin *skin = &data->skins[i];
                // skin->joints[] = array of nodes (bones)
                // skin->inverse_bind_matrices = the inverse bind pose matrices
            }
            
            // Read float data out of an accessor cgltf with helper functions
            /*
            float pos[3];
            cgltf_accessor_read_float(attr->data, vertex_index, pos, 3);
            */
            
            if(result == cgltf_result_success)
            {
                // Do stuff
                cgltf_free(data);
            }
            
            if(player.vertex_count == 0)
                OutputDebugStringA("Failed to load model\n");
        } break;
        
        case WM_DESTROY:
        {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(hglrc);
            ReleaseDC(hwnd, hdc);
            PostQuitMessage(0);
        } break;
        
        case WM_SIZE:
        {
            glViewport(0, 0, LOWORD(lparam), HIWORD(lparam));
        } break;
        
        default:
        {
            return DefWindowProc(hwnd, msg, wparam, lparam);
        } break;
    }
    
    return(Result);
}

void
run(HINSTANCE hinstance, int show)
{
    WNDCLASS wc = {};
    MSG      msg = {};
}

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
    
    // Pump one message
    //PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
    //TranslateMessage(&msg);
    //DispatchMessage(&msg);
    
    /*
    if(file_exists("game.log"))
    {
        delete_file("game.log");
    }

// Adds colors to windows console
#ifdef DEBUG
HANDLE hOut = GetStdHandl(STD_OUTPUT_HANDLE);
DWORD dwMode = 0;
GetConsoleMode(hOut, &dwMode);
dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
SetConsoleMode(hOut, dwMode);
#endif

// read
int FileSize = 0;
char* patchVersionText = read_file("version.txt", &fileSize, &transientStorage);
if(patchVersionText)
{
    const int majorVersion = 1;
    const int minorVersion = 0;
    int patchVersion = 0;
    patchVersion = atoi(patchVersionText);
    sprintf(gameState->versio
            */
    
    /* fixed timestep game loop */
    while(msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render();
        }
    }
    
    return (int)msg.wParam;
}
#endif