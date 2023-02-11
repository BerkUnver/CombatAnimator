#include <math.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
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
    cJSON_AddNumberToObject(json, STR_X, shape.transform.o.x);
    cJSON_AddNumberToObject(json, STR_Y, shape.transform.o.y);
    return json;
}

// all boilerplate
bool DeserializeShape(cJSON *json, CombatShape *out) {
    if (!cJSON_IsObject(json)) return false;
    
    out->transform = Transform2DIdentity();
    cJSON *xJson = cJSON_GetObjectItem(json, STR_X);
    if (!xJson || !cJSON_IsNumber(xJson)) return false;
    out->transform.o.x = (int) cJSON_GetNumberValue(xJson);

    cJSON *yJson = cJSON_GetObjectItem(json, STR_Y);
    if (!yJson || !cJSON_IsNumber(yJson)) return false;
    out->transform.o.y = (int) cJSON_GetNumberValue(yJson);

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

void DrawHandle(Vector2 pos, Color strokeColor) {
    DrawCircle(pos.x, pos.y, HANDLE_RADIUS, strokeColor);
    DrawCircle(pos.x, pos.y, HANDLE_INTERIOR_RADIUS, HANDLE_INTERIOR_COLOR);
}

void DrawCombatShape(Transform2D transform, CombatShape shape, bool handlesActive) {
    Color color;
    Color outlineColor;
    if (shape.boxType == HURTBOX) {
        color = HURTBOX_COLOR;
        outlineColor = HURTBOX_OUTLINE_COLOR;
    } else {
        color = HITBOX_COLOR;
        outlineColor = HITBOX_OUTLINE_COLOR;
    }

    Vector2 globalPos = Transform2DToGlobal(transform, shape.transform.o);

    switch (shape.shapeType) {
        case CIRCLE: {
            Vector2 radiusVector = {.x = shape.data.circleRadius, .y = 0.0f};
            Vector2 radiusPos = Transform2DBasisXFormInv(transform, radiusVector);
            float radius = Vector2Length(radiusPos);
            DrawCircle(globalPos.x, globalPos.y, radius, color);
            
            if (!handlesActive) break;
            DrawCircleLines(globalPos.x, globalPos.y, radius, outlineColor);
            DrawHandle(globalPos, outlineColor);
            DrawHandle(Vector2Add(globalPos, radiusPos), outlineColor);
        } break;

        case RECTANGLE: {
            Vector2 localHandlePos = {.x = shape.data.rectangle.rightX, .y = shape.data.rectangle.bottomY};
            Vector2 extents = Transform2DBasisXFormInv(transform, localHandlePos);
            Vector2 size = Vector2Scale(extents, 2.0f);
            Vector2 pos = Transform2DToGlobal(transform, Vector2Subtract(shape.transform.o, localHandlePos));
            Rectangle rect = {.x = pos.x, .y = pos.y, .width = size.x, .height = size.y};
            DrawRectangleRec(rect, color);

            if (!handlesActive) break;
            DrawRectangleLinesEx(rect, 1.0f, outlineColor);
            DrawHandle(globalPos, outlineColor);
            DrawHandle(Vector2Add(globalPos, extents), outlineColor);
        } break;

        case CAPSULE: {
            rlPushMatrix();
            rlTransform2DXForm(transform);
            rlPushMatrix();
            rlTransform2DXForm(shape.transform);
            float extentsY = shape.data.capsule.radius + shape.data.capsule.height;
            Rectangle rect = {
                .x = -shape.data.capsule.radius,
                .y = -extentsY,
                .width = shape.data.capsule.radius * 2.0f,
                .height = extentsY * 2.0f
            };
            DrawRectangleRounded(rect, 1.0f, COMBAT_SHAPE_SEGMENTS, color);

            if (handlesActive) DrawRectangleRoundedLines(rect, 1.0f, COMBAT_SHAPE_SEGMENTS, 0.0f, outlineColor);
            rlPopMatrix();
            rlPopMatrix();
            if (!handlesActive) break;

            Vector2 globalPos = Transform2DToGlobal(transform, shape.transform.o);
            Vector2 radius = {shape.data.capsule.radius, 0.0f};
            Vector2 height = {0.0f, shape.data.capsule.height};
            Vector2 globalRadius = Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(shape.transform, radius));
            Vector2 globalHeight = Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(shape.transform, height));
            DrawHandle(globalPos, outlineColor);
            DrawHandle(Vector2Add(globalRadius, globalPos), outlineColor);
            DrawHandle(Vector2Add(globalHeight, globalPos), outlineColor);
        } break;

        default: break;
    }

    if (shape.boxType == HITBOX) {
        Vector2 localKnockback = {.x = shape.hitboxKnockbackX, .y = shape.hitboxKnockbackY};
        Vector2 knockback = Transform2DToGlobal(transform, Vector2Add(shape.transform.o, localKnockback));
        DrawLine(globalPos.x, globalPos.y, knockback.x, knockback.y, outlineColor);
        if (handlesActive) DrawHandle(knockback, outlineColor);
    }
}

