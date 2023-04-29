#include <assert.h>
#include <math.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "cjson/cJSON.h"
#include "layer.h"
#include "transform_2d.h"

Layer LayerNew(ShapeType shapeType, LayerType layerType) {
    Layer shape;
    switch (shapeType) {
        case SHAPE_CIRCLE:
            shape.data.circleRadius = 24;
            break;
        case SHAPE_RECTANGLE:
            shape.data.rectangle.rightX = 24;
            shape.data.rectangle.bottomY = 24;
            break;
        case SHAPE_CAPSULE:
            shape.data.capsule.radius = 24;
            shape.data.capsule.height = 24;
            break;
        default:
            assert(false);
    }
    shape.shapeType = shapeType;

    switch (layerType) {
        case LAYER_TYPE_HURTBOX: break;
        case LAYER_TYPE_HITBOX:
            shape.hitboxStun = 1000;
            shape.hitboxDamage = 0;
            shape.hitboxKnockbackX = 2;
            shape.hitboxKnockbackY = -2;
            break;
        default:
            assert(false);
    }
    shape.transform = Transform2DIdentity();
    shape.type = layerType;
    return shape;
}

cJSON *LayerSerialize(Layer layer) {
    const char *shapeType;
    cJSON *json = cJSON_CreateObject();
    switch (layer.shapeType) {
        case SHAPE_CIRCLE:
            shapeType = STR_CIRCLE;
            cJSON_AddNumberToObject(json, STR_CIRCLE_RADIUS, layer.data.circleRadius);
            break;

        case SHAPE_RECTANGLE:
            shapeType = STR_RECTANGLE;
            cJSON_AddNumberToObject(json, STR_RECTANGLE_RIGHT_X, layer.data.rectangle.rightX);
            cJSON_AddNumberToObject(json, STR_RECTANGLE_BOTTOM_Y, layer.data.rectangle.bottomY);
            break;

        case SHAPE_CAPSULE:
            shapeType = STR_CAPSULE;
            cJSON_AddNumberToObject(json, STR_CAPSULE_RADIUS, layer.data.capsule.radius);
            cJSON_AddNumberToObject(json, STR_CAPSULE_HEIGHT, layer.data.capsule.height);
            cJSON_AddNumberToObject(json, STR_CAPSULE_ROTATION, Transform2DGetRotation(layer.transform));
            break;

        default:
            goto FAIL;
    }
    cJSON_AddStringToObject(json, STR_SHAPE_TYPE, shapeType);

    const char *layerType;
    switch (layer.type) {
        case LAYER_TYPE_HITBOX:
            layerType = STR_HITBOX;
            cJSON_AddNumberToObject(json, STR_HITBOX_STUN, layer.hitboxStun);
            cJSON_AddNumberToObject(json, STR_HITBOX_DAMAGE, layer.hitboxDamage);
            cJSON_AddNumberToObject(json, STR_HITBOX_KNOCKBACK_X, layer.hitboxKnockbackX);
            cJSON_AddNumberToObject(json, STR_HITBOX_KNOCKBACK_Y, layer.hitboxKnockbackY);
            break;

        case LAYER_TYPE_HURTBOX:
            layerType = STR_HURTBOX;
            break;

        default:
            goto FAIL;
    }

    cJSON_AddStringToObject(json, STR_BOX_TYPE, layerType);
    cJSON_AddNumberToObject(json, STR_X, layer.transform.o.x);
    cJSON_AddNumberToObject(json, STR_Y, layer.transform.o.y);
    return json;
    FAIL:

    cJSON_Delete(json);
    return NULL;
}

