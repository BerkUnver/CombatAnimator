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
#define KEY_NEW_FRAME KEY_N
#define KEY_NEW_FRAME_MODIFIER KEY_LEFT_ALT

#define KEY_NEW_SHAPE_MODIFIER KEY_LEFT_CONTROL
#define KEY_NEW_HURTBOX_MODIFIER KEY_LEFT_SHIFT
#define KEY_NEW_CIRCLE KEY_ONE
#define KEY_NEW_RECTANGLE KEY_TWO
#define KEY_NEW_CAPSULE KEY_THREE

#define DEFAULT_SHAPE_X 40.0f
#define DEFAULT_SHAPE_Y 40.0f
#define DEFAULT_HITBOX_KNOCKBACK_X 2
#define DEFAULT_HITBOX_KNOCKBACK_Y -2
#define DEFAULT_CIRCLE_RADIUS 24.0f
#define DEFAULT_RECTANGLE_RIGHT_X 24.0f
#define DEFAULT_RECTANGLE_BOTTOM_Y 24.0f
#define DEFAULT_CAPSULE_RADIUS 24.0f
#define DEFAULT_CAPSULE_HEIGHT 24.0f

#define KEY_SAVE KEY_S
#define KEY_SAVE_MODIFIER KEY_LEFT_CONTROL
#define SCALE_SPEED 0.75f
#define TEXTURE_HEIGHT_IN_WINDOW 0.5f
#define FRAME_TIME 0.1

#define COLOR_BACKGROUND GRAY
#define COLOR_SELECTED (Color) {123, 123, 123, 255}
#define FRAME_ROW_COLOR (Color) {50, 50, 50, 255}
#define FRAME_ROW_SIZE 32
#define FRAME_RHOMBUS_RADIUS 12
#define FRAME_RHOMBUS_UNSELECTED_COLOR (Color) {63, 63, 63, 255}
#define FRAME_RHOMBUS_SELECTED_COLOR RAYWHITE
#define COLOR_TEXT FRAME_ROW_COLOR

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


char *ChangeFileExtension(const char *fileName, const char *newExt) {
    int len = 0;
    char c;
    int dotIdx = -1;
    while ((c = fileName[len])) {
        if (c == '.') dotIdx = len;
        len++;
    }

    int newExtLen = strlen(newExt);
    char *newStr;

    if (dotIdx == -1) {
        newStr = malloc(sizeof(char) * (len + 1 + newExtLen));
        memcpy(newStr, fileName, sizeof(char) * len);
        newStr[len] = '.';
        memcpy(newStr + len + 1, fileName, sizeof(char) * newExtLen);
    } else {
        newStr = malloc(sizeof(char) * (dotIdx + 1 + newExtLen)); // size of string before and including dot
        memcpy(newStr, fileName, sizeof(char) * (dotIdx + 1));
        memcpy(newStr + dotIdx + 1, newExt, sizeof(char) * newExtLen);
    }

    return newStr;
}