bool IsCollidingHandle(Transform2D transform, Vector2 globalMousePos, Vector2 pos) {
    Vector2 globalPos = Transform2DToGlobal(transform, pos);
    Rectangle rect = {globalPos.x - HANDLE_RADIUS, globalPos.y - HANDLE_RADIUS, HANDLE_RADIUS * 2.0f, HANDLE_RADIUS * 2.0f};
    return CheckCollisionPointRec(globalMousePos, rect);
}

Handle SelectCombatShapeHandle(Transform2D transform, Vector2 mousePos, CombatShape shape) {
    if (shape.boxType == HITBOX) {
        Vector2 handlePos = shape.transform.o;
        handlePos.x += shape.hitboxKnockbackX;
        handlePos.y += shape.hitboxKnockbackY;
        if (IsCollidingHandle(transform, mousePos, handlePos)) {
            return HITBOX_KNOCKBACK;
        }
    }

    switch (shape.shapeType) {
        case CIRCLE: {
            Vector2 radiusPos = shape.transform.o;
            radiusPos.x += shape.data.circleRadius;
            if (IsCollidingHandle(transform, mousePos, radiusPos))
                return CIRCLE_RADIUS;
        } break;

        case RECTANGLE: {
            Vector2 handlePos = shape.transform.o;
            handlePos.x += shape.data.rectangle.rightX;
            handlePos.y += shape.data.rectangle.bottomY;
            if (IsCollidingHandle(transform, mousePos, handlePos))
                return RECTANGLE_CORNER;
        } break;

        case CAPSULE: {
            Vector2 radiusPos = shape.transform.o;
            radiusPos.x += shape.data.capsule.radius;
            if (IsCollidingHandle(transform, mousePos, radiusPos))
                return CAPSULE_RADIUS;
            
            Vector2 heightPos = shape.transform.o;
            heightPos.y += shape.data.capsule.height;
            if (IsCollidingHandle(transform, mousePos, heightPos))
                return CAPSULE_HEIGHT;
        } break;

        default: break;
    }


    if (IsCollidingHandle(transform, mousePos, shape.transform.o))
        return CENTER;
    return NONE;
}

float Max(float i, float j) { return i > j ? i : j; }

Vector2 Vector2Round(Vector2 vec) {
    return (Vector2) {.x = roundf(vec.x), .y = roundf(vec.y)};
}

Vector2 Vector2Max(Vector2 vec, float max) {
    return (Vector2) {.x = Max(vec.x, max), .y = Max(vec.y, max)};
}

bool SetCombatShapeHandle(Vector2 localMousePos, CombatShape *shape, Handle handle) {
    Vector2 handlePos = Vector2Round(Vector2Max(Vector2Subtract(localMousePos, shape->transform.o), 0.0f));

    switch (handle) {
        case CENTER:
            shape->transform.o = Vector2Round(localMousePos);
            return true;

        case HITBOX_KNOCKBACK:
            if (shape->boxType != HITBOX) return false;
            Vector2 knockback = Vector2Subtract(localMousePos, shape->transform.o);
            shape->hitboxKnockbackX = knockback.x;
            shape->hitboxKnockbackY = knockback.y;
            return true;

        case CIRCLE_RADIUS:
            if (shape->shapeType != CIRCLE) return false;
            shape->data.circleRadius = handlePos.x;
            return true;

        case RECTANGLE_CORNER:
            if (shape->shapeType != RECTANGLE) return false;
            shape->data.rectangle.rightX = handlePos.x;
            shape->data.rectangle.bottomY = handlePos.y;
            return true;

        case CAPSULE_RADIUS:
            if (shape->shapeType != CAPSULE) return false;
            shape->data.capsule.radius = handlePos.x;
            return true;

        case CAPSULE_HEIGHT:
            if (shape->shapeType != CAPSULE) return false;
            shape->data.capsule.height = handlePos.y;
            return true;

        default:
            return false;
    }
}
