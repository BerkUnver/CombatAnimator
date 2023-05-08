#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "layer.h"
#include "transform_2d.h"

void HandleDraw(Vector2 pos, Color strokeColor) {
    DrawCircle((int) pos.x, (int) pos.y, HANDLE_RADIUS, strokeColor);
    DrawCircle((int) pos.x, (int) pos.y, 6.0f, RAYWHITE);
}

void ShapeDraw(Shape shape, Transform2D transform, Color color, bool outline, Color outlineColor) {
    rlPushMatrix();
    rlTranslatef(transform.o.x, transform.o.y, 0);
    switch (shape.type) {
        case SHAPE_CIRCLE:
            DrawCircle(0, 0, (float) shape.circleRadius, color);
            if (outline) DrawCircleLines(0, 0, (float) shape.circleRadius, outlineColor);
            break;

        case SHAPE_RECTANGLE: {
            int x = -shape.rectangle.rightX;
            int y = -shape.rectangle.bottomY;
            int width = 2 * shape.rectangle.rightX;
            int height = 2 * shape.rectangle.bottomY;
            DrawRectangle(x, y, width, height, color);
            if (!outline) break;
            // have to draw it manually instead of DrawRectangleLines because the lines scale with the transform.
            int xWidth = x + width;
            int yHeight = y + height;
            DrawLine(x, y, xWidth, y, outlineColor);
            DrawLine(xWidth, y, xWidth, yHeight, outlineColor);
            DrawLine(xWidth, yHeight, x, yHeight, outlineColor);
            DrawLine(x, yHeight, x, y, outlineColor);
        } break;

        case SHAPE_CAPSULE: {
            rlRotatef(shape.capsule.rotation * 180.0f / PI, 0.0f, 0.0f, 1.0f);
            Rectangle rect = {
                .x = (float) -shape.capsule.radius,
                .y = (float) (-shape.capsule.radius - shape.capsule.height),
                .width = (float) shape.capsule.radius * 2.0f,
                .height = (float) (shape.capsule.radius + shape.capsule.height) * 2.0f
            };
            DrawRectangleRounded(rect, 1.0f, SHAPE_SEGMENTS, color);
            if (outline) DrawRectangleRoundedLines(rect, 1.0f, SHAPE_SEGMENTS, 0.0f, outlineColor);
        } break;
    }
    rlPopMatrix();
}

// TODO: fix so the transforms are multiplied separately instead of passed together.
void ShapeDrawHandles(Shape shape, Transform2D transform, Transform2D layerTransform, Color color) {
    Vector2 center = Transform2DToGlobal(transform, layerTransform.o);
    switch (shape.type) {
        case SHAPE_CIRCLE: {
            Vector2 radiusVector = {.x = (float) shape.circleRadius, .y = 0.0f};
            Vector2 radiusPos = Transform2DBasisXFormInv(transform, radiusVector);
            HandleDraw(Vector2Add(center, radiusPos), color);
        } break;

        case SHAPE_RECTANGLE: {
            Vector2 localHandlePos = {.x = (float) shape.rectangle.rightX, .y = (float) shape.rectangle.bottomY};
            Vector2 extents = Transform2DBasisXFormInv(transform, localHandlePos);
            HandleDraw(Vector2Add(center, extents), color);
        } break;

        case SHAPE_CAPSULE: {
            Vector2 radius = {.x = (float) shape.capsule.radius, .y = 0.0f};
            Vector2 height = {.x = 0.0f, .y = (float) shape.capsule.height};
            Vector2 rotation = {.x = 0.0f, .y = (float) -shape.capsule.height};
            Transform2D xform = Transform2DRotate(layerTransform, shape.capsule.rotation);

            Vector2 globalRadius = Vector2Add(center, Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(xform, radius)));
            Vector2 globalHeight = Vector2Add(center, Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(xform, height)));
            Vector2 globalRotation = Vector2Add(center, Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(xform, rotation)));

            HandleDraw(globalRadius, color);
            HandleDraw(globalHeight, color);
            HandleDraw(globalRotation, color);
        } break;
    }
}

