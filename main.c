#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include "combat_shape.h"
#include "editor_history.h"

#define VECTOR2_ZERO (Vector2) {0.0f, 0.0f}

#define KEY_EXIT KEY_E
#define KEY_EXIT_MODIFIER KEY_LEFT_CONTROL

#define TEST_IMAGE_PATH "Jab.png"
#define TEST_SAVE_PATH "Jab.json"
#define APP_NAME "Combat Animator"
#define WINDOW_X 800
#define WINDOW_Y 480
#define KEY_PLAY_ANIMATION KEY_ENTER
#define KEY_PREVIOUS_FRAME KEY_LEFT
#define KEY_NEXT_FRAME KEY_RIGHT
#define KEY_PREVIOUS_SHAPE KEY_UP
#define KEY_NEXT_SHAPE KEY_DOWN
#define MOUSE_BUTTON_SELECT MOUSE_BUTTON_LEFT
#define KEY_UNDO KEY_Z
#define KEY_UNDO_MODIFIER KEY_LEFT_CONTROL
#define KEY_REDO_MODIFIER KEY_LEFT_SHIFT
#define KEY_NEW_HITBOX KEY_N
#define KEY_NEW_HITBOX_MODIFIER KEY_LEFT_CONTROL
#define KEY_NEW_HURTBOX_MODIFIER KEY_LEFT_SHIFT
#define KEY_SAVE KEY_S
#define KEY_SAVE_MODIFIER KEY_LEFT_CONTROL
#define SCALE_SPEED 0.75f
#define FRAME_WIDTH 160
#define TEXTURE_HEIGHT_IN_WINDOW 0.5f
#define FRAME_TIME 0.1

#define COLOR_BACKGROUND GRAY
#define COLOR_SELECTED (Color) {123, 123, 123, 255}
#define FRAME_ROW_COLOR (Color) {50, 50, 50, 255}
#define FRAME_ROW_SIZE 32
#define FRAME_RHOMBUS_RADIUS 12
#define FRAME_RHOMBUS_UNSELECTED_COLOR (Color) {63, 63, 63, 255}
#define FRAME_RHOMBUS_SELECTED_COLOR RAYWHITE

#define SHAPE_ROW_SIZE 32
#define SHAPE_ICON_CIRCLE_RADIUS 12

void DrawRhombus(Vector2 pos, float xSize, float ySize, Color color) {
    Vector2 topPoint = {pos.x, pos.y - ySize};
    Vector2 leftPoint = {pos.x - xSize, pos.y};
    Vector2 rightPoint = {pos.x + xSize, pos.y};
    Vector2 bottomPoint = {pos.x, pos.y + ySize};
    DrawTriangle(rightPoint, topPoint, leftPoint, color); // must be in counterclockwise order
    DrawTriangle(leftPoint, bottomPoint, rightPoint, color);
}

