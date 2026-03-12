#include "arwin.h"

// -----------------------------------------------------------------------
// Update
// -----------------------------------------------------------------------
void
UpdateGame(GameState *game_state, float delta_time)
{
    Player *player = &game_state->player;
    
    player->velocity.x = 0.0f;
    player->velocity.z = 0.0f;
    player->isWalking = false;
    
    if(player->yaw > PI32)
        player->yaw -= 2.0f * PI32;
    if(player->yaw < -PI32)
        player->yaw += 2.0f * PI32;
    
    float forward_x = sinf(player->yaw);
    float forward_z = cosf(player->yaw);
    
    //TraceLog(LOG_INFO, "yaw: %.2f  fx: %.2f  fz: %.2f\n", player->yaw, forward_x, forward_z);
    
    if(IsKeyDown(KEY_UP))
    {
        player->position.x -= forward_x * PLAYER_SPEED * delta_time;
        player->position.z -= forward_z * PLAYER_SPEED * delta_time;
        player->isWalking = true;
    }
    if(IsKeyDown(KEY_DOWN))
    {
        player->position.x += forward_x * PLAYER_SPEED * delta_time;
        player->position.z += forward_z * PLAYER_SPEED * delta_time;
        player->isWalking = true;
    }
    
    if(IsKeyDown(KEY_LEFT))
        player->yaw += 2.0f * delta_time;
    if(IsKeyDown(KEY_RIGHT))
        player->yaw -= 2.0f * delta_time;
    
    int new_anim = player->isWalking ? 2 : 1;
    if(new_anim != player->anim_index)
    {
        player->anim_index = new_anim;
        player->anim_frame = 0;
    }
    
    if(player->anim_frame >= player->anims[player->anim_index].frameCount)
        player->anim_frame = 0;
    UpdateModelAnimation(player->model, player->anims[player->anim_index], player->anim_frame++);
}