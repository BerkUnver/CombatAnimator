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

CombatShape CombatShapeCapsule(int x, int y, int radius, int height, BoxType type) {
    CombatShape shape;
    shape.x = x;
    shape.y = y;
    shape.shapeType = CAPSULE;
    shape.data.capsule.radius = radius;
    shape.data.capsule.height = height;
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

        case CAPSULE: {
            float globalHeight = shape.data.capsule.height * scale;
            float globalRadius = shape.data.capsule.radius * scale;
            float rectX = x - globalRadius;
            float rectY = y - globalHeight - globalRadius;
            float width = globalRadius * 2;
            float height = (globalHeight + globalRadius) * 2;
            Rectangle rect = {rectX, rectY, width, height};
            DrawRectangleRounded(rect, 1.0f, COMBAT_SHAPE_SEGMENTS, color);
            if (!handlesActive) return;
            DrawRectangleRoundedLines(rect, 1.0f, COMBAT_SHAPE_SEGMENTS, 0.0f, outlineColor);
            DrawHandle(x, y, outlineColor);
            DrawHandle(x + globalRadius, y, outlineColor);
            DrawHandle(x, y + globalHeight, outlineColor); 
        } return;

        default: return;
    }
}

Handle SelectCombatShapeHandle(Vector2 mousePos, Vector2 pos, float scale, CombatShape shape) {
    bool IsCollidingHandle(float x, float y) {
        Rectangle rect = {x - HANDLE_RADIUS, y - HANDLE_RADIUS, HANDLE_RADIUS * 2.0f, HANDLE_RADIUS * 2.0f};
        return CheckCollisionPointRec(mousePos, rect);
    }

    float centerGlobalX = pos.x + shape.x * scale;
    float centerGlobalY = pos.y + shape.y * scale;
    
    switch (shape.shapeType) {
        case CIRCLE: {
            float radiusGlobalX = pos.x + (shape.x + shape.data.circleRadius) * scale;
            if (IsCollidingHandle(radiusGlobalX, centerGlobalY))
                return CIRCLE_RADIUS;
        } break;

        case RECTANGLE: {
            float handleGlobalX = pos.x + (shape.x + shape.data.rectangle.rightX) * scale;
            float handleGlobalY = pos.y + (shape.y + shape.data.rectangle.bottomY) * scale;
            if (IsCollidingHandle(handleGlobalX, handleGlobalY))
                return RECTANGLE_CORNER;
        } break;

        case CAPSULE: {
            float heightHandleGlobalY = pos.y + (shape.y + shape.data.capsule.height) * scale;
            if (IsCollidingHandle(centerGlobalX, heightHandleGlobalY))
                return CAPSULE_HEIGHT;
            
            float radiusHandleGlobalX =  pos.x + (shape.x + shape.data.capsule.radius) * scale;
            if (IsCollidingHandle(radiusHandleGlobalX, centerGlobalY))
                return CAPSULE_RADIUS;
        } break;

        default: break;
    }


    if (IsCollidingHandle(centerGlobalX, centerGlobalY))
        return CENTER;
    return NONE;
}

bool SetCombatShapeHandle(Vector2 mousePos, Vector2 pos, float scale, CombatShape *shape, Handle handle) {
    float globalX = round((mousePos.x - pos.x) / scale);
    float globalY = round((mousePos.y - pos.y) / scale);

    float x = globalX > shape->x ? globalX - shape->x : 0;
    float y = globalY > shape->y ? globalY - shape->y : 0;
    switch (handle) {
        case CENTER:
            shape->x = globalX;
            shape->y = globalY;
            return true;
        
        case CIRCLE_RADIUS:
            if (shape->shapeType != CIRCLE) return false;
            shape->data.circleRadius = x;
            return true;
        
        case RECTANGLE_CORNER:
            if (shape->shapeType != RECTANGLE) return false;
            shape->data.rectangle.rightX = x;
            shape->data.rectangle.bottomY = y;
            return true;
        
        case CAPSULE_HEIGHT:
            if (shape->shapeType != CAPSULE) return false;
            shape->data.capsule.height = y;
            return true;
        
        case CAPSULE_RADIUS:
            if (shape->shapeType != CAPSULE) return false;
            shape->data.capsule.radius = x;
            return true;

        default:
            return false;
    }
}
