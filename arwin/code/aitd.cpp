#include <math.h>
// -----------------------------------------------------------------------
// Update
// -----------------------------------------------------------------------
void
UpdateGame(void)
{
    if(GameState.player.yaw > PI32)
        GameState.player.yaw -= 2.0f * PI32;
    if(GameState.player.yaw < -PI32)
        GameState.player.yaw += 2.0f * PI32;
    
    float forward_x = sinf(GameState.player.yaw);
    float forward_z = cosf(GameState.player.yaw);
    
    char dbg[256];
    snprintf(dbg, sizeof(dbg), "yaw: %.2f  fx: %.2f  fz: %.2f\n",
             GameState.player.yaw, forward_x, forward_z);
    OutputDebugStringA(dbg);
    
    GameState.player.velocity.x = 0.0f;
    GameState.player.velocity.z = 0.0f;
    GameState.player.isWalking = false;
    
    if(GetAsyncKeyState(VK_UP) & 0x8000)
    {
        GameState.player.position.x += forward_x * PLAYER_SPEED * delta_time;
        GameState.player.position.z += forward_z * PLAYER_SPEED * delta_time;
        GameState.player.isWalking = true;
    }
    if(GetAsyncKeyState(VK_DOWN) & 0x8000)
    {
        GameState.player.position.x -= forward_x * PLAYER_SPEED * delta_time;
        GameState.player.position.z -= forward_z * PLAYER_SPEED * delta_time;
        GameState.player.isWalking = true;
    }
    
    if(GetAsyncKeyState(VK_LEFT)  & 0x8000)
        GameState.player.yaw -= 2.0f * delta_time;
    if(GetAsyncKeyState(VK_RIGHT) & 0x8000)
        GameState.player.yaw += 2.0f * delta_time;
    
}
// -----------------------------------------------------------------------
// Render
// -----------------------------------------------------------------------
void
RenderGame()
{
    // Delta time
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    delta_time = (float)(current_time.QuadPart - last_time.QuadPart) / (float)perf_freq.QuadPart;
    last_time  = current_time;
    
    // Animate
    //if(anim_time > 1.666f)
    //anim_time = 0.0f;
    if(delta_time > 0.1f)
        delta_time = 0.1f;
    GameState.player.anim_time += delta_time;
    if(GameState.player.isWalking)
        UpdateAnimation(&GameState.player.model, 2, GameState.player.anim_time);
    else
        UpdateAnimation(&GameState.player.model, 1, GameState.player.anim_time);
    
    // Clear
    float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    context->ClearRenderTargetView(render_target, clear_color);
    context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH, 1.0f, 0);
    
    // ---- Draw background (no depth write) ----
    context->OMSetDepthStencilState(ds_disabled, 0);
    context->VSSetShader(vs_background, NULL, 0);
    context->PSSetShader(ps_background, NULL, 0);
    context->IASetInputLayout(input_layout_bg);
    
    UINT stride = sizeof(VertexBG);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vb_background, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->PSSetShaderResources(0, 1, &srv_background);
    context->PSSetSamplers(0, 1, &sampler);
    context->Draw(6, 0);
    
    // ---- Draw player (with depth test) ----
    context->OMSetDepthStencilState(ds_enabled, 0);
    
    // Build MVP:  proj * view * (translate * rotateY)
    float trans[16], rot[16], model[16], view[16], proj[16], mv[16], mvp[16];
    //MatTranslate(trans, 0.0f, GameState.player.player_y, -1.5f);
    MatTranslate(trans, GameState.player.position.x, GameState.player.position.y, GameState.player.position.z);
    MatRotateY(rot, GameState.player.yaw * (180.0f / PI32));
    MatMul(model, trans, rot);
    //MatIdentity(view);
    MatLookAt(view,
              0.0f, 0.0f, -10.0f,   // camera eye position
              0.0f, 2.0f,  5.0f);  // look at target
    MatPerspective(proj, 60.0f, 1920.0f / 1080.0f, 0.1f, 100.0f);
    MatMul(mv,  view,  model);
    MatMul(mvp, proj,  mv);
    
    // Upload constant buffer
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    context->Map(cb_per_frame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, mvp, sizeof(mvp));
    context->Unmap(cb_per_frame, 0);
    
    RenderModel(&GameState.player.model, srv_model);
    
    char buf[256];
    snprintf(buf, sizeof(buf), "position: %.2f %.2f %.2f",
             GameState.player.position.x, GameState.player.position.y, GameState.player.position.z);
    DrawDebugString(10, 25, buf, COLOR_RED, 1);
    
    snprintf(buf, sizeof(buf), "velocty: %.2f %.2f %.2f",
             GameState.player.velocity.x, GameState.player.velocity.y, GameState.player.velocity.z);
    DrawDebugString(10, 25 + FontPixelHeight, buf, COLOR_RED, 1);
    
    snprintf(buf, sizeof(buf), "yaw: %.2f", GameState.player.yaw);
    DrawDebugString(10, 25 + FontPixelHeight * 2, buf, COLOR_RED, 1);
    
    FlushFontQuads(context, 1920.0f, 1080.0f);
    
    swapchain->Present(1, 0);
}