int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Please put the name of the png file to make an animation for as the argument to this file");
        return EXIT_FAILURE;
    }
    const char *texturePath = argv[1];

    InitWindow(WINDOW_X, WINDOW_Y, APP_NAME);
    SetTargetFPS(60);

    Texture2D texture = LoadTexture(texturePath);
    if (texture.id <= 0) {
        printf("Failed to load texture.\n");
        CloseWindow();
        return EXIT_FAILURE;
    }

    const int fontSize = GetFontDefault().baseSize;

    // load state from file or create new state if load failed
    char *savePath = ChangeFileExtension(texturePath, "json");
    FILE *loadFile = fopen(savePath, "r+");
    bool createNewSave = false;
    EditorState state;
    if (!loadFile) {
        createNewSave = true;
    } else {
        char buffer[1024]; // todo : do without buffer
        buffer[1023] = '\0';
        fread(buffer, sizeof(char), 1023, loadFile);
        cJSON *json = cJSON_Parse(buffer);
        if (!json) {
            createNewSave = true;
        } else {
            if (!DeserializeState(json, &state)) createNewSave = true;
            cJSON_Delete(json);
        }
        fclose(loadFile);
    }
    if (createNewSave) state = AllocEditorState(1);

    EditorHistory history = AllocEditorHistory(&state);

    float spriteScale = WINDOW_Y * TEXTURE_HEIGHT_IN_WINDOW / texture.height;
    Vector2 spritePos = {
        (WINDOW_X - texture.width / state.frameCount * spriteScale) / 2.0f,
        (WINDOW_Y - texture.height * spriteScale) / 2.0f
    };

    typedef enum Mode {
        IDLE,
        PLAYING,
        DRAGGING_HANDLE,
        PANNING_SPRITE,
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
            cJSON *saveJson = SerializeState(state);
            if (!saveJson) {
                puts("Failed to save file for unknown reason.");
            } else {
                char *str = cJSON_Print(saveJson);
                FILE *saveFile = fopen(savePath, "w+");
                fputs(str, saveFile);
                free(str);
                fclose(saveFile);
                cJSON_Delete(saveJson);
            }
        } else if (IsKeyPressed(KEY_UNDO) && IsKeyDown(KEY_UNDO_MODIFIER)) { // undo
            mode = IDLE;
            ChangeOptions option = UNDO;
            if (IsKeyDown(KEY_REDO_MODIFIER)) option = REDO;

            ChangeState(&history, &state, option);

        } else if (IsKeyPressed(KEY_NEW_FRAME) && IsKeyDown(KEY_NEW_FRAME_MODIFIER)) {
            AddFrame(&state);
            CommitState(&history, &state);
            mode = IDLE;

        } else if (IsKeyDown(KEY_NEW_SHAPE_MODIFIER)) { // VERY IMPORTANT THAT THIS IS THE LAST CALL THAT CHECKS KEY_LEFT_CONTROL
            CombatShape shape;

            bool newShapeInstanced = true;
            if (IsKeyPressed(KEY_NEW_CIRCLE)) {
                shape.shapeType = CIRCLE;
                shape.data.circleRadius = DEFAULT_CIRCLE_RADIUS;
            } else if (IsKeyPressed(KEY_NEW_RECTANGLE)) {
                shape.shapeType = RECTANGLE;
                shape.data.rectangle.rightX = DEFAULT_RECTANGLE_RIGHT_X;
                shape.data.rectangle.bottomY = DEFAULT_RECTANGLE_BOTTOM_Y;
            } else if (IsKeyPressed(KEY_NEW_CAPSULE)) {
                shape.shapeType = CAPSULE;
                shape.data.capsule.radius = DEFAULT_CAPSULE_RADIUS;
                shape.data.capsule.height = DEFAULT_CAPSULE_HEIGHT;
            } else {
                newShapeInstanced = false;
            }

            if (newShapeInstanced) {
                shape.x = DEFAULT_SHAPE_X;
                shape.y = DEFAULT_SHAPE_Y;
                if (IsKeyDown(KEY_NEW_HURTBOX_MODIFIER)) {
                    shape.boxType = HURTBOX;
                } else {
                    shape.boxType = HITBOX;
                    shape.hitboxKnockbackX = DEFAULT_HITBOX_KNOCKBACK_X;
                    shape.hitboxKnockbackY = DEFAULT_HITBOX_KNOCKBACK_Y;
                }
                AddShape(&state, shape);
                state.shapeIdx = state.shapeCount - 1;
                CommitState(&history, &state);
                mode = IDLE;
            }
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_SELECT)) {
            if (timelineY < mousePos.y && mousePos.y <= WINDOW_Y) {
                if (0.0f <= mousePos.x && mousePos.x < FRAME_ROW_SIZE * state.frameCount) {
                    mode = IDLE;
                    int oldFrameIdx = state.frameIdx;
                    state.frameIdx = Clamp( mousePos.x / FRAME_ROW_SIZE, 0, state.frameCount - 1); // clamp just to be safe
                    if (mousePos.y > hitboxRowY && state.shapeCount >= 1) {
                        state.shapeIdx = Clamp((mousePos.y - hitboxRowY) / SHAPE_ROW_SIZE, 0, state.shapeCount - 1);
                        if (oldFrameIdx == state.frameIdx) { // if frame is already selected toggle it.
                            bool active = !GetShapeActive(&state, state.frameIdx, state.shapeIdx);
                            SetShapeActive(&state, state.frameIdx, state.shapeIdx, active);
                            CommitState(&history, &state);
                        }
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
                    panningSpriteLocalPos.x = (mousePos.x - spritePos.x) / spriteScale;
                    panningSpriteLocalPos.y = (mousePos.y - spritePos.y) / spriteScale;
                    mode = PANNING_SPRITE;
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

        float frameWidth = texture.width / state.frameCount;
        Rectangle source = {frameWidth * state.frameIdx, 0.0f, frameWidth, texture.height};
        Rectangle dest = {spritePos.x, spritePos.y, frameWidth * spriteScale, texture.height * spriteScale};
        DrawTexturePro(texture, source, dest, VECTOR2_ZERO, 0.0f, WHITE);
        
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
            const char *text = TextFormat("%i", i + 1);
            int textX = frameCenter.x - MeasureText(text, fontSize) / 2.0f;
            int textY = frameCenter.y - fontSize / 2.0f;
            DrawText(text, textX, textY, fontSize, COLOR_TEXT);
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

    FreeEditorHistory(history);
    FreeEditorState(state);
    UnloadTexture(texture);
    free(savePath);
    CloseWindow();
    return EXIT_SUCCESS;
}