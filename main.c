#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "combat_shape.h"
#include "editor_history.h"
#include "string_buffer.h"
#include "transform_2d.h"

#define KEY_EXIT KEY_E
#define KEY_EXIT_MODIFIER KEY_LEFT_CONTROL

#define FILE_EXTENSION "json"
#define FLAG_UPDATE "-u"
#define APP_NAME "Combat Animator"
#define DEFAULT_SPRITE_WINDOW_X 800
#define DEFAULT_SPRITE_WINDOW_Y 400
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

#define KEY_REMOVE_FRAME KEY_BACKSPACE
#define KEY_REMOVE_FRAME_MODIFIER KEY_LEFT_ALT

#define KEY_REMOVE_SHAPE KEY_BACKSPACE
#define KEY_REMOVE_SHAPE_MODIFIER KEY_LEFT_CONTROL

#define KEY_NEW_SHAPE_MODIFIER KEY_LEFT_CONTROL
#define KEY_NEW_HURTBOX_MODIFIER KEY_LEFT_SHIFT
#define KEY_NEW_CIRCLE KEY_ONE
#define KEY_NEW_RECTANGLE KEY_TWO
#define KEY_NEW_CAPSULE KEY_THREE

#define KEY_FRAME_TOGGLE KEY_SPACE
#define COLOR_FRAME_POS_HANDLE (Color) {255, 123, 0, 255}
#define COLOR_FRAME_POS_HANDLE_PREVIOUS (Color) {161, 78, 0, 255}

#define KEY_SAVE KEY_S
#define KEY_SAVE_MODIFIER KEY_LEFT_CONTROL
#define SCALE_SPEED 0.75f
#define TEXTURE_HEIGHT_IN_WINDOW 0.5f

#define COLOR_BACKGROUND GRAY
#define COLOR_SELECTED (Color) {123, 123, 123, 255}
#define FRAME_ROW_COLOR (Color) {50, 50, 50, 255}
#define FRAME_ROW_SIZE 32
#define FRAME_RHOMBUS_RADIUS 12
#define FRAME_RHOMBUS_CANNOT_CANCEL_COLOR (Color) {63, 63, 63, 255}
#define FRAME_RHOMBUS_CAN_CANCEL_COLOR RAYWHITE
#define FRAME_ROW_TEXT_COLOR FRAME_ROW_COLOR

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
    char *dotIdx = strrchr(fileName, '.');
    int newExtLen = strlen(newExt);

    char *newStr;
    if (!dotIdx) { 
        int fileNameLen = strlen(fileName);
        newStr = malloc(sizeof(char) * (fileNameLen + 1 + newExtLen + 1));
        memcpy(newStr, fileName, sizeof(char) * fileNameLen);
        newStr[fileNameLen] = '.';
        strcpy(newStr + fileNameLen + 1, newExt);
    } else {
        int fileBaseLen = (int) (((unsigned long) dotIdx - (unsigned long) fileName) / sizeof(char) + 1);
        newStr = malloc(sizeof(char) * (fileBaseLen + 1 + newExtLen + 1));
        memcpy(newStr, fileName, sizeof(char) * fileBaseLen);
        strcpy(newStr + fileBaseLen, newExt);
    }
    return newStr;

}

