#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "editor_history.h"
#include "hitbox.h"

EditorState AllocEditorState(int frames) {
    EditorState state;
    state.frameCount = frames;
    state.hitboxCount = 0;
    state.hitboxes = NULL;
    state._hitboxActiveFrames = NULL;
    state.frameIdx = 0;
    state.hitboxIdx = -1;
    return state;
}

void FreeEditorState(EditorState state) {
    free(state.hitboxes);
    free(state._hitboxActiveFrames);
}

bool GetHitboxActive(EditorState *state, int frameIdx, int hitboxIdx) {
    return state->_hitboxActiveFrames[state->frameCount * hitboxIdx + frameIdx];
}

void SetHitboxActive(EditorState *state, int frameIdx, int hitboxIdx, bool active) {
    state->_hitboxActiveFrames[state->frameCount * hitboxIdx + frameIdx] = active;
}

void AddHitbox(EditorState *state, Hitbox hitbox) { // Should work on nullptrs
    int oldMax = state->hitboxCount * state->frameCount;
    state->hitboxCount++;
    state->hitboxes = realloc(state->hitboxes, sizeof(Hitbox) * state->hitboxCount);
    state->hitboxes[state->hitboxCount - 1] = hitbox;
    state->_hitboxActiveFrames = realloc(state->_hitboxActiveFrames, sizeof(bool) * state->hitboxCount * state->frameCount);
    int newMax = state->hitboxCount * state->frameCount;
    for (int i = oldMax; i < newMax; i++) { // init all new hitboxes to false;
        state->_hitboxActiveFrames[i] = false;
    }
}

EditorState EditorStateDeepCopy(EditorState state) {
    int activeFramesSize = sizeof(bool) * state.hitboxCount * state.frameCount;
    bool *activeFramesCopy = malloc(activeFramesSize);
    memcpy(activeFramesCopy, state._hitboxActiveFrames, activeFramesSize);

    int hitboxesSize = sizeof(Hitbox) * state.hitboxCount;
    Hitbox *hitboxesCopy = malloc(hitboxesSize);
    memcpy(hitboxesCopy, state.hitboxes, hitboxesSize);
    
    EditorState newState;
    newState.hitboxCount = state.hitboxCount;
    newState.frameCount = state.frameCount;
    newState.hitboxes = hitboxesCopy;
    newState._hitboxActiveFrames = activeFramesCopy;
    newState.frameIdx = state.frameIdx;
    newState.hitboxIdx = state.hitboxIdx;
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