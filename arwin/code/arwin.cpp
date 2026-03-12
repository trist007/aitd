#include "arwin.h"

// -----------------------------------------------------------------------
// Update
// -----------------------------------------------------------------------
void
UpdateGame(GameState *game_state, float delta_time)
{
    Player *player = &game_state->player;
    if(player->isWalking)
        player->anim_index = 2;
    else
        player->anim_index = 1;
    
    UpdateModelAnimation(player->model, player->anims[player->anim_index], player->anim_frame++);
    
    if(game_state->player.yaw > PI32)
        game_state->player.yaw -= 2.0f * PI32;
    if(game_state->player.yaw < -PI32)
        game_state->player.yaw += 2.0f * PI32;
    
    float forward_x = sinf(game_state->player.yaw);
    float forward_z = cosf(game_state->player.yaw);
    
    TraceLog(LOG_INFO, "yaw: %.2f  fx: %.2f  fz: %.2f\n",
             game_state->player.yaw, forward_x, forward_z);
    
    game_state->player.velocity.x = 0.0f;
    game_state->player.velocity.z = 0.0f;
    game_state->player.isWalking = false;
    
    if(IsKeyDown(KEY_UP))
    {
        game_state->player.position.x += forward_x * PLAYER_SPEED * delta_time;
        game_state->player.position.z += forward_z * PLAYER_SPEED * delta_time;
        game_state->player.isWalking = true;
    }
    if(IsKeyDown(KEY_DOWN))
    {
        game_state->player.position.x -= forward_x * PLAYER_SPEED * delta_time;
        game_state->player.position.z -= forward_z * PLAYER_SPEED * delta_time;
        game_state->player.isWalking = true;
    }
    
    if(IsKeyDown(KEY_LEFT))
        game_state->player.yaw -= 2.0f * delta_time;
    if(IsKeyDown(KEY_RIGHT))
        game_state->player.yaw += 2.0f * delta_time;
    
}