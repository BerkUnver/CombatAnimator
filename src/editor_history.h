#ifndef EDITOR_HISTORY_H
#define EDITOR_HISTORY_H

#include <stdlib.h>
#include "cjson/cJSON.h"
#include "layer.h"

#define HISTORY_BUFFER_SIZE_INCREMENT 1024
#define FRAME_DURATION_UNIT_PER_SECOND 1000.0f
#define FRAME_INFO_POS_DEFAULT (Vector2) {.x = 80.0f, .y = 64.0f}
#define FRAME_INFO_DEFAULT (FrameInfo) {.pos = FRAME_INFO_POS_DEFAULT, .duration = 100, .canCancel = false}

typedef struct FrameInfo {
    int duration;
    bool canCancel;
    Vector2 pos;
} FrameInfo;

typedef struct EditorState {
    Layer *layers;
    int layerCount;
    bool *_layerActiveFrames;
    FrameInfo *frames;
    int frameCount;
    int layerIdx;
    int frameIdx;
} EditorState;

typedef struct EditorHistory {
    EditorState *_states;
    int _currentStateIdx;
    int _mostRecentStateIdx;
    int _statesLength;
} EditorHistory;

typedef enum ChangeOptions {
    CHANGE_UNDO,
    CHANGE_REDO
} ChangeOptions;

EditorState EditorStateNew(int frameCount);
void EditorStateFree(EditorState *state);
bool EditorStateLayerActiveGet(EditorState *state, int frameIdx, int layerIdx);
void EditorStateLayerActiveSet(EditorState *state, int frameIdx, int layerIdx, bool enabled);


void EditorStateLayerAdd(EditorState *state, Layer layer);
bool EditorStateLayerRemove(EditorState *state, int idx);
void EditorStateAddFrame(EditorState *state);
bool EditorStateRemoveFrame(EditorState *state, int idx);
EditorState EditorStateDeepCopy(EditorState *state);

bool EditorStateSerialize(EditorState *state, const char *path);
bool EditorStateDeserialize(EditorState *state, const char *path);


EditorHistory EditorHistoryNew(EditorState *initial);
void EditorHistoryFree(EditorHistory *history);

void EditorHistoryCommitState(EditorHistory *history, EditorState *state);
void EditorHistoryChangeState(EditorHistory *history, EditorState *state, ChangeOptions option);

#endif
