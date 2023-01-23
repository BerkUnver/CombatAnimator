#include <stdlib.h>
#include <string.h>
#include "editor_history.h"
#include "hitbox.h"

EditorHistory *AllocEditorHistory(Hitbox hitbox, int frameCount) {
    if (frameCount < 0) return NULL;
    EditorHistory *history = malloc(sizeof(EditorHistory));
    history->_states = malloc(sizeof(EditorState) * HISTORY_BUFFER_SIZE_INCREMENT);
    history->_statesLength = HISTORY_BUFFER_SIZE_INCREMENT;
    history->_currentStateIdx = 0;
    history->_mostRecentStateIdx = 0;
    history->_states[0] = (EditorState) {hitbox, calloc(frameCount, sizeof(bool)), frameCount};
    return history;
}

void FreeEditorHistory(EditorHistory *history) {
    for (int i = 0; i < history->_mostRecentStateIdx; i++) {
        free(history->_states[i].hitboxActiveFrames);
    }
    free(history->_states);
    free(history);
}

/// @brief 
/// @param history 
/// @return Please do not do anything with this pointer after you call another function on history
EditorState *GetState(EditorHistory *history) {
    return &history->_states[history->_currentStateIdx];
}

void CommitState(EditorHistory *history) {
    EditorState oldState = history->_states[history->_currentStateIdx];

    // release byte arrays of frames
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

    bool *activeFramesCopy = malloc(sizeof(bool) * oldState.frameCount);
    memcpy(activeFramesCopy, oldState.hitboxActiveFrames, sizeof(bool) * oldState.frameCount);
    history->_states[history->_currentStateIdx] = (EditorState) {oldState.hitbox, activeFramesCopy, oldState.frameCount};
}

void UndoState(EditorHistory *history) {
    if (history->_currentStateIdx > 0) {
        history->_currentStateIdx--;
    }
}

void RedoState(EditorHistory *history) {
    if (history->_currentStateIdx < history->_mostRecentStateIdx) {
        history->_currentStateIdx++;
    }
}