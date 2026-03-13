/* date = March 11th 2026 5:15 pm */

#ifndef ARWIN_H
#define ARWIN_H

#define PLAYER_SPEED 2.0f
#define PLAYER_ACCELERATION 1.0f

#define PI32 3.14159265359f

enum playerAnimation
{
    TPOS = 0,
    IDLE,
    WALK
};

struct Player
{
    Model model;
    Vector3 position;
    Vector3 velocity;
    float yaw;
    bool isWalking;
    
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
    
    float screenWidth, screenHeight;
};
#endif //ARWIN_H