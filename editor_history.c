#include <stdlib.h>
#include <string.h>
#include "editor_history.h"
#include "combat_shape.h"
#include "cjson/cJSON.h"

EditorState AllocEditorState(int frames) {
    EditorState state;
    state.shapeCount = 0;
    state.shapes = NULL;
    state._shapeActiveFrames = NULL;

    state.frameCount = frames;
    state.frameIdx = 0;
    state.shapeIdx = -1;
    return state;
}

void FreeEditorState(EditorState *state) {
    free(state->shapes);
    free(state->_shapeActiveFrames);
}

bool GetShapeActive(EditorState *state, int frameIdx, int shapeIdx) {
    return state->_shapeActiveFrames[state->frameCount * shapeIdx + frameIdx];
}

void SetShapeActive(EditorState *state, int frameIdx, int shapeIdx, bool active) {
    state->_shapeActiveFrames[state->frameCount * shapeIdx + frameIdx] = active;
}

void AddShape(EditorState *state, CombatShape shape) { // Should work on nullptrs
    int oldMax = state->shapeCount * state->frameCount;
    state->shapeCount++;
    state->shapes = realloc(state->shapes, sizeof(CombatShape) * state->shapeCount);
    state->shapes[state->shapeCount - 1] = shape;
    state->_shapeActiveFrames = realloc(state->_shapeActiveFrames, sizeof(bool) * state->shapeCount * state->frameCount);
    int newMax = state->shapeCount * state->frameCount;
    for (int i = oldMax; i < newMax; i++) { // init all new hitboxes to false
        state->_shapeActiveFrames[i] = false;
    }
}

void AddFrame(EditorState *state) {
    int oldFrameCount = state->frameCount;
    bool* oldFrames = state->_shapeActiveFrames;
    state->frameCount++;
    state->_shapeActiveFrames = malloc(sizeof(bool) * state->frameCount * state->shapeCount);
    int chunkSize = (int) sizeof(bool) * oldFrameCount;
    for (int shapeIdx = 0; shapeIdx < state->shapeCount; shapeIdx++) {
        bool *startPtr = state->_shapeActiveFrames + shapeIdx * state->frameCount;
        bool *oldStartPtr = oldFrames + shapeIdx * oldFrameCount;
        memcpy(startPtr, oldStartPtr, chunkSize);
        startPtr[state->frameCount - 1] = false; // new frame is set to false;
    }
    free(oldFrames);
}

EditorState EditorStateDeepCopy(EditorState *state) {
    int activeFramesSize = sizeof(bool) * state->shapeCount * state->frameCount;
    bool *activeFramesCopy = malloc(activeFramesSize);
    memcpy(activeFramesCopy, state->_shapeActiveFrames, activeFramesSize);

    int shapesSize = sizeof(CombatShape) * state->shapeCount;
    CombatShape *shapesCopy = malloc(shapesSize);
    memcpy(shapesCopy, state->shapes, shapesSize);

    EditorState newState;
    newState.shapeCount = state->shapeCount;
    newState.frameCount = state->frameCount;
    newState.shapes = shapesCopy;
    newState._shapeActiveFrames = activeFramesCopy;
    newState.frameIdx = state->frameIdx;
    newState.shapeIdx = state->shapeIdx;
    return newState;
}

/// clones everything passed in, is safe.
EditorHistory AllocEditorHistory(EditorState *initial) {
    EditorHistory history;
    history._states = malloc(sizeof(EditorState) * HISTORY_BUFFER_SIZE_INCREMENT);
    history._statesLength = HISTORY_BUFFER_SIZE_INCREMENT;
    history._currentStateIdx = 0;
    history._mostRecentStateIdx = 0;
    history._states[0] = EditorStateDeepCopy(initial);
    return history;
}

void FreeEditorHistory(EditorHistory *history) {
    for (int i = 0; i < history->_mostRecentStateIdx; i++) {
        FreeEditorState(&history->_states[i]);
    }
    free(history->_states);
}

