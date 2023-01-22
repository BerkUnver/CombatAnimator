#include <stdlib.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"

const Vector2 VECTOR2_ZERO = {0.0f, 0.0f};

#define EXIT_KEY KEY_E
#define EXIT_KEY_MODIFIER KEY_LEFT_CONTROL

#define TEST_IMAGE_PATH "Jab.png"
#define MAX_SCALE 2.0f
#define MIN_SCALE 2.0f
#define SCROLL_SPEED 0.25f
#define APP_NAME "Combat Animator"
#define WINDOW_X 800
#define WINDOW_Y 480
#define PLAY_ANIMATION_KEY KEY_ENTER
#define PREVIOUS_FRAME_KEY KEY_LEFT
#define NEXT_FRAME_KEY KEY_RIGHT
#define SELECT_BUTTON MOUSE_BUTTON_LEFT

#define FRAME_WIDTH 160
#define TEXTURE_HEIGHT_IN_WINDOW 0.5f
#define FRAME_TIME 0.1f

#define BACKGROUND_COLOR GRAY
#define FRAME_BOX_COLOR GRAY
#define FRAME_BOX_SIZE 32
#define FRAME_RHOMBUS_RADIUS 12
const Color FRAME_RHOMBUS_UNSELECTED_COLOR = {123, 123, 123, 255};
#define FRAME_RHOMBUS_SELECTED_COLOR RAYWHITE



void DrawRhombus(Vector2 pos, float xSize, float ySize, Color color) {
    Vector2 topPoint = {pos.x, pos.y - ySize};
    Vector2 leftPoint = {pos.x - xSize, pos.y};
    Vector2 rightPoint = {pos.x + xSize, pos.y};
    Vector2 bottomPoint = {pos.x, pos.y + ySize};
    DrawTriangle(rightPoint, topPoint, leftPoint, color); // must be in counterclockwise order
    DrawTriangle(leftPoint, bottomPoint, rightPoint, color);
}

int main() {
    InitWindow(WINDOW_X, WINDOW_Y, APP_NAME);
    
    SetTargetFPS(60);

    Texture2D sprite = LoadTexture(TEST_IMAGE_PATH);
    
    float drawSizeX = WINDOW_Y * TEXTURE_HEIGHT_IN_WINDOW * FRAME_WIDTH / sprite.height;
    float drawSizeY = WINDOW_Y * TEXTURE_HEIGHT_IN_WINDOW;
    float drawX = (WINDOW_X - drawSizeX) / 2.0f;
    float drawY = (WINDOW_Y - drawSizeY) / 2.0f;
    Rectangle dest = {drawX, drawY, drawSizeX, drawSizeY};

    int frameIdx = 0;
    int frameCount = sprite.width / FRAME_WIDTH;

    float playingFrameTime = 0.0f;

    // typedef enum State {
    //     IDLE = 0,
    //     PLAYING = 1,
    //     DRAGGING = 2
    // } State;

    bool playing = false;
    
    while(!WindowShouldClose()) {
        
        if (IsKeyPressed(EXIT_KEY) && IsKeyDown(EXIT_KEY_MODIFIER))
            break;
        
        if (IsKeyPressed(PLAY_ANIMATION_KEY)) {
            if (playing) {
                playing  = false;
            } else {
                playing = true;
                playingFrameTime = 0.0f;
            }
        } else {
            int direction = (IsKeyPressed(NEXT_FRAME_KEY) ? 1 : 0) - (IsKeyPressed(PREVIOUS_FRAME_KEY) ? 1 : 0);
            if (direction) {
                playing = false;
                int newFrame = frameIdx + direction;
                
                if (newFrame < 0) frameIdx = frameCount - 1;
                else if (newFrame >= frameCount) frameIdx = 0;
                else frameIdx = newFrame;
            }
        }

        if (IsMouseButtonPressed(SELECT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            float mouseXInFrames = mousePos.x;
            float mouseYInFrames = mousePos.y + FRAME_BOX_SIZE - WINDOW_Y;

            if (mouseXInFrames >= 0.0f && mouseXInFrames < FRAME_BOX_SIZE * frameCount && mouseYInFrames >= 0.0f && mouseYInFrames < FRAME_BOX_SIZE) {
                playing = false;
                frameIdx = Clamp((int) mouseXInFrames / FRAME_BOX_SIZE, 0, frameCount - 1); // clamp just to be safe
            }
        }

        if (playing) {
            playingFrameTime += GetFrameTime();
            if (playingFrameTime >= FRAME_TIME)
            {
                playingFrameTime = fmod(playingFrameTime, FRAME_TIME);
                frameIdx++;
                if (frameIdx >= frameCount) frameIdx = 0;
            }
        }
        
        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        
        Rectangle source = {FRAME_WIDTH * frameIdx, 0.0f, FRAME_WIDTH, sprite.height};
        DrawRectangle(0, WINDOW_Y - FRAME_BOX_SIZE, frameCount * FRAME_BOX_SIZE, FRAME_BOX_SIZE, FRAME_BOX_COLOR);

        for (int i = 0; i < frameCount; i++) {
            Color color = i == frameIdx ? FRAME_RHOMBUS_SELECTED_COLOR : FRAME_RHOMBUS_UNSELECTED_COLOR;
            Vector2 center = {i * FRAME_BOX_SIZE + FRAME_BOX_SIZE / 2, WINDOW_Y - FRAME_BOX_SIZE / 2};
            DrawRhombus(center, FRAME_RHOMBUS_RADIUS, FRAME_RHOMBUS_RADIUS, color);
        }
        
        DrawTexturePro(sprite, source, dest, VECTOR2_ZERO, 0.0f, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return EXIT_SUCCESS;
}