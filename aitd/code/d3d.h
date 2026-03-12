/* date = March 11th 2026 4:38 pm */

#ifndef D3D_H
#define D3D_H

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

struct FontVertex {
    float x, y;       // screen pos
    float u, v;       // UV into atlas
    float r, g, b, a; // color
};


// -----------------------------------------------------------------------
// D3D11 globals
// -----------------------------------------------------------------------
HWND                    hwnd;
IDXGISwapChain         *swapchain; // manages the front and back buffers when call Present() flips them
ID3D11Device           *device;    // creates buffers, textures, and shaders
ID3D11DeviceContext    *context;   // records and submits draw commands
ID3D11RenderTargetView *render_target;
ID3D11DepthStencilView *depth_stencil_view;
ID3D11Texture2D        *depth_stencil_tex;

// Shaders
ID3D11VertexShader     *vs_model;
ID3D11PixelShader      *ps_model;
ID3D11VertexShader     *vs_background;
ID3D11PixelShader      *ps_background;
ID3D11InputLayout      *input_layout_model;
ID3D11InputLayout      *input_layout_bg;

// Buffers
ID3D11Buffer           *vb_model;       // skinned verts uploaded each frame (CPU skinning)
ID3D11Buffer           *ib_model;
ID3D11Buffer           *vb_background;
ID3D11Buffer           *cb_per_frame;

// Sampler / rasterizer
ID3D11SamplerState     *sampler;
ID3D11RasterizerState  *raster;
ID3D11DepthStencilState *ds_enabled;
ID3D11DepthStencilState *ds_disabled;

// Textures
ID3D11ShaderResourceView *srv_background;
ID3D11ShaderResourceView *srv_model;

// Fonts stb_truetype
static ID3D11ShaderResourceView *FontSRV;
static stbtt_bakedchar FontCdata[96];
static float FontPixelHeight = 32.0f;
stbtt_bakedchar cdata[96];
static FontVertex FontVertexBuffer[4096];  // cpu-side staging
static int FontVertexCount = 0;

static ID3D11Buffer *FontVB;         // created once at init, D3D11_USAGE_DYNAMIC
static ID3D11BlendState *FontBlend;
static ID3D11VertexShader *FontVS;
static ID3D11PixelShader *FontPS;
static ID3D11InputLayout *FontLayout;
static ID3D11SamplerState *FontSampler;
static ID3D11Buffer *ScreenCB;




// -----------------------------------------------------------------------
// Vertex layouts defines the data that goes to the GPU
// -----------------------------------------------------------------------
struct VertexModel
{
    float x, y, z;    // position
    float u, v;       // texture coords
    float r, g, b, a; // color
};

struct VertexBG
{
    float x, y;       // 2D position
    float u, v;       // UV for background a fullscreen quad
};

// -----------------------------------------------------------------------
// Constant buffer layout  (must be 16-byte aligned)
// -----------------------------------------------------------------------
struct CBPerFrame      // readable by s haders 16 floats 4x4 matrix map/update every frame
{
    float mvp[16];
};


// -----------------------------------------------------------------------
// Simple math helpers
// -----------------------------------------------------------------------
static void
MatMul(float *out, const float *a, const float *b) // matrix multiply
{
    float tmp[16] = {};
    for(int col = 0; col < 4; col++)
        for(int row = 0; row < 4; row++)
        for(int k = 0; k < 4; k++)
        tmp[col*4+row] += a[k*4+row] * b[col*4+k];
    for(int i = 0; i < 16; i++) out[i] = tmp[i];
}

static void
MatIdentity(float *m)
{
    memset(m, 0, 64);
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void
MatTranslate(float *m, float x, float y, float z)
{
    MatIdentity(m);
    m[12] = x; m[13] = y; m[14] = z;
}

static void
MatRotateY(float *m, float deg)                      // rotates around y access
{
    float r = deg * 3.14159265f / 180.0f;
    MatIdentity(m);
    m[0]  =  cosf(r);
    m[2]  =  sinf(r);
    m[8]  = -sinf(r);
    m[10] =  cosf(r);
}

static void
MatPerspective(float *m, float fov_deg, float aspect, float znear, float zfar)
{
    memset(m, 0, 64);
    float f = 1.0f / tanf(fov_deg * 0.5f * 3.14159265f / 180.0f);
    m[0]  = f / aspect;
    m[5]  = f;
    m[10] = zfar / (znear - zfar);
    m[11] = -1.0f;
    m[14] = (znear * zfar) / (znear - zfar);
}

// -----------------------------------------------------------------------
// HLSL shaders (compiled at runtime)
// -----------------------------------------------------------------------
static const char *shader_model_src = R"(
cbuffer CB : register(b0)
{
    float4x4 mvp;
};

Texture2D    tex : register(t0);
SamplerState smp : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD;
float4 color : COLOR;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD;
float4 color : COLOR;
};

