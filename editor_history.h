#ifndef EDITOR_HISTORY
#define EDITOR_HISTORY

#include <stdlib.h>
#include "hitbox.h"

#define HISTORY_BUFFER_SIZE_INCREMENT 1024

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


EditorHistory *AllocEditorHistory(Hitbox hitbox, int frameCount);
void FreeEditorHistory(EditorHistory *history);
EditorState *GetState(EditorHistory *history);
void CommitState(EditorHistory *history);
void UndoState(EditorHistory *history);
void RedoState(EditorHistory *history);

#endif