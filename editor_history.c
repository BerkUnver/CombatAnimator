#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cjson/cJSON.h"
#include "combat_shape.h"
#include "string_buffer.h"
#include "editor_history.h"

EditorState EditorStateNew(int frameCount) {
    FrameInfo *frames = malloc(sizeof(FrameInfo) * frameCount);
    for (int i = 0; i < frameCount; i++) frames[i] = FRAME_INFO_DEFAULT;

    return (EditorState) {
        .shapeCount = 0,
        .shapes = NULL,
        ._shapeActiveFrames = NULL,
        .frames = frames,
        .frameCount = frameCount,
        .frameIdx = 0,
        .shapeIdx = -1
    };
}

void EditorStateFree(EditorState *state) {
    free(state->shapes);
    free(state->_shapeActiveFrames);
    free(state->frames);
}

bool EditorStateShapeActiveGet(EditorState *state, int frameIdx, int shapeIdx) {
    return state->_shapeActiveFrames[state->frameCount * shapeIdx + frameIdx];
}

void EditorStateShapeActiveSet(EditorState *state, int frameIdx, int shapeIdx, bool enabled) {
    state->_shapeActiveFrames[state->frameCount * shapeIdx + frameIdx] = enabled;
}

void EditorStateAddShape(EditorState *state, CombatShape shape) {
    int oldMax = state->shapeCount * state->frameCount;
    state->shapeCount++;
    state->shapes = realloc(state->shapes, sizeof(CombatShape) * state->shapeCount);
    state->shapes[state->shapeCount - 1] = shape;
    int newMax = state->shapeCount * state->frameCount;
    state->_shapeActiveFrames = realloc(state->_shapeActiveFrames, sizeof(bool) * newMax);
    memset(state->_shapeActiveFrames + oldMax, false, (newMax - oldMax) * sizeof(bool));
}

bool EditorStateRemoveShape(EditorState *state, int idx) {
    if (idx < 0 || idx >= state->shapeCount) return false;
    
    for (int shapeIdx = idx + 1; shapeIdx < state->shapeCount; shapeIdx++) {
        int newShapeIdx = shapeIdx - 1;
        state->shapes[newShapeIdx] = state->shapes[shapeIdx];

        for (int frameIdx = 0; frameIdx < state->frameCount; frameIdx++) {
            state->_shapeActiveFrames[state->frameCount * newShapeIdx + frameIdx] = state->_shapeActiveFrames[state->frameCount * shapeIdx + frameIdx];
        }
    }
    state->shapeCount--;
    if (state->shapeIdx >= state->shapeCount) state->shapeIdx = state->shapeCount - 1;
    return true;
}

void EditorStateAddFrame(EditorState *state) {
    state->frameCount++; 
    state->frames = realloc(state->frames, sizeof(FrameInfo) * state->frameCount);
    state->frames[state->frameCount - 1] = state->frames[state->frameCount - 2];
    state->_shapeActiveFrames = realloc(state->_shapeActiveFrames, sizeof(bool) * state->frameCount * state->shapeCount);
    for (int chunkIdx = state->shapeCount - 1; chunkIdx >= 0; chunkIdx--) {
        state->_shapeActiveFrames[chunkIdx * state->frameCount + state->frameCount - 1] = false;
        for (int i = state->frameCount - 2; i >= 0; i--) {
            state->_shapeActiveFrames[chunkIdx * state->frameCount + i] = state->_shapeActiveFrames[chunkIdx * state->frameCount + i - chunkIdx];
        }
    }
}

bool EditorStateRemoveFrame(EditorState *state, int idx) {
    if (idx < 0 || idx >= state->frameCount || state->frameCount == 1) return false;
    int activeLen = state->frameCount * state->shapeCount;
    for (int i = 0; i < activeLen; i++) {
        int shift = (i + state->frameCount - 1 - idx) / state->frameCount; // get the index to move to
        state->_shapeActiveFrames[i - shift] = state->_shapeActiveFrames[i];
    }
    state->frameCount--;
    return true;
}

EditorState EditorStateDeepCopy(EditorState *state) {
    int activeFramesSize = sizeof(bool) * state->shapeCount * state->frameCount;
    bool *activeFramesCopy = malloc(activeFramesSize);
    memcpy(activeFramesCopy, state->_shapeActiveFrames, activeFramesSize);

    int shapesSize = sizeof(CombatShape) * state->shapeCount;
    CombatShape *shapesCopy = malloc(shapesSize);
    memcpy(shapesCopy, state->shapes, shapesSize);

    int framesSize = sizeof(FrameInfo) * state->frameCount;
    FrameInfo *framesCopy = malloc(framesSize);
    memcpy(framesCopy, state->frames, framesSize);

    return (EditorState) {
        .shapeCount = state->shapeCount,
        .frameCount = state->frameCount,
        .shapes = shapesCopy,
        ._shapeActiveFrames = activeFramesCopy,
        .frames = framesCopy,
        .frameIdx = state->frameIdx,

        .shapeIdx = state->shapeIdx
    };
}

