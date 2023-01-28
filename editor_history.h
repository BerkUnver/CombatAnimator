#ifndef EDITOR_HISTORY
#define EDITOR_HISTORY

#include <stdlib.h>
#include "cjson/cJSON.h"
#include "combat_shape.h"

#define HISTORY_BUFFER_SIZE_INCREMENT 1024

typedef struct EditorState {
    CombatShape *shapes;
    int shapeCount;
    bool *_shapeActiveFrames;

    int frameCount;
    int shapeIdx;
    int frameIdx;
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

EditorState AllocEditorState(int frames);
void FreeEditorState(EditorState state);
cJSON *SerializeState(EditorState state);

bool GetShapeActive(EditorState *state, int frameIdx, int shapeIdx);
void SetShapeActive(EditorState *state, int frameIdx, int shapeIdx, bool enabled);
void AddShape(EditorState *state, CombatShape shape);
EditorState EditorStateDeepCopy(EditorState state);

EditorHistory AllocEditorHistory(EditorState *initial);
void FreeEditorHistory(EditorHistory history);

void CommitState(EditorHistory *history, EditorState *state);
void ChangeState(EditorHistory *history, EditorState *state, ChangeOptions option);

#endif