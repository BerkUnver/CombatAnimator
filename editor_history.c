#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cjson/cJSON.h"
#include "layer.h"
#include "string_buffer.h"
#include "editor_history.h"

EditorState EditorStateNew(int frameCount) {
    FrameInfo *frames = malloc(sizeof(FrameInfo) * frameCount);
    for (int i = 0; i < frameCount; i++) frames[i] = FRAME_INFO_DEFAULT;

    return (EditorState) {
        .layerCount = 0,
        .layers = NULL,
        ._layerActiveFrames = NULL,
        .frames = frames,
        .frameCount = frameCount,
        .frameIdx = 0,
        .layerIdx = -1
    };
}

void EditorStateFree(EditorState *state) {
    free(state->layers);
    free(state->_layerActiveFrames);
    free(state->frames);
}

bool EditorStateLayerActiveGet(EditorState *state, int frameIdx, int layerIdx) {
    return state->_layerActiveFrames[state->frameCount * layerIdx + frameIdx];
}

void EditorStateLayerActiveSet(EditorState *state, int frameIdx, int layerIdx, bool enabled) {
    state->_layerActiveFrames[state->frameCount * layerIdx + frameIdx] = enabled;
}

void EditorStateLayerAdd(EditorState *state, Layer layer) {
    int oldMax = state->layerCount * state->frameCount;
    state->layerCount++;
    state->layers = realloc(state->layers, sizeof(Layer) * state->layerCount);
    state->layers[state->layerCount - 1] = layer;
    int newMax = state->layerCount * state->frameCount;
    state->_layerActiveFrames = realloc(state->_layerActiveFrames, sizeof(bool) * newMax);
    memset(state->_layerActiveFrames + oldMax, false, (newMax - oldMax) * sizeof(bool));
}

bool EditorStateLayerRemove(EditorState *state, int idx) {
    if (idx < 0 || idx >= state->layerCount) return false;
    
    for (int layerIdx = idx + 1; layerIdx < state->layerCount; layerIdx++) {
        int newLayerIdx = layerIdx - 1;
        state->layers[newLayerIdx] = state->layers[layerIdx];

        for (int frameIdx = 0; frameIdx < state->frameCount; frameIdx++) {
            state->_layerActiveFrames[state->frameCount * newLayerIdx + frameIdx] = state->_layerActiveFrames[state->frameCount * layerIdx + frameIdx];
        }
    }
    state->layerCount--;
    if (state->layerIdx >= state->layerCount) state->layerIdx = state->layerCount - 1;
    return true;
}

void EditorStateAddFrame(EditorState *state) {
    state->frameCount++; 
    state->frames = realloc(state->frames, sizeof(FrameInfo) * state->frameCount);
    state->frames[state->frameCount - 1] = state->frames[state->frameCount - 2];
    state->_layerActiveFrames = realloc(state->_layerActiveFrames, sizeof(bool) * state->frameCount * state->layerCount);
    for (int chunkIdx = state->layerCount - 1; chunkIdx >= 0; chunkIdx--) {
        state->_layerActiveFrames[chunkIdx * state->frameCount + state->frameCount - 1] = false;
        for (int i = state->frameCount - 2; i >= 0; i--) {
            state->_layerActiveFrames[chunkIdx * state->frameCount + i] = state->_layerActiveFrames[chunkIdx * state->frameCount + i - chunkIdx];
        }
    }
}

bool EditorStateRemoveFrame(EditorState *state, int idx) {
    if (idx < 0 || idx >= state->frameCount || state->frameCount == 1) return false;
    int activeLen = state->frameCount * state->layerCount;
    for (int i = 0; i < activeLen; i++) {
        int shift = (i + state->frameCount - 1 - idx) / state->frameCount; // get the index to move to
        state->_layerActiveFrames[i - shift] = state->_layerActiveFrames[i];
    }
    state->frameCount--;
    return true;
}

