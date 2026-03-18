#include "arwin.h"
// -----------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------
void
InitRoom(GameState *game_state, int room_id)
{
    switch(room_id)
    {
        case ROOM_1:
        {
            game_state->room[room_id].wall_count = 2;
            game_state->room[room_id].wall[0] = { {-1.80f, 7.77f}, {1.89f,-8.98f} }; 
            game_state->room[room_id].wall[1] = { {1.89f,-8.98}, {6.67f, -0.66f} }; 
            game_state->currentRoom = ROOM_1;
        } break;
        
        default:
        {
            TraceLog(LOG_WARNING, "InitRoom:, unknown room_id: %d", room_id);
        } break;
    }
}

void
CheckPlayerCollisions(Player *player, Room *room, float delta_time)
{
    Vector2 player_xz = { player->position.x, player->position.z };
    
    for(int i = 0;
        i < room->wall_count;
        i++)
    {
        if(CheckCollisionPointLine(player_xz, room->wall[i].start, room->wall[i].end, 1))
        {
            player->position.x -= player->velocity.x * delta_time;
            player->position.z -= player->velocity.z * delta_time;
            player->velocity.x = 0.0f;
            player->velocity.z = 0.0f;
            break;
        }
    }
}

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
    player->isSearching = false;
    
    if(player->yaw > PI32)
        player->yaw -= 2.0f * PI32;
    if(player->yaw < -PI32)
        player->yaw += 2.0f * PI32;
    
    float forward_x = sinf(player->yaw);
    float forward_z = cosf(player->yaw);
    
    float input_x = 0.0f;
    float input_y = 0.0f;
    float input_z = 0.0f;
    
    if(IsKeyDown(KEY_UP))
    {
        input_x += forward_x;
        input_z += forward_z;
        player->isWalking = true;
        player->isSearching = false;
    }
    if(IsKeyDown(KEY_DOWN))
    {
        input_x -= forward_x;
        input_z -= forward_z;
        player->isWalking = true;
        player->isSearching = false;
    }
    if(IsKeyDown(KEY_SPACE))
    {
        player->isSearching = true;
        player->isWalking = false;
    }
    
    if(player->isWalking)
    {
        player->velocity.x += input_x * PLAYER_ACCELERATION * delta_time;
        player->velocity.z += input_z * PLAYER_ACCELERATION * delta_time;
        
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
    
    // Updating position
    player->position.x += player->velocity.x * delta_time;
    player->position.y += player->velocity.y * delta_time;
    player->position.z += player->velocity.z * delta_time;
    
    CheckPlayerCollisions(player, &game_state->room[game_state->currentRoom], delta_time);
    
    int new_anim = player->isWalking ? WALK : IDLE;
    
    if(player->isSearching)
    {
        new_anim = SEARCH;
        if(new_anim != player->anim_index)
        {
            player->anim_index = SEARCH;
            player->anim_frame = 0;
        }
        
        // Clamping to last frame of SEARCH
        if(player->anim_frame < player->anim[SEARCH].keyframeCount - 1)
            player->anim_frame++;
    }
    else
    {
        if(new_anim != player->anim_index)
        {
            player->anim_index = new_anim;
            player->anim_frame = 0;
        }
        
        player->anim_frame++;
        if(player->anim_frame >= player->anim[player->anim_index].keyframeCount)
            player->anim_frame = 0;
    }
    
    UpdateModelAnimation(player->model, player->anim[player->anim_index], player->anim_frame);
    
}