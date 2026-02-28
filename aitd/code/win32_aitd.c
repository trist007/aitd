#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "colors.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

HWND hwnd;
HDC hdc;
HGLRC hglrc;
GLuint background;

typedef struct
{
    char          magic[4];
    unsigned int  version;
    unsigned int  vertex_count;
    unsigned int  face_count;
} MDLHeader;

typedef struct
{
    float x, y, z;
} MDLVertex;

typedef struct
{
    unsigned int v0, v1, v2;
    float r, g, b;
} MDLFace;

typedef struct
{
    MDLVertex *vertices;
    MDLFace   *faces;
    int        vertex_count;
    int        face_count;
} Model;

Model torso;

Model LoadModel(const char *filename)
{
    Model model = {0};
    MDLHeader header;
    
    FILE *f = fopen(filename, "rb");
    if(!f)
    {
        OutputDebugStringA("Failed to open model\n");
        return(model);
    }
    
    fread(&header, sizeof(MDLHeader), 1, f);
    
    if(header.magic[0] != 'M' || header.magic[1] != 'D' ||
       header.magic[2] != 'L' || header.magic[3] != 'A')
    {
        OutputDebugStringA("Invalid model file\n");
        fclose(f);
        return model;
    }
    
    model.vertex_count = header.vertex_count;
    model.face_count   = header.face_count;
    model.vertices     = malloc(sizeof(MDLVertex) * model.vertex_count);
    model.faces        = malloc(sizeof(MDLFace)   * model.face_count);
    
    fread(model.vertices, sizeof(MDLVertex), model.vertex_count, f);
    fread(model.faces,    sizeof(MDLFace),   model.face_count,   f);
    
    fclose(f);
    return model;
}

void RenderModel(Model *model)
{
    glBegin(GL_TRIANGLES);
    for(int i = 0; i < model->face_count; i++)
    {
        MDLFace   *face = &model->faces[i];
        MDLVertex *v0   = &model->vertices[face->v0];
        MDLVertex *v1   = &model->vertices[face->v1];
        MDLVertex *v2   = &model->vertices[face->v2];
        
        glColor3f(face->r, face->g, face->b);
        glVertex3f(v0->x, v0->y, v0->z);
        glVertex3f(v1->x, v1->y, v1->z);
        glVertex3f(v2->x, v2->y, v2->z);
    }
    glEnd();
}

void SetupPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd = {0};
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

void InitOpenGL(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    //glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_FLAT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1920.0/1080.0, 0.1, 100.0);
    //gluPerspective(60.0, 320.0/200.0, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

GLuint LoadTexture(const char *filename)
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


void Render(void)
{
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
    
    // Draw torso
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -5.0f);
    RenderModel(&torso);
    
    SwapBuffers(hdc);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg)
    {
        case WM_CREATE:
        hdc = GetDC(hwnd);
        SetupPixelFormat(hdc);
        hglrc = wglCreateContext(hdc);
        wglMakeCurrent(hdc, hglrc);
        InitOpenGL();
        background = LoadTexture("../data/textures/background.bmp");
        torso = LoadModel("../data/textures/torso.mdl");
        
        if(torso.vertex_count == 0)
            OutputDebugStringA("Failed to load model\n");
        //background = LoadTexture("C:/dev/aitd/aitd/data/textures/background.bmp");
        return 0;
        
        case WM_DESTROY:
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hglrc);
        ReleaseDC(hwnd, hdc);
        PostQuitMessage(0);
        return 0;
        
        case WM_SIZE:
        glViewport(0, 0, LOWORD(lparam), HIWORD(lparam));
        return 0;
        
        case WM_KEYDOWN:
        if(wparam == VK_ESCAPE)
            PostQuitMessage(0);
        return 0;
        
        default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprev, LPSTR cmdline, int show)
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

/*
#ifdef DEBUG
int main()
#else
    int CALLBACK WinMain(
        HINSTANCE //hInstance,
        HINSTANCE //hPrevInstance,
        LPSTR     //lpCmdLine,
        int       //nCmdShow
)
#endif

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
            }
*/