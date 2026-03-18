/* date = March 11th 2026 5:15 pm */

#ifndef ARWIN_H
#define ARWIN_H

#define PLAYER_MAX_SPEED 2.0f
#define PLAYER_ACCELERATION 10.0f
#define PLAYER_DECELERATION 5.0f

#define MAX_WALLS 16

#define PI32 3.14159265359f

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
    Vector2 start;
    Vector2 end;
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
    Camera camera;
    
    int currentRoom;
    Room room[ROOM_ID_COUNT];
    
    float screenWidth, screenHeight;
};
#endif //ARWIN_H