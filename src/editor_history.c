#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "cjson/cJSON.h"
#include "layer.h"
#include "string_buffer.h"
#include "editor_history.h"

EditorState EditorStateNew(int frameCount) {
    FrameInfo *frames = malloc(sizeof(FrameInfo) * frameCount);
    for (int i = 0; i < frameCount; i++) frames[i] = FRAME_INFO_DEFAULT;

    return (EditorState) {
        .layerCount = 0,
        .layers = NULL,
        ._layerActiveFrames = NULL,
        .frames = frames,
        .frameCount = frameCount,
        .frameIdx = 0,
        .layerIdx = -1
    };
}

void EditorStateFree(EditorState *state) {
    free(state->layers);
    free(state->_layerActiveFrames);
    free(state->frames);
}

bool EditorStateLayerActiveGet(EditorState *state, int frameIdx, int layerIdx) {
    return state->_layerActiveFrames[state->frameCount * layerIdx + frameIdx];
}

void EditorStateLayerActiveSet(EditorState *state, int frameIdx, int layerIdx, bool enabled) {
    state->_layerActiveFrames[state->frameCount * layerIdx + frameIdx] = enabled;
}

void EditorStateLayerAdd(EditorState *state, Layer layer) {
    int oldMax = state->layerCount * state->frameCount;
    state->layerCount++;
    state->layers = realloc(state->layers, sizeof(Layer) * state->layerCount);
    state->layers[state->layerCount - 1] = layer;
    int newMax = state->layerCount * state->frameCount;
    state->_layerActiveFrames = realloc(state->_layerActiveFrames, sizeof(bool) * newMax);
    memset(state->_layerActiveFrames + oldMax, false, (newMax - oldMax) * sizeof(bool));
}

bool EditorStateLayerRemove(EditorState *state, int idx) {
    if (idx < 0 || idx >= state->layerCount) return false;
    
    for (int layerIdx = idx + 1; layerIdx < state->layerCount; layerIdx++) {
        int newLayerIdx = layerIdx - 1;
        state->layers[newLayerIdx] = state->layers[layerIdx];

        for (int frameIdx = 0; frameIdx < state->frameCount; frameIdx++) {
            state->_layerActiveFrames[state->frameCount * newLayerIdx + frameIdx] = state->_layerActiveFrames[state->frameCount * layerIdx + frameIdx];
        }
    }
    state->layerCount--;
    if (state->layerIdx >= state->layerCount) state->layerIdx = state->layerCount - 1;
    return true;
}

void EditorStateAddFrame(EditorState *state) {
    state->frameCount++; 
    state->frames = realloc(state->frames, sizeof(FrameInfo) * state->frameCount);
    state->frames[state->frameCount - 1] = state->frames[state->frameCount - 2];
    state->_layerActiveFrames = realloc(state->_layerActiveFrames, sizeof(bool) * state->frameCount * state->layerCount);
    for (int chunkIdx = state->layerCount - 1; chunkIdx >= 0; chunkIdx--) {
        state->_layerActiveFrames[chunkIdx * state->frameCount + state->frameCount - 1] = false;
        for (int i = state->frameCount - 2; i >= 0; i--) {
            state->_layerActiveFrames[chunkIdx * state->frameCount + i] = state->_layerActiveFrames[chunkIdx * state->frameCount + i - chunkIdx];
        }
    }
}

bool EditorStateRemoveFrame(EditorState *state, int idx) {
    if (idx < 0 || idx >= state->frameCount || state->frameCount == 1) return false;
    int activeLen = state->frameCount * state->layerCount;
    for (int i = 0; i < activeLen; i++) {
        int shift = (i + state->frameCount - 1 - idx) / state->frameCount; // get the index to move to
        state->_layerActiveFrames[i - shift] = state->_layerActiveFrames[i];
    }
    state->frameCount--;
    return true;
}

EditorState EditorStateDeepCopy(EditorState *state) {
    int activeFramesSize = sizeof(bool) * state->layerCount * state->frameCount;
    bool *activeFramesCopy = malloc(activeFramesSize);
    memcpy(activeFramesCopy, state->_layerActiveFrames, activeFramesSize);

    int layersSize = sizeof(Layer) * state->layerCount;
    Layer *layersCopy = malloc(layersSize);
    memcpy(layersCopy, state->layers, layersSize);

    int framesSize = sizeof(FrameInfo) * state->frameCount;
    FrameInfo *framesCopy = malloc(framesSize);
    memcpy(framesCopy, state->frames, framesSize);

    return (EditorState) {
        .layerCount = state->layerCount,
        .frameCount = state->frameCount,
        .layers = layersCopy,
        ._layerActiveFrames = activeFramesCopy,
        .frames = framesCopy,
        .frameIdx = state->frameIdx,

        .layerIdx = state->layerIdx
    };
}

