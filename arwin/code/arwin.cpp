#include "arwin.h"

// -----------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------
Vector2
dist(Vector2 a, Vector2 b)
{
    Vector2 Result;
    
    Result.x = a.x - b.x;
    Result.y = a.y - b.y;
    
    return(Result);
}

void
InitRoom(GameState *game_state, int room_id)
{
    switch(room_id)
    {
        case ROOM_1:
        {
            game_state->room[room_id].wall_count = 3;
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

bool Rect2D_IntersectsLine(Rect2D rect, Line2D line)
{
    // Translate so rect is centered at origin
    float ax = line.a.x - rect.x;
    float az = line.a.y - rect.z;
    float bx = line.b.x - rect.x;
    float bz = line.b.y - rect.z;
    
    // AABB axes reject if segment is fully outside any slab
    if (fminf(ax, bx) > rect.w || fmaxf(ax, bx) < - rect.w) return(0);
    if (fminf(az, bz) > rect.h || fmaxf(az, bz) < - rect.h) return(0);
    
    // Check if rect is on one side of the line
    float dx = bx - ax;
    float dz = bz - az;
    
    // Normal of the segment, check signed distances of rect corners
    float n0 = dx * (rect.w - ax) - dz *  (rect.h - az);
    float n1 = dx * (rect.w - ax) - dz * (-rect.h - az);
    float n2 = dx *(-rect.w - ax) - dz *  (rect.h - az);
    float n3 = dx *(-rect.w - ax) - dz * (-rect.h - az);
    
    if (n0 > 0 && n1 > 0 && n2 > 0 && n3 > 0) return(0);
    if (n0 < 0 && n1 < 0 && n2 < 0 && n3 < 0) return(0);
    
    return(1);
}

bool
CheckPlayerWallMinkowskiCollision(Player *player, Vector3 *next_position, Wall *wall, Vector3 *push)
{
    // Player position
    Vector2 playerPosition = { next_position->x, next_position->z };
    
    // Wall line
    Vector2 wallLine = { wall->end.x - wall->start.x, wall->end.z - wall->start.z };
    float wallLineLength = sqrtf(wallLine.x * wallLine.x + wallLine.y * wallLine.y);
    
    // Wall tangent and normal
    Vector2 tangent = { wallLine.x / wallLineLength, wallLine.y / wallLineLength };
    Vector2 normal = { -tangent.y, tangent.x };
    
    // Project player center onto wall axes
    Vector2 projectPlayerOntoWallAxes = { playerPosition.x - wall->start.x, playerPosition.y - wall->start.z };
    float t_project = projectPlayerOntoWallAxes.x *tangent.x + projectPlayerOntoWallAxes.y * tangent.y; // along wall
    float n_project = projectPlayerOntoWallAxes.x *normal.x + projectPlayerOntoWallAxes.y * normal.y; // perpendicular wall
    
    // Minkowski sum by expanding wall segment by player half-extents, project player rect extents onto each wall axis
    float extent_t = fabsf(player->width / 2 * tangent.x) + fabsf(player->length / 2 * tangent.y);
    float extent_n = fabsf(player->width / 2 * normal.x) + fabsf(player->length / 2 * normal.y);
    
    float min_t = 0.0f - extent_t;
    float max_t = wallLineLength + extent_t;
    float min_n = -extent_n;
    float max_n = extent_n;
    
    // Check if player center is inside the Minkowski expanded wall region
    if(t_project < min_t || t_project > max_t) return false;
    if(n_project < min_n || n_project > max_n) return false;
    
    // Collision - find shallowest axis to push out
    float overlap_n = extent_n - fabsf(n_project);
    float overlap_t_left = t_project - min_t;
    float overlap_t_right = max_t - t_project;
    float overlap_t = fminf(overlap_t_left, overlap_t_right);
    
    if(overlap_n < overlap_t)
    {
        // Push along normal, player slides along the wallLine
        float sign = (n_project >= 0) ? 1.0f : -1.0f;
        push->x = normal.x * overlap_n * sign;
        push->z = normal.y * overlap_n * sign;
    }
    else
    {
        // Push along tangent (hit an endpoint)
        float sign = (t_project - (min_t + max_t) * 0.5f) >= 0 ? 1.0f : -1.0f;
        push->x = tangent.x * overlap_t * sign;
        push->z = tangent.y * overlap_t * sign;
    }
    
    return(true);
}

void
DebugDrawMinkowski(Player *player, Vector3 *next_position, Wall *wall, Camera3D camera)
{
    // Wall basis
    float wx = wall->end.x - wall->start.x;
    float wz = wall->end.z - wall->start.z;
    float wlen = sqrtf(wx*wx + wz*wz);
    float tx = wx/wlen, ty = wz/wlen;
    float nx = -ty,     ny =  tx;
    
    float ext_t = fabsf(player->width/2 * tx) + fabsf(player->length/2 * ty);
    float ext_n = fabsf(player->width/2 * nx) + fabsf(player->length/2 * ny);
    
    float y = next_position->y; // draw at player height
    
    // Minkowski expanded rect corners (world space)
    Vector3 corners[4] = {
        { wall->start.x - tx*ext_t - nx*ext_n, y, wall->start.z - ty*ext_t - ny*ext_n },
        { wall->end.x   + tx*ext_t - nx*ext_n, y, wall->end.z   + ty*ext_t - ny*ext_n },
        { wall->end.x   + tx*ext_t + nx*ext_n, y, wall->end.z   + ty*ext_t + ny*ext_n },
        { wall->start.x - tx*ext_t + nx*ext_n, y, wall->start.z - ty*ext_t + ny*ext_n },
    };
    
    // Draw Minkowski expanded rect
    for(int i = 0; i < 4; i++)
        DrawLine3D(corners[i], corners[(i+1) % 4], RED);
    
    // Draw raw wall
    DrawLine3D(
               (Vector3){ wall->start.x, y, wall->start.z },
               (Vector3){ wall->end.x,   y, wall->end.z   },
               WHITE
               );
    
    // Draw player rect (axis-aligned)
    float px = next_position->x, pz = next_position->z;
    float hw = player->width/2,  hl = player->length/2;
    Vector3 prect[4] = {
        { px-hw, y, pz-hl },
        { px+hw, y, pz-hl },
        { px+hw, y, pz+hl },
        { px-hw, y, pz+hl },
    };
    for(int i = 0; i < 4; i++)
        DrawLine3D(prect[i], prect[(i+1) % 4], BLUE);
    
    // Draw player center
    DrawSphere(*next_position, 0.05f, BLUE);
    
    // Draw wall normal at midpoint
    float mid_x = (wall->start.x + wall->end.x) * 0.5f;
    float mid_z = (wall->start.z + wall->end.z) * 0.5f;
    DrawLine3D(
               (Vector3){ mid_x,        y, mid_z        },
               (Vector3){ mid_x + nx,   y, mid_z + ny   },
               GREEN
               );
}

// -----------------------------------------------------------------------
// Update
// -----------------------------------------------------------------------
void
UpdateGame(GameState *game_state, float delta_time)
{
    Player *player = &game_state->player;
    
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
    
    // Collisions
    
    Vector3 next_position = {
        player->position.x + player->velocity.x * delta_time,
        player->position.y + player->velocity.y * delta_time,
        player->position.z + player->velocity.z * delta_time
    };
    
    for (int i = 0;
         i < game_state->room[game_state->currentRoom].wall_count;
         i++)
    {
        Vector3 push = {};
        if(CheckPlayerWallMinkowskiCollision(player, &next_position, &game_state->room[game_state->currentRoom].wall[i], &push))
        {
            next_position.x += push.x;
            next_position.z += push.z;
        }
    }
    
    player->position = next_position;
    
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
