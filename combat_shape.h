#ifndef COMBAT_SHAPE
#define COMBAT_SHAPE
#include "raylib.h"
#include "cjson/cJSON.h"
#include "transform_2d.h"

#define COMBAT_SHAPE_SEGMENTS 16
#define HANDLE_RADIUS 8.0f
#define HANDLE_INTERIOR_RADIUS 6.0f
#define HANDLE_INTERIOR_COLOR RAYWHITE

#define COMBAT_SHAPE_ALPHA 63

// Red
#define HITBOX_OUTLINE_COLOR (Color) {255, 0, 0, 255}
#define HITBOX_COLOR (Color) {255, 0, 0, COMBAT_SHAPE_ALPHA}

#define HITBOX_CIRCLE_INACTIVE_COLOR (Color) {63, 0, 0, 255}
#define HITBOX_CIRCLE_ACTIVE_COLOR HITBOX_OUTLINE_COLOR

// Blue (Teal)
#define HURTBOX_OUTLINE_COLOR (Color) {0, 255, 255, 255}
#define HURTBOX_COLOR (Color) {0, 255, 255, COMBAT_SHAPE_ALPHA}

#define HURTBOX_CIRCLE_INACTIVE_COLOR (Color) {0, 63, 63, 255}
#define HURTBOX_CIRCLE_ACTIVE_COLOR HURTBOX_OUTLINE_COLOR

#define TAG_LENGTH_MAX 8

// Strings used for serialization
#define STR_HITBOX "HITBOX"
#define STR_HITBOX_STUN "hitboxStun"
#define STR_HITBOX_DAMAGE "hitboxDamage"
#define STR_HITBOX_KNOCKBACK_X "hitboxKnockbackX"
#define STR_HITBOX_KNOCKBACK_Y "hitboxKnockbackY"
#define STR_HURTBOX "HURTBOX"
#define STR_CIRCLE "CIRCLE"
#define STR_RECTANGLE "RECTANGLE"
#define STR_CAPSULE "CAPSULE"
#define STR_CIRCLE_RADIUS "circleRadius"
#define STR_RECTANGLE_RIGHT_X "rectangleRightX"
#define STR_RECTANGLE_BOTTOM_Y "rectangleBottomY"
#define STR_CAPSULE_RADIUS "capsuleRadius"
#define STR_CAPSULE_HEIGHT "capsuleHeight"
#define STR_CAPSULE_ROTATION "capsuleRotation"
#define STR_SHAPE_TYPE "shapeType"
#define STR_BOX_TYPE "boxType"
#define STR_X "x"
#define STR_Y "y"

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

typedef enum BoxType {
    BOX_TYPE_HITBOX,
    BOX_TYPE_HURTBOX,
    BOX_TYPE_METADATA,
} BoxType;

typedef union ShapeData {
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
} ShapeData;

typedef enum ShapeType {
    SHAPE_CIRCLE,
    SHAPE_RECTANGLE,
    SHAPE_CAPSULE
} ShapeType;

typedef struct CombatShape {
    BoxType boxType;
    ShapeType shapeType;

    int hitboxKnockbackX; // defined only if boxType is BOX_TYPE_HITBOX
    int hitboxKnockbackY;
    int hitboxDamage;
    int hitboxStun;
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
    } data;

    Transform2D transform;
} CombatShape;

CombatShape CombatShapeNew(ShapeType shapeType, BoxType boxType);

cJSON *CombatShapeSerialize(CombatShape shape);
bool CombatShapeDeserialize(cJSON *json, int version, CombatShape *out);

bool HandleIsColliding(Transform2D globalTransform, Vector2 globalMousePos, Vector2 localPos);
void HandleDraw(Vector2 pos, Color strokeColor);

void CombatShapeDraw(Transform2D transform, CombatShape shape, bool handlesActive);
Handle CombatShapeSelectHandle(Transform2D transform, Vector2 globalMousePos, CombatShape shape);
bool CombatShapeSetHandle(Vector2 localMousePos, CombatShape *shape, Handle handle);

#endif