// all boilerplate
bool LayerDeserialize(cJSON *json, int version, Layer *out) {
    if (!cJSON_IsObject(json)) return false;
    
    out->transform = Transform2DIdentity();
    cJSON *xJson = cJSON_GetObjectItem(json, STR_X);
    if (!xJson || !cJSON_IsNumber(xJson)) return false;
    out->transform.o.x = (float) (int) cJSON_GetNumberValue(xJson);

    cJSON *yJson = cJSON_GetObjectItem(json, STR_Y);
    if (!yJson || !cJSON_IsNumber(yJson)) return false;
    out->transform.o.y = (float) (int) cJSON_GetNumberValue(yJson);

    cJSON *shapeTypeJson = cJSON_GetObjectItem(json, STR_SHAPE_TYPE);
    if (!shapeTypeJson || !cJSON_IsString(shapeTypeJson)) return false;
    char *shapeTypeStr = cJSON_GetStringValue(shapeTypeJson);
    if (strcmp(shapeTypeStr, STR_CIRCLE) == 0) {
        out->shapeType = SHAPE_CIRCLE;
        cJSON *radius = cJSON_GetObjectItem(json, STR_CIRCLE_RADIUS);
        if (!radius || !cJSON_IsNumber(radius)) return false;
        out->data.circleRadius = (int) cJSON_GetNumberValue(radius);

    } else if (strcmp(shapeTypeStr, STR_RECTANGLE) == 0) {
        out->shapeType = SHAPE_RECTANGLE;
        cJSON *rightX = cJSON_GetObjectItem(json, STR_RECTANGLE_RIGHT_X);
        if (!rightX || !cJSON_IsNumber(rightX)) return false;
        cJSON *bottomY = cJSON_GetObjectItem(json, STR_RECTANGLE_BOTTOM_Y);
        if (!bottomY || !cJSON_IsNumber(bottomY)) return false;
        out->data.rectangle.rightX = (int) cJSON_GetNumberValue(rightX);
        out->data.rectangle.bottomY = (int) cJSON_GetNumberValue(bottomY);

    } else if (strcmp(shapeTypeStr, STR_CAPSULE) == 0) {
        out->shapeType = SHAPE_CAPSULE;
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
    if (!strcmp(boxTypeStr, STR_HITBOX)) {
        cJSON *knockbackX = cJSON_GetObjectItem(json, STR_HITBOX_KNOCKBACK_X);
        if (!knockbackX || !cJSON_IsNumber(knockbackX)) return false;
        cJSON *knockbackY = cJSON_GetObjectItem(json, STR_HITBOX_KNOCKBACK_Y);
        if (!knockbackY || !cJSON_IsNumber(knockbackY)) return false;

        if (version >= 3) {
            cJSON *damage = cJSON_GetObjectItem(json, STR_HITBOX_DAMAGE);
            if (!damage || !cJSON_IsNumber(damage)) return false;
            cJSON *stun = cJSON_GetObjectItem(json, STR_HITBOX_STUN);
            if (!stun || !cJSON_IsNumber(stun)) return false;

            out->hitboxDamage = (int) cJSON_GetNumberValue(damage);
            out->hitboxStun = (int) cJSON_GetNumberValue(stun);
        } else {
            out->hitboxStun = 1000;
            out->hitboxDamage = 0;
        }

        out->type = LAYER_TYPE_HITBOX;
        out->hitboxKnockbackX = (int) cJSON_GetNumberValue(knockbackX);
        out->hitboxKnockbackY = (int) cJSON_GetNumberValue(knockbackY);

    } else if (!strcmp(boxTypeStr, STR_HURTBOX)) {
        out->type = LAYER_TYPE_HURTBOX;
    } else return false;

    return true;
}

void HandleDraw(Vector2 pos, Color strokeColor) {
    DrawCircle(pos.x, pos.y, HANDLE_RADIUS, strokeColor);
    DrawCircle(pos.x, pos.y, HANDLE_INTERIOR_RADIUS, HANDLE_INTERIOR_COLOR);
}

void LayerDraw(Transform2D transform, Layer layer, bool handlesActive) {
    Color color;
    Color outlineColor;
    if (layer.type == LAYER_TYPE_HURTBOX) {
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
    rlTranslatef(layer.transform.o.x, layer.transform.o.y, 0.0f);

    if (layer.type == LAYER_TYPE_HITBOX) DrawLine(0, 0, layer.hitboxKnockbackX, layer.hitboxKnockbackY, outlineColor);
    
    switch (layer.shapeType) {
        case SHAPE_CIRCLE:
            DrawCircle(0, 0, layer.data.circleRadius, color);
            if (handlesActive) DrawCircleLines(0, 0, layer.data.circleRadius, outlineColor);
            break;
        
        case SHAPE_RECTANGLE: {
            int x = -layer.data.rectangle.rightX;
            int y = -layer.data.rectangle.bottomY;
            int width = 2 * layer.data.rectangle.rightX;
            int height = 2 * layer.data.rectangle.bottomY;
            DrawRectangle(x, y, width, height, color);
            if (!handlesActive) break;
            // have to draw it manually instead of DrawRectangleLines because the lines scale with the transform.
            int xWidth = x + width;
            int yHeight = y + height;
            DrawLine(x, y, xWidth, y, outlineColor);
            DrawLine(xWidth, y, xWidth, yHeight, outlineColor);
            DrawLine(xWidth, yHeight, x, yHeight, outlineColor);
            DrawLine(x, yHeight, x, y, outlineColor);
        } break;
        
        case SHAPE_CAPSULE: {
            float rotation = Transform2DGetRotation(layer.transform) * 180.0f / PI;
            rlRotatef(rotation, 0.0f, 0.0f, 1.0f);
            Rectangle rect = {
                .x = -layer.data.capsule.radius,
                .y = -layer.data.capsule.radius - layer.data.capsule.height,
                .width = layer.data.capsule.radius * 2.0f,
                .height = (layer.data.capsule.radius + layer.data.capsule.height) * 2.0f
            };
            DrawRectangleRounded(rect, 1.0f, COMBAT_SHAPE_SEGMENTS, color);
            if (handlesActive) DrawRectangleRoundedLines(rect, 1.0f, COMBAT_SHAPE_SEGMENTS, 0.0f, outlineColor);
        } break;
        
        default: break;
    }

    rlPopMatrix();
    
    if (!handlesActive) return;

    Vector2 globalPos = Transform2DToGlobal(transform, layer.transform.o);
    HandleDraw(globalPos, outlineColor);
    switch (layer.shapeType) {
        case SHAPE_CIRCLE: {
            Vector2 radiusVector = {.x = layer.data.circleRadius, .y = 0.0f};
            Vector2 radiusPos = Transform2DBasisXFormInv(transform, radiusVector);
            HandleDraw(Vector2Add(globalPos, radiusPos), outlineColor);
        } break;

        case SHAPE_RECTANGLE: {
            Vector2 localHandlePos = {.x = layer.data.rectangle.rightX, .y = layer.data.rectangle.bottomY};
            Vector2 extents = Transform2DBasisXFormInv(transform, localHandlePos);
            HandleDraw(Vector2Add(globalPos, extents), outlineColor);
        } break;

        case SHAPE_CAPSULE: {
            Vector2 radius = {layer.data.capsule.radius, 0.0f};
            Vector2 height = {0.0f, layer.data.capsule.height};
            Vector2 rotationHandle = {0.0f, -layer.data.capsule.height};
            
            Vector2 globalRadius = Vector2Add(globalPos, Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(layer.transform, radius)));
            Vector2 globalHeight = Vector2Add(globalPos, Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(layer.transform, height)));
            Vector2 globalRotation = Vector2Add(globalPos, Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(layer.transform, rotationHandle)));

            HandleDraw(globalRadius, outlineColor);
            HandleDraw(globalHeight, outlineColor);
            HandleDraw(globalRotation, outlineColor);
        } break;
    }

    if (layer.type != LAYER_TYPE_HITBOX) return;
    Vector2 globalKnockback = Vector2Add(globalPos, Transform2DBasisXFormInv(transform, (Vector2) {layer.hitboxKnockbackX, layer.hitboxKnockbackY}));
    HandleDraw(globalKnockback, outlineColor);
}

