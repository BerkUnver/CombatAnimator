#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "editor_history.h"
#include "hitbox.h"

EditorState EditorStateDeepCopy(EditorState state) {
    int activeFramesSize = sizeof(bool) * state.frameCount;
    bool *activeFramesCopy = malloc(activeFramesSize);
    memcpy(activeFramesCopy, state.hitboxActiveFrames, activeFramesSize);
    return (EditorState) {state.hitbox, activeFramesCopy, state.frameCount};
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
        free(history._states[i].hitboxActiveFrames);
    }
    free(history._states);
}

void CommitState(EditorHistory *history, EditorState *state) {
    for (int i = history->_currentStateIdx + 1; i <= history->_mostRecentStateIdx; i++) {
        free(history->_states[i].hitboxActiveFrames);
    }

    history->_currentStateIdx++;
    history->_mostRecentStateIdx = history->_currentStateIdx;
    
    if (history->_currentStateIdx >= history->_statesLength) {
        // bool array pointers are copied over so it works.
        history->_states = realloc(history->_states, sizeof(EditorState) * (history->_statesLength + HISTORY_BUFFER_SIZE_INCREMENT)); 
        history->_statesLength += HISTORY_BUFFER_SIZE_INCREMENT;
    }

    // this is a near duplicate of the origin
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
    free(oldState.hitboxActiveFrames);
}