/* date = March 11th 2026 5:15 pm */

#ifndef AITD_H
#define AITD_H

#define PLAYER_SPEED 2.0f
#define PLAYER_ACCELERATION 1.0f

struct Vec3
{
    float x, y, z;
    
    Vec3 operator+(float scalar)
    {
        return {x + scalar, y + scalar, z + scalar};
    }
    
    Vec3& operator+=(float scalar)
    {
        x += scalar;
        y += scalar;
        z += scalar;
        return *this;
    }
};

struct Player
{
    Model model;
    Vec3 position;
    Vec3 velocity;
    float yaw = 0.0f;
    float anim_time    = 0.0f;
    float player_angle = -90.0f;
    float player_y     = -0.6f;
    bool isWalking = false;
};

struct game_state
{
    Player player;
};
#endif //AITD_H