bool HandleIsColliding(Transform2D globalTransform, Vector2 globalMousePos, Vector2 localPos) {
    Vector2 globalHandlePos = Transform2DToGlobal(globalTransform, localPos);
    Rectangle rect = {
        .x = globalHandlePos.x - HANDLE_RADIUS,
        .y = globalHandlePos.y - HANDLE_RADIUS,
        .height = 2.0f * HANDLE_RADIUS,
        .width = 2.0f * HANDLE_RADIUS
    };
    return CheckCollisionPointRec(globalMousePos, rect);
}

Handle LayerSelectHandle(Transform2D transform, Vector2 globalMousePos, Layer layer) {
    if (layer.type == LAYER_TYPE_HITBOX) {
        Vector2 knockbackPos = {layer.transform.o.x + layer.hitboxKnockbackX, layer.transform.o.y + layer.hitboxKnockbackY};
        if (HandleIsColliding(transform, globalMousePos, knockbackPos)) return HANDLE_HITBOX_KNOCKBACK;
    }

    switch (layer.shapeType) {
        case SHAPE_CIRCLE: {
            Vector2 radiusPos = Transform2DToGlobal(layer.transform, (Vector2) {layer.data.circleRadius, 0.0f});
            if (HandleIsColliding(transform, globalMousePos, radiusPos)) return HANDLE_CIRCLE_RADIUS;
        } break;
        
        case SHAPE_RECTANGLE: {
            Vector2 cornerPos = Transform2DToGlobal(layer.transform, (Vector2) {layer.data.rectangle.rightX, layer.data.rectangle.bottomY});
            if (HandleIsColliding(transform, globalMousePos, cornerPos)) return HANDLE_RECTANGLE_CORNER;
        } break;

        case SHAPE_CAPSULE: {
            Vector2 radiusPos = Transform2DToGlobal(layer.transform, (Vector2) {layer.data.capsule.radius, 0.0f});
            if (HandleIsColliding(transform, globalMousePos, radiusPos)) return HANDLE_CAPSULE_RADIUS;

            Vector2 heightPos = Transform2DToGlobal(layer.transform, (Vector2) {0.0f, layer.data.capsule.height});
            if (HandleIsColliding(transform, globalMousePos, heightPos)) return HANDLE_CAPSULE_HEIGHT;

            Vector2 rotationPos = Transform2DToGlobal(layer.transform, (Vector2) {0.0f, -layer.data.capsule.height});
            if (HandleIsColliding(transform, globalMousePos, rotationPos)) return HANDLE_CAPSULE_ROTATION;
        } break;

        default: break;
    }

    if (HandleIsColliding(transform, globalMousePos, layer.transform.o)) return HANDLE_CENTER;
    return HANDLE_NONE;
}



