#include <stdlib.h>
#include <stdio.h>
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

void FreeEditorState(EditorState state) {
    free(state.shapes);
    free(state._shapeActiveFrames);
}

bool GetShapeActive(EditorState *state, int frameIdx, int shapeIdx) {
    return state->_shapeActiveFrames[state->frameCount * shapeIdx + frameIdx];
}

void SetShapeActive(EditorState *state, int frameIdx, int shapeIdx, bool active) {
    state->_shapeActiveFrames[state->frameCount * shapeIdx + frameIdx] = active;
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

bool DeserializeState(cJSON *json, EditorState *out) {
    if (!cJSON_IsObject(json)) return false;

    cJSON *frameCount = cJSON_GetObjectItem(json, STR_FRAME_COUNT);
    if (!frameCount || !cJSON_IsNumber(frameCount)) return false;
    out->frameCount = (int) cJSON_GetNumberValue(frameCount);


    cJSON *shapes = cJSON_GetObjectItem(json, STR_SHAPES);
    if (!shapes || !cJSON_IsArray(shapes)) return false;
    out->shapeCount = cJSON_GetArraySize(shapes); // o(n), but that's ok, should never be that long
    if (out->shapeCount > 0) {
        out->shapes = malloc(sizeof(CombatShape) * out->shapeCount);
        cJSON *jsonShape;
        int shapeIdx = 0;
        cJSON_ArrayForEach(jsonShape, shapes) {
            CombatShape shape;
            if (!DeserializeShape(jsonShape, &shape)) {
                free(out->shapes);
                return false;
            }
            out->shapes[shapeIdx] = shape;
            shapeIdx++;
        }
    }

    cJSON *activeFrames = cJSON_GetObjectItem(json, STR_SHAPE_ACTIVE_FRAMES);
    if (!activeFrames || !cJSON_IsArray(activeFrames)) return false;
    int activeCount = out->shapeCount * out->frameCount; // frameCount will always be >= 1, if shapeCount = 0 then shapes is null
    if (activeCount > 0) {
        cJSON *active;
        out->_shapeActiveFrames = malloc(sizeof(bool) * activeCount);
        int activeIdx = 0;
        cJSON_ArrayForEach(active, activeFrames) {
            if (!cJSON_IsNumber(active) || activeIdx >= activeCount) {
                return false;
            }
            out->_shapeActiveFrames[activeIdx] = cJSON_GetNumberValue(active) == 0 ? false : true;
            activeIdx++;
        }
    }

    out->frameIdx = 0;
    out->shapeIdx = -1;
    return true;
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

EditorState EditorStateDeepCopy(EditorState state) {
    int activeFramesSize = sizeof(bool) * state.shapeCount * state.frameCount;
    bool *activeFramesCopy = malloc(activeFramesSize);
    memcpy(activeFramesCopy, state._shapeActiveFrames, activeFramesSize);

    int shapesSize = sizeof(CombatShape) * state.shapeCount;
    CombatShape *shapesCopy = malloc(shapesSize);
    memcpy(shapesCopy, state.shapes, shapesSize);
    
    EditorState newState;
    newState.shapeCount = state.shapeCount;
    newState.frameCount = state.frameCount;
    newState.shapes = shapesCopy;
    newState._shapeActiveFrames = activeFramesCopy;
    newState.frameIdx = state.frameIdx;
    newState.shapeIdx = state.shapeIdx;
    return newState;
}

EditorHistory AllocEditorHistory(EditorState *initial) {
    EditorHistory history;
    history._states = malloc(sizeof(EditorState) * HISTORY_BUFFER_SIZE_INCREMENT);
    history._statesLength = HISTORY_BUFFER_SIZE_INCREMENT;
    history._currentStateIdx = 0;
    history._mostRecentStateIdx = 0;
    
    history._states[0] = EditorStateDeepCopy(*initial);
    return history;
}

void FreeEditorHistory(EditorHistory history) {
    for (int i = 0; i < history._mostRecentStateIdx; i++) {
        FreeEditorState(history._states[i]);
    }
    free(history._states);
}

void CommitState(EditorHistory *history, EditorState *state) {
    for (int i = history->_currentStateIdx + 1; i <= history->_mostRecentStateIdx; i++) {
        FreeEditorState(history->_states[i]);
    }

    history->_currentStateIdx++;
    history->_mostRecentStateIdx = history->_currentStateIdx;
    
    if (history->_currentStateIdx >= history->_statesLength) {
        // bool array pointers are copied over so it works.
        history->_states = realloc(history->_states, sizeof(EditorState) * (history->_statesLength + HISTORY_BUFFER_SIZE_INCREMENT)); 
        history->_statesLength += HISTORY_BUFFER_SIZE_INCREMENT;
    }

    history->_states[history->_currentStateIdx] = EditorStateDeepCopy(*state);
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
    *state = EditorStateDeepCopy(history->_states[history->_currentStateIdx]);
    FreeEditorState(oldState);
}