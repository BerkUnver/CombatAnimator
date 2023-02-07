#include <math.h>
#include <string.h>
#include "raylib.h"
#include "rlgl.h"
#include "cjson/cJSON.h"
#include "combat_shape.h"
#include "transform_2d.h"

cJSON *SerializeShape(CombatShape shape) {
    const char *shapeType;
    cJSON *json = cJSON_CreateObject();
    switch (shape.shapeType) {
        case CIRCLE:
            shapeType = STR_CIRCLE;
            cJSON_AddNumberToObject(json, STR_CIRCLE_RADIUS, shape.data.circleRadius);
            break;

        case RECTANGLE:
            shapeType = STR_RECTANGLE;
            cJSON_AddNumberToObject(json, STR_RECTANGLE_RIGHT_X, shape.data.rectangle.rightX);
            cJSON_AddNumberToObject(json, STR_RECTANGLE_BOTTOM_Y, shape.data.rectangle.bottomY);
            break;

        case CAPSULE:
            shapeType = STR_CAPSULE;
            cJSON_AddNumberToObject(json, STR_CAPSULE_RADIUS, shape.data.capsule.radius);
            cJSON_AddNumberToObject(json, STR_CAPSULE_HEIGHT, shape.data.capsule.height);
            break;

        default:
            cJSON_Delete(json);
            return NULL;
    }
    cJSON_AddStringToObject(json, STR_SHAPE_TYPE, shapeType);

    const char *boxType;
    switch(shape.boxType) {
        case HITBOX:
            boxType = STR_HITBOX;
            cJSON_AddNumberToObject(json, STR_HITBOX_KNOCKBACK_X, shape.hitboxKnockbackX);
            cJSON_AddNumberToObject(json, STR_HITBOX_KNOCKBACK_Y, shape.hitboxKnockbackY);
            break;

        case HURTBOX:
            boxType = STR_HURTBOX;
            break;

        default:
            cJSON_Delete(json);
            return NULL;
    }

    cJSON_AddStringToObject(json, STR_BOX_TYPE, boxType);
    cJSON_AddNumberToObject(json, STR_X, shape.x);
    cJSON_AddNumberToObject(json, STR_Y, shape.y);
    return json;
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
    if (strcmp(shapeTypeStr, STR_CIRCLE) == 0) {
        out->shapeType = CIRCLE;
        cJSON *radius = cJSON_GetObjectItem(json, STR_CIRCLE_RADIUS);
        if (!radius || !cJSON_IsNumber(radius)) return false;
        out->data.circleRadius = (int) cJSON_GetNumberValue(radius);

    } else if (strcmp(shapeTypeStr, STR_RECTANGLE) == 0) {
        out->shapeType = RECTANGLE;
        cJSON *rightX = cJSON_GetObjectItem(json, STR_RECTANGLE_RIGHT_X);
        if (!rightX || !cJSON_IsNumber(rightX)) return false;
        cJSON *bottomY = cJSON_GetObjectItem(json, STR_RECTANGLE_BOTTOM_Y);
        if (!bottomY || !cJSON_IsNumber(bottomY)) return false;
        out->data.rectangle.rightX = (int) cJSON_GetNumberValue(rightX);
        out->data.rectangle.bottomY = (int) cJSON_GetNumberValue(bottomY);

    } else if (strcmp(shapeTypeStr, STR_CAPSULE) == 0) {
        out->shapeType = CAPSULE;
        cJSON *radius = cJSON_GetObjectItem(json, STR_CAPSULE_RADIUS);
        if (!radius || !cJSON_IsNumber(radius)) return false;
        cJSON *height = cJSON_GetObjectItem(json, STR_CAPSULE_HEIGHT);
        if (!height || !cJSON_IsNumber(height)) return false;
        out->data.capsule.rotation = 0.0f; // todo : remove and add proper serialization and deserialization.
        out->data.capsule.radius = (int) cJSON_GetNumberValue(radius);
        out->data.capsule.height = (int) cJSON_GetNumberValue(height);
    }
    else return false;

    cJSON *boxTypeJson = cJSON_GetObjectItem(json, STR_BOX_TYPE);
    if (!boxTypeJson || !cJSON_IsString(boxTypeJson)) return false;
    char *boxTypeStr = cJSON_GetStringValue(boxTypeJson);
    if (strcmp(boxTypeStr, STR_HITBOX) == 0) {
        cJSON *knockbackX = cJSON_GetObjectItem(json, STR_HITBOX_KNOCKBACK_X);
        if (!knockbackX || !cJSON_IsNumber(knockbackX)) return false;
        cJSON *knockbackY = cJSON_GetObjectItem(json, STR_HITBOX_KNOCKBACK_Y);
        if (!knockbackY || !cJSON_IsNumber(knockbackY)) return false;

        out->boxType = HITBOX;
        out->hitboxKnockbackX = (int) cJSON_GetNumberValue(knockbackX);
        out->hitboxKnockbackY = (int) cJSON_GetNumberValue(knockbackY);
    } else if (strcmp(boxTypeStr, STR_HURTBOX) == 0) {
        out->boxType = HURTBOX;
    } else return false;

    return true;
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
            if (!handlesActive) break;
            DrawCircleLines(x, y, r, outlineColor);
            DrawHandle(x, y, outlineColor);
            DrawHandle(x + r, y, outlineColor);
        } break;

        case RECTANGLE: {
            float rightX = shape.data.rectangle.rightX * scale;
            float bottomY = shape.data.rectangle.bottomY * scale;
            float rectX = x - rightX;
            float rectY = y - bottomY;
            float width = rightX * 2;
            float height = bottomY * 2;
            DrawRectangle(rectX, rectY, width, height, color);
            if (!handlesActive) break;
            DrawRectangleLines(rectX, rectY, width, height, outlineColor);
            DrawHandle(x, y, outlineColor);
            DrawHandle(x + rightX, y + bottomY, outlineColor);
        } break;

        case CAPSULE: {
            float globalHeight = shape.data.capsule.height * scale;
            float globalRadius = shape.data.capsule.radius * scale;
            float rectX = x - globalRadius;
            float rectY = y - globalHeight - globalRadius;
            float width = globalRadius * 2;
            float height = (globalHeight + globalRadius) * 2;
            Rectangle rect = {rectX, rectY, width, height};
            DrawRectangleRounded(rect, 1.0f, COMBAT_SHAPE_SEGMENTS, color);
            if (!handlesActive) break;
            DrawRectangleRoundedLines(rect, 1.0f, COMBAT_SHAPE_SEGMENTS, 0.0f, outlineColor);
            DrawHandle(x, y, outlineColor);
            DrawHandle(x + globalRadius, y, outlineColor);
            DrawHandle(x, y + globalHeight, outlineColor);
        } break;

        default: break;
    }

    if (shape.boxType == HITBOX) {
        int knockbackX = pos.x + (shape.x + shape.hitboxKnockbackX) * scale;
        int knockbackY = pos.y + (shape.y + shape.hitboxKnockbackY) * scale;
        DrawLine(x, y, knockbackX, knockbackY, outlineColor);
        if (handlesActive) DrawHandle(knockbackX, knockbackY, outlineColor);
    }
}

