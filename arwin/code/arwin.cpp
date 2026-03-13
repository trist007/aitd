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
    
    int new_anim = player->isWalking ? WALK : IDLE;
    if(new_anim != player->anim_index)
    {
        player->anim_index = new_anim;
        player->anim_frame = 0;
    }
    
    player->anim_frame++;
    if(player->anim_frame >= player->anim[player->anim_index].keyframeCount)
        player->anim_frame = 0;
#if 0
    for(int i = 0; i < player->anim_count; i++)
        TraceLog(LOG_INFO, "anim[%d] name: %s  frameCount: %d  boneCount: %d",
                 i,
                 player->anim[i].name,
                 player->anim[i].keyframeCount,
                 player->anim[i].boneCount);
#endif
    UpdateModelAnimation(player->model, player->anim[new_anim], player->anim_frame);
}