VS_OUT VSMain(VS_IN input)                 // vertex shader
{
    VS_OUT o;
    o.pos = mul(mvp, float4(input.pos, 1.0f));
    o.uv  = input.uv;
o.color = input.color;
    return o;
}

float4 PSMain(VS_OUT input) : SV_TARGET    // pixel shader
{
return input.color;
}
)";

static const char *shader_bg_src = R"(
Texture2D    tex : register(t0);
SamplerState smp : register(s0);

struct VS_IN
{
    float2 pos : POSITION;
    float2 uv  : TEXCOORD;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD;
};

VS_OUT VSMain(VS_IN input)               // background vertex shader
{
    VS_OUT o;
    o.pos = float4(input.pos, 0.0f, 1.0f);
    o.uv  = input.uv;
    return o;
}

float4 PSMain(VS_OUT input) : SV_TARGET
{
    return tex.Sample(smp, input.uv);
}
)";

static const char *shader_font_src = R"(

Texture2D    FontAtlas : register(t0);
SamplerState FontSampler : register(s0);
cbuffer ScreenCB : register(b0) { float2 ScreenSize; };
struct VS_IN
{
    float2 pos : POSITION;
    float2 uv  : TEXCOORD;
float4 color : COLOR;
};
struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD;
float4 color : COLOR;
};
VS_OUT VSMain(VS_IN input)
{
    VS_OUT o;
    o.uv = input.uv;
o.color = input.color;
    float2 ndc = (input.pos / ScreenSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    o.pos = float4(ndc, 0, 1);
    return o;
}
float4 PSMain(VS_OUT input) : SV_TARGET
{
    float alpha = FontAtlas.Sample(FontSampler, input.uv).r;
    //return float4(1, 1, 1, 1) * alpha; // color white
return float4(input.color.rgb, input.color.a * alpha);
}
)";
// -----------------------------------------------------------------------
// Load texture from raw pixel data into SRV
// -----------------------------------------------------------------------
static ID3D11ShaderResourceView *
CreateSRVFromPixels(unsigned char *pixels, int width, int height)
{
    D3D11_TEXTURE2D_DESC td = {};
    td.Width            = (UINT)width;
    td.Height           = (UINT)height;
    td.MipLevels        = 1;
    td.ArraySize        = 1;
    td.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_DEFAULT;
    td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    
    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem          = pixels;
    sd.SysMemPitch      = (UINT)(width * 4);
    
    ID3D11Texture2D          *tex = NULL;
    ID3D11ShaderResourceView *srv = NULL;
    device->CreateTexture2D(&td, &sd, &tex);
    device->CreateShaderResourceView(tex, NULL, &srv);
    tex->Release();
    return srv;
}

static ID3D11ShaderResourceView *
LoadTextureFromFile(const char *filename)
{
    int w, h, ch;
    unsigned char *pixels = stbi_load(filename, &w, &h, &ch, 4);
    if(!pixels)
    {
        OutputDebugStringA("Failed to load texture\n");
        return NULL;
    }
    ID3D11ShaderResourceView *srv = CreateSRVFromPixels(pixels, w, h);
    stbi_image_free(pixels);
    return srv;
}

static ID3D11ShaderResourceView *
LoadTextureFromMemory(unsigned char *raw, int raw_size)
{
    int w, h, ch;
    unsigned char *pixels = stbi_load_from_memory(raw, raw_size, &w, &h, &ch, 4);
    if(!pixels)
    {
        OutputDebugStringA("Failed to decode embedded texture\n");
        return NULL;
    }
    ID3D11ShaderResourceView *srv = CreateSRVFromPixels(pixels, w, h);
    stbi_image_free(pixels);
    return srv;
}

