#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "hitbox.h"
#include "editor_history.h"

#define VECTOR2_ZERO (Vector2) {0.0f, 0.0f}

#define EXIT_KEY KEY_E
#define EXIT_KEY_MODIFIER KEY_LEFT_CONTROL

#define TEST_IMAGE_PATH "Jab.png"
#define APP_NAME "Combat Animator"
#define WINDOW_X 800
#define WINDOW_Y 480
#define PLAY_ANIMATION_KEY KEY_ENTER
#define PREVIOUS_FRAME_KEY KEY_LEFT
#define NEXT_FRAME_KEY KEY_RIGHT
#define SELECT_BUTTON MOUSE_BUTTON_LEFT
#define UNDO_KEY KEY_Z
#define UNDO_KEY_MODIFIER KEY_LEFT_CONTROL
#define REDO_KEY_MODIFIER KEY_LEFT_SHIFT
#define SCALE_SPEED 0.75f
#define FRAME_WIDTH 160
#define TEXTURE_HEIGHT_IN_WINDOW 0.5f
#define FRAME_TIME 0.1

#define BACKGROUND_COLOR GRAY
#define FONT_COLOR RAYWHITE

const Hitbox DEFAULT_HITBOX = {40, 40, 24};
#define FRAME_ROW_COLOR (Color) {50, 50, 50, 255}
#define FRAME_ROW_SIZE 32
#define FRAME_RHOMBUS_RADIUS 12
#define FRAME_RHOMBUS_UNSELECTED_COLOR HITBOX_CIRCLE_INACTIVE_COLOR
#define FRAME_RHOMBUS_SELECTED_COLOR RAYWHITE

#define HITBOX_HEADER_MARGIN_X 8
#define HITBOX_HEADER_MARGIN_Y 4
#define HITBOX_HEADER_TEXT "Hitboxes"

