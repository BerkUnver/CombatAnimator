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

float Vector2Rotation(Vector2 vector) {
    return atan2f(vector.y, vector.x);
}

Transform2D Transform2DIdentity() {
    return (Transform2D) {
        .o = {.x = 0, .y = 0},
        .x = {.x = 1, .y = 0},
        .y = {.x = 0, .y = 1}
    };
}

Transform2D Transform2DFromPosition(Vector2 pos) {
    return (Transform2D) {
        .o = pos,
        .x = {1.0f, 0.0f},
        .y = {0.0f, 1.0f}
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

Transform2D Transform2DFromRotation(float rotation) {
    float cosine = cosf(rotation);
    float sine = sinf(rotation);
    return (Transform2D) {
        .x = (Vector2) {cosine, sine},
        .y = (Vector2) {-sine, cosine},
        .o = (Vector2) {0.0f, 0.0f}
    };
}

Transform2D Transform2DRotate(Transform2D transform, float rot) {
    return (Transform2D) {
        .o = transform.o,
        .x = Vector2Rotate(transform.x, rot),
        .y = Vector2Rotate(transform.y, rot)
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

Transform2D Transform2DMultiply(Transform2D a, Transform2D b) {
    return (Transform2D) {
        .x.x = a.x.x * b.x.x + a.y.x * b.x.y,
        .x.y = a.x.y * b.x.x + a.y.y * b.x.y,
        .y.x = a.x.x * b.y.x + a.y.x * b.y.y,
        .y.y = a.x.y * b.y.x + a.y.y * b.y.y,
        .o.x = a.x.x * b.o.x + a.y.x * b.o.y + a.o.x,
        .o.y = a.x.y * b.o.x + a.y.y * b.o.y + a.o.y
    };
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