bool LayerSetHandle(Vector2 localMousePos, Layer *layer, Handle handle) {
    Vector2 handlePos = Vector2Round(Vector2Max(Transform2DToLocal(layer->transform, localMousePos), 0.0f));
    Vector2 offset = Vector2Subtract(localMousePos, layer->transform.o);

    switch (handle) {
        case HANDLE_CENTER:
            layer->transform.o = Vector2Round(localMousePos);
            return true;

        case HANDLE_HITBOX_KNOCKBACK:
            if (layer->type != LAYER_TYPE_HITBOX) return false;
            Vector2 knockback = Vector2Round(offset);
            layer->hitboxKnockbackX = knockback.x;
            layer->hitboxKnockbackY = knockback.y;
            return true;

        case HANDLE_CIRCLE_RADIUS:
            if (layer->shapeType != SHAPE_CIRCLE) return false;
            layer->data.circleRadius = handlePos.x;
            return true;

        case HANDLE_RECTANGLE_CORNER:
            if (layer->shapeType != SHAPE_RECTANGLE) return false;
            layer->data.rectangle.rightX = handlePos.x;
            layer->data.rectangle.bottomY = handlePos.y;
            return true;

        case HANDLE_CAPSULE_RADIUS:
            if (layer->shapeType != SHAPE_CAPSULE) return false;
            layer->data.capsule.radius = handlePos.x;
            return true;

        case HANDLE_CAPSULE_HEIGHT:
            if (layer->shapeType != SHAPE_CAPSULE) return false;
            layer->data.capsule.height = handlePos.y;
            return true;
        
        case HANDLE_CAPSULE_ROTATION:
            if (layer->shapeType != SHAPE_CAPSULE) return false;
            float rotation = atan2(offset.y, offset.x) + PI / 2.0f;
            layer->transform = Transform2DRotate(layer->transform, rotation);
            return true;
        
        default:
            return false;
    }
}
