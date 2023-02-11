#ifndef TRANSFORM_2D
#define TRANSFORM_2D

#include "raylib.h"

typedef struct Transform2D {
    Vector2 o;
    Vector2 x;
    Vector2 y;
} Transform2D;

Transform2D Transform2DIdentity();

Vector2 Transform2DBasisXForm(Transform2D transform, Vector2 vector);
Vector2 Transform2DToLocal(Transform2D transform, Vector2 vector);

Vector2 Transform2DBasisXFormInv(Transform2D transform, Vector2 vector);
Vector2 Transform2DToGlobal(Transform2D transform, Vector2 vector);

Transform2D Transform2DRotate(Transform2D transform, float rotation);
float Transform2DGetRotation(Transform2D transform);

Transform2D Transform2DSetScale(Transform2D transform, Vector2 scale);
Transform2D Transform2DScale(Transform2D transform, Vector2 scale);

Matrix Transform2DToMatrix(Transform2D transform);
void rlTransform2DXForm(Transform2D transform);

#endif