bool IsCollidingHandle(Transform2D transform, Vector2 globalMousePos, Vector2 pos) {
    Vector2 globalPos = Transform2DToGlobal(transform, pos);
    Rectangle rect = {globalPos.x - HANDLE_RADIUS, globalPos.y - HANDLE_RADIUS, HANDLE_RADIUS * 2.0f, HANDLE_RADIUS * 2.0f};
    return CheckCollisionPointRec(globalMousePos, rect);
}

Handle SelectCombatShapeHandle(Transform2D transform, Vector2 mousePos, CombatShape shape) {
    if (shape.boxType == HITBOX) {
        Vector2 handlePos = {.x = shape.x + shape.hitboxKnockbackX, .y = shape.y + shape.hitboxKnockbackY};
        if (IsCollidingHandle(transform, mousePos, handlePos)) {
            return HITBOX_KNOCKBACK;
        }
    }

    switch (shape.shapeType) {
        case CIRCLE: {
            Vector2 radiusPos = {.x = shape.x + shape.data.circleRadius, .y = shape.y};
            if (IsCollidingHandle(transform, mousePos, radiusPos))
                return CIRCLE_RADIUS;
        } break;

        case RECTANGLE: {
            Vector2 handlePos = {.x = shape.x + shape.data.rectangle.rightX, .y = shape.y + shape.data.rectangle.bottomY};
            if (IsCollidingHandle(transform, mousePos, handlePos))
                return RECTANGLE_CORNER;
        } break;

        case CAPSULE: {
            Vector2 radiusPos = {.x = shape.x + shape.data.capsule.radius, .y = shape.y};
            if (IsCollidingHandle(transform, mousePos, radiusPos))
                return CAPSULE_RADIUS;
            
            Vector2 heightPos = {.x = shape.x, .y = shape.y + shape.data.capsule.height};
            if (IsCollidingHandle(transform, mousePos, heightPos))
                return CAPSULE_HEIGHT;
        } break;

        default: break;
    }


    Vector2 center = {.x = shape.x, .y = shape.y};
    if (IsCollidingHandle(transform, mousePos, center))
        return CENTER;
    return NONE;
}

float Max(float i, float j) { return i > j ? i : j; }

#include <stdio.h>
bool SetCombatShapeHandle(Vector2 localMousePos, CombatShape *shape, Handle handle) {
    printf("local mouse pos: (%f, %f)\n", localMousePos.x, localMousePos.y);
    fflush(stdout);
    int x = (int) roundf(Max(localMousePos.x - (float) shape->x, 0.0f));
    int y = (int) roundf(Max(localMousePos.y - (float) shape->y, 0.0f));

    switch (handle) {
        case CENTER:
            shape->x = roundf(localMousePos.x);
            shape->y = roundf(localMousePos.y);
            return true;

        case HITBOX_KNOCKBACK:
            if (shape->boxType != HITBOX) return false;
            shape->hitboxKnockbackX = roundf(localMousePos.x - shape->x);
            shape->hitboxKnockbackY = roundf(localMousePos.y - shape->y);
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

        case CAPSULE_RADIUS:
            if (shape->shapeType != CAPSULE) return false;
            shape->data.capsule.radius = x;
            return true;

        case CAPSULE_HEIGHT:
            if (shape->shapeType != CAPSULE) return false;
            shape->data.capsule.radius = y;
            return true;

        default:
            return false;
    }
}