// -----------------------------------------------------------------------
// Init D3D11
// -----------------------------------------------------------------------
static void
InitD3D11(void)
{
    DXGI_SWAP_CHAIN_DESC scd       = {};
    scd.BufferCount                = 1;
    scd.BufferDesc.Width           = 1920;
    scd.BufferDesc.Height          = 1080;
    scd.BufferDesc.Format          = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator   = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage                = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow               = hwnd;
    scd.SampleDesc.Count           = 1;
    scd.Windowed                   = TRUE;
    
    D3D11CreateDeviceAndSwapChain(                                         // creates everything in one call
                                  NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
                                  NULL, 0, D3D11_SDK_VERSION,
                                  &scd, &swapchain, &device, NULL, &context
                                  );
    
    // Render target
    ID3D11Texture2D *backbuf = NULL;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backbuf); // gets back buffer texture and wraps it in render target view
    device->CreateRenderTargetView(backbuf, NULL, &render_target);         // the thing you actually draw into
    backbuf->Release();
    
    // Depth stencil
    D3D11_TEXTURE2D_DESC dsd = {};
    dsd.Width            = 1920;
    dsd.Height           = 1080;
    dsd.MipLevels        = 1;
    dsd.ArraySize        = 1;
    dsd.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;                // 24 bits for depth 8 for stencil
    dsd.SampleDesc.Count = 1;
    dsd.Usage            = D3D11_USAGE_DEFAULT;
    dsd.BindFlags        = D3D11_BIND_DEPTH_STENCIL;
    device->CreateTexture2D(&dsd, NULL, &depth_stencil_tex);
    device->CreateDepthStencilView(depth_stencil_tex, NULL, &depth_stencil_view);
    
    context->OMSetRenderTargets(1, &render_target, depth_stencil_view);
    
    // Viewport
    D3D11_VIEWPORT vp = {};
    vp.Width    = 1920.0f;
    vp.Height   = 1080.0f;
    vp.MaxDepth = 1.0f;
    context->RSSetViewports(1, &vp);
    
    // Sampler
    D3D11_SAMPLER_DESC sampd = {};
    sampd.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT; // point filtering nearest neighbor no blending between texels
    sampd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // good for pixel art or crisp textures
    sampd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // Changge to D3D11_FILTER_MIN_MAG_MIP_LINEAR for smooth filtering
    sampd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    device->CreateSamplerState(&sampd, &sampler);
    
    // Rasterizer (no culling for now, matches original)
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;                // no backface culling
    device->CreateRasterizerState(&rd, &raster);
    context->RSSetState(raster);
    
    // Depth stencil states
    D3D11_DEPTH_STENCIL_DESC ds = {};
    ds.DepthEnable    = TRUE;
    ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    ds.DepthFunc      = D3D11_COMPARISON_LESS;
    device->CreateDepthStencilState(&ds, &ds_enabled);
    
    ds.DepthEnable = FALSE;
    device->CreateDepthStencilState(&ds, &ds_disabled);
    
    // Compile model shader
    ID3DBlob *vs_blob = NULL, *ps_blob = NULL, *err = NULL;
    
    D3DCompile(shader_model_src, strlen(shader_model_src), NULL, NULL, NULL, // shader compilation
               "VSMain", "vs_5_0", 0, 0, &vs_blob, &err);
    if(err) { OutputDebugStringA((char *)err->GetBufferPointer()); err->Release(); err = NULL; }
    
    D3DCompile(shader_model_src, strlen(shader_model_src), NULL, NULL, NULL, // shader compilation
               "PSMain", "ps_5_0", 0, 0, &ps_blob, &err);
    if(err) { OutputDebugStringA((char *)err->GetBufferPointer()); err->Release(); err = NULL; }
    
    device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &vs_model);
    device->CreatePixelShader (ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &ps_model);
    
    D3D11_INPUT_ELEMENT_DESC ied_model[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    device->CreateInputLayout(ied_model, 3,
                              vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
                              &input_layout_model);
    vs_blob->Release(); ps_blob->Release();
    
    // Compile background shader
    D3DCompile(shader_bg_src, strlen(shader_bg_src), NULL, NULL, NULL,
               "VSMain", "vs_5_0", 0, 0, &vs_blob, &err);
    if(err) { OutputDebugStringA((char *)err->GetBufferPointer()); err->Release(); err = NULL; }
    
    D3DCompile(shader_bg_src, strlen(shader_bg_src), NULL, NULL, NULL,
               "PSMain", "ps_5_0", 0, 0, &ps_blob, &err);
    if(err) { OutputDebugStringA((char *)err->GetBufferPointer()); err->Release(); err = NULL; }
    
    device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &vs_background);
    device->CreatePixelShader (ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &ps_background);
    
    D3D11_INPUT_ELEMENT_DESC ied_bg[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    device->CreateInputLayout(ied_bg, 2,
                              vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
                              &input_layout_bg);
    vs_blob->Release(); ps_blob->Release();
    
    // Background quad vertex buffer
    VertexBG bg_verts[] =
    {
        { -1.0f, -1.0f,  0.0f, 1.0f },
        { -1.0f,  1.0f,  0.0f, 0.0f },
        {  1.0f,  1.0f,  1.0f, 0.0f },
        { -1.0f, -1.0f,  0.0f, 1.0f },
        {  1.0f,  1.0f,  1.0f, 0.0f },
        {  1.0f, -1.0f,  1.0f, 1.0f },
    };
    D3D11_BUFFER_DESC bd = {};
    bd.Usage     = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.ByteWidth = sizeof(bg_verts);
    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = bg_verts;
    device->CreateBuffer(&bd, &sd, &vb_background);
    
    // Model vertex buffer (dynamic - CPU skinning writes each frame)
    D3D11_BUFFER_DESC mvbd = {};
    mvbd.Usage          = D3D11_USAGE_DYNAMIC;
    mvbd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    mvbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    mvbd.ByteWidth      = sizeof(VertexModel) * 65536; // enough for any mesh
    device->CreateBuffer(&mvbd, NULL, &vb_model);
    
    // Constant buffer
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage          = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbd.ByteWidth      = sizeof(CBPerFrame);
    device->CreateBuffer(&cbd, NULL, &cb_per_frame);
}

