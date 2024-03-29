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

#include "editor_history.h"
#include "layer.h"
#include "list.h"
#include "string_buffer.h"
#include "transform_2d.h"
#include "gui.h"

#define FILE_EXTENSION "json"
#define APP_NAME "Combat Animator"
#define DEFAULT_SPRITE_WINDOW_X 800
#define DEFAULT_SPRITE_WINDOW_Y 400
#define KEY_PLAY_ANIMATION KEY_ENTER
#define KEY_FRAME_PREVIOUS KEY_LEFT
#define KEY_FRAME_NEXT KEY_RIGHT
#define KEY_LAYER_PREVIOUS KEY_UP
#define KEY_LAYER_NEXT KEY_DOWN
#define MOUSE_BUTTON_SELECT MOUSE_BUTTON_LEFT
#define KEY_UNDO KEY_Z
#define KEY_UNDO_MODIFIER KEY_LEFT_CONTROL
#define KEY_REDO_MODIFIER KEY_LEFT_SHIFT

#define KEY_FRAME_NEW KEY_N
#define KEY_FRAME_NEW_MODIFIER KEY_LEFT_ALT

#define KEY_FRAME_REMOVE KEY_BACKSPACE
#define KEY_FRAME_REMOVE_MODIFIER KEY_LEFT_ALT

#define KEY_LAYER_REMOVE KEY_BACKSPACE
#define KEY_LAYER_REMOVE_MODIFIER KEY_LEFT_CONTROL

#define KEY_LAYER_NEW_MODIFIER KEY_LEFT_CONTROL

#define KEY_FRAME_TOGGLE KEY_SPACE
#define COLOR_FRAME_POS_HANDLE (Color) {255, 123, 0, 255}
#define COLOR_FRAME_POS_HANDLE_PREVIOUS (Color) {161, 78, 0, 255}

#define KEY_SAVE KEY_S
#define KEY_SAVE_MODIFIER KEY_LEFT_CONTROL
#define SCALE_SPEED 0.9375f
#define TEXTURE_HEIGHT_IN_WINDOW 0.5f

#define COLOR_BACKGROUND GRAY
#define COLOR_SELECTED (Color) {123, 123, 123, 255}

#define PANEL_WIDTH 256.0f
#define PANEL_COLOR (Color) {75, 75, 75, 255}

#define TIMELINE_COLOR (Color) {50, 50, 50, 255}
#define TIMELINE_HEADER_COLOR (Color) {40, 40, 40, 255}

#define FRAME_ROW_SIZE 32
#define FRAME_RHOMBUS_RADIUS 12
#define FRAME_RHOMBUS_CANNOT_CANCEL_COLOR (Color) {63, 63, 63, 255}
#define FRAME_RHOMBUS_CAN_CANCEL_COLOR RAYWHITE
#define FRAME_ROW_TEXT_COLOR TIMELINE_COLOR

#define LAYER_ROW_SIZE 32
#define LAYER_ICON_CIRCLE_RADIUS 12

// These functions are miscellaneous, I'm just putting them here for now.

