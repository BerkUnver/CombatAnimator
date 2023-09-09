#include "editor_history.h"

typedef enum EditingField {
    FIELD_NONE,
    FIELD_FRAME_DURATION,
    FIELD_LAYER_NAME,
    FIELD_HITBOX_DAMAGE,
    FIELD_HITBOX_STUN
} EditingField;

typedef enum EditorMode { // @todo: rename this or the other editor state.
    EDITOR_MODE_PLAYING,
    EDITOR_MODE_EDITING,
    EDITOR_MODE_DRAGGING_HANDLE,
    EDITOR_MODE_DRAGGING_FRAME_POS,
    EDITOR_MODE_PANNING_SPRITE
} EditorMode;

typedef struct Editor {
    EditorMode mode;
    float playingDelta;
    EditingField editingField;

    EditorState state;
    LIST(EditorState) states;
    int stateIdxCurrent;
    int stateIdxMostRecent;
} Editor;

Editor EditorNew(EditorState initial) {
    LIST(EditorState) states = LIST_NEW_SIZED(EditorState, 1);
    states[0] = EditorStateDeepCopy(initial);
    
    return (Editor) {
        .mode = EDITOR_MODE_EDITING,
        .editingField = FIELD_NONE,
        .state = initial,
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
    switch (editor->mode) {
        case EDITOR_MODE_DRAGGING_HANDLE:
        case EDITOR_MODE_DRAGGING_FRAME_POS:    
            EditorCommitState(editor);
            break;
    }

    switch (mode) {
        case EDITOR_MODE_EDITING:
            editor->editingField = FIELD_NONE;
            break;
    }

    editor->mode = mode;
}

void EditorCommitState(Editor *editor) {
    for (int i = editor->stateIdxCurrent + 1; i < editor->stateIdxMostRecent; i++) {
        EditorStateFree(editor->states + i);
    }

    editor->stateIdxCurrent++;
    editor->stateIdxMostRecent = editor->stateIdxCurrent;
    
    if (editor->stateIdxCurrent >= editor->statesLength) {
        LIST_ADD(&editor->states, EditorStateDeepCopy(editor->state));
    } else {
        editor->states[editor->stateIdxCurrent] = EditorStateDeepCopy(editor->state);
    }
}
