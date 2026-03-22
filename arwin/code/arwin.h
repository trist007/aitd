/* date = March 11th 2026 5:15 pm */

#ifndef ARWIN_H
#define ARWIN_H

#define PLAYER_MAX_SPEED 2.0f
#define PLAYER_ACCELERATION 10.0f
#define PLAYER_DECELERATION 5.0f

#define MAX_WALLS 16

#define PI32 3.14159265359f

struct Rect2D
{
    float x, z;
    float w, h;
};

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

struct Wall
{
    Vector3 start;
    Vector3 end;
};

struct Room
{
    int wall_count;
    Wall wall[MAX_WALLS];
};

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
#endif //ARWIN_H