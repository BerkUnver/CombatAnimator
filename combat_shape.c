#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "raylib.h"
#include "combat_shape.h"

CombatShape CombatShapeRectangle(int x, int y, int rightX, int bottomY, BoxType type) {
    CombatShape shape;
    shape.x = x;
    shape.y = y;
    shape.shapeType = RECTANGLE;
    shape.data.rectangle.rightX = rightX;
    shape.data.rectangle.bottomY = bottomY;
    shape.boxType = type;
    return shape;
}

CombatShape CombatShapeCircle(int x, int y, int radius, BoxType type) {
    CombatShape shape;
    shape.x = x;
    shape.y = y;
    shape.shapeType = CIRCLE;
    shape.data.circleRadius = radius;
    shape.boxType = type;
    return shape;
}

void DrawHandle(int x, int y, Color strokeColor) {
    DrawCircle(x, y, HANDLE_RADIUS, strokeColor);
    DrawCircle(x, y, HANDLE_INTERIOR_RADIUS, HANDLE_INTERIOR_COLOR);
}

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

    switch (shape.shapeType) {
        case CIRCLE: {
            float r = shape.data.circleRadius * scale;
            DrawCircle(x, y, r, color);
            if (!handlesActive) return;
            DrawCircleLines(x, y, r, outlineColor);
            DrawHandle(x, y, outlineColor);
            DrawHandle(x + r, y, outlineColor);
        } return;

        case RECTANGLE: {
            float rightX = shape.data.rectangle.rightX * scale;
            float bottomY = shape.data.rectangle.bottomY * scale;
            float rectX = x - rightX;
            float rectY = y - bottomY;
            float width = rightX * 2;
            float height = bottomY * 2;
            DrawRectangle(rectX, rectY, width, height, color);
            if (!handlesActive) return;
            DrawRectangleLines(rectX, rectY, width, height, outlineColor);
            DrawHandle(x, y, outlineColor);
            DrawHandle(x + rightX, y + bottomY, outlineColor);
        } return;

        default: return;
    }
}

Handle SelectCombatShapeHandle(Vector2 mousePos, Vector2 pos, float scale, CombatShape shape) {
    bool IsCollidingHandle(float x, float y) {
        Rectangle rect = {x - HANDLE_RADIUS, y - HANDLE_RADIUS, HANDLE_RADIUS * 2.0f, HANDLE_RADIUS * 2.0f};
        return CheckCollisionPointRec(mousePos, rect);
    }

    switch (shape.shapeType) {
        case CIRCLE: {
            float radiusGlobalX = pos.x + (shape.x + shape.data.circleRadius) * scale;
            float radiusGlobalY = pos.y + shape.y * scale;

            if (IsCollidingHandle(radiusGlobalX, radiusGlobalY)) {
                return CIRCLE_RADIUS;
            }
        } break;

        case RECTANGLE: {
            float handleGlobalX = pos.x + (shape.x + shape.data.rectangle.rightX) * scale;
            float handleGlobalY = pos.y + (shape.y + shape.data.rectangle.bottomY) * scale;
            printf("handle local pos: (%i, %i)\n", shape.data.rectangle.rightX, shape.data.rectangle.bottomY);
            printf("handle global pos: (%f, %f)\n", handleGlobalX, handleGlobalY);
            if (IsCollidingHandle(handleGlobalX, handleGlobalY)) {
                puts("Selected rectangle corner");
                return RECTANGLE_CORNER;
            }
        } break;

        default: break;
    }

    float centerGlobalX = pos.x + shape.x * scale;
    float centerGlobalY = pos.y + shape.y * scale;
    if (IsCollidingHandle(centerGlobalX, centerGlobalY)) {
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
        
        case CIRCLE_RADIUS:
            if (shape->shapeType != CIRCLE) return false;
            int newRadius = round(x - shape->x);
            shape->data.circleRadius = newRadius > 0 ? newRadius : 0;
            return true;
        
        case RECTANGLE_CORNER:
            if (shape->shapeType != RECTANGLE) return false;
            int rightX = round(x - shape->x);
            int bottomY = round(y - shape->y);
            shape->data.rectangle.rightX = rightX;
            shape->data.rectangle.bottomY = bottomY;
            return true;
        
        default:
            return false;
    }
}
