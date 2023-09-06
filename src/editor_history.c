#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "cJSON.h"
#include "layer.h"
#include "string_buffer.h"
#include "editor_history.h"

EditorState EditorStateNew(int frameCount) {
    FrameInfo *frames = malloc(sizeof(FrameInfo) * frameCount);
    for (int i = 0; i < frameCount; i++) frames[i] = FRAME_INFO_DEFAULT;

    return (EditorState) {
        .layerCount = 0,
        .layers = NULL,
        .frames = frames,
        .frameCount = frameCount,
        .frameIdx = 0,
        .layerIdx = -1
    };
}

void EditorStateFree(EditorState *state) {
    for (int i = 0; i < state->layerCount; i++) {
        LayerFree(state->layers + i);
    }
    free(state->layers);
    free(state->frames);
}

// Takes ownership of layer
void EditorStateLayerAdd(EditorState *state, Layer layer) {
    state->layerCount++;
    state->layers = realloc(state->layers, sizeof(Layer) * state->layerCount);
    state->layers[state->layerCount - 1] = layer;
}

bool EditorStateLayerRemove(EditorState *state, int idx) {
    if (idx < 0 || idx >= state->layerCount) return false;
    LayerFree(state->layers + idx);    
    for (int layerIdx = idx + 1; layerIdx < state->layerCount; layerIdx++) {
        int newLayerIdx = layerIdx - 1;
        state->layers[newLayerIdx] = state->layers[layerIdx];
    }
    state->layerCount--;
    if (state->layerIdx >= state->layerCount) state->layerIdx = state->layerCount - 1;
    return true;
}

void EditorStateAddFrame(EditorState *state, int idx) {
    if (idx < 0 || idx >= state->frameCount) assert(false);
    state->frameCount++; 
    state->frames = realloc(state->frames, sizeof(FrameInfo) * state->frameCount);

    // Duplicate last frame to use as the new frame. It has no allocations so we can just assign it.
    state->frames[state->frameCount - 1] = state->frames[state->frameCount - 2];

    // Resize each layer so it has the right amount of frames
    for (int i = 0; i < state->layerCount; i++) {
        assert(state->layers[i].frameCount == state->frameCount - 1);
        state->layers[i].frameCount++;

        state->layers[i].framesActive = realloc(state->layers[i].framesActive, sizeof(*state->layers[i].framesActive) * state->frameCount);
        state->layers[i].framesActive[state->frameCount - 1] = false;
    }
}

bool EditorStateRemoveFrame(EditorState *state, int idx) {
    if (idx < 0 || idx >= state->frameCount || state->frameCount == 1) return false;
   
    for (int i = idx + 1; i < state->frameCount; i++) {
        state->frames[i - 1] = state->frames[i];
    }

    for (int layerIdx = 0; layerIdx < state->layerCount; layerIdx++) {
        for (int frameIdx = idx + 1; frameIdx < state->frameCount; frameIdx++) {
            state->layers[layerIdx].framesActive[frameIdx - 1] = state->layers[layerIdx].framesActive[frameIdx];
        }
        assert(state->layers[layerIdx].frameCount == state->frameCount);
        state->layers[layerIdx].frameCount--;
    }
    state->frameCount--;
    return true;
}

