#include <stdlib.h>
#include <math.h>
#include "raylib.h"
#include "hitbox.h"

void DrawHitbox(Vector2 pos, float scale, Hitbox hitbox) {
    float x = pos.x + hitbox.x * scale;
    float y = pos.y + hitbox.y * scale;
    float r = hitbox.radius * scale;
    DrawCircle(x, y, r, HITBOX_COLOR);
    DrawCircle(x, y, HANDLE_RADIUS, HITBOX_OUTLINE_COLOR);
    DrawCircle(x, y, HANDLE_INTERIOR_RADIUS, HANDLE_INTERIOR_COLOR);
    DrawCircle(x + r, y, HANDLE_RADIUS, HITBOX_OUTLINE_COLOR);
    DrawCircle(x + r, y, HANDLE_INTERIOR_RADIUS, HANDLE_INTERIOR_COLOR);
}

Handle SelectHitboxHandle(Vector2 mousePos, Vector2 pos, float scale, Hitbox hitbox) {
    float radiusGlobalX = pos.x + (hitbox.x + hitbox.radius) * scale;
    float radiusGlobalY = pos.y + hitbox.y * scale;
    Rectangle radiusHandleRect = {radiusGlobalX - HANDLE_RADIUS, radiusGlobalY - HANDLE_RADIUS, HANDLE_RADIUS * 2.0f, HANDLE_RADIUS * 2.0f};

    if (CheckCollisionPointRec(mousePos, radiusHandleRect)) {
        return RADIUS;
    }
    
    float centerGlobalX = pos.x + hitbox.x * scale;
    float centerGlobalY = pos.y + hitbox.y * scale;
    Rectangle centerHandleRect = {centerGlobalX - HANDLE_RADIUS, centerGlobalY - HANDLE_RADIUS, HANDLE_RADIUS * 2.0f, HANDLE_RADIUS * 2.0f};

    if (CheckCollisionPointRec(mousePos, centerHandleRect)) {
        return CENTER;
    }
    
    return NONE;
}

bool SetHitboxHandle(Vector2 mousePos, Vector2 pos, float scale, Hitbox *hitbox, Handle handle) {
    float x = round((mousePos.x - pos.x) / scale);
    float y = round((mousePos.y - pos.y) / scale);
    
    switch (handle) {
        case CENTER:
            hitbox->x = x;
            hitbox->y = y;
            return true;
        case RADIUS:
            hitbox->radius = abs(x - hitbox->x);
            return true;
        default: return false;
    }
}
