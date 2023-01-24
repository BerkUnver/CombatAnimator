#ifndef EDITOR_HISTORY
#define EDITOR_HISTORY

#include <stdlib.h>
#include "hitbox.h"

#define HISTORY_BUFFER_SIZE_INCREMENT 1024

/// @brief THIS CONTAINS A MALLOC, DO NOT DUPLICATE
typedef struct EditorState {
    Hitbox hitbox;
    bool *hitboxActiveFrames;
    int frameCount;
} EditorState;


typedef struct EditorHistory {
    EditorState *_states;
    int _currentStateIdx;
    int _mostRecentStateIdx;
    int _statesLength;
} EditorHistory;

typedef enum ChangeOptions {
    UNDO,
    REDO
} ChangeOptions;

EditorState EditorStateDeepCopy(EditorState state);

EditorHistory AllocEditorHistory(EditorState *initial);
void FreeEditorHistory(EditorHistory history);

void CommitState(EditorHistory *history, EditorState *state);
void ChangeState(EditorHistory *history, EditorState *state, ChangeOptions option);

#endif