#include "editor_history.h"

typedef enum EditingField {
    FIELD_FRAME_DURATION,
    FIELD_LAYER_NAME,
    FIELD_HITBOX_DAMAGE,
    FIELD_HITBOX_STUN
} EditingField;

typedef enum EditorMode { // @todo: rename this or the other editor state.
    EDITOR_MODE_IDLE,
    EDITOR_MODE_PLAYING,
    EDITOR_MODE_EDITING,
    EDITOR_MODE_DRAGGING_HANDLE,
    EDITOR_MODE_DRAGGING_FRAME_POS,
    EDITOR_MODE_PANNING_SPRITE
} EditorMode;

typedef enum EditorChange {
    EDITOR_REDO,
    EDITOR_UNDO
} EditorChange;

typedef struct Editor {
    EditorMode mode;
    int playingFrameMilliseconds;
    EditingField editingField;
    Handle draggingHandle;
    
    LIST(EditorState) states;
    int stateIdxCurrent;
    int stateIdxMostRecent;
} Editor;

Editor EditorNew(EditorState *initial);
void EditorFree(Editor *editor);
void EditorSetMode(Editor *editor, EditorMode mode);
void EditorCommitState(Editor *editor, EditorState *state);
void EditorChangeState(Editor *editor, EditorChange change);