EditorState EditorStateDeepCopy(EditorState *state) {
    int activeFramesSize = sizeof(bool) * state->layerCount * state->frameCount;
    bool *activeFramesCopy = malloc(activeFramesSize);
    memcpy(activeFramesCopy, state->_layerActiveFrames, activeFramesSize);

    int layersSize = sizeof(Layer) * state->layerCount;
    Layer *layersCopy = malloc(layersSize);
    memcpy(layersCopy, state->layers, layersSize);

    int framesSize = sizeof(FrameInfo) * state->frameCount;
    FrameInfo *framesCopy = malloc(framesSize);
    memcpy(framesCopy, state->frames, framesSize);

    return (EditorState) {
        .layerCount = state->layerCount,
        .frameCount = state->frameCount,
        .layers = layersCopy,
        ._layerActiveFrames = activeFramesCopy,
        .frames = framesCopy,
        .frameIdx = state->frameIdx,

        .layerIdx = state->layerIdx
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
    if (option == CHANGE_UNDO){
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


/*
cJSON *EditorStateSerialize(EditorState *state) {
    cJSON *layers = cJSON_CreateArray();

    for (int i = 0; i < state->layerCount; i++) {
        cJSON *layer = LayerSerialize(state->layers[i]);
        if (!layer) {
            cJSON_Delete(layers);
            return NULL;
        }
        cJSON_AddItemToArray(layers, layer);
    }

    cJSON *activeFrames = cJSON_CreateArray();
    int activeCount = state->frameCount * state->layerCount;
    for (int i = 0; i < activeCount; i++) {
        cJSON *val = cJSON_CreateNumber(state->_layerActiveFrames[i]);
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
        cJSON_AddItemToArray(frames, frame);
    }

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, STR_MAGIC, STR_MAGIC_VALUE);
    cJSON_AddNumberToObject(json, STR_VERSION, VERSION_NUMBER);

    cJSON_AddItemToObject(json, STR_LAYER_ACTIVE_FRAMES, activeFrames);
    cJSON_AddItemToObject(json, STR_LAYERS, layers);
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
        int v = (int) cJSON_GetNumberValue(version_json);
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
            state->frames[frameIdx].duration = (int) cJSON_GetNumberValue(duration);
            
            cJSON *canCancel = cJSON_GetObjectItem(frame, STR_FRAME_INFO_CAN_CANCEL);
            if (!canCancel || !cJSON_IsBool(canCancel)) goto fail;
            state->frames[frameIdx].canCancel = cJSON_IsTrue(canCancel);

            if (version >= 2) {
                cJSON *x = cJSON_GetObjectItem(frame, STR_FRAME_INFO_X);
                if (!x || !cJSON_IsNumber(x)) goto fail;
                
                cJSON *y = cJSON_GetObjectItem(frame, STR_FRAME_INFO_Y);
                if (!y || !cJSON_IsNumber(y)) goto fail;
                state->frames[frameIdx].pos = (Vector2) {
                    .x = (float) cJSON_GetNumberValue(x),
                    .y = (float) cJSON_GetNumberValue(y)
                };
            }
            frameIdx++;
        }
    }

    cJSON *layers = cJSON_GetObjectItem(json, version >= 4 ? STR_LAYERS : "shapes");
    if (!layers || !cJSON_IsArray(layers)) goto fail;
    cJSON *jsonShape;
    cJSON_ArrayForEach(jsonShape, layers) {
        Layer layer;
        if (!LayerDeserialize(jsonShape, version, &layer)) goto fail;
        EditorStateLayerAdd(state, layer);
    }

    cJSON *activeFrames = cJSON_GetObjectItem(json, version >= 4 ? STR_LAYER_ACTIVE_FRAMES : "shapeActiveFrames");
    if (!activeFrames || !cJSON_IsArray(activeFrames)) goto fail;
    int activeCount = state->layerCount * state->frameCount;
    cJSON *active;
    int activeIdx = 0;
    cJSON_ArrayForEach(active, activeFrames) {
        if (!cJSON_IsNumber(active) || activeIdx >= activeCount) goto fail;
        state->_layerActiveFrames[activeIdx] = cJSON_GetNumberValue(active) != 0;
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
*/