/* date = March 11th 2026 5:15 pm */

#ifndef ARWIN_H
#define ARWIN_H

#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#define PLAYER_MAX_SPEED 2.0f
#define PLAYER_ACCELERATION 10.0f
#define PLAYER_DECELERATION 5.0f

#define MAX_WALLS 16

#define PI32 3.14159265359f

typedef struct Rect2D Rect2D;
struct Rect2D
{
    float x, z;
    float w, h;
};

typedef struct Line2D Line2D;
struct Line2D
{
    Vector2 a, b;
};

enum playerAnimation
{
    TPOSE = 0,
    IDLE,
    WALK,
    SEARCH
};

enum RoomId
{
    ROOM_1 = 0,
    ROOM_ID_COUNT
};

typedef struct Wall Wall;
struct Wall
{
    Vector3 start;
    Vector3 end;
};

typedef struct Room Room;
struct Room
{
    int wall_count;
    Wall wall[MAX_WALLS];
};

typedef struct Player Player;
struct Player
{
    Model model;
    Vector3 position;
    Vector3 velocity;
    float yaw;
    float width;
    float length;
    float height;
    bool isWalking;
    bool isSearching;
    
    ModelAnimation *anim;
    float anim_time;
    int anim_count;
    int anim_index;
    int anim_frame;
};

typedef struct GameState GameState;
struct GameState
{
    Player player;
    Texture2D background;
    Texture2D furniture_overlay;
    Texture2D furniture_overlay_back;
    Texture2D furniture_overlay_front;
    Camera camera;
    
    int currentRoom;
    Room room[ROOM_ID_COUNT];
    
    float screenWidth, screenHeight;
};

Vector2 dist(Vector2 a, Vector2 b);
void InitRoom(GameState *game_state, int room_id);
bool Rect2D_IntersectsLine(Rect2D rect, Line2D line);
bool CheckPlayerWallMinkowskiCollision(Player *player, Vector3 *next_position, Wall *wall, Vector3 *push);
void DebugDrawMinkowski(Player *player, Vector3 *next_position, Wall *wall, Camera3D camera);
void UpdateGame(GameState *game_state, float delta_time);

#endif //ARWIN_H