int main() {
    const CombatShape DEFAULT_HITBOX = CombatShapeRectangle(40, 40, 24, 24, HITBOX);
    const CombatShape DEFAULT_HURTBOX = CombatShapeRectangle(40, 40, 24, 24, HURTBOX);

    InitWindow(WINDOW_X, WINDOW_Y, APP_NAME);
    SetTargetFPS(60);

    Texture2D sprite = LoadTexture(TEST_IMAGE_PATH);

    float spriteScale = WINDOW_Y * TEXTURE_HEIGHT_IN_WINDOW / sprite.height;
    Vector2 spritePos = {(WINDOW_X - FRAME_WIDTH * spriteScale) / 2.0f, (WINDOW_Y - sprite.height * spriteScale) / 2.0f};

    EditorState state;

    FILE *file = fopen(TEST_SAVE_PATH, "r+");
    if (!file) {
        state = AllocEditorState(sprite.width / FRAME_WIDTH);
    } else {
        char buffer[1024];
        fread(buffer, sizeof(char), 1023, file);
        buffer[1023] = '\0';
        cJSON *json = cJSON_Parse(buffer);
        if (!json || !DeserializeState(json, &state))
            state = AllocEditorState(sprite.width / FRAME_WIDTH);
        free(json);
    }


    state.shapeIdx = 0; // temporary test code
    EditorHistory history = AllocEditorHistory(&state);

    typedef enum Mode {
        IDLE,
        PLAYING,
        DRAGGING_HANDLE,
        PANNING_SPRITE
    } Mode;
    Mode mode = IDLE;
    float playingFrameTime = 0.0f;
    Handle draggingHandle = NONE;
    Vector2 panningSpriteLocalPos = VECTOR2_ZERO;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_EXIT) && IsKeyDown(KEY_EXIT_MODIFIER))
            break;

        // updating these here and not after the model update causes changes to be reflected one frame late.
        int timelineHeight = FRAME_ROW_SIZE + SHAPE_ROW_SIZE * state.shapeCount;
        int timelineY = WINDOW_Y - timelineHeight;
        int hitboxRowY = timelineY + FRAME_ROW_SIZE;
        Vector2 mousePos = GetMousePosition();
        
        // model update here
        if (mode == DRAGGING_HANDLE) {
            if (IsMouseButtonReleased(MOUSE_BUTTON_SELECT)) {
                CommitState(&history, &state);
                mode = IDLE;
            } else {
                SetCombatShapeHandle(mousePos, spritePos, spriteScale, &state.shapes[state.shapeIdx], draggingHandle); // todo : temporary
            }
        } else if (mode == PANNING_SPRITE) {
            if (IsMouseButtonReleased(MOUSE_BUTTON_SELECT)) {
                mode = IDLE;
            } else {
                float globalPanX = spritePos.x + panningSpriteLocalPos.x * spriteScale;
                float globalPanY = spritePos.y + panningSpriteLocalPos.y * spriteScale;
                spritePos.x += mousePos.x - globalPanX;
                spritePos.y += mousePos.y - globalPanY;
            }
        } else if (IsKeyPressed(KEY_SAVE) && IsKeyDown(KEY_SAVE_MODIFIER)) {
            cJSON *saved = SerializeState(state);
            if (!saved) {
                puts("Failed to save file for unknown reason.");
            } else {
                char *str = cJSON_Print(saved);
                FILE *saveFile = fopen(TEST_SAVE_PATH, "w+");
                fwrite(str, sizeof(char), strlen(str), saveFile);
                free(str);
                fclose(saveFile);
            }
        } else if (IsKeyPressed(KEY_UNDO) && IsKeyDown(KEY_UNDO_MODIFIER)) { // undo
            mode = IDLE;
            ChangeOptions option = UNDO;
            if (IsKeyDown(KEY_REDO_MODIFIER)) option = REDO;

            ChangeState(&history, &state, option);

        } else if (IsKeyPressed(KEY_NEW_HITBOX) && IsKeyDown(KEY_NEW_HITBOX_MODIFIER)) {
            CombatShape shape = IsKeyDown(KEY_NEW_HURTBOX_MODIFIER) ? DEFAULT_HURTBOX : DEFAULT_HITBOX;
            AddShape(&state, shape);
            CommitState(&history, &state);
            mode = IDLE;

        } else if (IsMouseButtonPressed(MOUSE_BUTTON_SELECT)) {
            if (timelineY < mousePos.y && mousePos.y <= WINDOW_Y) {
                if (0.0f <= mousePos.x && mousePos.x < FRAME_ROW_SIZE * state.frameCount) { // toggle whether frame is active
                    mode = IDLE;
                    state.frameIdx = Clamp((int) mousePos.x / FRAME_ROW_SIZE, 0, state.frameCount - 1); // clamp just to be safe
                    if (mousePos.y > hitboxRowY && state.shapeCount >= 1) {
                        state.shapeIdx = Clamp((mousePos.y - hitboxRowY) / SHAPE_ROW_SIZE, 0, state.shapeCount - 1);
                        bool active = !GetShapeActive(&state, state.frameIdx, state.shapeIdx);
                        SetShapeActive(&state, state.frameIdx, state.shapeIdx, active);
                        CommitState(&history, &state);
                    } else {
                        state.shapeIdx = -1;
                    }
                }
            } else {
                if (state.shapeIdx >= 0) {
                    draggingHandle = SelectCombatShapeHandle(mousePos, spritePos, spriteScale, state.shapes[state.shapeIdx]);
                    // may set draggingHandle to none                  
                } else {
                    draggingHandle = NONE;
                }

                if (draggingHandle != NONE) {
                    mode = DRAGGING_HANDLE;
                } else {
                    mode = PANNING_SPRITE;
                    panningSpriteLocalPos.x = (mousePos.x - spritePos.x) / spriteScale;
                    panningSpriteLocalPos.y = (mousePos.y - spritePos.y) / spriteScale;
                }
            }
        } else if (IsKeyPressed(KEY_PLAY_ANIMATION)) {
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
            int frameDir = (IsKeyPressed(KEY_NEXT_FRAME) ? 1 : 0) - (IsKeyPressed(KEY_PREVIOUS_FRAME) ? 1 : 0);
            if (frameDir) {
                mode = IDLE;
                int newFrame = state.frameIdx + frameDir;

                if (newFrame < 0) state.frameIdx = state.frameCount - 1;
                else if (newFrame >= state.frameCount) state.frameIdx = 0;
                else state.frameIdx = newFrame;
            }

            int shapeDir = (IsKeyPressed(KEY_NEXT_SHAPE) ? 1 : 0) - (IsKeyPressed(KEY_PREVIOUS_SHAPE) ? 1 : 0);
            if (shapeDir) {
                mode = IDLE;
                state.shapeIdx = Clamp(state.shapeIdx + shapeDir, -1, state.shapeCount - 1);
            }
        }

        // playing tick update (not related to model)
        if (mode == PLAYING) {
            playingFrameTime += GetFrameTime();
            if (playingFrameTime >= FRAME_TIME)
            {
                playingFrameTime = fmod(playingFrameTime, FRAME_TIME);
                int newFrameIdx = state.frameIdx + 1;
                state.frameIdx = newFrameIdx >= state.frameCount ? 0 : newFrameIdx;
            }
        }
        
        // drawing
        BeginDrawing();
        ClearBackground(COLOR_BACKGROUND);
        
        Rectangle source = {FRAME_WIDTH * state.frameIdx, 0.0f, FRAME_WIDTH, sprite.height};
        Rectangle dest = {spritePos.x, spritePos.y, FRAME_WIDTH * spriteScale, sprite.height * spriteScale};
        DrawTexturePro(sprite, source, dest, VECTOR2_ZERO, 0.0f, WHITE);
        
        for (int i = 0; i < state.shapeCount; i++) {
            if (GetShapeActive(&state, state.frameIdx, i)) 
                DrawCombatShape(spritePos, spriteScale, state.shapes[i], i == state.shapeIdx);
        }

        DrawRectangle(0, timelineY, WINDOW_X, timelineHeight, FRAME_ROW_COLOR); // draw timeline background
        int xPos = FRAME_ROW_SIZE * state.frameIdx;
        int yPos = state.shapeIdx >= 0 ? timelineY + FRAME_ROW_SIZE + state.shapeIdx * SHAPE_ROW_SIZE : timelineY;
        DrawRectangle(xPos, yPos, FRAME_ROW_SIZE, FRAME_ROW_SIZE, COLOR_SELECTED);

        for (int i = 0; i < state.frameCount; i++) {
            int xPos = i * FRAME_ROW_SIZE + FRAME_ROW_SIZE / 2;
    
            Color frameColor = i == state.frameIdx ? FRAME_RHOMBUS_SELECTED_COLOR : FRAME_RHOMBUS_UNSELECTED_COLOR;
            Vector2 frameCenter = {xPos, timelineY + FRAME_ROW_SIZE / 2};
            DrawRhombus(frameCenter, FRAME_RHOMBUS_RADIUS, FRAME_RHOMBUS_RADIUS, frameColor);

            for (int j = 0; j < state.shapeCount; j++) {
                Color color;
                bool active = GetShapeActive(&state, i, j);
                if (state.shapes[j].boxType == HITBOX)
                    color = active ? HITBOX_CIRCLE_ACTIVE_COLOR : HITBOX_CIRCLE_INACTIVE_COLOR;
                else 
                    color = active ? HURTBOX_CIRCLE_ACTIVE_COLOR : HURTBOX_CIRCLE_INACTIVE_COLOR;
                int shapeY = hitboxRowY + SHAPE_ROW_SIZE * (j + 0.5);
                DrawCircle(xPos, shapeY, SHAPE_ICON_CIRCLE_RADIUS, color);
            }        
        }

        EndDrawing();
    }
    
    UnloadTexture(sprite);
    FreeEditorHistory(history);
    FreeEditorState(state);
    CloseWindow();
    return EXIT_SUCCESS;
}