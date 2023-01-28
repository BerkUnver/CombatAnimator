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
    for (int i = 0; i < state.shapeCount; i++) {
        CombatShape shape = state.shapes[i];
        const char *shapeType;
        const char *boxType;
        switch(shape.boxType) {
            case HITBOX: boxType = "HITBOX";
            case HURTBOX: boxType = "HURTBOX";
            default: return NULL;
        }
        
        cJSON *data = cJSON_CreateObject();
        switch (shape.shapeType) {
            case CIRCLE: 
                shapeType = "CIRCLE";
                cJSON_AddItemToObject(data, "radius", shape.data.circleRadius);
                break;
            case RECTANGLE: 
                shapeType = "RECTANGLE";
                cJSON_AddToObject(data, "rightX", shape.data.rectangle.rightX);
                cJSON_AddItemToObject(data, "bottomY", shape.data.rectangle.bottomY);
                break;
            case CAPSULE: 
                shapeType = "CAPSULE"; 
                cJSON_AddItemToObject(data, "radius", shape.data.capsule.radius);
                break;
            default: return NULL;
        }

        cJSON *jsonShape = cJSON_CreateObject();
        cJSON_AddItemToObject(jsonShape,"shapeType", cJSON_CreateString(shapeType));
        cJSON_AddItemToObject(jsonShape, "boxType", cJSON_CreateString(boxType));
    }
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