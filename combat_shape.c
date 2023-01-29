#include <math.h>
#include <string.h>
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
        case HITBOX: boxType = STR_HITBOX; break;
        case HURTBOX: boxType = STR_HURTBOX ; break;
        default: return NULL;
    }

    const char *shapeType;
    cJSON *data = cJSON_CreateObject();
    switch (shape.shapeType) {
        case CIRCLE:
            shapeType = STR_CIRCLE;
            cJSON_AddNumberToObject(data, STR_CIRCLE_RADIUS, shape.data.circleRadius);
            break;

        case RECTANGLE:
            shapeType = STR_RECTANGLE;
            cJSON_AddNumberToObject(data, STR_RECTANGLE_RIGHT_X, shape.data.rectangle.rightX);
            cJSON_AddNumberToObject(data, STR_RECTANGLE_BOTTOM_Y, shape.data.rectangle.bottomY);
            break;

        case CAPSULE:
            shapeType = STR_CAPSULE;
            cJSON_AddNumberToObject(data, STR_CAPSULE_RADIUS, shape.data.capsule.radius);
            cJSON_AddNumberToObject(data, STR_CAPSULE_HEIGHT, shape.data.capsule.height);
            break;

        default:
            cJSON_Delete(data);
            return NULL;
    }

    cJSON *jsonShape = cJSON_CreateObject();
    cJSON_AddStringToObject(jsonShape,STR_SHAPE_TYPE, shapeType);
    cJSON_AddStringToObject(jsonShape, STR_BOX_TYPE, boxType);
    cJSON_AddItemToObject(jsonShape, STR_DATA, data);
    cJSON_AddNumberToObject(jsonShape, STR_X, shape.x);
    cJSON_AddNumberToObject(jsonShape, STR_Y, shape.y);
    return jsonShape;
}

// all boilerplate
bool DeserializeShape(cJSON *json, CombatShape *out) {
    if (!cJSON_IsObject(json)) return false;

    cJSON *xJson = cJSON_GetObjectItem(json, STR_X);
    if (!xJson || !cJSON_IsNumber(xJson)) return false;
    out->x = (int) cJSON_GetNumberValue(xJson);

    cJSON *yJson = cJSON_GetObjectItem(json, STR_Y);
    if (!yJson || !cJSON_IsNumber(yJson)) return false;
    out->y = (int) cJSON_GetNumberValue(yJson);

    cJSON *shapeTypeJson = cJSON_GetObjectItem(json, STR_SHAPE_TYPE);
    if (!shapeTypeJson || !cJSON_IsString(shapeTypeJson)) return false;
    char *shapeTypeStr = cJSON_GetStringValue(shapeTypeJson);
    if (strcmp(shapeTypeStr, STR_CIRCLE) == 0) out->shapeType = CIRCLE;
    else if (strcmp(shapeTypeStr, STR_RECTANGLE) == 0) out->shapeType = RECTANGLE;
    else if (strcmp(shapeTypeStr, STR_CAPSULE) == 0) out->shapeType = CAPSULE;
    else return false;

    cJSON *boxTypeJson = cJSON_GetObjectItem(json, STR_BOX_TYPE);
    if (!boxTypeJson || !cJSON_IsString(boxTypeJson)) return false;
    char *boxTypeStr = cJSON_GetStringValue(boxTypeJson);
    if (strcmp(boxTypeStr, STR_HITBOX) == 0) out->boxType = HITBOX;
    else if (strcmp(boxTypeStr, STR_HURTBOX) == 0) out->boxType = HURTBOX;
    else return false;

    cJSON *data = cJSON_GetObjectItem(json, STR_DATA);
    if (!data || !cJSON_IsObject(data)) return false;
    switch (out->shapeType) {
        case CIRCLE: {
            cJSON *radius = cJSON_GetObjectItem(data, STR_CIRCLE_RADIUS);
            if (!radius || !cJSON_IsNumber(radius)) return false;
            out->data.circleRadius = (int) cJSON_GetNumberValue(radius);
        } return true;

        case RECTANGLE: {
            cJSON *rightX = cJSON_GetObjectItem(data, STR_RECTANGLE_RIGHT_X);
            if (!rightX || !cJSON_IsNumber(rightX)) return false;
            cJSON *bottomY = cJSON_GetObjectItem(data, STR_RECTANGLE_BOTTOM_Y);
            if (!bottomY || !cJSON_IsNumber(bottomY)) return false;
            out->data.rectangle.rightX = (int) cJSON_GetNumberValue(rightX);
            out->data.rectangle.bottomY = (int) cJSON_GetNumberValue(bottomY);
        } return true;

        case CAPSULE: {
            cJSON *radius = cJSON_GetObjectItem(data, STR_CAPSULE_RADIUS);
            if (!radius || !cJSON_IsNumber(radius)) return false;
            cJSON *height = cJSON_GetObjectItem(data, STR_CAPSULE_HEIGHT);
            if (!height || !cJSON_IsNumber(height)) return false;
            out->data.capsule.radius = (int) cJSON_GetNumberValue(radius);
            out->data.capsule.height = (int) cJSON_GetNumberValue(height);
        } return true;

        default: return false; // unreachable
    }
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
                DrawHandle(x, y, outlineColor);
                DrawHandle(x + globalRadius, y, outlineColor);
                DrawHandle(x, y + globalRadius, outlineColor);
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
            float radiusX = pos.x + (shape.x + shape.data.capsule.radius) * scale;
            if (IsCollidingHandle(mousePos, radiusX, centerGlobalY))
                return CAPSULE_RADIUS;

            float heightY = pos.y + (shape.y + shape.data.capsule.height) * scale;
            if (IsCollidingHandle(mousePos, centerGlobalX, heightY))
                return CAPSULE_HEIGHT;
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