/// clones everything passed in, is safe.
EditorHistory EditorHistoryNew(EditorState *initial) {
    EditorHistory history;
    history._states = malloc(sizeof(EditorState) * HISTORY_BUFFER_SIZE_INCREMENT);
    history._states[0] = EditorStateDeepCopy(initial);
    history._statesLength = HISTORY_BUFFER_SIZE_INCREMENT;
    history._currentStateIdx = 0;
    history._mostRecentStateIdx = 0;
    return history;
}

void EditorHistoryFree(EditorHistory *history) {
    for (int i = 0; i <= history->_mostRecentStateIdx; i++) {
        EditorStateFree(&history->_states[i]);
    }
    free(history->_states);
}

void EditorHistoryCommitState(EditorHistory *history, EditorState *state) {
    for (int i = history->_currentStateIdx + 1; i <= history->_mostRecentStateIdx; i++) {
        EditorStateFree(&history->_states[i]);
    }

    history->_currentStateIdx++;
    history->_mostRecentStateIdx = history->_currentStateIdx;
    
    if (history->_currentStateIdx >= history->_statesLength) {
        // array pointers are copied over so it works.
        history->_states = realloc(history->_states, sizeof(EditorState) * (history->_statesLength + HISTORY_BUFFER_SIZE_INCREMENT)); 
        history->_statesLength += HISTORY_BUFFER_SIZE_INCREMENT;
    }

    history->_states[history->_currentStateIdx] = EditorStateDeepCopy(state);
}

/// @brief
/// @param history 
/// @param state Replaced with the state changed to. All cleanup is done automatically. 
/// @param option 
void EditorHistoryChangeState(EditorHistory *history, EditorState *state, ChangeOptions option) {
    if (option == UNDO){
        if (history->_currentStateIdx <= 0) return;
        history->_currentStateIdx--;
    } else {
        if (history->_currentStateIdx >= history->_mostRecentStateIdx) return;
        history->_currentStateIdx++;
    }

    EditorState oldState = *state;
    *state = EditorStateDeepCopy(&history->_states[history->_currentStateIdx]);
    EditorStateFree(&oldState);
}



cJSON *EditorStateSerialize(EditorState *state) {
    cJSON *shapes = cJSON_CreateArray();

    for (int i = 0; i < state->shapeCount; i++) { // should not enter if state.shapes is null cause count is also 0
        cJSON *shape = CombatShapeSerialize(state->shapes[i]);
        if (!shape) {
            cJSON_Delete(shapes);
            return NULL;
        }
        cJSON_AddItemToArray(shapes, shape);
    }

    cJSON *activeFrames = cJSON_CreateArray();
    int activeCount = state->frameCount * state->shapeCount;
    for (int i = 0; i < activeCount; i++) {
        cJSON *val = cJSON_CreateNumber(state->_shapeActiveFrames[i]);
        cJSON_AddItemToArray(activeFrames, val);
    }

    cJSON *frames = cJSON_CreateArray();
    for (int i = 0; i < state->frameCount; i++) {
        cJSON *frame = cJSON_CreateObject();
        FrameInfo frameInfo = state->frames[i];
        cJSON_AddNumberToObject(frame, STR_FRAME_INFO_DURATION, frameInfo.duration);
        cJSON_AddBoolToObject(frame, STR_FRAME_INFO_CAN_CANCEL, frameInfo.canCancel);
        cJSON_AddNumberToObject(frame, STR_FRAME_INFO_X, frameInfo.pos.x);
        cJSON_AddNumberToObject(frame, STR_FRAME_INFO_Y, frameInfo.pos.y);
        if (frameInfo.velocity.x != 0) cJSON_AddNumberToObject(frame, STR_FRAME_INFO_VELOCITY_X, frameInfo.velocity.x);
        if (frameInfo.velocity.y != 0) cJSON_AddNumberToObject(frame, STR_FRAME_INFO_VELOCITY_Y, frameInfo.velocity.y);
        cJSON_AddItemToArray(frames, frame);
    }

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, STR_MAGIC, STR_MAGIC_VALUE);
    cJSON_AddNumberToObject(json, STR_VERSION, VERSION_NUMBER);

    cJSON_AddItemToObject(json, STR_SHAPE_ACTIVE_FRAMES, activeFrames);
    cJSON_AddItemToObject(json, STR_SHAPES, shapes);
    cJSON_AddItemToObject(json, STR_FRAMES, frames);
    return json;
}