void CommitState(EditorHistory *history, EditorState *state) {
    for (int i = history->_currentStateIdx + 1; i <= history->_mostRecentStateIdx; i++) {
        FreeEditorState(&history->_states[i]);
    }

    history->_currentStateIdx++;
    history->_mostRecentStateIdx = history->_currentStateIdx;
    
    if (history->_currentStateIdx >= history->_statesLength) {
        // bool array pointers are copied over so it works.
        history->_states = realloc(history->_states, sizeof(EditorState) * (history->_statesLength + HISTORY_BUFFER_SIZE_INCREMENT)); 
        history->_statesLength += HISTORY_BUFFER_SIZE_INCREMENT;
    }

    history->_states[history->_currentStateIdx] = EditorStateDeepCopy(state);
}

/// @brief
/// @param history 
/// @param state Replaced with the state changed to. All cleanup is done automatically. 
/// @param option 
void ChangeState(EditorHistory *history, EditorState *state, ChangeOptions option) {
    if (option == UNDO){
        if (history->_currentStateIdx <= 0) return;
        history->_currentStateIdx--;
    } else {
        if (history->_currentStateIdx >= history->_mostRecentStateIdx) return;
        history->_currentStateIdx++;
    }

    EditorState oldState = *state;
    *state = EditorStateDeepCopy(&history->_states[history->_currentStateIdx]);
    FreeEditorState(&oldState);
}



cJSON *SerializeState(EditorState state) {
    cJSON *shapes = cJSON_CreateArray();

    for (int i = 0; i < state.shapeCount; i++) { // should not enter if state.shapes is null cause count is also 0
        cJSON *shape = SerializeShape(state.shapes[i]);
        if (!shape) {
            cJSON_Delete(shapes);
            return NULL;
        }
        cJSON_AddItemToArray(shapes, shape);
    }

    cJSON *activeFrames = cJSON_CreateArray();
    int activeCount = state.frameCount * state.shapeCount;
    for (int i = 0; i < activeCount; i++) {
        cJSON *val = cJSON_CreateNumber(state._shapeActiveFrames[i]);
        cJSON_AddItemToArray(activeFrames, val);
    }

    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json, STR_SHAPE_ACTIVE_FRAMES, activeFrames);
    cJSON_AddItemToObject(json, STR_SHAPES, shapes);
    cJSON_AddNumberToObject(json, STR_FRAME_COUNT, state.frameCount);
    return json;
}

bool DeserializeState(cJSON *json, EditorState *state) {
    if (!cJSON_IsObject(json)) return false;

    cJSON *frameCountJson = cJSON_GetObjectItem(json, STR_FRAME_COUNT);
    if (!frameCountJson || !cJSON_IsNumber(frameCountJson)) return false;
    int frameCount = (int) cJSON_GetNumberValue(frameCountJson);

    cJSON *shapes = cJSON_GetObjectItem(json, STR_SHAPES);
    if (!shapes || !cJSON_IsArray(shapes)) return false;

    cJSON *activeFrames = cJSON_GetObjectItem(json, STR_SHAPE_ACTIVE_FRAMES);
    if (!activeFrames || !cJSON_IsArray(activeFrames)) return false;

    *state = AllocEditorState(frameCount);
    cJSON *jsonShape;
    cJSON_ArrayForEach(jsonShape, shapes) {
        CombatShape shape;
        if (!DeserializeShape(jsonShape, &shape)) {
            FreeEditorState(state);
            return false;
        }
        AddShape(state, shape);
    }

    int activeCount = state->shapeCount * state->frameCount; // frameCount will always be >= 1, if shapeCount = 0 then shapes is null
    cJSON *active;
    int activeIdx = 0;
    cJSON_ArrayForEach(active, activeFrames) {
        if (!cJSON_IsNumber(active) || activeIdx >= activeCount) {
            FreeEditorState(state);
            return false;
        }
        state->_shapeActiveFrames[activeIdx] = cJSON_GetNumberValue(active) == 0 ? false : true;
        activeIdx++;
    }

    return true;
}