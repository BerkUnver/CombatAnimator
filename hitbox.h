#ifndef HITBOX
#define HITBOX
#include "raylib.h"

#define HANDLE_RADIUS 8.0f
#define HANDLE_INTERIOR_RADIUS 6.0f
#define HANDLE_INTERIOR_COLOR RAYWHITE

#define HITBOX_OUTLINE_COLOR (Color) {255, 0, 0, 255}
#define HITBOX_COLOR (Color) {255, 0, 0, 123}
#define HITBOX_ROW_SIZE 32
#define HITBOX_CIRCLE_INACTIVE_COLOR (Color) {63, 63, 63, 255}
#define HITBOX_CIRCLE_ACTIVE_COLOR RAYWHITE
#define HITBOX_CIRCLE_RADIUS 12

typedef enum Handle {
    NONE = -1,
    CENTER = 0,
    RADIUS = 1
} Handle;

typedef struct Hitbox {
    int x;
    int y;
    float radius;
} Hitbox;

void DrawHitbox(Vector2 pos, float scale, Hitbox hitbox);
Handle SelectHitboxHandle(Vector2 mousePos, Vector2 pos, float scale, Hitbox hitbox);
bool SetHitboxHandle(Vector2 mousePos, Vector2 pos, float scale, Hitbox *hitbox, Handle handle);

#endif