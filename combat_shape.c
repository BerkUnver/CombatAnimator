#include <math.h>
#include <stdio.h>
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
            cJSON_AddNumberToObject(json, STR_CAPSULE_ROTATION, Transform2DGetRotation(shape.transform));
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
        cJSON *rotation = cJSON_GetObjectItem(json, STR_CAPSULE_ROTATION);
        if (!rotation || !cJSON_IsNumber(rotation)) return false;

        out->data.capsule.radius = (int) cJSON_GetNumberValue(radius);
        out->data.capsule.height = (int) cJSON_GetNumberValue(height);
        out->transform = Transform2DRotate(out->transform, (float) cJSON_GetNumberValue(rotation));
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
    rlPushMatrix();
    rlTransform2DXForm(transform);
    
    // todo: The following is a hack to account for Transform2DToMatrix being broken.
    // We translate and later rotate the matrix instead. 
    rlTranslatef(shape.transform.o.x, shape.transform.o.y, 0.0f); 
    
    switch (shape.shapeType) {
        case CIRCLE:
            DrawCircle(0, 0, shape.data.circleRadius, color);
            if (handlesActive) DrawCircleLines(0, 0, shape.data.circleRadius, outlineColor);
            break;
        case RECTANGLE: {
            int x = -shape.data.rectangle.rightX;
            int y = -shape.data.rectangle.bottomY;
            int width = 2 * shape.data.rectangle.rightX;
            int height = 2 * shape.data.rectangle.bottomY;
            DrawRectangle(x, y, width, height, color);
            if (handlesActive) DrawRectangleLines(x, y, width, height, outlineColor);
        } break;
        case CAPSULE: {
            float rotation = Transform2DGetRotation(shape.transform) * 180.0f / PI;
            printf("Rotation(degrees): %f\n", rotation);
            fflush(stdout);
            rlRotatef(rotation, 0.0f, 0.0f, 1.0f);
            Rectangle rect = {
                .x = -shape.data.capsule.radius,
                .y = -shape.data.capsule.radius - shape.data.capsule.height,
                .width = shape.data.capsule.radius * 2.0f,
                .height = (shape.data.capsule.radius + shape.data.capsule.height) * 2.0f
            };
            DrawRectangleRounded(rect, 1.0f, COMBAT_SHAPE_SEGMENTS, color);
            if (handlesActive) DrawRectangleRoundedLines(rect, 1.0f, COMBAT_SHAPE_SEGMENTS, 0.0f, outlineColor);
        } break;
        default: break;
    }

    if (shape.boxType == HITBOX) DrawLine(0, 0, shape.hitboxKnockbackX, shape.hitboxKnockbackY, outlineColor);
    rlPopMatrix();
    
    if (!handlesActive) return;
    
    Vector2 globalPos = Transform2DToGlobal(transform, shape.transform.o);
    DrawHandle(globalPos, outlineColor);
    
    switch (shape.shapeType) {
        case CIRCLE: {
            Vector2 radiusVector = {.x = shape.data.circleRadius, .y = 0.0f};
            Vector2 radiusPos = Transform2DBasisXFormInv(transform, radiusVector);        
            DrawHandle(Vector2Add(globalPos, radiusPos), outlineColor);
        } break;

        case RECTANGLE: {
            Vector2 localHandlePos = {.x = shape.data.rectangle.rightX, .y = shape.data.rectangle.bottomY};
            Vector2 extents = Transform2DBasisXFormInv(transform, localHandlePos);
            DrawHandle(Vector2Add(globalPos, extents), outlineColor);
        } break;

        case CAPSULE: {
            Vector2 radius = {shape.data.capsule.radius, 0.0f};
            Vector2 height = {0.0f, shape.data.capsule.height};
            Vector2 rotationHandle = {0.0f, -shape.data.capsule.height};
            
            // not sure why I am using basis xform inv here
            Vector2 globalRadius = Vector2Add(globalPos, Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(shape.transform, radius)));
            Vector2 globalHeight = Vector2Add(globalPos, Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(shape.transform, height)));
            Vector2 globalRotation = Vector2Add(globalPos, Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(shape.transform, rotationHandle)));

            DrawHandle(globalRadius, outlineColor);
            DrawHandle(globalHeight, outlineColor);
            DrawHandle(globalRotation, outlineColor);
        } break;
    }
}

bool IsCollidingHandle(Transform2D globalTransform, Vector2 globalMousePos, Vector2 localHandlePos) {
    Vector2 globalHandlePos = Transform2DToGlobal(globalTransform, localHandlePos);
    Rectangle rect = {
        .x = globalHandlePos.x - HANDLE_RADIUS,
        .y = globalHandlePos.y - HANDLE_RADIUS,
        .height = 2.0f * HANDLE_RADIUS,
        .width = 2.0f * HANDLE_RADIUS
    };
    return CheckCollisionPointRec(globalMousePos, rect);
}

Handle SelectCombatShapeHandle(Transform2D transform, Vector2 mousePos, CombatShape shape) {
    if (shape.boxType == HITBOX) {
        Vector2 knockbackPos = {shape.transform.o.x + shape.hitboxKnockbackX, shape.transform.o.y + shape.hitboxKnockbackY};
        if (IsCollidingHandle(transform, mousePos, knockbackPos)) return HITBOX_KNOCKBACK;
    }

    switch (shape.shapeType) {
        case CIRCLE: {
            Vector2 radiusPos = Transform2DToGlobal(shape.transform, (Vector2) {shape.data.circleRadius, 0.0f});
            if (IsCollidingHandle(transform, mousePos, radiusPos)) return CIRCLE_RADIUS;
        } break;
        
        case RECTANGLE: {
            Vector2 cornerPos = Transform2DToGlobal(shape.transform, (Vector2) {shape.data.rectangle.rightX, shape.data.rectangle.bottomY});
            if (IsCollidingHandle(transform, mousePos, cornerPos)) return RECTANGLE_CORNER;
        } break;

        case CAPSULE: {
            Vector2 radiusPos = Transform2DToGlobal(shape.transform, (Vector2) {shape.data.capsule.radius, 0.0f});
            if (IsCollidingHandle(transform, mousePos, radiusPos)) return CAPSULE_RADIUS;

            Vector2 heightPos = Transform2DToGlobal(shape.transform, (Vector2) {0.0f, shape.data.capsule.height});
            if (IsCollidingHandle(transform, mousePos, heightPos)) return CAPSULE_HEIGHT;

            Vector2 rotationPos = Transform2DToGlobal(shape.transform, (Vector2) {0.0f, -shape.data.capsule.height});
            if (IsCollidingHandle(transform, mousePos, rotationPos)) return CAPSULE_ROTATION;
        } break;

        default: break;
    }

    if (IsCollidingHandle(transform, mousePos, shape.transform.o)) return CENTER;
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
    Vector2 handlePos = Vector2Round(Vector2Max(Transform2DToLocal(shape->transform, localMousePos), 0.0f));
    Vector2 offset = Vector2Subtract(localMousePos, shape->transform.o);

    switch (handle) {
        case CENTER:
            shape->transform.o = Vector2Round(localMousePos);
            return true;

        case HITBOX_KNOCKBACK:
            if (shape->boxType != HITBOX) return false;
            Vector2 knockback = Vector2Round(offset);
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
        
        case CAPSULE_ROTATION:
            if (shape->shapeType != CAPSULE) return false;
            float rotation = atan2(offset.y, offset.x) + PI / 2.0f;
            shape->transform = Transform2DRotate(shape->transform, rotation);
        
        default:
            return false;
    }
}
