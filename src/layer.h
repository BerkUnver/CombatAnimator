#ifndef LAYER_H
#define LAYER_H

#include "raylib.h"
#include "cjson/cJSON.h"
#include "transform_2d.h"

#define SHAPE_SEGMENTS 16
#define HANDLE_RADIUS 8.0f
// hitbox
// hurtbox
// knockback
// Red
#define LAYER_HITBOX_COLOR_OUTLINE (Color) {255, 0, 0, 255}
#define LAYER_HITBOX_COLOR (Color) {255, 0, 0, 63}
#define LAYER_HITBOX_COLOR_TIMELINE_INACTIVE (Color) {63, 0, 0, 255}
#define LAYER_HITBOX_COLOR_TIMELINE_ACTIVE LAYER_HITBOX_COLOR_OUTLINE
#define LAYER_HURTBOX_COLOR_OUTLINE (Color) {0, 255, 255, 255}
#define LAYER_HURTBOX_COLOR (Color) {0, 255, 255, 63}
#define LAYER_HURTBOX_COLOR_TIMELINE_INACTIVE (Color) {0, 63, 63, 255}
#define LAYER_HURTBOX_COLOR_TIMELINE_ACTIVE LAYER_HURTBOX_COLOR_OUTLINE
#define LAYER_METADATA_TAG_LENGTH 16
#define LAYER_METADATA_COLOR_OUTLINE (Color) {255, 0, 255, 255}
#define LAYER_METADATA_COLOR_TIMELINE_INACTIVE (Color) {63, 0, 63, 255}
#define LAYER_METADATA_COLOR_TIMELINE_ACTIVE LAYER_METADATA_COLOR_OUTLINE


typedef enum Handle {
    HANDLE_NONE,
    HANDLE_CENTER,
    HANDLE_HITBOX_KNOCKBACK,
    HANDLE_CIRCLE_RADIUS,
    HANDLE_RECTANGLE_CORNER,
    HANDLE_CAPSULE_HEIGHT,
    HANDLE_CAPSULE_RADIUS,
    HANDLE_CAPSULE_ROTATION
} Handle;

typedef enum ShapeType {
    SHAPE_CIRCLE,
    SHAPE_RECTANGLE,
    SHAPE_CAPSULE
} ShapeType;

typedef struct Shape {
    ShapeType type;
    union {
        int circleRadius;
        struct {
            int rightX;
            int bottomY;
        } rectangle;
        struct {
            int radius;
            int height;
            float rotation;
        } capsule;
    };
} Shape;

typedef enum LayerType {
    LAYER_HURTBOX,
    LAYER_HITBOX,
    LAYER_METADATA,
    // LAYER_SPLINE
} LayerType;

typedef struct SplinePoints {
    Vector2 position;
    float extentsLeft;
    float extentsRight;
    float rotation;
} SplinePoints;

typedef struct Layer {
    Transform2D transform;
    LayerType type;

    int frameCount;
    bool *framesActive;
    union {
        Shape hurtboxShape;
        struct {
            int knockbackX;
            int knockbackY;
            int damage;
            int stun;
            Shape shape;
        } hitbox;
        char metadataTag[LAYER_METADATA_TAG_LENGTH]; // @TODO replace with string buffer
         
        // SplinePoint *splinePoints; 
        // Length is frameCount.
        // A specific index is assumed to be undefined when its equivalent framesActive index is undefined.
    };
} Layer;

// No layer init function because creating a layer is too complex to do in a single function because of the unions.
void LayerFree(Layer *layer);
bool HandleIsColliding(Transform2D globalTransform, Vector2 globalMousePos, Vector2 localPos);
void HandleDraw(Vector2 pos, Color strokeColor);

void LayerDraw(Layer *layer, int frame, Transform2D transform, bool handlesActive);
Handle LayerHandleSelect(Layer *layer, Transform2D transform, Vector2 globalMousePos);
bool LayerHandleSet(Layer *layer, Handle handle, Vector2 localMousePos);

cJSON *ShapeSerialize(Shape shape);
bool ShapeDeserialize(cJSON *json, Shape *shape, int version);
#endif
