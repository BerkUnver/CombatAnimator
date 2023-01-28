#ifndef COMBAT_SHAPE
#define COMBAT_SHAPE
#include "raylib.h"
#include "cjson/cJSON.h"

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

typedef enum Handle {
    NONE,
    CENTER,
    CIRCLE_RADIUS,
    RECTANGLE_CORNER,
    CAPSULE_HEIGHT,
    CAPSULE_RADIUS
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
    } capsule;
} ShapeData;

typedef enum ShapeType {
    CIRCLE,
    RECTANGLE,
    CAPSULE
} ShapeType;

typedef struct CombatShape {
    BoxType boxType;
    ShapeType shapeType;
    ShapeData data;
    int x;
    int y;
} CombatShape;

CombatShape CombatShapeRectangle(int x, int y, int rightX, int bottomY, BoxType type);
CombatShape CombatShapeCircle(int x, int y, int radius, BoxType type);
CombatShape CombatShapeCapsule(int x, int y, int radius, int height, BoxType type);

cJSON *SerializeShape(CombatShape shape);

void DrawCombatShape(Vector2 pos, float scale, CombatShape shape, bool handlesActive);
Handle SelectCombatShapeHandle(Vector2 mousePos, Vector2 pos, float scale, CombatShape shape);
bool SetCombatShapeHandle(Vector2 mousePos, Vector2 pos, float scale, CombatShape *shape, Handle handle);

#endif