// -----------------------------------------------------------------------
// Render model  (CPU skinning, upload verts each frame)
// -----------------------------------------------------------------------
static void
RenderModel(Model *model, ID3D11ShaderResourceView *srv)
{
    // Build skinned verts on CPU (same logic as original)
    int total_verts = model->face_count * 3;
    VertexModel *verts = (VertexModel *)malloc(sizeof(VertexModel) * total_verts);
    
    for(int i = 0; i < model->face_count; i++)
    {
        MDLFace *face = &model->faces[i];
        for(int c = 0; c < 3; c++)
        {
            unsigned int vi = (c == 0) ? face->v0 : (c == 1) ? face->v1 : face->v2;
            MDLVertex *v    = &model->vertices[vi];
            
            float px = v->x, py = v->y, pz = v->z;
            float ox = 0,    oy = 0,    oz = 0;
            
            if(model->bone_count > 0 && model->bone_weights)
            {
                for(int b = 0; b < 4; b++) // cpu skinning
                {
                    float w  = model->bone_weights[vi].weights[b];
                    if(w == 0.0f) continue;
                    int   bi = model->bone_weights[vi].joints[b];
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
            
            int idx           = i*3 + c;
            verts[idx].x      = ox;
            verts[idx].y      = oy;
            verts[idx].z      = oz;
            verts[idx].u      = face->u[c];
            verts[idx].v      = face->v[c];
            verts[idx].r      = face->r;
            verts[idx].g      = face->g;
            verts[idx].b      = face->b;
            verts[idx].a      = 1.0f;
        }
    }
    
    // Upload to GPU
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    context->Map(vb_model, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, verts, sizeof(VertexModel) * total_verts);
    context->Unmap(vb_model, 0);
    free(verts);
    
    // Set shaders
    context->VSSetShader(vs_model, NULL, 0);
    context->PSSetShader(ps_model, NULL, 0);
    context->IASetInputLayout(input_layout_model);
    
    UINT stride = sizeof(VertexModel);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vb_model, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    context->PSSetShaderResources(0, 1, &srv);
    context->PSSetSamplers(0, 1, &sampler);
    context->VSSetConstantBuffers(0, 1, &cb_per_frame);
    
    context->Draw(total_verts, 0);
}

static void
InitSTBTrueType(char *font_path)
{
    static unsigned char font_buffer[1024*1024];
    static unsigned char font_bitmap[512*512];
    stbtt_fontinfo font;
    
    FILE *file = fopen(font_path, "rb");
    if (!file) return;
    
    size_t size = fread(font_buffer, 1, sizeof(font_buffer), file);
    fclose(file);
    if (size == 0) return;
    
    stbtt_InitFont(&font, font_buffer, 0);
    stbtt_BakeFontBitmap(font_buffer, 0, FontPixelHeight,
                         font_bitmap, 512, 512, 32, 96, FontCdata);
    
    D3D11_TEXTURE2D_DESC td = {};
    td.Width            = 512;
    td.Height           = 512;
    td.MipLevels        = 1;
    td.ArraySize        = 1;
    td.Format           = DXGI_FORMAT_R8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_DEFAULT;
    td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    
    D3D11_SUBRESOURCE_DATA fsd = {};
    fsd.pSysMem     = font_bitmap;
    fsd.SysMemPitch = 512;
    
    ID3D11Texture2D *font_tex = NULL;
    device->CreateTexture2D(&td, &fsd, &font_tex);
    device->CreateShaderResourceView(font_tex, NULL, &FontSRV);
    font_tex->Release();
    
    
    // compile font shaders
    ID3DBlob *vs_blob = NULL, *ps_blob = NULL, *err = NULL;
    D3DCompile(shader_font_src, strlen(shader_font_src), NULL, NULL, NULL,
               "VSMain", "vs_5_0", 0, 0, &vs_blob, &err);
    if(err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); err = NULL; }
    
    D3DCompile(shader_font_src, strlen(shader_font_src), NULL, NULL, NULL,
               "PSMain", "ps_5_0", 0, 0, &ps_blob, &err);
    if(err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); err = NULL; }
    
    device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &FontVS);
    device->CreatePixelShader (ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &FontPS);
    
    D3D11_INPUT_ELEMENT_DESC ied_font[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    device->CreateInputLayout(ied_font, 3,
                              vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
                              &FontLayout);
    if(!FontLayout) OutputDebugStringA("FontLayout creation failed\n");
    vs_blob->Release(); ps_blob->Release();
    
    // font blend state (src_alpha / inv_src_alpha)
    D3D11_BLEND_DESC bld = {};
    bld.RenderTarget[0].BlendEnable           = TRUE;
    bld.RenderTarget[0].SrcBlend             = D3D11_BLEND_SRC_ALPHA;
    bld.RenderTarget[0].DestBlend            = D3D11_BLEND_INV_SRC_ALPHA;
    bld.RenderTarget[0].BlendOp              = D3D11_BLEND_OP_ADD;
    bld.RenderTarget[0].SrcBlendAlpha        = D3D11_BLEND_ONE;
    bld.RenderTarget[0].DestBlendAlpha       = D3D11_BLEND_ZERO;
    bld.RenderTarget[0].BlendOpAlpha         = D3D11_BLEND_OP_ADD;
    bld.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&bld, &FontBlend);
    
    // font sampler (linear, clamp - better for font atlas than point/wrap)
    D3D11_SAMPLER_DESC sd = {};
    sd.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    device->CreateSamplerState(&sd, &FontSampler);
    
    // ScreenCB
    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth      = 16;  // float2 padded to 16 bytes for cbuffer alignment
    cbd.Usage          = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&cbd, NULL, &ScreenCB);
    
    // FontVB
    D3D11_BUFFER_DESC vbd = {};
    vbd.ByteWidth      = sizeof(FontVertexBuffer);
    vbd.Usage          = D3D11_USAGE_DYNAMIC;
    vbd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&vbd, NULL, &FontVB);
    
}

