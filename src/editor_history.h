#ifndef EDITOR_HISTORY_H
#define EDITOR_HISTORY_H

#include <stdlib.h>
#include "cJSON.h"
#include "layer.h"
#include "list.h"

#define HISTORY_BUFFER_SIZE_INCREMENT 1024
#define FRAME_DURATION_UNIT_PER_SECOND 1000.0f


// no version: initial version. Treated as 0 internally.
// 1: changed frameDurations array to frames array. Each frame is a struct containing whether it is cancellable and its duration in ms.
// 2: Added position Vector2 for each frame that indicates the root movement of the character doing the animation.
// 3: Added hitbox stun and hitbox damage to CombatShape.
// 4: Very large rework.
//      Added metadata layer type.
//      Stopped inlining shape properties into each layer. Now shapes are a separate struct.
//      Renamed "shapes" to "layers" and "shapeActiveFrames" to "layerActiveFrames".
//      Renamed "boxType" to "type" in layer.
// 5: Intermediary development step. (Gonna be real I don't remember what this was)
// 6: Moved the array of which layers are active from the saved object into each individual layer.
//      Non-breaking revision: Added bezier curve layer type.
//      Right now we are just serializing the data for frames where the layer is active as null.
//      This introduces a bunch of UB and should be improved. Because it is localized to this feature it should be okay for now.
// 7: Every layer type now has a name.
//      Metadata layers have been removed in favor of an "empty" layer. This is functionally equivalent to the metadata layer.
//      Names have been autogenerated for other types of layers depending on their index. ("Layer 0, Layer 1, etc.")
//      Renamed bezier curve positions from "positionX" and "postitionY" to "x" and "y" to be more consistent.
// 8: Renamed the hurtbox layer to the shape layer.
//      It can be used for different kinds shapes now (i.e. hurtboxes, windboxes, grab boxes, etc.)
//      It has a flags field that can be set and then interpreted by whatever is playing the animation to get these effects.

// Oldest supported version of the file format
#define FILE_VERSION_OLDEST 7
// Most recent file version
#define FILE_VERSION_CURRENT 8

typedef struct FrameInfo {
    int duration;
    bool canCancel;
    Vector2 pos;
} FrameInfo;

typedef struct EditorState {
    Layer *layers;
    int layerCount;
    FrameInfo *frames;
    int frameCount;
    int layerIdx;
    int frameIdx;
} EditorState;

typedef struct EditorHistory {
    LIST(EditorState) _states;
    int _currentStateIdx;
    int _mostRecentStateIdx;
} EditorHistory;

typedef enum ChangeOptions {
    CHANGE_UNDO,
    CHANGE_REDO
} ChangeOptions;

EditorState EditorStateNew(int frameCount);
void EditorStateFree(EditorState *state);

void EditorStateLayerAdd(EditorState *state, Layer layer);
bool EditorStateLayerRemove(EditorState *state, int idx);
void EditorStateAddFrame(EditorState *state, int idx);
bool EditorStateRemoveFrame(EditorState *state, int idx);
EditorState EditorStateDeepCopy(EditorState *state);

bool EditorStateSerialize(EditorState *state, const char *path);
bool EditorStateDeserialize(EditorState *state, const char *path);


EditorHistory EditorHistoryNew(EditorState *initial);
void EditorHistoryFree(EditorHistory *history);

void EditorHistoryCommitState(EditorHistory *history, EditorState *state);
void EditorHistoryChangeState(EditorHistory *history, EditorState *state, ChangeOptions option);

#endif
