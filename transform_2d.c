#include <math.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "transform_2d.h"


float Max(float i, float j) { return i > j ? i : j; }

Vector2 Vector2Round(Vector2 vec) {
    return (Vector2) {.x = roundf(vec.x), .y = roundf(vec.y)};
}

Vector2 Vector2Max(Vector2 vec, float max) {
    return (Vector2) {.x = Max(vec.x, max), .y = Max(vec.y, max)};
}

Transform2D Transform2DIdentity() {
    return (Transform2D) {
        .o = {.x = 0, .y = 0},
        .x = {.x = 1, .y = 0},
        .y = {.x = 0, .y = 1}
    };
}

Vector2 Transform2DBasisXForm(Transform2D transform, Vector2 vector) {
    Vector2 result;
    result.x = (vector.x * transform.y.y - vector.y * transform.y.x) / (transform.x.x * transform.y.y - transform.x.y * transform.y.x);
    result.y = (vector.y - transform.x.y * result.x) / transform.y.y;
    return result;
}

Vector2 Transform2DToLocal(Transform2D transform, Vector2 vector) {
    Vector2 base = Vector2Subtract(vector, transform.o);
    return Transform2DBasisXForm(transform, base);
}

Vector2 Transform2DToGlobal(Transform2D transform, Vector2 vector) {
    Vector2 base = Transform2DBasisXFormInv(transform, vector);
    return Vector2Add(base, transform.o);
}


Transform2D Transform2DRotate(Transform2D transform, float rot) {
    return (Transform2D) {
        .o = transform.o,
        .x = {.x = cosf(rot), .y = sinf(rot)},
        .y = {.x = -sinf(rot), .y = cosf(rot)}
    };
}

float Transform2DGetRotation(Transform2D transform) {
    return atan2(transform.x.y, transform.x.x);
}

Transform2D Transform2DSetScale(Transform2D transform, Vector2 scale) {
    return (Transform2D) {
        .x = Vector2Scale(Vector2Normalize(transform.x), scale.x),
        .y = Vector2Scale(Vector2Normalize(transform.y), scale.y)
    };
}

Transform2D Transform2DScale(Transform2D transform, Vector2 scale) {
    transform.x = Vector2Multiply(transform.x, scale);
    transform.y = Vector2Multiply(transform.y, scale);
    return transform;
}

Vector2 Transform2DBasisXFormInv(Transform2D transform, Vector2 vector) {
    return (Vector2) {
        .x = transform.x.x * vector.x + transform.y.x * vector.y,
        .y = transform.x.y * vector.x + transform.y.y * vector.y
    };
}

Matrix Transform2DToMatrix(Transform2D transform) {
    Vector2 pos = Transform2DBasisXForm(transform, transform.o);
    // rlgl matrix stores position after transform is calculated, we store it before. Must convert between the two.
    return (Matrix) {
        .m0 = transform.x.x,
        .m1 = transform.y.x,
        .m2 = 0.0f,
        .m3 = 0.0f,

        .m4 = transform.x.y,
        .m5 = transform.y.y,
        .m6 = 0.0f,
        .m7 = 0.0f,
        
        .m8 = 0.0f,
        .m9 = 0.0f,
        .m10 = 1.0f,
        .m11 = 0.0f,
        
        .m12 = pos.x,
        .m13 = pos.y,
        .m14 = 0.0f,
        .m15 = 1.0f
    };
}

void rlTransform2DXForm(Transform2D transform) {
    Matrix matrix = Transform2DToMatrix(transform);  
    rlMultMatrixf((float *) &matrix);
    rlTranslatef(matrix.m12, matrix.m13, 0.0f);
}