/// clones everything passed in, is safe.
EditorHistory EditorHistoryNew(EditorState *initial) {
    EditorHistory history;
    history._states = malloc(sizeof(EditorState) * HISTORY_BUFFER_SIZE_INCREMENT);
    history._states[0] = EditorStateDeepCopy(initial);
    history._statesLength = HISTORY_BUFFER_SIZE_INCREMENT;
    history._currentStateIdx = 0;
    history._mostRecentStateIdx = 0;
    return history;
}

void EditorHistoryFree(EditorHistory *history) {
    for (int i = 0; i <= history->_mostRecentStateIdx; i++) {
        EditorStateFree(&history->_states[i]);
    }
    free(history->_states);
}

void EditorHistoryCommitState(EditorHistory *history, EditorState *state) {
    for (int i = history->_currentStateIdx + 1; i <= history->_mostRecentStateIdx; i++) {
        EditorStateFree(&history->_states[i]);
    }

    history->_currentStateIdx++;
    history->_mostRecentStateIdx = history->_currentStateIdx;
    
    if (history->_currentStateIdx >= history->_statesLength) {
        // array pointers are copied over so it works.
        history->_states = realloc(history->_states, sizeof(EditorState) * (history->_statesLength + HISTORY_BUFFER_SIZE_INCREMENT)); 
        history->_statesLength += HISTORY_BUFFER_SIZE_INCREMENT;
    }

    history->_states[history->_currentStateIdx] = EditorStateDeepCopy(state);
}

/// @brief
/// @param history 
/// @param state Replaced with the state changed to. All cleanup is done automatically. 
/// @param option 
void EditorHistoryChangeState(EditorHistory *history, EditorState *state, ChangeOptions option) {
    if (option == CHANGE_UNDO){
        if (history->_currentStateIdx <= 0) return;
        history->_currentStateIdx--;
    } else {
        if (history->_currentStateIdx >= history->_mostRecentStateIdx) return;
        history->_currentStateIdx++;
    }

    EditorState oldState = *state;
    *state = EditorStateDeepCopy(&history->_states[history->_currentStateIdx]);
    EditorStateFree(&oldState);
}

bool EditorStateSerialize(EditorState *state, const char *path) {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "magic", "CombatAnimator");

    // no version: initial version. Treated as 0 internally.
    // 1: changed frameDurations array to frames array. Each frame is a struct containing whether it is cancellable and its duration in ms.
    // 2: Added position Vector2 for each frame that indicates the root movement of the character doing the animation.
    // 3: Added hitbox stun and hitbox damage to CombatShape.
    // 4: Very large rework.
    //      Added metadata layer type.
    //      Stopped inlining shape properties into each layer. Now shapes are a separate struct.
    //      Renamed "shapes" to "layers" and "shapeActiveFrames" to "layerActiveFrames".
    //      Renamed "boxType" to "type" in layer.
    cJSON_AddNumberToObject(json, "version", 4);

    cJSON *layers = cJSON_CreateArray();
    for (int i = 0; i < state->layerCount; i++) {
        Layer *layer = state->layers + i;
        cJSON *layerJson = cJSON_CreateObject();
        cJSON_AddNumberToObject(layerJson, "x", layer->transform.o.x);
        cJSON_AddNumberToObject(layerJson, "y", layer->transform.o.y);
        switch (state->layers[i].type) {
            case LAYER_HITBOX:
                cJSON_AddStringToObject(layerJson, "type", "HITBOX");
                cJSON *hitbox = cJSON_CreateObject();
                cJSON_AddNumberToObject(hitbox, "knockbackX", layer->hitbox.knockbackX);
                cJSON_AddNumberToObject(hitbox, "knockbackY", layer->hitbox.knockbackY);
                cJSON_AddNumberToObject(hitbox, "damage", layer->hitbox.damage);
                cJSON_AddNumberToObject(hitbox, "stun", layer->hitbox.stun);
                cJSON_AddItemToObject(hitbox, "shape", ShapeSerialize(layer->hitbox.shape));
                cJSON_AddItemToObject(layerJson, "hitbox", hitbox);
                break;
            case LAYER_HURTBOX:
                cJSON_AddStringToObject(layerJson, "type", "HURTBOX");
                cJSON_AddItemToObject(layerJson, "hurtboxShape", ShapeSerialize(layer->hurtboxShape));
                break;
            case LAYER_METADATA:
                cJSON_AddStringToObject(layerJson, "type", "METADATA");
                cJSON_AddStringToObject(layerJson, "metadataTag", layer->metadataTag);
                break;
        }
        cJSON_AddItemToArray(layers, layerJson);
    }
    cJSON_AddItemToObject(json, "layers", layers);

    cJSON *frames = cJSON_CreateArray();
    for (int i = 0; i < state->frameCount; i++) {
        cJSON *frame = cJSON_CreateObject();
        FrameInfo frameInfo = state->frames[i];
        cJSON_AddNumberToObject(frame, "duration", frameInfo.duration);
        cJSON_AddBoolToObject(frame, "canCancel", frameInfo.canCancel);
        cJSON_AddNumberToObject(frame, "x", frameInfo.pos.x);
        cJSON_AddNumberToObject(frame, "y", frameInfo.pos.y);
        cJSON_AddItemToArray(frames, frame);
    }
    cJSON_AddItemToObject(json, "frames", frames);

    cJSON *activeFrames = cJSON_CreateArray();
    int activeCount = state->frameCount * state->layerCount;
    for (int i = 0; i < activeCount; i++) {
        cJSON *val = cJSON_CreateNumber(state->_layerActiveFrames[i]);
        cJSON_AddItemToArray(activeFrames, val);
    }
    cJSON_AddItemToObject(json, "layerActiveFrames", activeFrames);

    FILE *file = fopen(path, "w+");
    if (!file) {
        cJSON_Delete(json);
        return false;
    }
    char *str = cJSON_Print(json);
    cJSON_Delete(json);
    fputs(str, file);
    free(str);
    fclose(file);
    return true;
}