void EmitQuad(stbtt_aligned_quad q, float r, float g, float b, float a)
{
    if (FontVertexCount + 6 > 4096) return;  // overflow guard
    
    FontVertex *v = FontVertexBuffer + FontVertexCount;
    
    // triangle 1
    v[0] = { q.x0, q.y0, q.s0, q.t0, r, g, b, a };
    v[1] = { q.x1, q.y0, q.s1, q.t0, r, g, b, a };
    v[2] = { q.x1, q.y1, q.s1, q.t1, r, g, b, a };
    // triangle 2
    v[3] = { q.x0, q.y0, q.s0, q.t0, r, g, b, a };
    v[4] = { q.x1, q.y1, q.s1, q.t1, r, g, b, a };
    v[5] = { q.x0, q.y1, q.s0, q.t1, r, g, b, a };
    
    FontVertexCount += 6;
}

void FlushFontQuads(ID3D11DeviceContext *ctx, float screen_w, float screen_h)
{
    if (FontVertexCount == 0) return;
    
    /*
char dbg[256];
    snprintf(dbg, sizeof(dbg), "FlushFontQuads: count=%d\n", FontVertexCount);
    OutputDebugStringA(dbg);
    
    if (FontVertexCount == 0) return;
    
    if(!FontVB)     { OutputDebugStringA("FontVB is NULL\n");     return; }
    if(!FontVS)     { OutputDebugStringA("FontVS is NULL\n");     return; }
    if(!FontPS)     { OutputDebugStringA("FontPS is NULL\n");     return; }
    if(!FontLayout) { OutputDebugStringA("FontLayout is NULL\n"); return; }
    if(!FontSRV)    { OutputDebugStringA("FontSRV is NULL\n");    return; }
    if(!ScreenCB)   { OutputDebugStringA("ScreenCB is NULL\n");   return; }
    if(!FontBlend)  { OutputDebugStringA("FontBlend is NULL\n");  return; }
    if(!FontSampler){ OutputDebugStringA("FontSampler is NULL\n");return; }
    */
    
    // upload to GPU
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ctx->Map(FontVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, FontVertexBuffer, FontVertexCount * sizeof(FontVertex));
    ctx->Unmap(FontVB, 0);
    
    // save render state you care about (blend, shaders, topology, etc.)
    // ... or just stomp it if this is debug-only
    
    // set pipeline
    float screen_size[2] = { screen_w, screen_h };
    //ctx->UpdateSubresource(ScreenCB, 0, NULL, &screen_size, 0, 0);
    D3D11_MAPPED_SUBRESOURCE cb_mapped = {};
    ctx->Map(ScreenCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &cb_mapped);
    memcpy(cb_mapped.pData, screen_size, sizeof(screen_size));
    ctx->Unmap(ScreenCB, 0);
    
    ctx->OMSetDepthStencilState(ds_disabled, 0);  // disable depth so text always draws on top
    ctx->OMSetRenderTargets(1, &render_target, NULL);  // add this, NULL depth so nothing blocks it
    
    ctx->VSSetShader(FontVS, NULL, 0);
    ctx->PSSetShader(FontPS, NULL, 0);
    ctx->VSSetConstantBuffers(0, 1, &ScreenCB);
    ctx->PSSetShaderResources(0, 1, &FontSRV);
    ctx->PSSetSamplers(0, 1, &FontSampler);
    ctx->IASetInputLayout(FontLayout);
    
    UINT stride = sizeof(FontVertex), offset = 0;
    ctx->IASetVertexBuffers(0, 1, &FontVB, &stride, &offset);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    ctx->OMSetBlendState(FontBlend, NULL, 0xFFFFFFFF);
    
    ctx->Draw(FontVertexCount, 0);
    
    // reset for next frame
    ctx->OMSetDepthStencilState(ds_enabled, 0);  // disable depth so text always draws on top
    FontVertexCount = 0;
}

