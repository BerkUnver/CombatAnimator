#ifndef COMBAT_SHAPE
#define COMBAT_SHAPE
#include "raylib.h"

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
#define HURTBOX_CIRLE_ACTIVE_COLOR HURTBOX_OUTLINE_COLOR

typedef enum Handle {
    NONE = -1,
    CENTER = 0,
    RADIUS = 1
} Handle;

typedef enum BoxType {
    HITBOX,
    HURTBOX
} BoxType;

typedef struct CombatShape {
    BoxType boxType;
    int x;
    int y;
    int radius;
} CombatShape;

void DrawCombatShape(Vector2 pos, float scale, CombatShape shape, bool handlesActive);
Handle SelectCombatShapeHandle(Vector2 mousePos, Vector2 pos, float scale, CombatShape shape);
bool SetCombatShapeHandle(Vector2 mousePos, Vector2 pos, float scale, CombatShape *shape, Handle handle);

#endif