void RecursiveUpdate(const char *path) {
    
    DIR *dir = opendir(path);
    if (!dir) {
        printf("Failed to open directory at %s\n", path);
        return;
    }

    struct dirent *directoryEntry; // works because this is only accessed before recursive update call.
    // This is stored in static memory, so it gets overwritten when readdir is called again.
    
    while ((directoryEntry = readdir(dir))) {
        // it would be very bad if this didn't work
        if (strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0) continue;

        StringBuffer fullPath = StringBufferNew();
        StringBufferAddString(&fullPath, path);
        StringBufferAddChar(&fullPath, '/');
        StringBufferAddString(&fullPath, directoryEntry->d_name);

        struct stat fileStat;
        if (stat(fullPath.raw, &fileStat) != 0) {
            printf("Failed to obtain information about the file at %s. Skipping.\n", fullPath.raw);
        // no symlink support
        } else if (S_ISDIR(fileStat.st_mode)) {
            RecursiveUpdate(fullPath.raw);
        } else if (S_ISREG(fileStat.st_mode)) {
            char *dot = strrchr(directoryEntry->d_name, '.');            
            if (dot && strcmp(dot, "."FILE_EXTENSION) == 0) {
                EditorState state;
                if (EditorStateReadFromFile(&state, fullPath.raw)) {
                    cJSON *json = EditorStateSerialize(state);
                    EditorStateWriteToFile(&state, fullPath.raw);
                    cJSON_Delete(json);
                    EditorStateFree(&state);
                    printf("Successfully updated the combat animation file at %s.\n", fullPath.raw);
                } else {
                    printf("Failed to read a valid combat animation from the file at %s. Skipping.\n", fullPath.raw);
                }
            }
        }
        StringBufferFree(&fullPath);
    }
    closedir(dir);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("Please put the name of the png file to make an animation for as the argument to this application.");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], FLAG_UPDATE) == 0) { // first argument is to recursively update all files in the given folder.    
        RecursiveUpdate(".");
        return EXIT_SUCCESS;
    }

    const char *texturePath = argv[1];

    InitWindow(1, 1, APP_NAME);
    SetTargetFPS(60);

    Texture2D texture = LoadTexture(texturePath);
    if (texture.id <= 0) {
        printf("Failed to load texture.\n");
        CloseWindow();
        return EXIT_FAILURE;
    }

    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    const int fontSize = GetFontDefault().baseSize;

    // load state from file or create new state if load failed
    char *savePath = ChangeFileExtension(texturePath, FILE_EXTENSION);
    EditorState state;
    if (!EditorStateReadFromFile(&state, savePath)) state = EditorStateNew(1);

    EditorHistory history = EditorHistoryNew(&state);

    const float startScale = DEFAULT_SPRITE_WINDOW_Y * TEXTURE_HEIGHT_IN_WINDOW / texture.height;
    Transform2D transform = Transform2DIdentity();
    transform = Transform2DSetScale(transform, (Vector2) {.x = startScale, .y = startScale});
    const int guiInitialHeight = state.shapeCount * FRAME_ROW_SIZE + FRAME_ROW_SIZE;
    SetWindowSize(DEFAULT_SPRITE_WINDOW_X, DEFAULT_SPRITE_WINDOW_Y + guiInitialHeight);
    
    transform.o = (Vector2) {
        .x = (DEFAULT_SPRITE_WINDOW_X - texture.width / state.frameCount * startScale) / 2.0f,
        .y = (DEFAULT_SPRITE_WINDOW_Y - texture.height * startScale) / 2.0f
    };
    

    typedef enum Mode {
        MODE_IDLE,
        MODE_PLAYING,
        MODE_DRAGGING_HANDLE,
        MODE_DRAGGING_FRAME_POS,
        MODE_PANNING_SPRITE,
        MODE_EDIT_FRAME_DURATION,
        MODE_EDIT_HITBOX_DAMAGE,
        MODE_EDIT_HITBOX_STUN
    } Mode;
    Mode mode = MODE_IDLE;

    int playingFrameTime = 0;
    Handle draggingHandle = NONE;
    Vector2 panningSpriteLocalPos = VECTOR2_ZERO;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_EXIT) && IsKeyDown(KEY_EXIT_MODIFIER))
            break;

        // updating these here and not after the model update causes changes to be reflected one frame late.
        int windowX = GetScreenWidth();
        int windowY = GetScreenHeight();
        int timelineHeight = FRAME_ROW_SIZE + SHAPE_ROW_SIZE * state.shapeCount;
        int timelineY = windowY - timelineHeight;
        int hitboxRowY = timelineY + FRAME_ROW_SIZE;
        Vector2 mousePos = GetMousePosition();

        const float mouseWheel = GetMouseWheelMove();
        if (mouseWheel != 0.0f) {
            Vector2 localMousePos = Transform2DToLocal(transform, mousePos);
            float scaleSpeed = mouseWheel > 0.0f ? 1.0f / SCALE_SPEED : SCALE_SPEED;
            Vector2 scale = {.x = scaleSpeed, .y = scaleSpeed};
            transform = Transform2DScale(transform, scale);
            transform.o = Vector2Subtract(mousePos, Transform2DBasisXFormInv(transform, localMousePos));
        }
        
        switch (mode) {
            case MODE_EDIT_FRAME_DURATION:
            case MODE_EDIT_HITBOX_DAMAGE:
            case MODE_EDIT_HITBOX_STUN:
                if (IsKeyPressed(KEY_ENTER)) {
                    EditorHistoryCommitState(&history, &state);
                    mode = MODE_IDLE;
                }
                break;

            case MODE_DRAGGING_HANDLE:
                if (IsMouseButtonReleased(MOUSE_BUTTON_SELECT)) {
                    EditorHistoryCommitState(&history, &state);
                    mode = MODE_IDLE;
                } else {
                    Vector2 localMousePos = Transform2DToLocal(transform, mousePos);
                    assert(CombatShapeSetHandle(localMousePos, &state.shapes[state.shapeIdx], draggingHandle));
                }
                break;

            case MODE_DRAGGING_FRAME_POS:
                if (IsMouseButtonReleased(MOUSE_BUTTON_SELECT)) {
                    EditorHistoryCommitState(&history, &state);
                    mode = MODE_IDLE;
                } else {
                    Vector2 localMousePos = Transform2DToLocal(transform, mousePos);
                    state.frames[state.frameIdx].pos = Vector2Round(localMousePos);
                }
                break;

            case MODE_PANNING_SPRITE:
                if (IsMouseButtonReleased(MOUSE_BUTTON_SELECT)) {
                    mode = MODE_IDLE;
                } else {
                    Vector2 globalPan = Transform2DToGlobal(transform, panningSpriteLocalPos);
                    transform.o = Vector2Add(transform.o, Vector2Subtract(mousePos, globalPan));
                }
                break;

            case MODE_PLAYING:
            case MODE_IDLE:
                if (IsKeyPressed(KEY_SAVE) && IsKeyDown(KEY_SAVE_MODIFIER)) {
                    if (!EditorStateWriteToFile(&state, savePath)) {
                        puts("Failed to save file for unknown reason.");
                    }
                } else if (IsKeyPressed(KEY_UNDO) && IsKeyDown(KEY_UNDO_MODIFIER)) {
                    mode = MODE_IDLE;
                    ChangeOptions option = UNDO;
                    if (IsKeyDown(KEY_REDO_MODIFIER)) option = REDO;

                    EditorHistoryChangeState(&history, &state, option);

                } else if (IsKeyPressed(KEY_NEW_FRAME) && IsKeyDown(KEY_NEW_FRAME_MODIFIER)) {
                    EditorStateAddFrame(&state);
                    state.frameIdx = state.frameCount - 1;
                    EditorHistoryCommitState(&history, &state);
                    mode = MODE_IDLE;
                
                } else if (IsKeyPressed(KEY_REMOVE_FRAME) && IsKeyDown(KEY_REMOVE_FRAME_MODIFIER) && state.frameCount > 1) {
                    EditorStateRemoveFrame(&state, state.frameIdx);
                    if (state.frameIdx >= state.frameCount) state.frameIdx = state.frameCount - 1;
                    EditorHistoryCommitState(&history, &state);
                    mode = MODE_IDLE;

                } else if (IsKeyPressed(KEY_REMOVE_SHAPE) && IsKeyDown(KEY_REMOVE_SHAPE_MODIFIER) && state.shapeIdx >= 0) {
                    EditorStateRemoveShape(&state, state.shapeIdx);
                    EditorHistoryCommitState(&history, &state);
                    mode = MODE_IDLE;
                
                } else if (IsKeyPressed(KEY_FRAME_TOGGLE)) {
                    if (state.shapeIdx < 0) {
                        state.frames[state.frameIdx].canCancel = !state.frames[state.frameIdx].canCancel;
                    } else {
                        bool active = !EditorStateShapeActiveGet(&state, state.frameIdx, state.shapeIdx);
                        EditorStateShapeActiveSet(&state, state.frameIdx, state.shapeIdx, active);
                    }
                    mode = MODE_IDLE;
                    EditorHistoryCommitState(&history, &state);
                
                } else if (IsKeyDown(KEY_NEW_SHAPE_MODIFIER)) { // VERY IMPORTANT THAT THIS IS THE LAST CALL THAT CHECKS KEY_LEFT_CTRL
                    bool newShapeInstanced = true;
                    ShapeType shapeType;
                    
                    if (IsKeyPressed(KEY_NEW_CIRCLE))           shapeType = CIRCLE;
                    else if (IsKeyPressed(KEY_NEW_RECTANGLE))   shapeType = RECTANGLE;
                    else if (IsKeyPressed(KEY_NEW_CAPSULE))     shapeType = CAPSULE;
                    else                                        newShapeInstanced = false;

                    if (newShapeInstanced) {
                        BoxType boxType = IsKeyDown(KEY_NEW_HURTBOX_MODIFIER) ? HURTBOX : HITBOX;
                        CombatShape shape = CombatShapeNew(shapeType, boxType);
                        shape.transform.o = (Vector2) { // spawn shape at center of frame.
                            .x = (float) texture.width / (float) (state.frameCount * 2), 
                            .y = (float) texture.height / 2.0f
                        };

                        EditorStateAddShape(&state, shape);
                        state.shapeIdx = state.shapeCount - 1;
                        EditorHistoryCommitState(&history, &state);
                        mode = MODE_IDLE;
                    }
                } else if (IsMouseButtonPressed(MOUSE_BUTTON_SELECT) && mousePos.y < timelineY) {
                    if (HandleIsColliding(transform, mousePos, state.frames[state.frameIdx].pos)) {
                        mode = MODE_DRAGGING_FRAME_POS;
                    } else {
                        draggingHandle = state.shapeIdx >= 0 ? CombatShapeSelectHandle(transform, mousePos,
                                                                                       state.shapes[state.shapeIdx]) : NONE;
                        if (draggingHandle != NONE) {
                            mode = MODE_DRAGGING_HANDLE;
                        } else {
                            panningSpriteLocalPos = Transform2DToLocal(transform, mousePos);
                            mode = MODE_PANNING_SPRITE;
                        }
                    }
                } else if (IsKeyPressed(KEY_PLAY_ANIMATION)) {
                    if (mode == MODE_PLAYING) {
                        mode = MODE_IDLE;
                    } else {
                        mode = MODE_PLAYING;
                        playingFrameTime = 0;
                    }
                } else {
                    int frameDir = (IsKeyPressed(KEY_NEXT_FRAME) ? 1 : 0) - (IsKeyPressed(KEY_PREVIOUS_FRAME) ? 1 : 0);
                    if (frameDir) {
                        mode = MODE_IDLE;
                        int newFrame = state.frameIdx + frameDir;

                        if (newFrame < 0) state.frameIdx = state.frameCount - 1;
                        else if (newFrame >= state.frameCount) state.frameIdx = 0;
                        else state.frameIdx = newFrame;
                    }

                    int shapeDir = (IsKeyPressed(KEY_NEXT_SHAPE) ? 1 : 0) - (IsKeyPressed(KEY_PREVIOUS_SHAPE) ? 1 : 0);
                    if (shapeDir) {
                        mode = MODE_IDLE;
                        state.shapeIdx = Clamp(state.shapeIdx + shapeDir, -1, state.shapeCount - 1);
                    }
                }
                break;

            default:
                assert(false);
                break;

        }

        // playing tick update (not related to model)
        if (mode == MODE_PLAYING) {
            playingFrameTime += (int) (GetFrameTime() * FRAME_DURATION_UNIT_PER_SECOND);
            int frameDuration = state.frames[state.frameIdx].duration;
            if (playingFrameTime >= frameDuration)
            {
                playingFrameTime -= frameDuration;
                int newFrameIdx = state.frameIdx + 1;
                state.frameIdx = newFrameIdx >= state.frameCount ? 0 : newFrameIdx;
            }
        }
        
        // drawing
        BeginDrawing();
        ClearBackground(COLOR_BACKGROUND);

        // draw texture
        rlPushMatrix();
        rlTransform2DXForm(transform);
        float frameWidth = texture.width / state.frameCount;
        Rectangle source = {
            .x = frameWidth * state.frameIdx, 
            .y = 0.0f, 
            .width = frameWidth, 
            .height = texture.height
        };
        Rectangle dest = {
            .x = 0.0f,
            .y = 0.0f,
            .width = frameWidth,
            .height = texture.height
        };
        DrawTexturePro(texture, source, dest, VECTOR2_ZERO, 0.0f, WHITE);
        rlPopMatrix();

        // draw combat shapes
        for (int i = 0; i < state.shapeCount; i++) {
            if (EditorStateShapeActiveGet(&state, state.frameIdx, i)) {
                CombatShapeDraw(transform, state.shapes[i], i == state.shapeIdx);
            }
        }
        
        // draw frame pos handle
        if (state.frameIdx > 0) {
            Vector2 globalFramePosPrevious = Transform2DToGlobal(transform, state.frames[state.frameIdx - 1].pos);
            DrawCircle(globalFramePosPrevious.x, globalFramePosPrevious.y, HANDLE_RADIUS, COLOR_FRAME_POS_HANDLE_PREVIOUS);  
        }
        Vector2 globalFramePos = Transform2DToGlobal(transform, state.frames[state.frameIdx].pos);
        HandleDraw(globalFramePos, COLOR_FRAME_POS_HANDLE);

        // draw frame duration value box
        Rectangle rectFrameDurationLabel = { .x = 0, .y = 0, .width = 128, .height = (float) fontSize + 8};
        GuiLabel(rectFrameDurationLabel, "Frame Duration (ms)");
        Rectangle rectFrameDurationValue = rectFrameDurationLabel;
        rectFrameDurationValue.x += rectFrameDurationValue.width;
        rectFrameDurationValue.width = 32;
        if (GuiValueBox(rectFrameDurationValue, NULL, &state.frames[state.frameIdx].duration, 1, INT_MAX, mode == MODE_EDIT_FRAME_DURATION)) {
            mode = MODE_EDIT_FRAME_DURATION;
        }
        
        // draw hitbox damage and stun value boxes if a hitbox is currently selected.
        if (state.shapeIdx >= 0 && state.shapes[state.shapeIdx].boxType == HITBOX) {
            Rectangle rectDamageLabel = rectFrameDurationLabel;
            rectDamageLabel.y += rectDamageLabel.height;
            GuiLabel(rectDamageLabel, "Damage");
            
            Rectangle rectDamageValue = rectDamageLabel;
            rectDamageValue.x += rectDamageValue.width;
            rectDamageValue.width = rectFrameDurationValue.width;
            if (GuiValueBox(rectDamageValue, NULL, &state.shapes[state.shapeIdx].hitboxDamage, 0, INT_MAX, mode == MODE_EDIT_HITBOX_DAMAGE)) {
                mode = MODE_EDIT_HITBOX_DAMAGE;
            }

            Rectangle rectStunLabel = rectDamageLabel;
            rectStunLabel.y += rectStunLabel.height;
            GuiLabel(rectStunLabel, "Stun (ms)");
            
            Rectangle rectStunValue = rectStunLabel;
            rectStunValue.x += rectStunValue.width;
            rectStunValue.width = rectFrameDurationValue.width;
            if (GuiValueBox(rectStunValue, NULL, &state.shapes[state.shapeIdx].hitboxStun, 0, INT_MAX, mode == MODE_EDIT_HITBOX_STUN)) {
                mode = MODE_EDIT_HITBOX_STUN;
            }
        }
        

        
        // draw timeline
        DrawRectangle(0, timelineY, windowX, timelineHeight, FRAME_ROW_COLOR); // draw timeline background
        int selectedX = FRAME_ROW_SIZE * state.frameIdx;
        int selectedY = state.shapeIdx >= 0 ? timelineY + FRAME_ROW_SIZE + state.shapeIdx * SHAPE_ROW_SIZE : timelineY;
        DrawRectangle(selectedX, selectedY, FRAME_ROW_SIZE, FRAME_ROW_SIZE, COLOR_SELECTED);

        for (int i = 0; i < state.frameCount; i++) {
            int xPos = i * FRAME_ROW_SIZE + FRAME_ROW_SIZE / 2;

            Color frameColor = state.frames[i].canCancel ? FRAME_RHOMBUS_CAN_CANCEL_COLOR : FRAME_RHOMBUS_CANNOT_CANCEL_COLOR;
            Vector2 frameCenter = {xPos, timelineY + FRAME_ROW_SIZE / 2};
            DrawRhombus(frameCenter, FRAME_RHOMBUS_RADIUS, FRAME_RHOMBUS_RADIUS, frameColor);
            
            const char *text = TextFormat("%i", i + 1);
            int textX = frameCenter.x - MeasureText(text, fontSize) / 2.0f;
            int textY = frameCenter.y - fontSize / 2.0f;
            DrawText(text, textX, textY, fontSize, FRAME_ROW_TEXT_COLOR);
            for (int j = 0; j < state.shapeCount; j++) {
                Color color;
                bool active = EditorStateShapeActiveGet(&state, i, j);
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

    EditorHistoryFree(&history);
    EditorStateFree(&state);
    UnloadTexture(texture);
    free(savePath);
    CloseWindow();
    return EXIT_SUCCESS;
}