bool EditorStateDeserialize(EditorState *out, const char *path) {
    struct stat st;
    if (stat(path, &st) < 0) {
        printf("Failed to get information about file %s for deserialization.\n", path);
        return false;
    }

    FILE *file = fopen(path, "r+");
    if (!file) {
        printf("Failed to open file %s to deserialize\n", path);
        return false;
    }


    char *str = malloc(st.st_size + sizeof(char));
    size_t size = fread(str, 1, st.st_size, file);
    str[size] = '\0';
    fclose(file);
    cJSON *json = cJSON_Parse(str);
    free(str);

    if (!json) {
        printf("Failed to deserialize the JSON from the file %s.\n", path);
        return false;
    }

    cJSON *magic = cJSON_GetObjectItem(json, "magic");
    if (!magic || !cJSON_IsString(magic)) goto delete_json;
    cJSON *versionJson = cJSON_GetObjectItem(json, "version");
    int version;
    if (!versionJson) {
        version = 0;
    } else if (!cJSON_IsNumber(versionJson)) {
        goto delete_json;
    } else {
        version = (int) cJSON_GetNumberValue(versionJson);
    }

    if (version < 0 || version > 4) goto delete_json;

    cJSON *frames = cJSON_GetObjectItem(json, "frames");
    if (!cJSON_IsArray(frames)) goto delete_json;
    *out = EditorStateNew(cJSON_GetArraySize(frames));

#define RETURN_FAIL do { printf("Failed to parse file %s. Error: %s at line %i.\n", path, __FILE__, __LINE__); goto delete_editor_state; } while(0)
    
    cJSON *frameJson;
    int frameIdx = 0;
    cJSON_ArrayForEach(frameJson, frames) {
        cJSON *x = cJSON_GetObjectItem(frameJson, "x");
        if (!cJSON_IsNumber(x)) RETURN_FAIL;
        cJSON *y = cJSON_GetObjectItem(frameJson, "y");
        if (!cJSON_IsNumber(y)) RETURN_FAIL;
        cJSON *duration = cJSON_GetObjectItem(frameJson, "duration");
        if (!cJSON_IsNumber(duration)) RETURN_FAIL;
        cJSON *canCancel = cJSON_GetObjectItem(frameJson, "canCancel");
        if (!cJSON_IsBool(canCancel)) RETURN_FAIL;
        out->frames[frameIdx] = (FrameInfo) {
            .pos = (Vector2) {
                    .x = (float) cJSON_GetNumberValue(x),
                    .y = (float)  cJSON_GetNumberValue(y)
            },
            .duration = (int) cJSON_GetNumberValue(duration),
            .canCancel = cJSON_IsTrue(canCancel)
        };
        frameIdx++;
    }

    cJSON *layers = cJSON_GetObjectItem(json, version >= 4 ? "layers" : "shapes");
    if (!cJSON_IsArray(layers)) RETURN_FAIL;
    cJSON *layerJson;
    cJSON_ArrayForEach(layerJson, layers) {
        if (!cJSON_IsObject(layerJson)) RETURN_FAIL;
        cJSON *x = cJSON_GetObjectItem(layerJson, "x");
        if (!cJSON_IsNumber(x)) RETURN_FAIL;
        cJSON *y = cJSON_GetObjectItem(layerJson, "y");
        if(!cJSON_IsNumber(y)) RETURN_FAIL;
        cJSON *type = cJSON_GetObjectItem(layerJson, version >= 4 ? "type" : "boxType");
        if (!cJSON_IsString(type)) RETURN_FAIL;
        const char *typeString = cJSON_GetStringValue(type);
        Layer layer;
        if (!strcmp(typeString, "HITBOX")) {
            cJSON *knockbackX;
            cJSON *knockbackY;
            cJSON *damage;
            cJSON *stun;
            cJSON *shape;

            if (version >= 4) {
                cJSON *hitbox = cJSON_GetObjectItem(layerJson, "hitbox");
                if (!cJSON_IsObject(hitbox)) RETURN_FAIL;
                knockbackX = cJSON_GetObjectItem(hitbox, "knockbackX");
                knockbackY = cJSON_GetObjectItem(hitbox, "knockbackY");
                stun = cJSON_GetObjectItem(hitbox, "stun");
                damage = cJSON_GetObjectItem(hitbox, "damage");
                shape = cJSON_GetObjectItem(hitbox, "shape");
            } else {
                knockbackX = cJSON_GetObjectItem(layerJson, "hitboxKnockbackX");
                knockbackY = cJSON_GetObjectItem(layerJson, "hitboxKnockbackY");
                stun = cJSON_GetObjectItem(layerJson, "hitboxStun");
                damage = cJSON_GetObjectItem(layerJson, "hitboxDamage");
                shape = layerJson; // shape is inlined in old versions.
            }

            if (!cJSON_IsNumber(knockbackX)) RETURN_FAIL;
            if (!cJSON_IsNumber(knockbackY)) RETURN_FAIL;
            if (!cJSON_IsNumber(stun)) RETURN_FAIL;
            if (!cJSON_IsNumber(damage)) RETURN_FAIL;
            if (!ShapeDeserialize(shape, &layer.hitbox.shape, version)) RETURN_FAIL;

            layer.type = LAYER_HITBOX;
            layer.hitbox.knockbackX = (int) cJSON_GetNumberValue(knockbackX);
            layer.hitbox.knockbackY = (int) cJSON_GetNumberValue(knockbackY);
            layer.hitbox.stun = (int) cJSON_GetNumberValue(stun);
            layer.hitbox.damage = (int) cJSON_GetNumberValue(damage);

        } else if (!strcmp(typeString, "HURTBOX")) {
            cJSON *shape = version >= 4 ? cJSON_GetObjectItem(layerJson, "hurtboxShape") : layerJson;
            if (!shape || !ShapeDeserialize(shape, &layer.hurtboxShape, version)) RETURN_FAIL;
            layer.type = LAYER_HURTBOX;

        } else if (!strcmp(typeString, "METADATA")) {
            if (version < 4) RETURN_FAIL;
            cJSON *tag = cJSON_GetObjectItem(layerJson, "metadataTag");
            if (!tag || !cJSON_IsString(tag)) RETURN_FAIL;
            layer.type = LAYER_METADATA;
            strncpy(layer.metadataTag, cJSON_GetStringValue(tag), LAYER_METADATA_TAG_LENGTH - 1);

        } else RETURN_FAIL;

        layer.transform = Transform2DIdentity();
        layer.transform.o = (Vector2) {
            .x = (float) cJSON_GetNumberValue(x),
            .y = (float) cJSON_GetNumberValue(y)
        };

        EditorStateLayerAdd(out, layer);
    }

    cJSON *activeFrames = cJSON_GetObjectItem(json, version >= 4 ? "layerActiveFrames" : "shapeActiveFrames");
    if (!cJSON_IsArray(activeFrames)) RETURN_FAIL;
    cJSON *active;
    int activeFrameIdx = 0;
    cJSON_ArrayForEach(active, activeFrames) {
        if (!cJSON_IsNumber(active)) RETURN_FAIL;
        out->_layerActiveFrames[activeFrameIdx] = cJSON_GetNumberValue(active) != 0;
        activeFrameIdx++;
    }

    cJSON_Delete(json);
    return true;

#undef RETURN_FAIL
    delete_editor_state:
    EditorStateFree(out);
    delete_json:
    cJSON_Delete(json);
    return false;
}