#include <math.h>
#include <stdbool.h>
#include <assert.h>
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
    // todo: this is a hack for rlTransform2DXForm not working.
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
            float rotation = Transform2DGetRotation(transform) * 180.0f / PI;
            rlRotatef(rotation, 0.0f, 0.0f, 1.0f);
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
            Vector2 rotationHandle = {.x = 0.0f, .y = (float) -shape.capsule.height};

            Vector2 globalRadius = Vector2Add(center, Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(layerTransform, radius)));
            Vector2 globalHeight = Vector2Add(center, Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(layerTransform, height)));
            Vector2 globalRotation = Vector2Add(center, Transform2DBasisXFormInv(transform, Transform2DBasisXFormInv(layerTransform, rotationHandle)));

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
            DrawLine(0, 0, layer->hitbox.knockbackX, layer->hitbox.knockbackY, colorOutline);
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

Handle ShapeHandleIsColliding(Shape shape, Transform2D transform, Transform2D layerTransform, Vector2 globalMousePos) {
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
            Vector2 radiusPos = Transform2DToGlobal(layerTransform, (Vector2) {(float) shape.capsule.radius, 0.0f});
            if (HandleIsColliding(transform, globalMousePos, radiusPos)) return HANDLE_CAPSULE_RADIUS;

            Vector2 heightPos = Transform2DToGlobal(layerTransform, (Vector2) {0.0f, (float) shape.capsule.height});
            if (HandleIsColliding(transform, globalMousePos, heightPos)) return HANDLE_CAPSULE_HEIGHT;

            Vector2 rotationPos = Transform2DToGlobal(layerTransform, (Vector2) {0.0f, (float) -shape.capsule.height});
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
            Handle handle = ShapeHandleIsColliding(layer->hitbox.shape, transform, layer->transform, globalMousePos);
            if (handle != HANDLE_NONE) return handle;
        } break;

        case LAYER_HURTBOX: {
            Handle handle = ShapeHandleIsColliding(layer->hurtboxShape, transform, layer->transform, globalMousePos);
            if (handle != HANDLE_NONE) return handle;
        } break;

        case LAYER_METADATA:
            break;
    }
    if (HandleIsColliding(transform, globalMousePos, layer->transform.o)) return HANDLE_CENTER;
    return HANDLE_NONE;
}


bool ShapeHandleSet(Shape *shape, Handle handle, Vector2 handlePos) {
    switch (handle) {
        case HANDLE_CIRCLE_RADIUS:
            if (shape->type != SHAPE_CIRCLE) return false;
            shape->circleRadius = (int) handlePos.x;
            return true;
        case HANDLE_RECTANGLE_CORNER:
            if (shape->type != SHAPE_RECTANGLE) return false;
            shape->rectangle.rightX = (int) handlePos.x;
            shape->rectangle.bottomY = (int) handlePos.y;
            return true;
        case HANDLE_CAPSULE_RADIUS:
            if (shape->type != SHAPE_CAPSULE) return false;
            shape->capsule.radius = (int) handlePos.x;
            return true;

        case HANDLE_CAPSULE_HEIGHT:
            if (shape->type != SHAPE_CAPSULE) return false;
            shape->capsule.height = (int) handlePos.y;
            return true;

        default:
            return false;
    }
}

bool LayerHandleSet(Layer *layer, Handle handle, Vector2 localMousePos) {
    if (handle == HANDLE_CENTER) {
        layer->transform.o = Vector2Round(localMousePos);
        return true;
    }

    Vector2 handlePos = Vector2Round(Vector2Max(Transform2DToLocal(layer->transform, localMousePos), 0.0f));
    Vector2 offset = Vector2Subtract(localMousePos, layer->transform.o);

    switch (layer->type) {
        case LAYER_HITBOX:
            if (handle == HANDLE_HITBOX_KNOCKBACK) {
                Vector2 knockback = Vector2Round(offset);
                layer->hitbox.knockbackX = (int) knockback.x;
                layer->hitbox.knockbackY = (int) knockback.y;
                return true;
            }
            if (handle == HANDLE_CAPSULE_ROTATION) {
                if (layer->hitbox.shape.type != SHAPE_CAPSULE) return false;
                float rotation = atan2(offset.y, offset.x) + PI / 2.0f;
                layer->transform = Transform2DRotate(layer->transform, rotation);
                return true;
            }
            return ShapeHandleSet(&layer->hitbox.shape, handle, handlePos);
        case LAYER_HURTBOX:
            if (handle == HANDLE_CAPSULE_ROTATION) {
                if (layer->hurtboxShape.type != SHAPE_CAPSULE) return false;
                float rotation = atan2(offset.y, offset.x) + PI / 2.0f;
                layer->transform = Transform2DRotate(layer->transform, rotation);
                return true;
            }
            return ShapeHandleSet(&layer->hurtboxShape, handle, handlePos);
        case LAYER_METADATA:
            return false;
    }
    assert(false);
}
