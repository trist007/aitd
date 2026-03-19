#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "arwin.cpp"

#define PLAYER_MODEL "../arwin/data/models/arwin8.glb"
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
    if(IsWindowFullscreen())
        ToggleFullscreen();
    SetWindowSize(1920, 1080);
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    
    game_state->background = LoadTexture("../arwin/data/textures/background.bmp");
    game_state->player.model = LoadModel(PLAYER_MODEL);
    if(!IsModelValid(game_state->player.model))
        TraceLog(LOG_INFO, "Model is not valid\n");
    
    if(game_state->player.model.meshes[0].animVertices != NULL)
        TraceLog(LOG_INFO, "Raylib setup for CPU Skinning");
    else
        TraceLog(LOG_INFO, "Raylib setup for GPU Skinning");
    
    
    for(int i = 0; i < game_state->player.model.meshCount; i++)
        game_state->player.model.boneMatrices; // just checking they exist
    
    game_state->player.anim_count = 2;
    game_state->player.anim = LoadModelAnimations(PLAYER_MODEL, &game_state->player.anim_count);
    
#ifdef DEBUG
    printf("anim_count: %d\n", game_state->player.anim_count);
    printf("boneCount (anim): %d\n", game_state->player.anim[0].boneCount);
    printf("keyframeCount: %d\n", game_state->player.anim[0].keyframeCount);
    printf("boneCount (model): %d\n", game_state->player.model.skeleton.boneCount);
    printf("boneMatrices: %p\n", game_state->player.model.boneMatrices);
#endif
    if(!IsModelAnimationValid(game_state->player.model, game_state->player.anim[0]))
        TraceLog(LOG_DEBUG, "Model animation is not valid\n");
    
    UpdateModelAnimation(game_state->player.model,
                         game_state->player.anim[IDLE],
                         game_state->player.anim_frame);
    
    InitRoom(game_state, ROOM_1);
    
    // Player starting position
    game_state->player.position = { 2.0f, 0.0f, 3.0f };
    
    // Define the camera to look into our 3d world
    game_state->camera = { 0 };
    game_state->camera.position = Vector3{ 8.0f, 3.0f, 5.0f }; // Camera position
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
        
        
        DrawText("Pos",                                             40,  10,  20, BLACK);
        DrawText("Velocity",                                        120, 10,  20, BLACK);
        DrawText(TextFormat("X: "),                                 10,  30,  20, BLACK);
        DrawText(TextFormat("%.2f", game_state->player.position.x), 40,  30,  20, BLACK);
        DrawText(TextFormat("%.2f", game_state->player.velocity.x), 120, 30,  20, BLACK);
        
        DrawText(TextFormat("Y: "),                                 10,  50,  20, BLACK);
        DrawText(TextFormat("%.2f", game_state->player.position.y), 40,  50,  20, BLACK);
        DrawText(TextFormat("%.2f", game_state->player.velocity.y), 120, 50,  20, BLACK);
        
        
        DrawText(TextFormat("Z: "),                                 10,  70,  20, BLACK);
        DrawText(TextFormat("%.2f", game_state->player.position.z), 40,  70,  20, BLACK);
        DrawText(TextFormat("%.2f", game_state->player.velocity.z), 120, 70,  20, BLACK);
        
        DrawText(TextFormat("Yaw %.2f",
                            game_state->player.yaw), 10, 100, 20, BLACK);
        
        DrawText(TextFormat("Animation %s\n",
                            game_state->player.anim[game_state->player.anim_index].name), 10, 120, 20, BLACK);
        
        DrawText(TextFormat("Animation frame %d\n",
                            game_state->player.anim_frame), 10, 140, 20, BLACK);
        
        DrawText(TextFormat("Raylib version: %s\n",
                            RAYLIB_VERSION), 10, 160, 20, BLACK);
        EndDrawing();
        //----------------------------------------------------------------------------------
    }
    
    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
    
    return 0;
}