bool EditorStateDeserialize(cJSON *json, EditorState *state) {
    if (!cJSON_IsObject(json)) return false;
    
    cJSON *version_json = cJSON_GetObjectItem(json, STR_VERSION);
    int version;
    if (!version_json)
        version = 0;
    else if (!cJSON_IsNumber(version_json)) 
        return false;
    else {
        int v = cJSON_GetNumberValue(version_json);
        if (v <= 0 || VERSION_NUMBER < v) return false;
        version = v;
    }

    if (version == 0) {
        cJSON *frameDurations = cJSON_GetObjectItem(json, STR_FRAME_DURATIONS);
        if (!frameDurations || !cJSON_IsArray(frameDurations)) return false;
        int frameCount = cJSON_GetArraySize(frameDurations);
        if (frameCount <= 0) return false;
        *state = EditorStateNew(frameCount);
        
        cJSON *frameDuration;
        int frameIdx = 0;
        cJSON_ArrayForEach(frameDuration, frameDurations) {
            if (!cJSON_IsNumber(frameDuration)) goto fail;
            state->frames[frameIdx].duration = (int) cJSON_GetNumberValue(frameDuration);
            frameIdx++;
        }
    } else {
        cJSON *magic = cJSON_GetObjectItem(json, STR_MAGIC);
        if (!magic || !cJSON_IsString(magic) || strcmp(cJSON_GetStringValue(magic), STR_MAGIC_VALUE) != 0) return false;
        
        cJSON *frames = cJSON_GetObjectItem(json, STR_FRAMES);
        if (!frames || !cJSON_IsArray(frames)) return false;
        int frameCount = cJSON_GetArraySize(frames);
        *state = EditorStateNew(frameCount);

        cJSON *frame;
        int frameIdx = 0;
        cJSON_ArrayForEach(frame, frames) {
            if (!cJSON_IsObject(frame)) goto fail;

            cJSON *duration = cJSON_GetObjectItem(frame, STR_FRAME_INFO_DURATION);
            if (!duration || !cJSON_IsNumber(duration)) goto fail; 
            state->frames[frameIdx].duration = cJSON_GetNumberValue(duration);
            
            cJSON *canCancel = cJSON_GetObjectItem(frame, STR_FRAME_INFO_CAN_CANCEL);
            if (!canCancel || !cJSON_IsBool(canCancel)) goto fail;
            state->frames[frameIdx].canCancel = cJSON_IsTrue(canCancel);

            if (version > 1) {
                cJSON *x = cJSON_GetObjectItem(frame, STR_FRAME_INFO_X);
                if (!x || !cJSON_IsNumber(x)) goto fail;
                
                cJSON *y = cJSON_GetObjectItem(frame, STR_FRAME_INFO_Y);
                if (!y || !cJSON_IsNumber(y)) goto fail;
                state->frames[frameIdx].pos = (Vector2) {.x = cJSON_GetNumberValue(x), .y = cJSON_GetNumberValue(y)};
                
                cJSON *velocityX = cJSON_GetObjectItem(frame, STR_FRAME_INFO_VELOCITY_X);
                if (velocityX) {
                    if (!cJSON_IsNumber(velocityX)) goto fail;
                    state->frames[frameIdx].velocity.x = cJSON_GetNumberValue(velocityX);
                }

                cJSON *velocityY = cJSON_GetObjectItem(frame, STR_FRAME_INFO_VELOCITY_Y);
                if (velocityY) {
                    if (!cJSON_IsNumber(velocityY)) goto fail;
                    state->frames[frameIdx].velocity.y = cJSON_GetNumberValue(velocityY);
                }
            }

            frameIdx++;
        }
    }

    cJSON *shapes = cJSON_GetObjectItem(json, STR_SHAPES);
    if (!shapes || !cJSON_IsArray(shapes)) goto fail;
    cJSON *jsonShape;
    cJSON_ArrayForEach(jsonShape, shapes) {
        CombatShape shape;
        if (!CombatShapeDeserialize(jsonShape, version, &shape)) goto fail;
        EditorStateAddShape(state, shape);
    }

    cJSON *activeFrames = cJSON_GetObjectItem(json, STR_SHAPE_ACTIVE_FRAMES);
    if (!activeFrames || !cJSON_IsArray(activeFrames)) goto fail;
    int activeCount = state->shapeCount * state->frameCount; // frameCount will always be >= 1, if shapeCount = 0 then shapes is null
    cJSON *active;
    int activeIdx = 0;
    cJSON_ArrayForEach(active, activeFrames) {
        if (!cJSON_IsNumber(active) || activeIdx >= activeCount) goto fail;
        state->_shapeActiveFrames[activeIdx] = cJSON_GetNumberValue(active) != 0;
        activeIdx++;
    }

    return true;

    fail:
    EditorStateFree(state);
    return false;
}

bool EditorStateReadFromFile(EditorState *state, const char *path) {
    FILE *file = fopen(path, "r+");
    if (!file) return false;
    StringBuffer buffer = StringBufferNew();
    int c;
    while ((c = fgetc(file)) != EOF) {
        StringBufferAddChar(&buffer, (char) c);
    }

    fclose(file);
    cJSON *json = cJSON_Parse(buffer.raw);
    StringBufferFree(&buffer);

    if (!json) return false;
    
    bool success = EditorStateDeserialize(json, state);
    cJSON_Delete(json);
    return success;
}

bool EditorStateWriteToFile(EditorState *state, const char *path) {
    cJSON *json = EditorStateSerialize(state);
    if (!json) return false;
    
    char *str = cJSON_Print(json);
    FILE *saveFile = fopen(path, "w+");
    if (!saveFile) {
        cJSON_Delete(json);
        return false;
    }
    fputs(str, saveFile);
    free(str);
    fclose(saveFile);
    cJSON_Delete(json);
    return true;
}
