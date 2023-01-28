#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "raylib.h"
#include "rlgl.h"
#include "cjson/cJSON.h"
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


cJSON *SerializeShape(CombatShape shape) {
    const char *boxType;
    switch(shape.boxType) {
        case HITBOX: boxType = "HITBOX"; break;
        case HURTBOX: boxType = "HURTBOX"; break;
        default: return NULL;
    }

    const char *shapeType;
    cJSON *data = cJSON_CreateObject();
    switch (shape.shapeType) {
        case CIRCLE:
            shapeType = "CIRCLE";
            cJSON_AddNumberToObject(data, "radius", shape.data.circleRadius);
            break;

        case RECTANGLE:
            shapeType = "RECTANGLE";
            cJSON_AddNumberToObject(data, "rightX", shape.data.rectangle.rightX);
            cJSON_AddNumberToObject(data, "bottomY", shape.data.rectangle.bottomY);
            break;

        case CAPSULE:
            shapeType = "CAPSULE";
            cJSON_AddNumberToObject(data, "radius", shape.data.capsule.radius);
            cJSON_AddNumberToObject(data, "height", shape.data.capsule.height);
            break;

        default:
            cJSON_Delete(data);
            return NULL;
    }

    cJSON *jsonShape = cJSON_CreateObject();
    cJSON_AddStringToObject(jsonShape,"shapeType", shapeType);
    cJSON_AddStringToObject(jsonShape, "boxType", boxType);
    cJSON_AddItemToObject(jsonShape, "data", data);
    return jsonShape;
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
            if (handlesActive) {
                DrawRectangleRoundedLines(rect, 1.0f, COMBAT_SHAPE_SEGMENTS, 0.0f, outlineColor);
                DrawHandle(0, 0, outlineColor);
                DrawHandle(globalRadius, 0, outlineColor);
                DrawHandle(0, globalHeight, outlineColor); 
            }
            rlPopMatrix();
        } return;

        default: return;
    }
}

bool IsCollidingHandle(Vector2 mousePos, float x, float y) {
    Rectangle rect = {x - HANDLE_RADIUS, y - HANDLE_RADIUS, HANDLE_RADIUS * 2.0f, HANDLE_RADIUS * 2.0f};
    return CheckCollisionPointRec(mousePos, rect);
}
Handle SelectCombatShapeHandle(Vector2 mousePos, Vector2 pos, float scale, CombatShape shape) {
    float centerGlobalX = pos.x + shape.x * scale;
    float centerGlobalY = pos.y + shape.y * scale;
    
    switch (shape.shapeType) {
        case CIRCLE: {
            float radiusGlobalX = pos.x + (shape.x + shape.data.circleRadius) * scale;
            if (IsCollidingHandle(mousePos, radiusGlobalX, centerGlobalY))
                return CIRCLE_RADIUS;
        } break;

        case RECTANGLE: {
            float handleGlobalX = pos.x + (shape.x + shape.data.rectangle.rightX) * scale;
            float handleGlobalY = pos.y + (shape.y + shape.data.rectangle.bottomY) * scale;
            if (IsCollidingHandle(mousePos,handleGlobalX, handleGlobalY))
                return RECTANGLE_CORNER;
        } break;

        case CAPSULE: {
            float heightX = pos.x + (shape.x + shape.data.capsule.height) * scale;
            float heightY = pos.y + (shape.y + shape.data.capsule.height) * scale;
            if (IsCollidingHandle(mousePos, heightX, heightY))
                return CAPSULE_HEIGHT;
            
            float radiusX = pos.x + (shape.x + shape.data.capsule.radius) * scale;
            float radiusY = pos.y + (shape.y + shape.data.capsule.radius) * scale;
            if (IsCollidingHandle(mousePos, radiusX, radiusY))
                return CAPSULE_RADIUS;
        } break;

        default: break;
    }


    if (IsCollidingHandle(mousePos, centerGlobalX, centerGlobalY))
        return CENTER;
    return NONE;
}

float Max(float i, float j) { return i > j ? i : j; }

bool SetCombatShapeHandle(Vector2 mousePos, Vector2 pos, float scale, CombatShape *shape, Handle handle) {
    float localX = (mousePos.x - pos.x) / scale;
    float localY = (mousePos.y - pos.y) / scale;

    float x = roundf(Max(localX - shape->x, 0.0f));
    float y = roundf(Max(localY - shape->y, 0.0f));

    if (handle == CENTER) {
        shape->x = roundf(localX);
        shape->y = roundf(localY);
        return true;
    }

    switch (shape->shapeType) {
        case CIRCLE:
            if (handle != CIRCLE_RADIUS) return false;
            shape->data.circleRadius = x;
            return true;
        
        case RECTANGLE:
            if (handle != RECTANGLE_CORNER) return false;
            shape->data.rectangle.rightX = x;
            shape->data.rectangle.bottomY = y;
            return true;

        case CAPSULE:
            if (handle == CAPSULE_RADIUS) {
                shape->data.capsule.radius = x;
                return true;
            } else if (handle == CAPSULE_HEIGHT) {
                shape->data.capsule.height = y;
                return true;
            } else {
                return false;
            }
        default: return false;
    }
}
