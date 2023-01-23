#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"

#include "hitbox.h"

#define VECTOR2_ZERO (Vector2) {0.0f, 0.0f}

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
#define FRAME_TIME 0.1

#define BACKGROUND_COLOR GRAY

#define FRAME_ROW_COLOR GRAY
#define FRAME_ROW_SIZE 32
#define FRAME_RHOMBUS_RADIUS 12
#define FRAME_RHOMBUS_UNSELECTED_COLOR (Color) {123, 123, 123, 255}
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

    float spriteScale = WINDOW_Y * TEXTURE_HEIGHT_IN_WINDOW / sprite.height;
    float drawSizeX = FRAME_WIDTH * spriteScale;
    float drawSizeY = sprite.height * spriteScale;
    Vector2 spritePos = {(WINDOW_X - drawSizeX) / 2.0f, (WINDOW_Y - drawSizeY) / 2.0f};

    int frameIdx = 0;
    int frameCount = sprite.width / FRAME_WIDTH;
    bool *hitboxActiveFrames = calloc(sizeof(bool), frameCount);

    Hitbox hitbox = {40.0f, 40.0f, 24.0f};

    typedef enum State {
        IDLE = 0,
        PLAYING = 1,
        DRAGGING_HANDLE = 2
    } State;
    State state = IDLE;
    float playingFrameTime = 0.0f;
    Handle draggingHandle = NONE;

    while (!WindowShouldClose()) {
        
        if (IsKeyPressed(EXIT_KEY) && IsKeyDown(EXIT_KEY_MODIFIER))
            break;
        
        if (state == DRAGGING_HANDLE) {
            if (IsMouseButtonReleased(SELECT_BUTTON)) {
                state = IDLE;
            } else {
                SetHitboxHandle(GetMousePosition(), spritePos, spriteScale, &hitbox, draggingHandle); // not handling illegal handle set for now b/c it shouldn't happen.
            }
        } else if (IsMouseButtonPressed(SELECT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            float mouseXInFrames = mousePos.x;
            float mouseYInFrames = mousePos.y - (WINDOW_Y - HITBOX_ROW_SIZE - FRAME_ROW_SIZE);


            if (mouseXInFrames >= 0.0f && mouseXInFrames < FRAME_ROW_SIZE * frameCount && mouseYInFrames >= 0.0f && mouseYInFrames < FRAME_ROW_SIZE + HITBOX_ROW_SIZE) {
                state = IDLE;
                frameIdx = Clamp((int) mouseXInFrames / FRAME_ROW_SIZE, 0, frameCount - 1); // clamp just to be safe

                if (mouseYInFrames > FRAME_ROW_SIZE) hitboxActiveFrames[frameIdx] = !hitboxActiveFrames[frameIdx];
            } else if ((draggingHandle = SelectHitboxHandle(mousePos, spritePos, spriteScale, hitbox)) != NONE) {
                state = DRAGGING_HANDLE;
            }
        } else if (IsKeyPressed(PLAY_ANIMATION_KEY)) {
            if (state == PLAYING) {
                state = IDLE;
            } else {
                state = PLAYING;
                playingFrameTime = 0.0f;
            }
        } else {
            int direction = (IsKeyPressed(NEXT_FRAME_KEY) ? 1 : 0) - (IsKeyPressed(PREVIOUS_FRAME_KEY) ? 1 : 0);
            if (direction) {
                state = IDLE;
                int newFrame = frameIdx + direction;

                if (newFrame < 0) frameIdx = frameCount - 1;
                else if (newFrame >= frameCount) frameIdx = 0;
                else frameIdx = newFrame;
            }
        }

        if (state == PLAYING) {
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
        Rectangle dest = {spritePos.x, spritePos.y, drawSizeX, drawSizeY};
        DrawTexturePro(sprite, source, dest, VECTOR2_ZERO, 0.0f, WHITE);
        
        if (hitboxActiveFrames[frameIdx]) {
            DrawHitbox(spritePos, spriteScale, hitbox);
        }

        DrawRectangle(0, WINDOW_Y - FRAME_ROW_SIZE, frameCount * FRAME_ROW_SIZE, FRAME_ROW_SIZE, FRAME_ROW_COLOR);
        for (int i = 0; i < frameCount; i++) {
            int xPos = i * FRAME_ROW_SIZE + FRAME_ROW_SIZE / 2;
    
            Color hitboxColor = hitboxActiveFrames[i] ? HITBOX_CIRCLE_ACTIVE_COLOR : HITBOX_CIRCLE_INACTIVE_COLOR;
            int hitboxYPos = WINDOW_Y - HITBOX_ROW_SIZE / 2;
            DrawCircle(xPos, hitboxYPos, HITBOX_CIRCLE_RADIUS, hitboxColor);

            Color frameColor = i == frameIdx ? FRAME_RHOMBUS_SELECTED_COLOR : FRAME_RHOMBUS_UNSELECTED_COLOR;
            Vector2 frameCenter = {xPos, WINDOW_Y - HITBOX_ROW_SIZE - FRAME_ROW_SIZE / 2};
            DrawRhombus(frameCenter, FRAME_RHOMBUS_RADIUS, FRAME_RHOMBUS_RADIUS, frameColor);
        }

        EndDrawing();
    }

    free(hitboxActiveFrames);
    CloseWindow();
    return EXIT_SUCCESS;
}