void LayerDraw(Layer *layer, Transform2D transform, bool handlesActive) {
    Color colorOutline;
    Color color;
    rlPushMatrix();
    rlTransform2DXForm(transform);
    switch (layer->type) {
        case LAYER_HITBOX:
            colorOutline = LAYER_HITBOX_COLOR_OUTLINE;
            color = LAYER_HITBOX_COLOR;
            ShapeDraw(layer->hitbox.shape, layer->transform, color, handlesActive, colorOutline);
            DrawLine(layer->transform.o.x,
                     layer->transform.o.y,
                     layer->transform.o.x + layer->hitbox.knockbackX,
                     layer->transform.o.y + layer->hitbox.knockbackY,
                     colorOutline);
            break;
        case LAYER_HURTBOX:
            colorOutline = LAYER_HURTBOX_COLOR_OUTLINE;
            color = LAYER_HURTBOX_COLOR;
            ShapeDraw(layer->hurtboxShape, layer->transform, color, handlesActive, colorOutline);
            break;
        case LAYER_METADATA:
            colorOutline =  LAYER_METADATA_COLOR_OUTLINE;
            break;
    }
    rlPopMatrix();

    if (!handlesActive) return;
    Vector2 centerGlobal = Transform2DToGlobal(transform, layer->transform.o);
    HandleDraw(centerGlobal, colorOutline);
    switch (layer->type) {
        case LAYER_HITBOX:
            ShapeDrawHandles(layer->hitbox.shape, transform, layer->transform, colorOutline);
            Vector2 knockbackLocal = (Vector2) {
                .x = (float) layer->hitbox.knockbackX,
                .y = (float) layer->hitbox.knockbackY
            };
            Vector2 knockbackGlobal = Vector2Add(centerGlobal, Transform2DBasisXFormInv(transform, knockbackLocal));
            HandleDraw(knockbackGlobal, colorOutline);
            break;
        case LAYER_HURTBOX:
            ShapeDrawHandles(layer->hurtboxShape, transform, layer->transform, colorOutline);
            break;
        case LAYER_METADATA:
            break;
    }

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

Handle ShapeHandleSelect(Shape shape, Transform2D transform, Transform2D layerTransform, Vector2 globalMousePos) {
    switch (shape.type) {
        case SHAPE_CIRCLE: {
            Vector2 radiusPos = Transform2DToGlobal(layerTransform, (Vector2) {.x = (float) shape.circleRadius, .y = 0.0f});
            if (HandleIsColliding(transform, globalMousePos, radiusPos)) return HANDLE_CIRCLE_RADIUS;
        } break;

        case SHAPE_RECTANGLE: {
            Vector2 rectHandle = {
                .x = (float) shape.rectangle.rightX,
                .y = (float) shape.rectangle.bottomY
            };
            Vector2 cornerPos = Transform2DToGlobal(layerTransform, rectHandle);
            if (HandleIsColliding(transform, globalMousePos, cornerPos)) return HANDLE_RECTANGLE_CORNER;
        } break;

        case SHAPE_CAPSULE: {
            Transform2D xform = Transform2DRotate(layerTransform, shape.capsule.rotation);
            Vector2 radiusPos = Transform2DToGlobal(xform, (Vector2) {(float) shape.capsule.radius, 0.0f});
            if (HandleIsColliding(transform, globalMousePos, radiusPos)) return HANDLE_CAPSULE_RADIUS;

            Vector2 heightPos = Transform2DToGlobal(xform, (Vector2) {0.0f, (float) shape.capsule.height});
            if (HandleIsColliding(transform, globalMousePos, heightPos)) return HANDLE_CAPSULE_HEIGHT;

            Vector2 rotationPos = Transform2DToGlobal(xform, (Vector2) {0.0f, (float) -shape.capsule.height});
            if (HandleIsColliding(transform, globalMousePos, rotationPos)) return HANDLE_CAPSULE_ROTATION;
        } break;
    }
    return HANDLE_NONE;
}

Handle LayerHandleSelect(Layer *layer, Transform2D transform, Vector2 globalMousePos) {
    switch (layer->type) {
        case LAYER_HITBOX: {
            Vector2 knockbackHandle = {
                    .x = layer->transform.o.x + (float) layer->hitbox.knockbackX,
                    .y = layer->transform.o.y + (float) layer->hitbox.knockbackY
            };
            if (HandleIsColliding(transform, globalMousePos, knockbackHandle)) return HANDLE_HITBOX_KNOCKBACK;
            Handle handle = ShapeHandleSelect(layer->hitbox.shape, transform, layer->transform, globalMousePos);
            if (handle != HANDLE_NONE) return handle;
        } break;

        case LAYER_HURTBOX: {
            Handle handle = ShapeHandleSelect(layer->hurtboxShape, transform, layer->transform, globalMousePos);
            if (handle != HANDLE_NONE) return handle;
        } break;

        case LAYER_METADATA:
            break;
    }
    if (HandleIsColliding(transform, globalMousePos, layer->transform.o)) return HANDLE_CENTER;
    return HANDLE_NONE;
}


bool ShapeHandleSet(Shape *shape, Handle handle, Vector2 handlePos) {
    switch (shape->type) {
        case SHAPE_CIRCLE:
            if (handle != HANDLE_CIRCLE_RADIUS) return false;
            shape->circleRadius = (int) (handlePos.x > 0 ? 0 : handlePos.x);
            return true;

        case SHAPE_RECTANGLE:
            if (handle != HANDLE_RECTANGLE_CORNER) return false;
            shape->rectangle.rightX = (int) (handlePos.x > 0 ? 0 : handlePos.x);
            shape->rectangle.bottomY = (int) (handlePos.y > 0 ? 0 : handlePos.y);
            return true;

        case SHAPE_CAPSULE:
            if (handle == HANDLE_CAPSULE_ROTATION) {
                shape->capsule.rotation = atan2f(handlePos.y, handlePos.x) + PI / 2.0f;
                return true;
            }
            Vector2 handleRotated = Vector2Max(Vector2Rotate(handlePos, -shape->capsule.rotation), 0.0f);
            if (handle == HANDLE_CAPSULE_HEIGHT) {
                shape->capsule.height = (int) handleRotated.y;
                return true;
            }
            if (handle == HANDLE_CAPSULE_RADIUS) {
                shape->capsule.radius = (int) handleRotated.x;
                return true;
            }
            return false;
    }
    assert(false);
}

bool LayerHandleSet(Layer *layer, Handle handle, Vector2 localMousePos) {
    if (handle == HANDLE_CENTER) {
        layer->transform.o = Vector2Round(localMousePos);
        return true;
    }

    Vector2 handlePos = Vector2Subtract(localMousePos, layer->transform.o);

    switch (layer->type) {
        case LAYER_HITBOX:
            if (handle == HANDLE_HITBOX_KNOCKBACK) {
                Vector2 knockback = Vector2Round(handlePos);
                layer->hitbox.knockbackX = (int) knockback.x;
                layer->hitbox.knockbackY = (int) knockback.y;
                return true;
            }
            return ShapeHandleSet(&layer->hitbox.shape, handle, handlePos);
        case LAYER_HURTBOX:
            return ShapeHandleSet(&layer->hurtboxShape, handle, handlePos);
        case LAYER_METADATA:
            return false;
    }
    assert(false);
}

cJSON *ShapeSerialize(Shape shape) {
    cJSON *shapeJson = cJSON_CreateObject();
    switch (shape.type) {
        case SHAPE_CIRCLE:
            cJSON_AddStringToObject(shapeJson, "type", "CIRCLE");
            cJSON_AddNumberToObject(shapeJson, "circleRadius", shape.circleRadius);
            break;
        case SHAPE_RECTANGLE:
            cJSON_AddStringToObject(shapeJson, "type", "RECTANGLE");
            cJSON *rect = cJSON_CreateObject();
            cJSON_AddNumberToObject(rect, "rightX", shape.rectangle.rightX);
            cJSON_AddNumberToObject(rect, "bottomY", shape.rectangle.bottomY);
            cJSON_AddItemToObject(shapeJson, "rectangle", rect);
            break;
        case SHAPE_CAPSULE:
            cJSON_AddStringToObject(shapeJson, "type", "CAPSULE");
            cJSON *capsule = cJSON_CreateObject();
            cJSON_AddNumberToObject(capsule, "height", shape.capsule.height);
            cJSON_AddNumberToObject(capsule, "radius", shape.capsule.radius);
            cJSON_AddNumberToObject(capsule, "rotation", shape.capsule.rotation);
            cJSON_AddItemToObject(shapeJson, "capsule", capsule);
            // Ignoring serializing rotation for now, will figure out a nice way to make it work.
            break;
    }
    return shapeJson;
}

bool ShapeDeserialize(cJSON *json, Shape *shape, int version) {
#define RETURN_FAIL do { printf("Cannot deserialize shape. Error: %s at %i\n", __FILE__, __LINE__); return false; } while (0)
    if (!json || !cJSON_IsObject(json)) RETURN_FAIL;
    cJSON *type = cJSON_GetObjectItem(json, version >= 4 ? "type" : "shapeType");
    if (!cJSON_IsString(type)) RETURN_FAIL;
    char *typeString = cJSON_GetStringValue(type);
    if (!strcmp(typeString, "CIRCLE")) {
        cJSON *radius = cJSON_GetObjectItem(json, "circleRadius");
        if (!cJSON_IsNumber(radius)) RETURN_FAIL;
        shape->type = SHAPE_CIRCLE;
        shape->circleRadius = (int) cJSON_GetNumberValue(radius);
        return true;
    } else if (!strcmp(typeString, "RECTANGLE")) {
        cJSON *rightX;
        cJSON *bottomY;

        if (version >= 4) {
            cJSON *rect = cJSON_GetObjectItem(json, "rectangle");
            if (!cJSON_IsObject(rect)) RETURN_FAIL;
            rightX = cJSON_GetObjectItem(rect, "rightX");
            bottomY = cJSON_GetObjectItem(rect, "bottomY");
        } else {
            rightX = cJSON_GetObjectItem(json, "rectangleRightX");
            bottomY = cJSON_GetObjectItem(json, "rectangleBottomY");
        }

        if (!cJSON_IsNumber(rightX)) RETURN_FAIL;
        if (!cJSON_IsNumber(bottomY)) RETURN_FAIL;

        shape->type = SHAPE_RECTANGLE;
        shape->rectangle.rightX = (int) cJSON_GetNumberValue(rightX);
        shape->rectangle.bottomY = (int) cJSON_GetNumberValue(bottomY);
        return true;

    } else if (!strcmp(typeString, "CAPSULE")) {
        cJSON *height;
        cJSON *radius;
        cJSON *rotation;

        if (version >= 4) {
            cJSON *capsule = cJSON_GetObjectItem(json, "capsule");
            if (!cJSON_IsObject(capsule)) RETURN_FAIL;
            height = cJSON_GetObjectItem(capsule, "height");
            radius = cJSON_GetObjectItem(capsule, "radius");
            rotation = cJSON_GetObjectItem(capsule, "rotation");
        } else {
            height = cJSON_GetObjectItem(json, "capsuleHeight");
            radius = cJSON_GetObjectItem(json, "capsuleRadius");
            rotation = cJSON_GetObjectItem(json, "capsuleRotation");
        }

        if (!cJSON_IsNumber(height)) RETURN_FAIL;
        if (!cJSON_IsNumber(radius)) RETURN_FAIL;
        if (!cJSON_IsNumber(rotation)) RETURN_FAIL;

        shape->type = SHAPE_CAPSULE;
        shape->capsule.height = (int) cJSON_GetNumberValue(height);
        shape->capsule.radius = (int) cJSON_GetNumberValue(radius);
        shape->capsule.rotation = (float) cJSON_GetNumberValue(rotation);
        return true;

    } else RETURN_FAIL;
}