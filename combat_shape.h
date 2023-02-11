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

#define HITBOX_OUTLINE_COLOR (Color) {255, 0, 0, 255}
#define HITBOX_COLOR (Color) {255, 0, 0, COMBAT_SHAPE_ALPHA}

#define HITBOX_CIRCLE_INACTIVE_COLOR (Color) {63, 0, 0, 255}
#define HITBOX_CIRCLE_ACTIVE_COLOR HITBOX_OUTLINE_COLOR

#define HURTBOX_OUTLINE_COLOR (Color) {0, 255, 255, 255}
#define HURTBOX_COLOR (Color) {0, 255, 255, COMBAT_SHAPE_ALPHA}

#define HURTBOX_CIRCLE_INACTIVE_COLOR (Color) {0, 63, 63, 255}
#define HURTBOX_CIRCLE_ACTIVE_COLOR HURTBOX_OUTLINE_COLOR

#define STR_HITBOX "HITBOX"
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
    NONE,
    CENTER,
    HITBOX_KNOCKBACK,
    CIRCLE_RADIUS,
    RECTANGLE_CORNER,
    CAPSULE_HEIGHT,
    CAPSULE_RADIUS,
    CAPSULE_ROTATION
} Handle;

typedef enum BoxType {
    HITBOX,
    HURTBOX
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
    CIRCLE,
    RECTANGLE,
    CAPSULE
} ShapeType;

typedef struct CombatShape {
    BoxType boxType;
    int hitboxKnockbackX; // defined only if boxType is HITBOX
    int hitboxKnockbackY;
    ShapeType shapeType;
    ShapeData data;
    Transform2D transform;
} CombatShape;

cJSON *SerializeShape(CombatShape shape);
bool DeserializeShape(cJSON *json, CombatShape *out);

void DrawCombatShape(Transform2D transform, CombatShape shape, bool handlesActive);
Handle SelectCombatShapeHandle(Transform2D transform, Vector2 globalMousePos, CombatShape shape);
bool SetCombatShapeHandle(Vector2 localMousePos, CombatShape *shape, Handle handle);

#endif
