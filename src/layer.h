#ifndef LAYER_H
#define LAYER_H

#include "raylib.h"
#include "cJSON.h"
#include "transform_2d.h"
#include "list.h"

#define SHAPE_SEGMENTS 16
#define HANDLE_RADIUS 8.0f
#define BEZIER_SEGMENTS 16

#define LAYER_NAME_BUFFER_INITIAL_SIZE 32
#define LAYER_NAME_BUFFER_RESIZE_MULTIPLIER 1.5f

typedef enum Handle {
    HANDLE_NONE,
    HANDLE_CENTER,
    HANDLE_HITBOX_KNOCKBACK,
    HANDLE_CIRCLE_RADIUS,
    HANDLE_RECTANGLE_CORNER,
    HANDLE_CAPSULE_HEIGHT,
    HANDLE_CAPSULE_RADIUS,
    HANDLE_CAPSULE_ROTATION,
    HANDLE_BEZIER_CENTER,
    HANDLE_BEZIER_LEFT,
    HANDLE_BEZIER_RIGHT
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

// When you update this pls make sure to update the layer colors.
typedef enum LayerType {
    LAYER_HITBOX,
    LAYER_SHAPE,
    LAYER_BEZIER,
    LAYER_EMPTY
} LayerType;

extern Color layerColors[];

typedef struct BezierPoint {
    Vector2 position;
    float extentsLeft;
    float extentsRight;
    float rotation;
} BezierPoint;

Vector2 BezierLerp(BezierPoint p0, BezierPoint p1, float lerp);

typedef struct Layer {
    Transform2D transform;
    LayerType type;
    
    char *name;
    int nameBufferLength; // byte length of the buffer

    LIST(bool) framesActive;
    union {
        struct {
            int knockbackX;
            int knockbackY;
            int damage;
            int stun;
            Shape shape;
        } hitbox;
        
        struct {
            unsigned int flags;
            Shape shape;
        } shape;

        LIST(BezierPoint) bezierPoints;
        // Length is frameCount.
        // A specific index is assumed to be undefined when its equivalent framesActive index is undefined.
    };
} Layer;

// No layer init function because creating a layer is too complex to do in a single function because of the unions.
void LayerFree(Layer *layer);
bool HandleIsColliding(Transform2D globalTransform, Vector2 globalMousePos, Vector2 localPos);
void HandleDraw(Vector2 pos, Color strokeColor);

void LayerDraw(Layer *layer, int frame, Transform2D transform, bool handlesActive);
Handle LayerHandleSelect(Layer *layer, int frame, Transform2D transform, Vector2 globalMousePos);
bool LayerHandleSet(Layer *layer, int frame, Handle handle, Vector2 localMousePos);

cJSON *ShapeSerialize(Shape shape);
bool ShapeDeserialize(cJSON *json, Shape *shape, int version);
#endif
