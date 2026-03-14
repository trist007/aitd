#include "arwin.h"

// -----------------------------------------------------------------------
// Update
// -----------------------------------------------------------------------
void
UpdateGame(GameState *game_state, float delta_time)
{
    Player *player = &game_state->player;
    
    //player->velocity.x = 0.0f;
    //player->velocity.z = 0.0f;
    player->isWalking = false;
    
    if(player->yaw > PI32)
        player->yaw -= 2.0f * PI32;
    if(player->yaw < -PI32)
        player->yaw += 2.0f * PI32;
    
    float forward_x = sinf(player->yaw);
    float forward_z = cosf(player->yaw);
    
    float wish_x = 0.0f;
    float wish_y = 0.0f;
    float wish_z = 0.0f;
    
    if(IsKeyDown(KEY_UP))
    {
        wish_x -= forward_x;
        wish_z -= forward_z;
        player->isWalking = true;
    }
    if(IsKeyDown(KEY_DOWN))
    {
        wish_x += forward_x;
        wish_z += forward_z;
        player->isWalking = true;
    }
    
    if(player->isWalking)
    {
        player->velocity.x += wish_x * PLAYER_ACCELERATION * delta_time;
        player->velocity.z += wish_z * PLAYER_ACCELERATION * delta_time;
        
        // Clamp to max speed
        float speed = sqrtf(player->velocity.x * player->velocity.x +
                            player->velocity.z * player->velocity.z);
        
        if(speed > PLAYER_MAX_SPEED)
        {
            float scale = PLAYER_MAX_SPEED / speed;
            player->velocity.x *= scale;
            player->velocity.z *= scale;
        }
    }
    else
    {
        float speed = sqrtf(player->velocity.x * player->velocity.x +
                            player->velocity.z * player->velocity.z);
        
        if(speed > 0.0f)
        {
            float drop = speed * PLAYER_DECELERATION * delta_time;
            float new_speed = speed - drop;
            if(new_speed < 0.0f) new_speed = 0.0f;
            float scale = new_speed / speed;
            player->velocity.x *= scale;
            player->velocity.z *= scale;
            
        }
    }
    
    if(IsKeyDown(KEY_LEFT))
        player->yaw += 2.0f * delta_time;
    if(IsKeyDown(KEY_RIGHT))
        player->yaw -= 2.0f * delta_time;
    
    player->position.x += player->velocity.x * delta_time;
    player->position.y += player->velocity.y * delta_time;
    player->position.z += player->velocity.z * delta_time;
    
    int new_anim = player->isWalking ? WALK : IDLE;
    if(new_anim != player->anim_index)
    {
        player->anim_index = new_anim;
        player->anim_frame = 0;
    }
    
    player->anim_frame++;
    if(player->anim_frame >= player->anim[player->anim_index].keyframeCount)
        player->anim_frame = 0;
    UpdateModelAnimation(player->model, player->anim[new_anim], player->anim_frame);
    
}