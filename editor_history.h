#ifndef EDITOR_HISTORY
#define EDITOR_HISTORY

#include <stdlib.h>
#include "cjson/cJSON.h"
#include "combat_shape.h"

#define HISTORY_BUFFER_SIZE_INCREMENT 1024
#define FRAME_DURATION_UNIT_PER_SECOND 1000.0f
#define FRAME_INFO_POS_DEFAULT (Vector2) {.x = 80.0f, .y = 64.0f}
#define FRAME_INFO_DEFAULT (FrameInfo) {.pos = FRAME_INFO_POS_DEFAULT, .duration = 100, .canCancel = false, .velocity = (Vector2) {0, 0}}
#define STR_SHAPES "shapes"
#define STR_FRAMES "frames"
#define STR_FRAME_DURATIONS "frameDurations"
#define STR_FRAME_INFO_DURATION "duration"
#define STR_FRAME_INFO_CAN_CANCEL "canCancel"
#define STR_FRAME_INFO_X "x"
#define STR_FRAME_INFO_Y "y"
#define STR_FRAME_INFO_VELOCITY_X "velocityX"
#define STR_FRAME_INFO_VELOCITY_Y "velocityY"
#define STR_SHAPE_ACTIVE_FRAMES "shapeActiveFrames"
#define STR_MAGIC "magic"
#define STR_MAGIC_VALUE "CombatAnimator"
#define STR_VERSION "version"

// no version: initial version. Treated as 0 internally.
// 1: changed frameDurations array to frames array. Each frame is a struct containing whether or not it is cancellable and its duration in ms.
// 2: Added position Vector2 for each frame that indicates the root movement of the character doing the animation.
// 3: Added hitbox stun and hitbox damage to CombatShape. Added frame velocity in a later version, but this is optional so I will not increment the version number.
#define VERSION_NUMBER 3

typedef struct FrameInfo {
    int duration;
    bool canCancel;
    Vector2 pos;
    Vector2 velocity;
} FrameInfo;

typedef struct EditorState {
    CombatShape *shapes;
    int shapeCount;
    bool *_shapeActiveFrames;
    FrameInfo *frames;
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

EditorState EditorStateNew(int frameCount);
void EditorStateFree(EditorState *state);
bool EditorStateShapeActiveGet(EditorState *state, int frameIdx, int shapeIdx);
void EditorStateShapeActiveSet(EditorState *state, int frameIdx, int shapeIdx, bool enabled);


void EditorStateAddShape(EditorState *state, CombatShape shape);
bool EditorStateRemoveShape(EditorState *state, int idx);
void EditorStateAddFrame(EditorState *state);
bool EditorStateRemoveFrame(EditorState *state, int idx);
EditorState EditorStateDeepCopy(EditorState *state);
cJSON *EditorStateSerialize(EditorState *state);
bool EditorStateDeserialize(cJSON *json, EditorState *state);
bool EditorStateReadFromFile(EditorState *state, const char *path);
bool EditorStateWriteToFile(EditorState *state, const char *path);

EditorHistory EditorHistoryNew(EditorState *initial);
void EditorHistoryFree(EditorHistory *history);

void EditorHistoryCommitState(EditorHistory *history, EditorState *state);
void EditorHistoryChangeState(EditorHistory *history, EditorState *state, ChangeOptions option);

#endif
