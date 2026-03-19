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
            game_state->room[room_id].wall_count = 3;
            //game_state->room[room_id].wall[0] = { {-2.2f, 0.0f, 7.90f}, {1.65f, 0.0f, -9.30f} }; 
            //game_state->room[room_id].wall[1] = { {1.65f, 0.0f,-9.30}, {6.90f, 0.0f, -0.66f} }; 
            //game_state->room[room_id].wall[0] = { {-9.0f, 0.0f, 4.65f}, {5.72f, 0.0f, 0.30f} }; 
            //game_state->room[room_id].wall[1] = { {5.72f, 0.0f,0.30}, {7.70f, 0.0f, 9.00f} }; 
            game_state->room[room_id].wall[0] = { {-7.85f, 0.0f, 6.95f}, {4.88f, 0.0f, 4.82f} }; 
            game_state->room[room_id].wall[1] = { {4.88f, 0.0f, 4.82}, {4.77f, 0.0f, 9.19f} }; 
            game_state->room[room_id].wall[2] = { {4.77f, 0.0f, 9.18f}, {-5.49f, 0.0f, 10.19f} }; 
            game_state->currentRoom = ROOM_1;
        } break;
        
        default:
        {
            TraceLog(LOG_WARNING, "InitRoom:, unknown room_id: %d", room_id);
        } break;
    }
}

void
CheckPlayerWallCollisions(Player *player, Room *room, float delta_time)
{
    // compute where player will be next frame
    Vector2 next_pos = { 
        player->position.x + player->velocity.x * delta_time,
        player->position.z + player->velocity.z * delta_time
    };
    
    for(int i = 0;
        i < room->wall_count;
        i++)
    {
        if(CheckCollisionPointLine(next_pos,
                                   Vector2{room->wall[i].start.x, room->wall[i].start.z },
                                   Vector2{room->wall[i].end.x, room->wall[i].end.z },
                                   1))
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
    
    CheckPlayerWallCollisions(player, &game_state->room[game_state->currentRoom], delta_time);
    
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
