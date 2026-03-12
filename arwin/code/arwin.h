/* date = March 11th 2026 5:15 pm */

#ifndef ARWIN_H
#define ARWIN_H

#define PLAYER_SPEED 2.0f
#define PLAYER_ACCELERATION 1.0f

#define PI32 3.14159265359f

struct Player
{
    Model model;
    Vector3 position;
    Vector3 velocity;
    float yaw = 0.0f;
    float player_angle = 0.0f;
    bool isWalking = false;
    
    ModelAnimation *anims;
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
