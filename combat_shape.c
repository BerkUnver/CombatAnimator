#include <stdlib.h>
#include <math.h>
#include "raylib.h"
#include "combat_shape.h"

void DrawCombatShape(Vector2 pos, float scale, CombatShape shape, bool handlesActive) {
    Color color;
    Color outlineColor;
    if (shape.boxType == HURTBOX) {
        color = HURTBOX_COLOR;
        outlineColor = HURTBOX_OUTLINE_COLOR;
    } else {
        color = HITBOX_COLOR;
        outlineColor = HITBOX_OUTLINE_COLOR;
    }

    float x = pos.x + shape.x * scale;
    float y = pos.y + shape.y * scale;
    float r = shape.radius * scale;
    DrawCircle(x, y, r, color);
    if (!handlesActive) return;
    DrawCircleLines(x, y, r, outlineColor);
    DrawCircle(x, y, HANDLE_RADIUS, outlineColor);
    DrawCircle(x, y, HANDLE_INTERIOR_RADIUS, HANDLE_INTERIOR_COLOR);
    DrawCircle(x + r, y, HANDLE_RADIUS, outlineColor);
    DrawCircle(x + r, y, HANDLE_INTERIOR_RADIUS, HANDLE_INTERIOR_COLOR);
}

Handle SelectCombatShapeHandle(Vector2 mousePos, Vector2 pos, float scale, CombatShape shape) {
    float radiusGlobalX = pos.x + (shape.x + shape.radius) * scale;
    float radiusGlobalY = pos.y + shape.y * scale;
    Rectangle radiusHandleRect = {radiusGlobalX - HANDLE_RADIUS, radiusGlobalY - HANDLE_RADIUS, HANDLE_RADIUS * 2.0f, HANDLE_RADIUS * 2.0f};

    if (CheckCollisionPointRec(mousePos, radiusHandleRect)) {
        return RADIUS;
    }
    
    float centerGlobalX = pos.x + shape.x * scale;
    float centerGlobalY = pos.y + shape.y * scale;
    Rectangle centerHandleRect = {centerGlobalX - HANDLE_RADIUS, centerGlobalY - HANDLE_RADIUS, HANDLE_RADIUS * 2.0f, HANDLE_RADIUS * 2.0f};

    if (CheckCollisionPointRec(mousePos, centerHandleRect)) {
        return CENTER;
    }

    return NONE;
}

bool SetCombatShapeHandle(Vector2 mousePos, Vector2 pos, float scale, CombatShape *shape, Handle handle) {
    float x = round((mousePos.x - pos.x) / scale);
    float y = round((mousePos.y - pos.y) / scale);
    
    switch (handle) {
        case CENTER:
            shape->x = x;
            shape->y = y;
            return true;
        
        case RADIUS: {
            int newRadius = round(x - shape->x);
            shape->radius = newRadius > 0 ? newRadius : 0;
        } return true;
        
        default:
            return false;
    }
}