#define HITBOX_ROW_SIZE 32

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
    const int fontSize = GetFontDefault().baseSize;
    
    const int hitboxHeaderHeight = fontSize + HITBOX_HEADER_MARGIN_Y * 2; 
    const int timelineHeight = FRAME_ROW_SIZE + hitboxHeaderHeight + HITBOX_ROW_SIZE;
    const int timelineY = WINDOW_Y - timelineHeight;
    const int hitboxHeaderY = timelineY + FRAME_ROW_SIZE;
    const int hitboxHeaderTextX = HITBOX_HEADER_MARGIN_X;
    // const int hitboxHeaderTextWidth = MeasureText(HITBOX_HEADER_TEXT, fontSize);
    // const int hitboxHeaderTextHeight = hitboxHeaderHeight;
    const int hitboxHeaderTextY = hitboxHeaderY + HITBOX_HEADER_MARGIN_Y;
    const int hitboxRowY = hitboxHeaderY + hitboxHeaderHeight;
    
    float spriteScale = WINDOW_Y * TEXTURE_HEIGHT_IN_WINDOW / sprite.height;
    Vector2 spritePos = {(WINDOW_X - FRAME_WIDTH * spriteScale) / 2.0f, (WINDOW_Y - sprite.height * spriteScale) / 2.0f};

    
    int frameIdx = 0;
    int frameCount = sprite.width / FRAME_WIDTH;
    EditorState state = {DEFAULT_HITBOX, calloc(frameCount, sizeof(bool)), frameCount};
    EditorHistory history = AllocEditorHistory(&state);

    typedef enum Mode {
        IDLE = 0,
        PLAYING = 1,
        DRAGGING_HANDLE = 2,
        PANNING_SPRITE = 3
    } Mode;
    Mode mode = IDLE;
    float playingFrameTime = 0.0f;
    Handle draggingHandle = NONE;
    Vector2 panningSpriteLocalPos = VECTOR2_ZERO;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(EXIT_KEY) && IsKeyDown(EXIT_KEY_MODIFIER))
            break;

        Vector2 mousePos = GetMousePosition();
        
        if (mode == DRAGGING_HANDLE) {
            if (IsMouseButtonReleased(SELECT_BUTTON)) {
                CommitState(&history, &state);
                mode = IDLE;
            } else {
                SetHitboxHandle(mousePos, spritePos, spriteScale, &state.hitbox, draggingHandle); // not handling illegal handle set for now b/c it shouldn't happen.
            }
        } else if (mode == PANNING_SPRITE) {
            if (IsMouseButtonReleased(SELECT_BUTTON)) {
                mode = IDLE;
            } else {
                float globalPanX = spritePos.x + panningSpriteLocalPos.x * spriteScale;
                float globalPanY = spritePos.y + panningSpriteLocalPos.y * spriteScale;
                spritePos.x += mousePos.x - globalPanX;
                spritePos.y += mousePos.y - globalPanY;
            }
        } else if (IsKeyPressed(UNDO_KEY) && IsKeyDown(UNDO_KEY_MODIFIER)) { // undo
            mode = IDLE;
            ChangeOptions option = UNDO;
            if (IsKeyDown(REDO_KEY_MODIFIER)) option = REDO;

            ChangeState(&history, &state, option);

            int maxIdx = state.frameCount - 1;
            if (frameIdx > maxIdx) frameIdx = maxIdx;

        } else if (IsMouseButtonPressed(SELECT_BUTTON)) {
            if (timelineY < mousePos.y && mousePos.y <= WINDOW_Y) {
                if (0.0f <= mousePos.x && mousePos.x < FRAME_ROW_SIZE * state.frameCount) { // toggle whether frame is active
                    mode = IDLE;
                    frameIdx = Clamp((int) mousePos.x / FRAME_ROW_SIZE, 0, state.frameCount - 1); // clamp just to be safe
                    if (mousePos.y > hitboxRowY) {
                        state.hitboxActiveFrames[frameIdx] = !state.hitboxActiveFrames[frameIdx];
                        CommitState(&history, &state);
                    };
                }
            } else {
                draggingHandle = SelectHitboxHandle(mousePos, spritePos, spriteScale, state.hitbox);
                if (draggingHandle != NONE) {
                    mode = DRAGGING_HANDLE;
                } else {
                    mode = PANNING_SPRITE;
                    panningSpriteLocalPos.x = (mousePos.x - spritePos.x) / spriteScale;
                    panningSpriteLocalPos.y = (mousePos.y - spritePos.y) / spriteScale;
                }
            }
        } else if (IsKeyPressed(PLAY_ANIMATION_KEY)) {
            if (mode == PLAYING) {
                mode = IDLE;
            } else {
                mode = PLAYING;
                playingFrameTime = 0.0f;
            }
        } else if (GetMouseWheelMove() != 0.0f) {
            float localX = (mousePos.x - spritePos.x) / spriteScale;
            float localY = (mousePos.y - spritePos.y) / spriteScale;
            spriteScale *= GetMouseWheelMove() > 0 ? 1.0f / SCALE_SPEED : SCALE_SPEED;
            spritePos.x = mousePos.x - localX * spriteScale;
            spritePos.y = mousePos.y - localY * spriteScale;

        } else {
            int direction = (IsKeyPressed(NEXT_FRAME_KEY) ? 1 : 0) - (IsKeyPressed(PREVIOUS_FRAME_KEY) ? 1 : 0);
            if (direction) {
                mode = IDLE;
                int newFrame = frameIdx + direction;

                if (newFrame < 0) frameIdx = state.frameCount - 1;
                else if (newFrame >= state.frameCount) frameIdx = 0;
                else frameIdx = newFrame;
            }
        }

        if (mode == PLAYING) {
            playingFrameTime += GetFrameTime();
            if (playingFrameTime >= FRAME_TIME)
            {
                playingFrameTime = fmod(playingFrameTime, FRAME_TIME);
                frameIdx++;
                if (frameIdx >= state.frameCount) frameIdx = 0;
            }
        }
        
        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        
        Rectangle source = {FRAME_WIDTH * frameIdx, 0.0f, FRAME_WIDTH, sprite.height};
        Rectangle dest = {spritePos.x, spritePos.y, FRAME_WIDTH * spriteScale, sprite.height * spriteScale};
        DrawTexturePro(sprite, source, dest, VECTOR2_ZERO, 0.0f, WHITE);
        
        if (state.hitboxActiveFrames[frameIdx]) {
            DrawHitbox(spritePos, spriteScale, state.hitbox);
        }


        DrawRectangle(0, timelineY, WINDOW_X, timelineHeight, FRAME_ROW_COLOR); // draw timeline background
        DrawText(HITBOX_HEADER_TEXT, hitboxHeaderTextX, hitboxHeaderTextY, fontSize, FONT_COLOR);
        
        for (int i = 0; i < state.frameCount; i++) {
            int xPos = i * FRAME_ROW_SIZE + FRAME_ROW_SIZE / 2;
    
            Color hitboxColor = state.hitboxActiveFrames[i] ? HITBOX_CIRCLE_ACTIVE_COLOR : HITBOX_CIRCLE_INACTIVE_COLOR;
            int hitboxYPos = hitboxRowY + HITBOX_ROW_SIZE / 2;
            DrawCircle(xPos, hitboxYPos, HITBOX_CIRCLE_RADIUS, hitboxColor);

            Color frameColor = i == frameIdx ? FRAME_RHOMBUS_SELECTED_COLOR : FRAME_RHOMBUS_UNSELECTED_COLOR;
            Vector2 frameCenter = {xPos, timelineY + FRAME_ROW_SIZE / 2};
            DrawRhombus(frameCenter, FRAME_RHOMBUS_RADIUS, FRAME_RHOMBUS_RADIUS, frameColor);
        }

        EndDrawing();
    }
    
    UnloadTexture(sprite);
    FreeEditorHistory(history);
    free(state.hitboxActiveFrames);
    CloseWindow();
    return EXIT_SUCCESS;
}