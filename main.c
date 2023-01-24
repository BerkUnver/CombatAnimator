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
#define REDO_KEY_MODIFIER KEY_LEFT_ALT
#define SCALE_SPEED 0.75f
#define FRAME_WIDTH 160
#define TEXTURE_HEIGHT_IN_WINDOW 0.5f
#define FRAME_TIME 0.1

#define BACKGROUND_COLOR GRAY
const Hitbox DEFAULT_HITBOX = {40, 40, 24};
#define FRAME_ROW_COLOR (Color) {50, 50, 50, 255}
#define FRAME_ROW_SIZE 32
#define FRAME_RHOMBUS_RADIUS 12
#define FRAME_RHOMBUS_UNSELECTED_COLOR HITBOX_CIRCLE_INACTIVE_COLOR
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
    Vector2 spritePos = {(WINDOW_X - FRAME_WIDTH * spriteScale) / 2.0f, (WINDOW_Y - sprite.height * spriteScale) / 2.0f};

    
    int frameIdx = 0;
    int frameCount = sprite.width / FRAME_WIDTH;
    EditorState editorState = {DEFAULT_HITBOX, calloc(frameCount, sizeof(bool)), frameCount};
    EditorHistory history = AllocEditorHistory(&editorState);

    typedef enum State {
        IDLE = 0,
        PLAYING = 1,
        DRAGGING_HANDLE = 2,
        PANNING_SPRITE = 3
    } State;
    State state = IDLE;
    float playingFrameTime = 0.0f;
    Handle draggingHandle = NONE;
    Vector2 panningSpriteLocalPos = VECTOR2_ZERO;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(EXIT_KEY) && IsKeyDown(EXIT_KEY_MODIFIER))
            break;

        Vector2 mousePos = GetMousePosition();
        
        if (state == DRAGGING_HANDLE) {
            if (IsMouseButtonReleased(SELECT_BUTTON)) {
                CommitState(&history, &editorState);
                state = IDLE;
            } else {
                SetHitboxHandle(mousePos, spritePos, spriteScale, &editorState.hitbox, draggingHandle); // not handling illegal handle set for now b/c it shouldn't happen.
            }
        } else if (state == PANNING_SPRITE) {
            if (IsMouseButtonReleased(SELECT_BUTTON)) {
                state = IDLE;
            } else {
                float globalPanX = spritePos.x + panningSpriteLocalPos.x * spriteScale;
                float globalPanY = spritePos.y + panningSpriteLocalPos.y * spriteScale;
                spritePos.x += mousePos.x - globalPanX;
                spritePos.y += mousePos.y - globalPanY;
            }
        } else if (IsKeyPressed(UNDO_KEY) && IsKeyDown(UNDO_KEY_MODIFIER)) { // undo
            state = IDLE;
            ChangeOptions option = UNDO;
            if (IsKeyDown(REDO_KEY_MODIFIER)) option = REDO;

            ChangeState(&history, &editorState, option);

            int maxIdx = editorState.frameCount - 1;
            if (frameIdx > maxIdx) frameIdx = maxIdx;

        } else if (IsMouseButtonPressed(SELECT_BUTTON)) {
            if (WINDOW_Y - FRAME_ROW_SIZE - HITBOX_ROW_SIZE < mousePos.y && mousePos.y <= WINDOW_Y) {
                if (0.0f <= mousePos.x && mousePos.x < FRAME_ROW_SIZE * editorState.frameCount) { // toggle whether frame is active
                    state = IDLE;
                    frameIdx = Clamp((int) mousePos.x / FRAME_ROW_SIZE, 0, editorState.frameCount - 1); // clamp just to be safe
                    if (mousePos.y > WINDOW_Y - HITBOX_ROW_SIZE) {
                        editorState.hitboxActiveFrames[frameIdx] = !editorState.hitboxActiveFrames[frameIdx];
                        CommitState(&history, &editorState);
                    };
                }
            } else {
                draggingHandle = SelectHitboxHandle(mousePos, spritePos, spriteScale, editorState.hitbox);
                if (draggingHandle != NONE) {
                    state = DRAGGING_HANDLE;
                } else {
                    state = PANNING_SPRITE;
                    panningSpriteLocalPos.x = (mousePos.x - spritePos.x) / spriteScale;
                    panningSpriteLocalPos.y = (mousePos.y - spritePos.y) / spriteScale;
                }
            }
        } else if (IsKeyPressed(PLAY_ANIMATION_KEY)) {
            if (state == PLAYING) {
                state = IDLE;
            } else {
                state = PLAYING;
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
                state = IDLE;
                int newFrame = frameIdx + direction;

                if (newFrame < 0) frameIdx = editorState.frameCount - 1;
                else if (newFrame >= editorState.frameCount) frameIdx = 0;
                else frameIdx = newFrame;
            }
        }

        if (state == PLAYING) {
            playingFrameTime += GetFrameTime();
            if (playingFrameTime >= FRAME_TIME)
            {
                playingFrameTime = fmod(playingFrameTime, FRAME_TIME);
                frameIdx++;
                if (frameIdx >= editorState.frameCount) frameIdx = 0;
            }
        }
        
        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        
        Rectangle source = {FRAME_WIDTH * frameIdx, 0.0f, FRAME_WIDTH, sprite.height};
        Rectangle dest = {spritePos.x, spritePos.y, FRAME_WIDTH * spriteScale, sprite.height * spriteScale};
        DrawTexturePro(sprite, source, dest, VECTOR2_ZERO, 0.0f, WHITE);
        
        if (editorState.hitboxActiveFrames[frameIdx]) {
            DrawHitbox(spritePos, spriteScale, editorState.hitbox);
        }

        DrawRectangle(0, WINDOW_Y - FRAME_ROW_SIZE - HITBOX_ROW_SIZE, WINDOW_X, FRAME_ROW_SIZE + HITBOX_ROW_SIZE, FRAME_ROW_COLOR);
        for (int i = 0; i < editorState.frameCount; i++) {
            int xPos = i * FRAME_ROW_SIZE + FRAME_ROW_SIZE / 2;
    
            Color hitboxColor = editorState.hitboxActiveFrames[i] ? HITBOX_CIRCLE_ACTIVE_COLOR : HITBOX_CIRCLE_INACTIVE_COLOR;
            int hitboxYPos = WINDOW_Y - HITBOX_ROW_SIZE / 2;
            DrawCircle(xPos, hitboxYPos, HITBOX_CIRCLE_RADIUS, hitboxColor);

            Color frameColor = i == frameIdx ? FRAME_RHOMBUS_SELECTED_COLOR : FRAME_RHOMBUS_UNSELECTED_COLOR;
            Vector2 frameCenter = {xPos, WINDOW_Y - HITBOX_ROW_SIZE - FRAME_ROW_SIZE / 2};
            DrawRhombus(frameCenter, FRAME_RHOMBUS_RADIUS, FRAME_RHOMBUS_RADIUS, frameColor);
        }

        EndDrawing();
    }
    
    UnloadTexture(sprite);
    FreeEditorHistory(history);
    free(editorState.hitboxActiveFrames);
    CloseWindow();
    return EXIT_SUCCESS;
}