void DrawDebugString(float x, float y, const char *text, float r, float g, float b, float a)
{
    // stbtt uses top-left origin; adjust y baseline as needed
    float xpos = x, ypos = y;
    
    for (const char *c = text; *c; c++) {
        if (*c < 32 || *c >= 128) continue;
        stbtt_aligned_quad q;
        // opengl_fillrule=0 for D3D (top-left origin)
        stbtt_GetBakedQuad(FontCdata, 512, 512, *c - 32, &xpos, &ypos, &q, 1);
        
        EmitQuad(q, r, g, b, a);  // push into your vertex buffer
    }
}

static void
MatLookAt(float *m, float ex, float ey, float ez,
          float cx, float cy, float cz)
{
    // forward vector eye to target
    float fx = cx-ex, fy = cy-ey, fz = cz-ez;
    float len = sqrtf(fx*fx + fy*fy + fz*fz);
    fx/=len; fy/=len; fz/=len;
    
    // right vector forward
    float rx = fy*0 - fz*1, ry = fz*0 - fx*0, rz = fx*1 - fy*0;
    len = sqrtf(rx*rx + ry*ry + rz*rz);
    rx/=len; ry/=len; rz/=len;
    
    float ux = ry*fz - rz*fy;
    float uy = rz*fx - rx*fz;
    float uz = rx*fy - ry*fx;
    
    MatIdentity(m);
    m[0]=rx; m[4]=ry; m[8]=rz;
    m[1]=ux; m[5]=uy; m[9]=uz;
    m[2]=-fx; m[6]=-fy; m[10]=-fz;
    m[12]=-(rx*ex+ry*ey+rz*ez);
    m[13]=-(ux*ex+uy*ey+uz*ez);
    m[14]= (fx*ex+fy*ey+fz*ez);
}
#endif //D3D_H
