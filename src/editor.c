#include "editor_history.h"
#include "editor.h"

Editor EditorNew(EditorState *initial) {
    LIST(EditorState) states = LIST_NEW_SIZED(EditorState, 1);
    states[0] = EditorStateDeepCopy(initial);
    
    return (Editor) {
        .mode = EDITOR_MODE_IDLE,
        .states = states,
        .stateIdxCurrent = 0,
        .stateIdxMostRecent = 0
    };
}

void EditorFree(Editor *editor) {
    int count = LIST_COUNT(editor->states);
    for (int i = 0; i < editor->stateIdxMostRecent; i++) EditorStateFree(editor.states + i);
    LIST_FREE(editor->states);
}

void EditorSetMode(Editor *editor, EditorMode mode) {
    switch (mode) {
        case EDITOR_MODE_PLAYING:
            editor->playingFrameMilliseconds = 0;
            break;
    }

    editor->mode = mode;
}

void EditorCommitState(Editor *editor, EditorState *state) {
    for (int i = editor->stateIdxCurrent + 1; i < editor->stateIdxMostRecent; i++) {
        EditorStateFree(editor->states + i);
    }

    editor->stateIdxCurrent++;
    editor->stateIdxMostRecent = editor->stateIdxCurrent;
    
    if (editor->stateIdxCurrent >= editor->statesLength) {
        LIST_ADD(&editor->states, EditorStateDeepCopy(state));
    } else {
        editor->states[editor->stateIdxCurrent] = EditorStateDeepCopy(editor->state);
    }
}

void EditorChangeState(Editor *editor, EditorState *state, EditorChange change) {
    switch (change) {
        case EDITOR_UNDO:
            if (editor->_stateIdxCurrent <= 0) return;
            editor->_stateIdxCurrent--;
            break;
        case EDITOR_REDO:
            if (editor->_stateIdxCurrent >= editor->_stateIdxMostRecent) return;
            editor->stateIdxCurrent++;
            break;
        default: return;
    }

    EditorState oldState = *state;
    *state = EditorStateDeepCopy(&editor->_states[editor->_stateIdxCurrent]);
    EditorStateFree(&oldState);
}