EditorState EditorStateDeepCopy(EditorState *state) {
    Layer *layersCopy = NULL;
    if (state->layerCount > 0) {
        // Bulk copy everything from each layer, then replace the malloc'ed parts with copies. 
        int layersSize = sizeof(Layer) * state->layerCount;
        layersCopy = malloc(layersSize);
        memcpy(layersCopy, state->layers, layersSize);
        
        for (int i = 0; i < state->layerCount; i++) {
            Layer *layer = layersCopy + i;

            char *name = malloc(layer->nameBufferLength);
            memcpy(name, layer->name, layer->nameBufferLength);
            layer->name = name;

            bool *framesActive = malloc(sizeof(bool) * state->frameCount);
            memcpy(framesActive, layer->framesActive, sizeof(bool) * state->frameCount); 
            layer->framesActive = framesActive;

            if (layer->type == LAYER_BEZIER) {
                BezierPoint *points = malloc(sizeof(BezierPoint) * state->frameCount);
                memcpy(points, layer->bezierPoints, sizeof(BezierPoint) * state->frameCount);
                layer->bezierPoints = points;
            }
        }
    }
    
    int framesSize = sizeof(FrameInfo) * state->frameCount;
    FrameInfo *framesCopy = malloc(framesSize);
    memcpy(framesCopy, state->frames, framesSize);

    return (EditorState) {
        .layerCount = state->layerCount,
        .frameCount = state->frameCount,
        .layers = layersCopy,
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

    cJSON_AddNumberToObject(json, "version", FILE_VERSION_CURRENT);

    cJSON *layers = cJSON_CreateArray();
    for (int layerIdx = 0; layerIdx < state->layerCount; layerIdx++) {
        Layer *layer = state->layers + layerIdx;
        cJSON *layerJson = cJSON_CreateObject();
        cJSON_AddNumberToObject(layerJson, "x", layer->transform.o.x);
        cJSON_AddNumberToObject(layerJson, "y", layer->transform.o.y);
        
        cJSON *framesActiveJson = cJSON_CreateArray();
        for (int frameIdx = 0; frameIdx < layer->frameCount; frameIdx++) {
            cJSON *val = cJSON_CreateBool(layer->framesActive[frameIdx]);
            cJSON_AddItemToArray(framesActiveJson, val);
        }
        cJSON_AddItemToObject(layerJson, "framesActive", framesActiveJson);
        
        // We can create a direct reference because we are going to free the json object right afterwards.
        cJSON *nameJson = cJSON_CreateStringReference(layer->name); 
        cJSON_AddItemToObject(layerJson, "name", nameJson);

        switch (layer->type) {
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
            case LAYER_SHAPE:
                cJSON_AddStringToObject(layerJson, "type", "SHAPE");
                cJSON *shape = cJSON_CreateObject();
                cJSON_AddItemToObject(shape, "shape", ShapeSerialize(layer->shape.shape));
                cJSON_AddNumberToObject(shape, "flags", layer->shape.flags);
                cJSON_AddItemToObject(layerJson, "shape", shape);
                break;
            case LAYER_BEZIER:
                cJSON_AddStringToObject(layerJson, "type", "BEZIER");
                cJSON *bezierPointsJson = cJSON_CreateArray();
                for (int frameIdx = 0; frameIdx < layer->frameCount; frameIdx++) {
                    if (!layer->framesActive[frameIdx]) continue;
                    BezierPoint bezier = layer->bezierPoints[frameIdx];
                    cJSON *bezierJson = cJSON_CreateObject();
                    cJSON_AddNumberToObject(bezierJson, "x", bezier.position.x);
                    cJSON_AddNumberToObject(bezierJson, "y", bezier.position.y);
                    cJSON_AddNumberToObject(bezierJson, "rotation", bezier.rotation);
                    cJSON_AddNumberToObject(bezierJson, "extentsLeft", bezier.extentsLeft);
                    cJSON_AddNumberToObject(bezierJson, "extentsRight", bezier.extentsRight);
                    
                    cJSON_AddItemToArray(bezierPointsJson, bezierJson);
                }
                cJSON_AddItemToObject(layerJson, "bezierPoints", bezierPointsJson);
                break;
            case LAYER_EMPTY:
                cJSON_AddStringToObject(layerJson, "type", "EMPTY");
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

#define ERROR_GOTO(label) do {printf("Failed to parse file %s. Error: %s at line %i.\n", path, __FILE__, __LINE__); goto label;} while (0)
    cJSON *magic = cJSON_GetObjectItem(json, "magic");
    if (!cJSON_IsString(magic)) ERROR_GOTO(delete_json);
    cJSON *versionJson = cJSON_GetObjectItem(json, "version");
    int version;
    if (!versionJson) {
        version = 0;
    } else if (!cJSON_IsNumber(versionJson)) {
        ERROR_GOTO(delete_json);
    } else {
        version = (int) cJSON_GetNumberValue(versionJson);
    }

    if (version < 0 || version > FILE_VERSION_CURRENT) {
        printf("Invalid version number %i\n", version);
        ERROR_GOTO(delete_json);
    } else if (version < FILE_VERSION_OLDEST) {
        printf("Version %i no longer supported.\n", version);
        ERROR_GOTO(delete_json);
    }

    cJSON *frames = cJSON_GetObjectItem(json, "frames");
    if (!cJSON_IsArray(frames)) ERROR_GOTO(delete_json);
    *out = EditorStateNew(cJSON_GetArraySize(frames));

    cJSON *frameJson;
    int frameIdx = 0;
    cJSON_ArrayForEach(frameJson, frames) {
        cJSON *x = cJSON_GetObjectItem(frameJson, "x");
        if (!cJSON_IsNumber(x)) ERROR_GOTO(delete_editor_state);
        cJSON *y = cJSON_GetObjectItem(frameJson, "y");
        if (!cJSON_IsNumber(y)) ERROR_GOTO(delete_editor_state);
        cJSON *duration = cJSON_GetObjectItem(frameJson, "duration");
        if (!cJSON_IsNumber(duration)) ERROR_GOTO(delete_editor_state);
        cJSON *canCancel = cJSON_GetObjectItem(frameJson, "canCancel");
        if (!cJSON_IsBool(canCancel)) ERROR_GOTO(delete_editor_state);
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

    cJSON *layers = cJSON_GetObjectItem(json, "layers");
    if (!cJSON_IsArray(layers)) ERROR_GOTO(delete_editor_state);
    cJSON *layerJson;
    cJSON_ArrayForEach(layerJson, layers) {
        if (!cJSON_IsObject(layerJson)) ERROR_GOTO(delete_editor_state);
        
        Layer layer;
        
        cJSON *x = cJSON_GetObjectItem(layerJson, "x");
        if (!cJSON_IsNumber(x)) ERROR_GOTO(delete_editor_state);
        cJSON *y = cJSON_GetObjectItem(layerJson, "y");
        if (!cJSON_IsNumber(y)) ERROR_GOTO(delete_editor_state);
        Vector2 position = {
            (float) cJSON_GetNumberValue(x),
            (float) cJSON_GetNumberValue(y)
        };
        layer.transform = Transform2DFromPosition(position);
         
        layer.frameCount = out->frameCount;
        layer.framesActive = malloc(sizeof(*layer.framesActive) * out->frameCount);

        cJSON *framesActive = cJSON_GetObjectItem(layerJson, "framesActive");
        if (!cJSON_IsArray(framesActive)) ERROR_GOTO(delete_frames_active);
        
        int frameIdx = 0;
        cJSON *frameActive;
        cJSON_ArrayForEach(frameActive, framesActive) {
            if (frameIdx >= layer.frameCount || !cJSON_IsBool(frameActive)) ERROR_GOTO(delete_frames_active);
            layer.framesActive[frameIdx] = cJSON_IsTrue(frameActive);
            frameIdx++;
        }
        
        cJSON *type = cJSON_GetObjectItem(layerJson, "type");
        if (!cJSON_IsString(type)) ERROR_GOTO(delete_frames_active);
        const char *typeString = cJSON_GetStringValue(type);
     
        cJSON *nameJson = cJSON_GetObjectItem(layerJson, "name");
        if (!cJSON_IsString(nameJson)) ERROR_GOTO(delete_frames_active);
        char *name = cJSON_GetStringValue(nameJson);
        
        layer.nameBufferLength = strlen(name) + 1;
        if (layer.nameBufferLength < LAYER_NAME_BUFFER_INITIAL_SIZE) layer.nameBufferLength = LAYER_NAME_BUFFER_INITIAL_SIZE;
        layer.name = malloc(layer.nameBufferLength);
        strcpy(layer.name, name);

        if (!strcmp(typeString, "HITBOX")) {
            cJSON *hitbox = cJSON_GetObjectItem(layerJson, "hitbox");
            if (!cJSON_IsObject(hitbox)) ERROR_GOTO(delete_name);
            
            cJSON *knockbackX = cJSON_GetObjectItem(hitbox, "knockbackX");
            cJSON *knockbackY = cJSON_GetObjectItem(hitbox, "knockbackY");
            cJSON *stun = cJSON_GetObjectItem(hitbox, "stun");
            cJSON *damage = cJSON_GetObjectItem(hitbox, "damage");
            cJSON *shape = cJSON_GetObjectItem(hitbox, "shape");

            if (!cJSON_IsNumber(knockbackX))    ERROR_GOTO(delete_name);
            if (!cJSON_IsNumber(knockbackY))    ERROR_GOTO(delete_name);
            if (!cJSON_IsNumber(stun))          ERROR_GOTO(delete_name);
            if (!cJSON_IsNumber(damage))        ERROR_GOTO(delete_name);
            if (!ShapeDeserialize(shape, &layer.hitbox.shape, version)) ERROR_GOTO(delete_name);

            layer.type = LAYER_HITBOX;
            layer.hitbox.knockbackX = (int) cJSON_GetNumberValue(knockbackX);
            layer.hitbox.knockbackY = (int) cJSON_GetNumberValue(knockbackY);
            layer.hitbox.stun = (int) cJSON_GetNumberValue(stun);
            layer.hitbox.damage = (int) cJSON_GetNumberValue(damage);
        
        } else if (!strcmp(typeString, "HURTBOX") && version <= 7) {
            layer.type = LAYER_SHAPE;
            cJSON *shape = cJSON_GetObjectItem(layerJson, "hurtboxShape");
            if (!shape || !ShapeDeserialize(shape, &layer.shape.shape, version)) ERROR_GOTO(delete_name);
            layer.shape.flags = 1;
        
        } else if (!strcmp(typeString, "SHAPE") && version >= 8) {
            layer.type = LAYER_SHAPE;
            cJSON *shapeLayer = cJSON_GetObjectItem(layerJson, "shape");
            if (!cJSON_IsObject(shapeLayer)) ERROR_GOTO(delete_name);
            cJSON *shape = cJSON_GetObjectItem(shapeLayer, "shape");
            if (!shape || !ShapeDeserialize(shape, &layer.shape.shape, version)) ERROR_GOTO(delete_name);
            cJSON *flags = cJSON_GetObjectItem(shapeLayer, "flags");
            if (!cJSON_IsNumber(flags)) ERROR_GOTO(delete_name);
            layer.shape.flags = cJSON_GetNumberValue(flags);
        
        } else if (!strcmp(typeString, "EMPTY")) {
            layer.type = LAYER_EMPTY;
        
        } else if (!strcmp(typeString, "BEZIER")) {
            layer.type = LAYER_BEZIER;
            
            cJSON *bezierPointsJson = cJSON_GetObjectItem(layerJson, "bezierPoints");
            if (!cJSON_IsArray(bezierPointsJson)) ERROR_GOTO(delete_name);
            layer.bezierPoints = malloc(sizeof(BezierPoint) * cJSON_GetArraySize(bezierPointsJson));
            
            int frameIdx = 0;
            cJSON *bezierPointJson;
            cJSON_ArrayForEach(bezierPointJson, bezierPointsJson) {
                if (frameIdx >= layer.frameCount) ERROR_GOTO(delete_bezier_points);
                
                if (cJSON_IsNull(bezierPointJson)) continue; 
                if (!cJSON_IsObject(bezierPointJson)) ERROR_GOTO(delete_bezier_points);
               
                cJSON *positionX = cJSON_GetObjectItem(bezierPointJson,"x");
                if (!cJSON_IsNumber(positionX)) ERROR_GOTO(delete_bezier_points);
                cJSON *positionY = cJSON_GetObjectItem(bezierPointJson, "y");
                if (!cJSON_IsNumber(positionY)) ERROR_GOTO(delete_bezier_points);
                cJSON *extentsLeft = cJSON_GetObjectItem(bezierPointJson, "extentsLeft");
                if (!cJSON_IsNumber(extentsLeft)) ERROR_GOTO(delete_bezier_points);
                cJSON *extentsRight = cJSON_GetObjectItem(bezierPointJson, "extentsRight");
                if (!cJSON_IsNumber(extentsRight)) ERROR_GOTO(delete_bezier_points);
                cJSON *rotation = cJSON_GetObjectItem(bezierPointJson, "rotation");
                if (!cJSON_IsNumber(rotation)) ERROR_GOTO(delete_bezier_points);

                layer.bezierPoints[frameIdx] = (BezierPoint) {
                    .position = (Vector2) {cJSON_GetNumberValue(positionX), cJSON_GetNumberValue(positionY)},
                    .extentsLeft = cJSON_GetNumberValue(extentsLeft),
                    .extentsRight = cJSON_GetNumberValue(extentsRight),
                    .rotation = cJSON_GetNumberValue(rotation)
                };
                frameIdx++;
                
                while (false) {
delete_bezier_points:
                    free(layer.bezierPoints);
                    goto delete_name;
                }
            }
        } else ERROR_GOTO(delete_name);
    
        EditorStateLayerAdd(out, layer);
        continue;

delete_name:
        free(layer.name);
delete_frames_active:
        free(layer.framesActive);
        goto delete_editor_state;
    }

    cJSON_Delete(json);
    return true;

delete_editor_state:
    EditorStateFree(out);
delete_json:
    cJSON_Delete(json);
    return false;
#undef ERROR_GOTO
}
