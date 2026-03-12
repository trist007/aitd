#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include "arwin.cpp"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1920;
    const int screenHeight = 1080;
    
    GameState gameState = {};
    GameState *game_state = &gameState;
    
    InitWindow(game_state->screenWidth, game_state->screenHeight, "Arwinian Chronicles");
    SetWindowSize(1920, 1080);
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    
    game_state->background = LoadTexture("../data/textures/background.bmp");
    game_state->player.model = LoadModel("../data/models/Arwin2.glb");
    //UpdateModelAnimation(game_state->player.model, game_state->player.anims[0], 0);
    
    for(int i = 0; i < game_state->player.model.meshCount; i++)
        game_state->player.model.meshes[i].boneMatrices; // just checking they exist
    
    game_state->player.anims = LoadModelAnimations("../data/models/Arwin2.glb", &game_state->player.anim_count);
    
    // Define the camera to look into our 3d world
    game_state->camera = { 0 };
    game_state->camera.position = Vector3{ 8.0f, 6.0f, 5.0f }; // Camera position
    game_state->camera.target = Vector3{ 0.0f, 1.0f, 0.0f };      // Camera looking at point
    game_state->camera.up = Vector3{ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    game_state->camera.fovy = 60.0f;                                // Camera field-of-view Y
    game_state->camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
    
    //DisableCursor();                    // Limit cursor to relative movement inside the window
    
    SetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------
    
    // Main game loop
    while (!WindowShouldClose())        // Detect window close button or ESC key
    {
        float deltaTime = GetFrameTime();
        
        // Update
        //----------------------------------------------------------------------------------
        //UpdateCamera(&camera, CAMERA_THIRD_PERSON);
        UpdateGame(game_state, deltaTime);
        
        //----------------------------------------------------------------------------------
        
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
        
        ClearBackground(RAYWHITE);
        DrawTexturePro(game_state->background,
                       Rectangle{0, 0, 1024, 512},
                       Rectangle{0, 0, 1920, 1080},
                       Vector2{0, 0},
                       0.0f,
                       WHITE);
        
        BeginMode3D(game_state->camera);
        
        //DrawCube(cubePosition, 2.0f, 2.0f, 2.0f, RED);
        //DrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, MAROON);
        //DrawGrid(10, 1.0f);
        DrawModelEx(game_state->player.model,
                    game_state->player.position,
                    Vector3{0,1,0},
                    game_state->player.yaw * (180.0f/PI32),
                    Vector3{1,1,1}, WHITE);
        
        EndMode3D();
        
        DrawText(TextFormat("Position %.2f %.2f %.2f",
                            game_state->player.position.x,
                            game_state->player.position.y,
                            game_state->player.position.z), 10, 10, 20, RED);
        
        DrawText(TextFormat("Velocity %.2f %.2f %.2f",
                            game_state->player.position.x,
                            game_state->player.position.y,
                            game_state->player.position.z), 10, 30, 20, RED);
        
        
        DrawText(TextFormat("Yaw %.2f",
                            game_state->player.yaw), 10, 50, 20, RED);
        
        DrawText(TextFormat("Animation %s\n",
                            game_state->player.anims[game_state->player.anim_index].name), 10, 70, 20, RED);
        EndDrawing();
        //----------------------------------------------------------------------------------
    }
    
    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
    
    return 0;
}