// return true when the state of the flags changed
bool GuiFlags(Rectangle rect, unsigned int *flags) {
    int width = rect.width / 8;
    int height = rect.height / 4;
    bool flagsChanged = false;

    for (int x = 0; x < 8; x++)
    for (int y = 0; y < 4; y++) {
        Rectangle rectToggle = {
            .x = rect.x + x * width,
            .y = rect.y + y * height,
            .width = width,
            .height = height,
        };
        char name[3];
        int idx = x * 4 + y;
        sprintf(name, "%i", idx);
        unsigned int mask = 1u << idx;
        unsigned int flagsOld = *flags;

        if (GuiToggle(rectToggle, name, *flags & mask)) {
            *flags |= mask; 
        } else {
            *flags &= ~mask;
        }

        if (*flags != flagsOld) flagsChanged = true;
    }

    return flagsChanged;
}

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
        int fileBaseLen = (int) (((unsigned long long) dotIdx - (unsigned long long) fileName) / sizeof(char) + 1);
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

        StringBuffer fullPathBuffer = StringBufferNew();
        StringBufferAddString(&fullPathBuffer, path);
        StringBufferAddChar(&fullPathBuffer, '/');
        StringBufferAddString(&fullPathBuffer, directoryEntry->d_name);

        char *fullPath = StringBufferFree(&fullPathBuffer); 
        struct stat fileStat;
        if (stat(fullPath, &fileStat) != 0) {
            printf("Failed to obtain information about the file at %s. Skipping.\n", fullPath);
        // no symlink support
        } else if (S_ISDIR(fileStat.st_mode)) {
            RecursiveUpdate(fullPath);
        } else if (S_ISREG(fileStat.st_mode)) {
            char *dot = strrchr(directoryEntry->d_name, '.');            
            if (dot && strcmp(dot, "."FILE_EXTENSION) == 0) {
                EditorState state;
                if (EditorStateDeserialize(&state, fullPath)) {
                    EditorStateSerialize(&state, fullPath);
                    EditorStateFree(&state);
                    printf("Successfully updated the combat animation file at %s.\n", fullPath);
                } else {
                    printf("Failed to read a valid combat animation from the file at %s. Skipping.\n", fullPath);
                }
            }
        }
        free(fullPath);
    }
    closedir(dir);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("Please put the name of the png file to make an animation for as the argument to this application.");
        return EXIT_FAILURE;
    }

    if (!strcmp(argv[1], "-u")) { // first argument is to recursively update all files in the given folder.
        RecursiveUpdate(".");
        return EXIT_SUCCESS;
    } else if (!strcmp(argv[1], "-t")) {

        for (int i = FILE_VERSION_OLDEST; i < FILE_VERSION_CURRENT; i++) { // make sure each version can still deserialize
            char fileName[sizeof("tests/Jab.json") + 10];
            sprintf(fileName, "tests/Jab%i.json", i); // idk how to make this work when the application is not being run from its home directory
            EditorState state;
            bool success = EditorStateDeserialize(&state, fileName);
            if (success) EditorStateFree(&state);
            printf("Version %i Deserialize success: %s\n", i, success ? "yes" : "no");
        }
        return EXIT_SUCCESS;
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    // We set the window size later so we can expand to have the right number of rows.
    // We need to init the window first so we can load the texture.
    InitWindow(1, 1, APP_NAME); 
    SetTargetFPS(60);

    
    const char *name = argv[1];
    
    StringBuffer textureNameBuffer = StringBufferNew();
    StringBufferAddString(&textureNameBuffer, name);
    StringBufferAddString(&textureNameBuffer, ".png");

    char *textureName = StringBufferFree(&textureNameBuffer);
    Texture2D texture = LoadTexture(textureName);
    free(textureName);

    if (texture.id <= 0) {
        printf("Failed to load texture.\n");
        CloseWindow();
        return EXIT_FAILURE;
    }
   
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(RAYWHITE));
    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(TEXTBOX, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

    Font fontDefault = GetFontDefault();
    const int fontSize = fontDefault.baseSize;
    
    // load state from file or create new state if load failed
    StringBuffer savePathBuffer = StringBufferNew();
    StringBufferAddString(&savePathBuffer, name);
    StringBufferAddString(&savePathBuffer, "."FILE_EXTENSION);
    char *savePath = StringBufferFree(&savePathBuffer);

    EditorState state;
    if (!EditorStateDeserialize(&state, savePath)) state = EditorStateNew(1);

    EditorHistory history = EditorHistoryNew(&state);

    const float startScale = DEFAULT_SPRITE_WINDOW_Y * TEXTURE_HEIGHT_IN_WINDOW / texture.height;
    
	Transform2D transform = Transform2DIdentity();
    transform = Transform2DSetScale(transform, (Vector2) {.x = startScale, .y = startScale});
	transform.o = (Vector2) {
        .x = (DEFAULT_SPRITE_WINDOW_X - texture.width / state.frameCount * startScale) / 2.0f,
        .y = (DEFAULT_SPRITE_WINDOW_Y - texture.height * startScale) / 2.0f
    };

    const int guiInitialHeight = state.layerCount * FRAME_ROW_SIZE + FRAME_ROW_SIZE;
    
	int initialWindowWidth = DEFAULT_SPRITE_WINDOW_X;
	int initialWindowHeight = DEFAULT_SPRITE_WINDOW_Y + guiInitialHeight;

	int monitor = GetCurrentMonitor();
	SetWindowSize(initialWindowWidth, initialWindowHeight);
	int initialWindowX = (GetMonitorWidth(monitor) - initialWindowWidth) / 2;
	int initialWindowY = (GetMonitorHeight(monitor) - initialWindowHeight) / 2;
	SetWindowPosition(initialWindowX, initialWindowY);
    
    typedef enum Mode {
        MODE_IDLE,
        MODE_PLAYING,
        MODE_DRAGGING_HANDLE,
        MODE_DRAGGING_FRAME_POS,
        MODE_PANNING_SPRITE,
        MODE_EDIT_FRAME_DURATION,
        MODE_EDIT_LAYER_NAME,
        MODE_EDIT_HITBOX_DAMAGE,
        MODE_EDIT_HITBOX_STUN,
    } Mode;
    Mode mode = MODE_IDLE;

    int playingFrameTime = 0;
    Handle draggingHandle = HANDLE_NONE;
    Vector2 panningSpriteLocalPos = VECTOR2_ZERO;
     
    while (!WindowShouldClose()) {

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
            case MODE_EDIT_LAYER_NAME:
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
                    bool snapping = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
                    assert(LayerHandleSet(state.layers + state.layerIdx, state.frameIdx, draggingHandle, localMousePos, snapping));
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
                    if (!EditorStateSerialize(&state, savePath)) {
                        puts("Failed to save file for unknown reason.");
                    }
                } else if (IsKeyPressed(KEY_UNDO) && IsKeyDown(KEY_UNDO_MODIFIER)) {
                    mode = MODE_IDLE;
                    ChangeOptions option = CHANGE_UNDO;
                    if (IsKeyDown(KEY_REDO_MODIFIER)) option = CHANGE_REDO;

                    EditorHistoryChangeState(&history, &state, option);

                } else if (IsKeyPressed(KEY_FRAME_NEW) && IsKeyDown(KEY_FRAME_NEW_MODIFIER)) {
                    EditorStateAddFrame(&state, state.frameIdx);
                    state.frameIdx = state.frameCount - 1;
                    EditorHistoryCommitState(&history, &state);
                    mode = MODE_IDLE;
                
                } else if (IsKeyPressed(KEY_FRAME_REMOVE) && IsKeyDown(KEY_FRAME_REMOVE_MODIFIER) && state.frameCount > 1) {
                    EditorStateRemoveFrame(&state, state.frameIdx);
                    if (state.frameIdx >= state.frameCount) state.frameIdx = state.frameCount - 1;
                    EditorHistoryCommitState(&history, &state);
                    mode = MODE_IDLE;

                } else if (IsKeyPressed(KEY_LAYER_REMOVE) && IsKeyDown(KEY_LAYER_REMOVE_MODIFIER) && state.layerIdx >= 0) {
                    EditorStateLayerRemove(&state, state.layerIdx);
                    EditorHistoryCommitState(&history, &state);
                    mode = MODE_IDLE;
                
                } else if (IsKeyPressed(KEY_FRAME_TOGGLE)) {
                    if (state.layerIdx < 0) {
                        state.frames[state.frameIdx].canCancel = !state.frames[state.frameIdx].canCancel;
                    } else {
                        Layer *layer = state.layers + state.layerIdx;
                        
                        bool active = !layer->framesActive[state.frameIdx];
                        layer->framesActive[state.frameIdx] = active;
                        
                        if (active && layer->type == LAYER_BEZIER) { // Initialize a new bezier point

                            BezierPoint point;
                            point.extentsLeft = 10.0f,
                            point.extentsRight = 10.0f,
                            point.rotation = 0.0f;
                            
                            // If the points around this bezier are enabled, interpolate between them to find the bezier position.
                            bool surroundingPoints = 
                                state.frameCount >= 3 
                                && state.frameIdx > 0 
                                && state.frameIdx < state.frameCount - 1
                                && layer->framesActive[state.frameIdx - 1]
                                && layer->framesActive[state.frameIdx + 1];
                            
                            if (surroundingPoints) { // Lerp between surrouding points if they exist
                                BezierPoint prev = layer->bezierPoints[state.frameIdx - 1];
                                BezierPoint next = layer->bezierPoints[state.frameIdx + 1];
                                point.position = Vector2Lerp(prev.position, next.position, 0.5f);
                            } else if (state.frameIdx < state.frameCount - 1 && layer->framesActive[state.frameIdx + 1]) {
                                Vector2 origin = layer->bezierPoints[state.frameIdx + 1].position;
                                origin.x -= 10.0f;
                                point.position = origin;
                            } else if (state.frameIdx > 0 && layer->framesActive[state.frameIdx - 1]) {
                                Vector2 origin = layer->bezierPoints[state.frameIdx - 1].position;
                                origin.x += 10.0f;
                                point.position = origin;
                            } else {
                                point.position = (Vector2) {0.0f, 0.0f};    
                            }

                            layer->bezierPoints[state.frameIdx] = point;
                        }     
                    }
                    mode = MODE_IDLE;
                    EditorHistoryCommitState(&history, &state);
                
                } else if (IsKeyDown(KEY_LAYER_NEW_MODIFIER)) { // VERY IMPORTANT THAT THIS IS THE LAST CALL THAT CHECKS KEY_LEFT_CTRL
                    Layer layer;
                    layer.transform = Transform2DIdentity();
                    layer.transform.o = (Vector2) { // spawn at the center of the frame.
                            .x = (float) (texture.width / (state.frameCount * 2)),
                            .y = (float) (texture.height / 2)
                    };
                    

                    if (IsKeyPressed(KEY_E)) { // Empty
                        layer.type = LAYER_EMPTY;

                    } else if (IsKeyPressed(KEY_B)) { // Bezier
                        layer.type = LAYER_BEZIER;
                        layer.bezierPoints = LIST_NEW_SIZED(BezierPoint, state.frameCount);

                    } else if (IsKeyDown(KEY_H) || IsKeyDown(KEY_N)) { // Hurtbox or Hitbox
                        Shape shape;
                        if (IsKeyPressed(KEY_ONE)) {
                            shape.type = SHAPE_CIRCLE;
                            shape.circleRadius = 24;
                        } else if (IsKeyPressed(KEY_TWO)) {
                            shape.type = SHAPE_RECTANGLE;
                            shape.rectangle.rightX = 24;
                            shape.rectangle.bottomY = 24;
                        } else if (IsKeyPressed(KEY_THREE)) {
                            shape.type = SHAPE_CAPSULE;
                            shape.capsule.radius = 24;
                            shape.capsule.height = 24;
                            shape.capsule.rotation = 0;
                        } else goto layerNotInstanced;

                        if (IsKeyDown(KEY_H)) {
                            layer.type = LAYER_SHAPE;
                            layer.shape.shape = shape;
                            layer.shape.flags = 1;
                        } else {
                            layer.type = LAYER_HITBOX;
                            layer.hitbox.shape = shape;
                            layer.hitbox.knockbackX = 2;
                            layer.hitbox.knockbackY = -2;
                            layer.hitbox.damage = 0;
                            layer.hitbox.stun = 1000;
                        }

                    } else {
                        goto layerNotInstanced;
                    }
                
                    layer.nameBufferLength = LAYER_NAME_BUFFER_INITIAL_SIZE;
                    layer.name = malloc(layer.nameBufferLength);                    
                    // Right now we are not enforcing the uniqueness of layer names.
                    snprintf(layer.name, layer.nameBufferLength, "Layer %i", state.layerCount);

                    layer.framesActive = LIST_NEW_SIZED(bool, state.frameCount);
                    memset(layer.framesActive, 0, sizeof(bool) * state.frameCount);
                    
                    EditorStateLayerAdd(&state, layer);
                    state.layerIdx = state.layerCount - 1;
                    EditorHistoryCommitState(&history, &state);
                    mode = MODE_IDLE;

                    layerNotInstanced:;// don't add the layer.
                } else if (IsMouseButtonPressed(MOUSE_BUTTON_SELECT)) {
                    if (HandleIsColliding(transform, mousePos, state.frames[state.frameIdx].pos)) {
                        mode = MODE_DRAGGING_FRAME_POS;
                    } else {
                        if (state.layerIdx >= 0) {
                            draggingHandle = LayerHandleSelect(state.layers + state.layerIdx, state.frameIdx, transform, mousePos);
                        } else {
                            draggingHandle = HANDLE_NONE;
                        }

                        if (draggingHandle != HANDLE_NONE) {
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
                    int frameDir = (IsKeyPressed(KEY_FRAME_NEXT) ? 1 : 0) - (IsKeyPressed(KEY_FRAME_PREVIOUS) ? 1 : 0);
                    if (frameDir) {
                        mode = MODE_IDLE;
                        int newFrame = state.frameIdx + frameDir;

                        if (newFrame < 0) state.frameIdx = state.frameCount - 1;
                        else if (newFrame >= state.frameCount) state.frameIdx = 0;
                        else state.frameIdx = newFrame;
                    }

                    int layerDir = (IsKeyPressed(KEY_LAYER_NEXT) ? 1 : 0) - (IsKeyPressed(KEY_LAYER_PREVIOUS) ? 1 : 0);
                    if (layerDir) {
                        mode = MODE_IDLE;
                        state.layerIdx = Clamp(state.layerIdx + layerDir, -1, state.layerCount - 1);
                    }
                }
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

        int windowX = GetScreenWidth();
        int windowY = GetScreenHeight();
        int hitboxRowY = windowY - LAYER_ROW_SIZE * state.layerCount;
        int timelineY = hitboxRowY - FRAME_ROW_SIZE;
        int timelineHeight = windowY - timelineY;
        
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

        // draw layers
        for (int i = 0; i < state.layerCount; i++) {
            LayerDraw(state.layers + i, state.frameIdx, transform, i == state.layerIdx);
        }
        
        // draw frame pos handle
        if (state.frameIdx > 0) {
            Vector2 globalFramePosPrevious = Transform2DToGlobal(transform, state.frames[state.frameIdx - 1].pos);
            DrawCircle(globalFramePosPrevious.x, globalFramePosPrevious.y, HANDLE_RADIUS, COLOR_FRAME_POS_HANDLE_PREVIOUS);  
        }
        Vector2 globalFramePos = Transform2DToGlobal(transform, state.frames[state.frameIdx].pos);
        HandleDraw(globalFramePos, COLOR_FRAME_POS_HANDLE);

        // draw frame duration value box
        float rectStroke = (float) (fontSize + 8);
        Rectangle rectLabel = { 0.0f, 0.0f, 128.0f, rectStroke };
        Rectangle rectValue = { 128.0f, 0.0f, 128.0f, rectStroke };

        // Draw gui
        
        GuiLabel(rectLabel, "Frame Duration (ms)");
        if (GuiValueBox(rectValue, NULL, &state.frames[state.frameIdx].duration, 1, INT_MAX, mode == MODE_EDIT_FRAME_DURATION)) {
            mode = MODE_EDIT_FRAME_DURATION;
        }

        if (state.layerIdx >= 0) {    
            rectLabel.y += rectStroke;
            rectValue.y += rectStroke;
            
            GuiLabel(rectLabel, "Layer Name");
            Layer *layer = state.layers + state.layerIdx;
            if (GuiTextBox(rectValue, layer->name, layer->nameBufferLength, mode == MODE_EDIT_LAYER_NAME)) {
                if (mode == MODE_EDIT_LAYER_NAME) {
                    EditorHistoryCommitState(&history, &state);
                    mode = MODE_IDLE;
                } else {
                    mode = MODE_EDIT_LAYER_NAME;
                }
            }
            
            // If the user has filled up the name buffer, reallocate it.
            if (mode == MODE_EDIT_LAYER_NAME && strlen(layer->name) == layer->nameBufferLength - 1) {
                layer->nameBufferLength = (int) (((float) layer->nameBufferLength) * LAYER_NAME_BUFFER_RESIZE_MULTIPLIER);
                layer->name = realloc(layer->name, layer->nameBufferLength);
            }

            rectLabel.y += rectStroke;
            rectValue.y += rectStroke;
            
            switch (state.layers[state.layerIdx].type) {
                case LAYER_HITBOX:
                    GuiLabel(rectLabel, "Hitbox Damage");
                    if (GuiValueBox(rectValue, NULL, &state.layers[state.layerIdx].hitbox.damage, 0, INT_MAX, mode == MODE_EDIT_HITBOX_DAMAGE)) {
                        if (mode == MODE_EDIT_HITBOX_DAMAGE) {
                            EditorHistoryCommitState(&history, &state);
                            mode = MODE_IDLE;
                        } else {
                            mode = MODE_EDIT_HITBOX_DAMAGE;
                        }
                    }
                    
                    rectLabel.y += rectStroke;
                    rectValue.y += rectStroke;

                    GuiLabel(rectLabel, "Hitbox Stun (ms)");
                    if (GuiValueBox(rectValue, NULL, &state.layers[state.layerIdx].hitbox.stun, 0, INT_MAX, mode == MODE_EDIT_HITBOX_STUN)) {
                        if (mode == MODE_EDIT_HITBOX_STUN) {
                            EditorHistoryCommitState(&history, &state);
                            mode = MODE_IDLE;
                        } else {
                            mode = MODE_EDIT_HITBOX_STUN;
                        }
                    }
                    break;

                case LAYER_SHAPE:
                    Rectangle rectFlagsLabel = rectLabel;
                    Rectangle rectFlagsValue = rectValue;
                    
                    rectFlagsLabel.height *= 3;
                    rectFlagsValue.height *= 3;

                    GuiLabel(rectFlagsLabel, "Shape type");
                    if (GuiFlags(rectFlagsValue, &state.layers[state.layerIdx].shape.flags)) {
                        EditorHistoryCommitState(&history, &state);
                    }
                    break;

                case LAYER_BEZIER:
                case LAYER_EMPTY:
                    break;
            }
        }
        
        
        // draw timeline
        DrawRectangle(0, timelineY, windowX, timelineHeight, TIMELINE_COLOR); // draw timeline background
        int selectedX = FRAME_ROW_SIZE * state.frameIdx;
        int selectedY = state.layerIdx >= 0 ? timelineY + FRAME_ROW_SIZE + state.layerIdx * LAYER_ROW_SIZE : timelineY;
        DrawRectangle(selectedX, selectedY, FRAME_ROW_SIZE, FRAME_ROW_SIZE, COLOR_SELECTED);

        for (int i = 0; i < state.frameCount; i++) {
            int xPos = i * FRAME_ROW_SIZE + FRAME_ROW_SIZE / 2;

            Color frameColor = state.frames[i].canCancel ? FRAME_RHOMBUS_CAN_CANCEL_COLOR : FRAME_RHOMBUS_CANNOT_CANCEL_COLOR;
            Vector2 frameCenter = {(float) xPos, (float) timelineY + FRAME_ROW_SIZE / 2.0f};
            DrawRhombus(frameCenter, FRAME_RHOMBUS_RADIUS, FRAME_RHOMBUS_RADIUS, frameColor);
            
            const char *text = TextFormat("%i", i + 1);
            int textX = (int) (frameCenter.x - (float) MeasureText(text, fontSize) / 2.0f);
            int textY = (int) (frameCenter.y - (float) fontSize / 2.0f);
            DrawText(text, textX, textY, fontSize, FRAME_ROW_TEXT_COLOR);
            for (int j = 0; j < state.layerCount; j++) {
                Color color = layerColors[state.layers[j].type];
                if (!state.layers[j].framesActive[i]) {
                    color.r /= 4;
                    color.g /= 4;
                    color.b /= 4;
                }
                int layerY = hitboxRowY + (int) (LAYER_ROW_SIZE * ((float) j + 0.5f));
                DrawCircle(xPos, layerY, LAYER_ICON_CIRCLE_RADIUS, color);
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
